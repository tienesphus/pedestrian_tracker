#include <iostream>
#include <vector>
#include <unistd.h>

#include "tbb/flow_graph.h"
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
     *   |         +------------------+
     *   |         V                  V
     *   |      display        video_streamer
     *   |         | continue_msg     | continue_msg         
     *   |         |                  |
     *   |         +---------+--------+
     *   |                   V
     *   |              limitter_joint
     *   |                   | tuple<continue_msg, continue_msg>
     *   |                   V
     *   |              limiter_fb
     *   |                   | continue_msg
     *   +-------------------+
     *
    */            
    flow::graph g;
    
    VideoCapture cap(input);
    bool quit = false;
    int frame_count = 0;
    
    // src: Reads images from the input stream.
    //      It will continuously try to push more images down the stream
    flow::source_node<Ptr<Mat>> src(g,
        [&cap, &quit, &frame_count](Ptr<Mat> &v) -> bool {
            std::cout << ("SOURCE frame" + std::to_string(frame_count++)) << std::endl;
            Mat* frame = new Mat();
            if (quit) {
                return false;
            }
            bool res = cap.read(*frame);
            if (res) {
                v = Ptr<Mat>(frame);
                std::cout << ("SOURCE FIN") << std::endl;
                return true;
            } else {
                return false;
            }
        }, false);
    
    // frame_limiter: by default, infinite frames can be in the graph at one.
    //                This node will limit the graph so that only MAX_FRAMES
    //                will be processed at once.
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
        [&world_config](Ptr<WorldState> world) -> Ptr<Mat> {
            std::cout << "START DRAWER" << std::endl;
            world->draw();
            world_config.draw(*(world->display));
            std::cout << "END DRAWER" << std::endl;
            return world->display;
        }
    );
    
    // display: Displays a Mat on the screen
    flow::function_node<Ptr<Mat>, flow::continue_msg> display(g, flow::serial, 
        [&quit](const Ptr<Mat> frame) -> flow::continue_msg {
            // TODO: only the main thread should call GUI functions.
            // Calling them here causes error messages on shutdown
            // Also - it completely locks up. Don't know why that is though...
            std::cout << "START DISPLAY" << std::endl;
            if (!quit) {
                std::cout << "  imshow()" << std::endl;
                imshow("output", *frame);
                std::cout << "  waitKey()" << std::endl;
                int key = waitKey(500);
                std::cout << "  done" << std::endl;
                if (key == 'q') {
                    std::cout << "  QUIT REGISTERED" << std::endl;
                    quit = true;
                    std::cout << "  destroyWindow()" << std::endl;
                    destroyWindow("output");
                }
            }
            std::cout << "END DISPLAY" << std::endl;
            return flow::continue_msg();
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
    
    // limiter_joint: Ensures that both the display code and the streaming
    //                code can't fall behind. IDK if this is actually nesscessary;
    //                maybe just one node's output will work fine?
    flow::join_node<flow::tuple<flow::continue_msg, flow::continue_msg>> limiter_joint(g);

    // limitter_fb: Converter from a tuple<continue_msg> into a continue_msg
    //              so it can be plugged into the frame_limiter node
    flow::function_node<flow::tuple<flow::continue_msg, flow::continue_msg>, flow::continue_msg> limiter_fb(g, flow::serial,
        [](const flow::tuple<flow::continue_msg, flow::continue_msg>) {
            std::cout << "START LIMITER_FB" << std::endl;
            std::cout << "END LIMITER_FB" << std::endl;
            return flow::continue_msg();
        }
    );
    
    flow::make_edge(src,            frame_limiter);
    flow::make_edge(frame_limiter,  n_detector);
    flow::make_edge(n_detector,     n_tracker);
    flow::make_edge(n_tracker,      drawer);
    flow::make_edge(drawer,         display);
    flow::make_edge(drawer,         video_streamer);
    flow::make_edge(display,        flow::input_port<0>(limiter_joint));
    flow::make_edge(video_streamer, flow::input_port<1>(limiter_joint));
    flow::make_edge(limiter_joint,  limiter_fb);
    flow::make_edge(limiter_fb,     frame_limiter.decrement);
    
    src.activate();
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
