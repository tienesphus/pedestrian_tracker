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
     *            src
     *             | Ptr<Mat>
     *             v
     *   +--> frame_limiter
     *   |         | Ptr<Mat>
     *   |         v
     *   |      detector
     *   |         | Ptr<Detections>
     *   |         v
     *   |      tracker
     *   |         | Ptr<WorldState>
     *   |         v
     *   |      drawer
     *   |         | Ptr<Mat>
     *   |         V
     *   |    video_streamer
     *   |         | continue_msg
     *   |         V
     *   |     tick_counter
     *   |         | continue_msg
     *   +---------+
     *
    */            
    flow::graph g;
    tbb::concurrent_queue<Ptr<Mat>> display_queue;
    
    VideoCapture cap(input);
    bool quit = false;
    
    // src: Reads images from the input stream.
    //      It will continuously try to push more images down the stream
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
    
    // frame_limiter: by default, infinite frames can be in the graph at one.
    //                This node will limit the graph so that only MAX_FRAMES
    //                will be processed at once. Without limiting, runnaway
    //                occurs (src will keep producing into an unlimited buffer)
    const int MAX_FRAMES = 2;
    flow::limiter_node<Ptr<Mat>> frame_limiter(g, MAX_FRAMES);
    
    // n_detector: Finds things in the image
    Detector detector(net_config);
    flow::function_node<Ptr<Mat>, Ptr<Detections>> n_detector(g, flow::serial,
        [&detector, draw](const Ptr<Mat> input) -> Ptr<Detections> {
            std::cout << "START DETECTIONS" << std::endl;
            auto detections = detector.process(input);
            if (draw)
                detections->draw();
            std::cout << "END DETECTIONS" << std::endl;
            return detections;
        }
    );
    
    // n_tracker: Tracks detections
    Tracker tracker(world_config);
    flow::function_node<Ptr<Detections>, Ptr<WorldState>> n_tracker(g, flow::serial,
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
    
    // drawer: Draws the state of the world to a Mat
    flow::function_node<Ptr<WorldState>, Ptr<Mat>> drawer(g, flow::serial,
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
    
    // video_streamer: Writes images to a stream
    flow::function_node<Ptr<Mat>, flow::continue_msg> video_streamer(g, flow::serial,
        [](const Ptr<Mat> input) {
            // TODO: stream these results somewhere
            // this should plug into the GStreamer stuff somehow...
            std::cout << "START STREAM" << std::endl;
            std::cout << "END STREAM" << std::endl;
            return flow::continue_msg();
        }
    );
    
    // tick_counter: Calculates the speed that the program is running at by moving average.
    int frame_count = 0;
    const int AVG_SIZE = 10;
    tbb::tick_count ticks[AVG_SIZE] = {tbb::tick_count::now()};
    
    flow::function_node<flow::continue_msg, flow::continue_msg> tick_counter(g, flow::serial,
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
    flow::make_edge(src,            frame_limiter);
    flow::make_edge(frame_limiter,  n_detector);
    flow::make_edge(n_detector,     n_tracker);
    flow::make_edge(n_tracker,      drawer);
    flow::make_edge(drawer,         video_streamer);
    flow::make_edge(video_streamer, tick_counter);
    flow::make_edge(tick_counter,   frame_limiter.decrement);
    
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
