#include <iostream>
#include <vector>

#include <tbb/flow_graph.h>
#include <tbb/tick_count.h>
#include <tbb/concurrent_queue.h>

#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>

#include "detection.hpp"
#include "world.hpp"
#include "tracker.hpp"
#include "utils.hpp"

/**
 * Builds and runs a flow graph with the the given configeration options
 */
void run_parallel(const std::string& input, const NetConfigIR &net_config, const WorldConfig world_config, bool draw)
{
    
    namespace flow = tbb::flow;
    using namespace cv;
    
    /*
     * This code below produces the following flow graph:
     * 
     *            src
     *             | Ptr<Mat>
     *             v
     *   +-->-- throttle
     *   |         | Ptr<Mat>
     *   |         |
     *   |         +-------> pre_detect
     *   |         |            | continue_msg
     *   |         |            v
     *   |         |          detect
     *   |         v            | Ptr<Mat>
     *   |       joint  <-------+
     *   |         | tuple<Mat, Mat>
     *   |         v
     *   |    post_detect
     *   |         | Ptr<Detections>
     *   |         v
     *   |    track_n_draw
     *   |         | Ptr<Mat>
     *   |         v
     *   |       stream              
     *   |         | continue_msg
     *   |         v
     *   |       monitor
     *   |         | continue_msg
     *   +---------+
    */            
    
    // shared variables
    flow::graph g;
    tbb::concurrent_queue<Ptr<Mat>> display_queue;
    bool quit = false;
    
    std::cout << "Initialise Video" << std::endl;
    
    // src: Reads images from the input stream.
    //      It will continuously try to push more images down the stream
    VideoCapture cap(input);
    std::cout << "Video inited" << std::endl;
    flow::source_node<Ptr<Mat>> src(g,
        [&cap, &quit](Ptr<Mat> &v) -> bool {
            std::cout << "START SRC" << std::endl;
            if (quit) {
                return false;
            }
            Mat* frame = new Mat();
            // read three times so that we effectively are running real time
            bool res = cap.read(*frame) 
                        && cap.read(*frame) 
                        && cap.read(*frame);
            if (res) {
                v = Ptr<Mat>(frame);
                std::cout << "SOURCE END" << std::endl;
                return true;
            } else {
                return false;
            }
        }, false);
    
    std::cout << "Init limiter" << std::endl;
    
    // throttle: by default, infinite frames can be in the graph at one.
    //           This node will limit the graph so that only MAX_FRAMES
    //           will be processed at once. Without limiting, runnaway
    //           occurs (src will keep producing into an unlimited buffer)
    const int MAX_FRAMES = 2;
    flow::limiter_node<Ptr<Mat>> throttle(g, MAX_FRAMES);
    
    std::cout << "Init detector" << std::endl;
    
    // The detect nodes all world together through the Detector object.
    // Note that the detect node is the bottle-neck in the pipeline, thus,
    // as much as possible should be moved into pre_detect or post_detect.
    Detector detector(net_config);
    dnn::Net net = net_config.make_network();
    
    // pre_detect: Preprocesses the image into a 'blob' so it is ready 
    //             to feed into a detection algorithm
    flow::function_node<Ptr<Mat>, flow::continue_msg> pre_detect(g, flow::serial,
        [&detector, &net](const Ptr<Mat> input) -> flow::continue_msg {
            std::cout << "START PRE DETECT" << std::endl;
            detector.pre_process(*input, net);
            std::cout << "END PRE DETECT" << std::endl;
            return flow::continue_msg();
        }
    );
    
    // detect: Takes the preprocessed blob
    flow::function_node<flow::continue_msg, Ptr<Mat>> detect(g, flow::serial,
        [&detector, &net](flow::continue_msg _) -> Ptr<Mat> {
            std::cout << "START DETECT" << std::endl;
            auto detections = detector.process(net);
            std::cout << "END DETECT" << std::endl;
            return detections;
        }
    );
    
    // joint: combines the results from the original image and the detection data
    flow::join_node<flow::tuple<Ptr<Mat>, Ptr<Mat>>> joint(g);
    
    // post_detect: Takes the raw detection output, interperets it, and
    //              produces some meaningful results
    flow::function_node<flow::tuple<Ptr<Mat>, Ptr<Mat>>, Ptr<Detections>> post_detect(g, flow::serial,
        [&detector](const flow::tuple<Ptr<Mat>, Ptr<Mat>> input) -> Ptr<Detections> {
            std::cout << "START POST DETECT" << std::endl;
            Ptr<Mat> original = std::get<0>(input);
            Ptr<Mat> results  = std::get<1>(input);
            
            auto detections = detector.post_process(original, *results);
            std::cout << "END POST DETECT" << std::endl;
            return detections;
        }
    );
    
    std::cout << "Init tracker" << std::endl;
    
    // track: Tracks detections
    Tracker tracker(world_config);
    flow::function_node<Ptr<Detections>, flow::tuple<Ptr<WorldState>, Ptr<Mat>>> track_n_draw(g, flow::serial,
        [&tracker, draw, &world_config, &display_queue](const Ptr<Detections> detections) -> flow::tuple<Ptr<WorldState>, Ptr<Mat>>
        {
            std::cout << "START TRACK" << std::endl;
            Ptr<WorldState> state = Ptr<WorldState>(new WorldState(tracker.process(*detections)));
            std::cout << "END TRACK" << std::endl;
            
            std::cout << "START RENDER" << std::endl;
            // TODO figure out how to put this into it's own node
            // Trouble is that Tracker's data is temporary, so, if Tracker::draw()
            // is called latter, it may or may not be okay.
            Ptr<Mat> display;
            if (draw)
            {
                display = Ptr<Mat>(new cv::Mat());
                detections->get_frame()->copyTo(*display);
                
                detections->draw(*display);
                tracker.draw(*display);
                state->draw(*display);
                world_config.draw(*display);
                
                display_queue.push(display);
                
            }
            std::cout << "END RENDER" << std::endl;
            
            return std::make_tuple(state, display);
        }
    );
    
    std::cout << "Init writter" << std::endl;
    
    // stream: Writes images to a stream
    flow::function_node<flow::tuple<Ptr<WorldState>, Ptr<Mat>>, flow::continue_msg> stream(g, flow::serial,
        [](const flow::tuple<Ptr<WorldState>, Ptr<Mat>> input)
        {
            std::cout << "START STREAM" << std::endl;
            
            Ptr<WorldState> state = std::get<0>(input);
            Ptr<Mat> display      = std::get<1>(input);
            
            // TODO: stream these results somewhere
            // this should plug into the GStreamer stuff somehow...
            
            std::cout << "END STREAM" << std::endl;
            return flow::continue_msg();
        }
    );
    
    std::cout << "Init frame count" << std::endl;
    
    // monitor: Calculates the speed that the program is running at by moving average.
    int frame_count = 0;
    const int AVG_SIZE = 10;
    tbb::tick_count ticks[AVG_SIZE] = {tbb::tick_count::now()};
    
    flow::function_node<flow::continue_msg, flow::continue_msg> monitor(g, flow::serial,
        [&frame_count, &ticks, AVG_SIZE](flow::continue_msg)
        {
            std::cout << "START TICK COUNTER" << std::endl;
            
            frame_count++;
            tbb::tick_count now = tbb::tick_count::now();
            tbb::tick_count prev = ticks[frame_count % AVG_SIZE];
            ticks[frame_count % AVG_SIZE] = now;
            float fps = AVG_SIZE/(now-prev).seconds();
            
            std::cout << ("END TICK  frame: " + std::to_string(frame_count) + " fps: " + std::to_string(fps)) << std::endl;
            return flow::continue_msg();
        }
    );
        
    std::cout << "making edges" << std::endl;
        
    // Setup flow dependencis
    flow::make_edge(src,          throttle);
    flow::make_edge(throttle,     pre_detect);
    flow::make_edge(pre_detect,   detect);
    flow::make_edge(throttle,     flow::input_port<0>(joint));
    flow::make_edge(detect,       flow::input_port<1>(joint));
    flow::make_edge(joint,        post_detect);
    flow::make_edge(post_detect,  track_n_draw);
    flow::make_edge(track_n_draw, stream);
    flow::make_edge(stream,       monitor);
    flow::make_edge(monitor,      throttle.decrement);
    
    // Begin running stuff
    std::cout << "Starting video" << std::endl;
    src.activate();
    std::cout << "Video started" << std::endl;
    
    // The display code _must_ be run on the main thread. Thus, we pass
    // display frames here through a queue
    Ptr<Mat> display_img;
    while (!quit && draw) {
        if (display_queue.try_pop(display_img))
        {
            std::cout << "imshow()" << std::endl;
            imshow("output", *display_img);
            std::cout << "waitkey()" << std::endl;
        }
        // TODO test if waitKey allows tbb to use this core
        int key = waitKey(20);
        if (key =='q')
            quit = true;
    }
    
    // Allow the last few frames to pass through
    // If this is not done, invalid memory can occurs
    g.wait_for_all();
}

/**
 * A single threaded version of run_parallel. Useful for debugging.
 */
void run_series(const std::string& input, const NetConfigIR &net_config, const WorldConfig world_config, bool draw)
{
    
    using namespace cv;
    
    bool quit = false;
    
    std::cout << "Initialise Video" << std::endl;
    VideoCapture cap(input);
    
    std::cout << "Init detector" << std::endl;
    Detector detector(net_config);
    cv::dnn::Net net = net_config.make_network();
    
    std::cout << "Init tracker" << std::endl;
    Tracker tracker(world_config);
    
    std::cout << "Init frame count" << std::endl;
    int frame_count = 0;
    const int AVG_SIZE = 10;
    tbb::tick_count ticks[AVG_SIZE] = {tbb::tick_count::now()};
    
    while (!quit) {
    
        std::cout << "SOURCE START" << std::endl;
        Ptr<Mat> frame = Ptr<Mat>(new Mat());
        // read three times so that we effectively are running real time
        bool res = cap.read(*frame) 
                    && cap.read(*frame) 
                    && cap.read(*frame);
        std::cout << "SOURCE END" << std::endl;
    
        std::cout << "START PRE DETECT" << std::endl;
        detector.pre_process(*frame, net);
        std::cout << "END PRE DETECT" << std::endl;

        std::cout << "START DETECT" << std::endl;
        auto detections_raw = detector.process(net);
        std::cout << "END DETECT" << std::endl;
        
        std::cout << "START POST DETECT" << std::endl;
        auto detections = detector.post_process(frame, *detections_raw);
        std::cout << "END POST DETECT" << std::endl;
    
        std::cout << "START TRACK" << std::endl;
        Ptr<WorldState> state = Ptr<WorldState>(new WorldState(tracker.process(*detections)));
        std::cout << "END TRACK" << std::endl;
        
        std::cout << "START RENDER" << std::endl;
        Mat display;
        frame->copyTo(display);
        detections->draw(display);
        tracker.draw(display);
        state->draw(display);
        world_config.draw(display);
        std::cout << "END RENDER" << std::endl;
        
        std::cout << "START DISPLAY" << std::endl;
        imshow("output", display);
        int key = waitKey(20);
        if (key =='q')
            quit = true;
        std::cout << "END DISPLAY" << std::endl;        
        
        std::cout << "START TICK COUNTER" << std::endl;            
        frame_count++;
        tbb::tick_count now = tbb::tick_count::now();
        tbb::tick_count prev = ticks[frame_count % AVG_SIZE];
        ticks[frame_count % AVG_SIZE] = now;
        float fps = AVG_SIZE/(now-prev).seconds();
        std::cout << ("END TICK  frame: " + std::to_string(frame_count) + " fps: " + std::to_string(fps)) << std::endl;
    }
}

int main() {
    
    NetConfigIR net_config {
        0.5f,               // thresh
        15,                 // clazz
        cv::Size(300, 300), // size
        2/255.0,            // scale
        cv::Scalar(127.5, 127.5, 127.5),     // mean
        "../models/MobileNetSSD_IE/MobileNetSSD.xml", // xml
        "../models/MobileNetSSD_IE/MobileNetSSD.bin"  // bin
    };
    
    WorldConfig world_config = WorldConfig::from_file("../config.csv");
    
    std::string input = "../../samplevideos/pi3_20181213/2018-12-13--08-26-02--snippit-1.mp4";
    
    std::cout << input << std::endl;
    
    
    run_parallel(input, net_config, world_config, true);
    //run_series(input, net_config, world_config, true);
    
    return 0;
} 
