#include <iostream>
#include <vector>

#include <tbb/flow_graph.h>
#include <tbb/tick_count.h>
#include <tbb/concurrent_queue.h>

#include <opencv2/highgui.hpp>

#include "libbuscount.hpp"

using namespace std;
using namespace tbb;

BusCounter::BusCounter(NetConfigIR config, BusCounter::src_cb_t src, BusCounter::dest_cb_t dest)
: _detector(config), _src(src), _dest(dest)
{
    std::cout << "Init detector" << std::endl;
    _net = _detector.make_network();
}

//
// Builds and runs a graph with the the given configeration options
//

// pre_detect: Preprocesses the image into a 'blob' so it is ready 
//             to feed into a detection algorithm
flow::continue_msg BusCounter::pre_detect(const cv::Ptr<cv::Mat> input)
{
    cout << "START PRE DETECT" << endl;
    _detector.pre_process(*input, _net);
    cout << "END PRE DETECT" << endl;

    return flow::continue_msg();
}

cv::Ptr<cv::Mat> BusCounter::detect()
{
    std::cout << "START DETECT" << std::endl;
    auto detections = _detector.process(_net);
    std::cout << "END DETECT" << std::endl;

    return detections;
}

cv::Ptr<Detections> BusCounter::post_detect(const flow::tuple<cv::Ptr<cv::Mat>, cv::Ptr<cv::Mat>> input)
{
    std::cout << "START POST DETECT" << std::endl;
    cv::Ptr<cv::Mat> original = std::get<0>(input);
    cv::Ptr<cv::Mat> results  = std::get<1>(input);
    
    auto detections = _detector.post_process(original, *results);
    std::cout << "END POST DETECT" << std::endl;

    return detections;
}

std::tuple<cv::Ptr<WorldState>, cv::Ptr<cv::Mat>> BusCounter::track(const cv::Ptr<Detections> detections)
{
    std::cout << "START TRACK" << std::endl;
    cv::Ptr<WorldState> state = cv::Ptr<WorldState>(new WorldState(tracker.process(*detections)));
    std::cout << "END TRACK" << std::endl;
    
    std::cout << "START RENDER" << std::endl;
    // TODO figure out how to put this into it's own node
    // Trouble is that Tracker's data is temporary, so, if Tracker::draw()
    // is called latter, it may or may not be okay.
    cv::Ptr<cv::Mat> display;
    if (draw)
    {
        /*
        display = cv::Ptr<cv::Mat>(new cv::Mat());
        detections->get_frame()->copyTo(*display);
        
        detections->draw(*display);
        tracker.draw(*display);
        state->draw(*display);
        world_config.draw(*display);
        
        display_queue.push(display);
        */
        
    }
    std::cout << "END RENDER" << std::endl;
    
    return std::make_tuple(state, display);
}

flow::continue_msg BusCounter::monitor()
{
    std::cout << "START TICK COUNTER" << std::endl;
    
    frame_count++;
    tbb::tick_count now = tbb::tick_count::now();
    tbb::tick_count prev = ticks[frame_count % AVG_SIZE];
    ticks[frame_count % AVG_SIZE] = now;
    float fps = AVG_SIZE/(now-prev).seconds();
    
    std::cout << "END TICK  frame: " << frame_count << " fps: " << fps << std::endl;

    return flow::continue_msg();
}

void BusCounter::run(const NetConfigIR &net_config, const WorldConfig world_config)
{
    
    /*
    namespace flow = tbb::flow;
    using namespace cv;
    
    //
    // This code below produces the following flow graph:
    // 
    //            src
    //             | Ptr<cv::Mat>
    //             v
    //   +-->-- throttle
    //   |         | Ptr<cv::Mat>
    //   |         |
    //   |         +-------> pre_detect
    //   |         |            | continue_msg
    //   |         |            v
    //   |         |          detect
    //   |         v            | Ptr<cv::Mat>
    //   |       joint  <-------+
    //   |         | tuple<cv::Mat, cv::Mat>
    //   |         v
    //   |    post_detect
    //   |         | Ptr<Detections>
    //   |         v
    //   |       track
    //   |         | Ptr<WorldState>
    //   |         v
    //   |       draw              
    //   |         | continue_msg
    //   |         v
    //   |       monitor
    //   |         | continue_msg
    //   +---------+
    //            
    
    // shared variables
    flow::graph g;
    tbb::concurrent_queue<Ptr<cv::Mat>> display_queue;
    bool quit = false;
    
    // src: Reads images from the input stream.
    //      It will continuously try to push more images down the stream
    VideoCapture cap(input);
    flow::source_node<Ptr<cv::Mat>> src(g,
        [&cap, &quit](Ptr<cv::Mat> &v) -> bool {
            std::cout << "START SRC" << std::endl;
            if (quit) {
                return false;
            }
            cv::Mat* frame = new cv::Mat();
            // read three times so that we effectively are running real time
            bool res = cap.read(*frame) 
                        && cap.read(*frame) 
                        && cap.read(*frame);
            if (res) {
                v = Ptr<cv::Mat>(frame);
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
    flow::limiter_node<Ptr<cv::Mat>> throttle(g, MAX_FRAMES);
    
    // The detect nodes all world together through the Detector object.
    // Note that the detect node is the bottle-neck in the pipeline, thus,
    // as much as possible should be moved into pre_detect or post_detect.
    Detector detector(net_config);
    
    // detect: Takes the preprocessed blob
    flow::function_node<flow::continue_msg, Ptr<cv::Mat>> detect(g, flow::serial,
        [&detector](flow::continue_msg _) -> Ptr<cv::Mat> {
            std::cout << "START DETECT" << std::endl;
            auto detections = detector.process();
            std::cout << "END DETECT" << std::endl;
            return detections;
        }
    );
    
    // joint: combines the results from the original image and the detection data
    flow::join_node<flow::tuple<Ptr<cv::Mat>, Ptr<cv::Mat>>> joint(g);
    
    // post_detect: Takes the raw detection output, interperets it, and
    //              produces some meaningful results
    flow::function_node<flow::tuple<Ptr<cv::Mat>, Ptr<cv::Mat>>, Ptr<Detections>> post_detect(g, flow::serial,
        [&detector](const flow::tuple<Ptr<cv::Mat>, Ptr<cv::Mat>> input) -> Ptr<Detections> {
            std::cout << "START POST DETECT" << std::endl;
            Ptr<cv::Mat> original = std::get<0>(input);
            Ptr<cv::Mat> results  = std::get<1>(input);
            
            auto detections = detector.post_process(original, *results);
            std::cout << "END POST DETECT" << std::endl;
            return detections;
        }
    );
    
    // track: Tracks detections
    Tracker tracker(world_config);
    flow::function_node<Ptr<Detections>, flow::tuple<Ptr<WorldState>>> track(g, flow::serial,
        [&tracker, &world_config](const Ptr<Detections> detections) -> Ptr<WorldState>
        {
            std::cout << "START TRACK" << std::endl;
            Ptr<WorldState> state = Ptr<WorldState>(new WorldState(tracker.process(*detections)));
            std::cout << "END TRACK" << std::endl;
            
            std::cout << "START RENDER" << std::endl;

	    return state;
	}
     );

     flow::function_node<Ptr<flow::tuple<Ptr<WorldState>>, flow::tuple<Ptr<WorldState>, Ptr<cv::Mat>>> draw(g, flow::serial,
        [&tracker, &world_config, &display_queue](const Ptr<WorldState> state) -> flow::tuple<Ptr<WorldState>, Ptr<cv::Mat>>
	{
            // TODO figure out how to put this into it's own node
            // Trouble is that Tracker's data is temporary, so, if Tracker::draw()
            // is called latter, it may or may not be okay.
            Ptr<cv::Mat> display;
	    display = Ptr<cv::Mat>(new cv::Mat());
	    detections->get_frame()->copyTo(*display);
	    
	    detections->draw(*display);
	    tracker.draw(*display);
	    state->draw(*display);
	    world_config.draw(*display);
	    
	    display_queue.push(display);
                
            std::cout << "END RENDER" << std::endl;
            
            return flow::continue_msg()
        }
    );
    
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
        
    // Setup flow dependencis
    flow::make_edge(src,          throttle);
    flow::make_edge(throttle,     pre_detect);
    flow::make_edge(pre_detect,   detect);
    flow::make_edge(throttle,     flow::input_port<0>(joint));
    flow::make_edge(detect,       flow::input_port<1>(joint));
    flow::make_edge(joint,        post_detect);
    flow::make_edge(post_detect,  track);
    flow::make_edge(track,        draw);
    flow::make_edge(draw,         monitor);
    flow::make_edge(monitor,      throttle.decrement);
    
    // Begin running stuff
    src.activate();
    
    // The display code _must_ be run on the main thread. Thus, we pass
    // display frames here through a queue
    Ptr<cv::Mat> display_img;
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
    */
}


//
// Builds and runs a flow graph with the the given configeration options
//
void BusCounter::run_parallel(const NetConfigIR &net_config, const WorldConfig world_config)
{
    /*
    namespace flow = tbb::flow;
    using namespace cv;
    
    //
    // This code below produces the following flow graph:
    // 
    //            src
    //             | Ptr<cv::Mat>
    //             v
    //   +-->-- throttle
    //   |         | Ptr<cv::Mat>
    //   |         |
    //   |         +-------> pre_detect
    //   |         |            | continue_msg
    //   |         |            v
    //   |         |          detect
    //   |         v            | Ptr<cv::Mat>
    //   |       joint  <-------+
    //   |         | tuple<cv::Mat, cv::Mat>
    //   |         v
    //   |    post_detect
    //   |         | Ptr<Detections>
    //   |         v
    //   |    track_n_draw
    //   |         | Ptr<cv::Mat>
    //   |         v
    //   |       stream              
    //   |         | continue_msg
    //   |         v
    //   |       monitor
    //   |         | continue_msg
    //   +---------+
    //            
    
    // shared variables
    flow::graph g;
    tbb::concurrent_queue<Ptr<cv::Mat>> display_queue;
    bool quit = false;
    
    std::cout << "Initialise Video" << std::endl;
    
    // src: Reads images from the input stream.
    //      It will continuously try to push more images down the stream
    VideoCapture cap(input);
    std::cout << "Video inited" << std::endl;
    flow::source_node<Ptr<cv::Mat>> src(g,
        [&cap, &quit](Ptr<cv::Mat> &v) -> bool {
            std::cout << "START SRC" << std::endl;
            if (quit) {
                return false;
            }
            cv::Mat* frame = new cv::Mat();
            // read three times so that we effectively are running real time
            bool res = cap.read(*frame) 
                        && cap.read(*frame) 
                        && cap.read(*frame);
            if (res) {
                v = Ptr<cv::Mat>(frame);
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
    flow::limiter_node<Ptr<cv::Mat>> throttle(g, MAX_FRAMES);
    
    std::cout << "Init detector" << std::endl;
    
    // The detect nodes all world together through the Detector object.
    // Note that the detect node is the bottle-neck in the pipeline, thus,
    // as much as possible should be moved into pre_detect or post_detect.
    Detector detector(net_config);
    dnn::Net net = net_config.make_network();
    
    // pre_detect: Preprocesses the image into a 'blob' so it is ready 
    //             to feed into a detection algorithm
    flow::function_node<Ptr<cv::Mat>, flow::continue_msg> pre_detect(g, flow::serial,
        [&detector, &net](const Ptr<cv::Mat> input) -> flow::continue_msg {
            std::cout << "START PRE DETECT" << std::endl;
            detector.pre_process(*input, net);
            std::cout << "END PRE DETECT" << std::endl;
            return flow::continue_msg();
        }
    );
    
    // detect: Takes the preprocessed blob
    flow::function_node<flow::continue_msg, Ptr<cv::Mat>> detect(g, flow::serial,
        [&detector, &net](flow::continue_msg _) -> Ptr<cv::Mat> {
            std::cout << "START DETECT" << std::endl;
            auto detections = detector.process(net);
            std::cout << "END DETECT" << std::endl;
            return detections;
        }
    );
    
    // joint: combines the results from the original image and the detection data
    flow::join_node<flow::tuple<Ptr<cv::Mat>, Ptr<cv::Mat>>> joint(g);
    
    // post_detect: Takes the raw detection output, interperets it, and
    //              produces some meaningful results
    flow::function_node<flow::tuple<Ptr<cv::Mat>, Ptr<cv::Mat>>, Ptr<Detections>> post_detect(g, flow::serial,
        [&detector](const flow::tuple<Ptr<cv::Mat>, Ptr<cv::Mat>> input) -> Ptr<Detections> {
            std::cout << "START POST DETECT" << std::endl;
            Ptr<cv::Mat> original = std::get<0>(input);
            Ptr<cv::Mat> results  = std::get<1>(input);
            
            auto detections = detector.post_process(original, *results);
            std::cout << "END POST DETECT" << std::endl;
            return detections;
        }
    );
    
    std::cout << "Init tracker" << std::endl;
    
    // track: Tracks detections
    Tracker tracker(world_config);
    flow::function_node<Ptr<Detections>, flow::tuple<Ptr<WorldState>, Ptr<cv::Mat>>> track_n_draw(g, flow::serial,
        [&tracker, draw, &world_config, &display_queue](const Ptr<Detections> detections) -> flow::tuple<Ptr<WorldState>, Ptr<cv::Mat>>
        {
            std::cout << "START TRACK" << std::endl;
            Ptr<WorldState> state = Ptr<WorldState>(new WorldState(tracker.process(*detections)));
            std::cout << "END TRACK" << std::endl;
            
            std::cout << "START RENDER" << std::endl;
            // TODO figure out how to put this into it's own node
            // Trouble is that Tracker's data is temporary, so, if Tracker::draw()
            // is called latter, it may or may not be okay.
            Ptr<cv::Mat> display;
            if (draw)
            {
                display = Ptr<cv::Mat>(new cv::Mat());
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
    flow::function_node<flow::tuple<Ptr<WorldState>, Ptr<cv::Mat>>, flow::continue_msg> stream(g, flow::serial,
        [](const flow::tuple<Ptr<WorldState>, Ptr<cv::Mat>> input)
        {
            std::cout << "START STREAM" << std::endl;
            
            Ptr<WorldState> state = std::get<0>(input);
            Ptr<cv::Mat> display      = std::get<1>(input);
            
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
    Ptr<cv::Mat> display_img;
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
    */
}

//
// A single threaded version of run_parallel. Useful for debugging.
//
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
        Ptr<cv::Mat> frame = Ptr<cv::Mat>(new cv::Mat());
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
        cv::Mat display;
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
