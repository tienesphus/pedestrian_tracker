#include <iostream>
#include <vector>

#include "tbb/flow_graph.h"
#include "tbb/tick_count.h"
#include "tbb/concurrent_queue.h"

#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>

#include "detection.hpp"
#include "world.hpp"
#include "tracker.hpp"
#include "utils.hpp"

/**
 * Builds and runs a graph with the the given configeration options
 */
void run_graph(const std::string& input, const NetConfigIR &net_config, const WorldConfig world_config, bool draw) {
    
    namespace flow = tbb::flow;
    using namespace cv;
    
    /*
     * This code below produces the following flow graph:
     * 
     * TODO pipe src, detect and track into render so all the drawing code is together
     * 
     *            src
     *             | Ptr<Mat>
     *             v
     *   +-->   throttle
     *   |         | Ptr<Mat>
     *   |         v
     *   |    pre_detect
     *   |         | Ptr<Mat>
     *   |         v
     *   |       detect
     *   |         | Ptr<Mat>
     *   |         v
     *   |    post_detect
     *   |         | Ptr<Detections>
     *   |         v
     *   |       track
     *   |         | Ptr<WorldState>
     *   |         v
     *   |       render
     *   |         | Ptr<Mat>
     *   |         V
     *   |      stream
     *   |         | continue_msg
     *   |         V
     *   |      monitor
     *   |         | continue_msg
     *   +---------+
     *
    */            
    
    // shared variables
    flow::graph g;
    tbb::concurrent_queue<Ptr<Mat>> display_queue;
    bool quit = false;
    
    // src: Reads images from the input stream.
    //      It will continuously try to push more images down the stream
    VideoCapture cap(input);
    flow::source_node<Ptr<Mat>> src(g,
        [&cap, &quit](Ptr<Mat> &v) -> bool {
            std::cout << "START SRC" << std::endl;
            if (quit) {
                return false;
            }
            Mat* frame = new Mat();
            bool res = cap.read(*frame);
            if (res) {
                v = Ptr<Mat>(frame);
                std::cout << "SOURCE END" << std::endl;
                return true;
            } else {
                return false;
            }
        }, false);
    
    // throttle: by default, infinite frames can be in the graph at one.
    //           This node will limit the graph so that only MAX_FRAMES
    //           will be processed at once. Without limiting, runnaway
    //           occurs (src will keep producing into an unlimited buffer)
    const int MAX_FRAMES = 2;
    flow::limiter_node<Ptr<Mat>> throttle(g, MAX_FRAMES);
    
    // The detect nodes all world together through the Detector object.
    // Note that the detect node is the bottle-neck in the pipeline, thus,
    // as much as possible should be moved into pre_detect or post_detect.
    Detector detector(net_config);
    
    // pre_detect: Preprocesses the image into a 'blob' so it is ready 
    //             to feed into a detection algorithm
    // TODO implement pre_detect
    
    // detect: Takes the preprocessed blob
    flow::function_node<Ptr<Mat>, Ptr<Detections>> detect(g, flow::serial,
        [&detector, draw](const Ptr<Mat> input) -> Ptr<Detections> {
            std::cout << "START DETECTIONS" << std::endl;
            auto detections = detector.process(input);
            if (draw)
                detections->draw();
            std::cout << "END DETECTIONS" << std::endl;
            return detections;
        }
    );
    
    // post_detect: Takes the raw detection output, interperets it, and
    //              produces some meaningful results
    // TODO implement post_detect
    
    // track: Tracks detections
    Tracker tracker(world_config);
    flow::function_node<Ptr<Detections>, Ptr<WorldState>> track(g, flow::serial,
        [&tracker, draw](const Ptr<Detections> detections) -> Ptr<WorldState> {
            std::cout << "START TRACK" << std::endl;
            WorldState s_state = tracker.process(*detections);
            Ptr<WorldState> state = Ptr<WorldState>(new WorldState(s_state));
            if (draw)
                tracker.draw(*(state->display));
            std::cout << "END TRACK" << std::endl;
            return state;
        }
    );
    
    // render: Draws the state of the world to a Mat. Also Passes the
    //         drawing to display_queue to be displayed
    flow::function_node<Ptr<WorldState>, Ptr<Mat>> render(g, flow::serial,
        [&world_config, draw, &display_queue](Ptr<WorldState> world) -> Ptr<Mat> {
            std::cout << "START DRAWER" << std::endl;
            if (draw)
            {
                world->draw();
                world_config.draw(*(world->display));
                display_queue.push(world->display);
            }
            std::cout << "END DRAWER" << std::endl;
            return world->display;
        }
    );
    
    // stream: Writes images to a stream
    flow::function_node<Ptr<Mat>, flow::continue_msg> stream(g, flow::serial,
        [](const Ptr<Mat> input) {
            // TODO: stream these results somewhere
            // this should plug into the GStreamer stuff somehow...
            std::cout << "START STREAM" << std::endl;
            std::cout << "END STREAM" << std::endl;
            return flow::continue_msg();
        }
    );
    
    // monitor: Calculates the speed that the program is running at by moving average.
    int frame_count = 0;
    const int AVG_SIZE = 10;
    tbb::tick_count ticks[AVG_SIZE] = {tbb::tick_count::now()};
    
    flow::function_node<flow::continue_msg, flow::continue_msg> monitor(g, flow::serial,
        [&frame_count, &ticks, AVG_SIZE](flow::continue_msg) {
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
        
    // Setup flow dependencis
    flow::make_edge(src,      throttle);
    flow::make_edge(throttle, detect);
    flow::make_edge(detect,   track);
    flow::make_edge(track,    render);
    flow::make_edge(render,   stream);
    flow::make_edge(stream,   monitor);
    flow::make_edge(monitor,  throttle.decrement);
    
    // Begin running stuff
    src.activate();
    
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

int main() {
    
    NetConfigIR net_config {
        0.5f,               // thresh
        15,                 // clazz
        cv::Size(300, 300), // size
        2/255.0,            // scale
        cv::Scalar(127.5, 127.5, 127.5),     // mean
        "/home/cv/code/cpp_counting/models/MobileNetSSD_IE/MobileNetSSD.xml", // xml
        "/home/cv/code/cpp_counting/models/MobileNetSSD_IE/MobileNetSSD.bin"  // bin
    };
    
    WorldConfig world_config = WorldConfig::from_file("/home/cv/code/cpp_counting/config.csv");
    
    std::string input = "/home/cv/code/samplevideos/pi3_20181213/2018-12-13--08-26-02--snippit-1.mp4";
    
    
    
    run_graph(input, net_config, world_config, true);
    
    return 0;
} 
