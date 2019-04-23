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
void run_graph(const std::string& input, const NetConfigIR &net_config, const WorldConfig world_config) {
    
    using namespace tbb::flow;
    using namespace cv;
    
    /*
     * This code below produces the following Graph:
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
    graph g;
    
    VideoCapture cap(input);
    bool quit = false;
    int frame_count = 0;
    
    // src: Reads images from the input stream.
    //      It will continuously try to push more images down the stream
    source_node<Ptr<Mat>> src(g,
        [&cap, &quit, &frame_count](Ptr<Mat> &v) -> bool {
            std::cout << ("SOURCE frame" + std::to_string(frame_count++)) << std::endl;
            Mat* frame = new Mat();
            if (quit) {
                return false;
            }
            bool res = cap.read(*frame);
            if (res) {
                v = Ptr<Mat>(frame);
                return true;
            } else {
                return false;
            }
        }, false);
    
    // frame_limiter: by default, infinite frames can be in the graph at one.
    //                This node will limit the graph so that only MAX_FRAMES
    //                will be processed at once.
    const int MAX_FRAMES = 2;
    limiter_node<Ptr<Mat>> frame_limiter(g, MAX_FRAMES);
    
    // n_detector: Finds things in the image
    Detector detector(net_config);
    function_node<Ptr<Mat>, Ptr<Detections>> n_detector(g, serial,
        [&detector](const Ptr<Mat> input) {
            return detector.process(input);
        }
    );
    
    // n_tracker: Tracks detections
    Tracker tracker(world_config);
    function_node<Ptr<Detections>, Ptr<WorldState>> n_tracker(g, serial,
        [&tracker](const Ptr<Detections> detections) -> Ptr<WorldState> {
            return tracker.process(*detections);
        }
    );
    
    // drawer: Draws the state of the world to a Mat
    function_node<Ptr<WorldState>, Ptr<Mat>> drawer(g, serial,
        [](const Ptr<WorldState> world){
            Ptr<Mat> display = Ptr<Mat>(new Mat());
            world->draw(*display);
            return display;
        }
    );
    
    // display: Displays a Mat on the screen
    function_node<Ptr<Mat>, continue_msg> display(g, serial, 
        [&quit](const Ptr<Mat> frame) -> continue_msg {
            // TODO: only the main thread should call GUI functions.
            // Calling them here causes error messages on shutdown
            if (!quit) {                
                imshow("output", *frame);
                int key = waitKey(1);
                if (key == 'q') {
                    quit = true;
                    destroyWindow("output");
                }
            }
            return continue_msg();
        }
    );
    
    // video_streamer: Writes images to a stream
    function_node<Ptr<Mat>, continue_msg> video_streamer(g, serial,
        [](const Ptr<Mat> input) {
            // TODO: stream these results somewhere
            // this should plug into the GStreamer stuff somehow...
            return continue_msg();
        }
    );
    
    // limiter_joint: Ensures that both the display code and the streaming
    //                code can't fall behind. IDK if this is actually nesscessary;
    //                maybe just one node's output will work fine?
    join_node<tuple<continue_msg, continue_msg>> limiter_joint(g);

    // limitter_fb: Converter from a tuple<continue_msg> into a continue_msg
    //              so it can be plugged into the frame_limiter node
    function_node<tuple<continue_msg, continue_msg>, continue_msg> limiter_fb(g, serial,
        [](const tuple<continue_msg, continue_msg>) {
            return continue_msg();
        }
    );
    
    make_edge(src, frame_limiter);
    make_edge(frame_limiter, n_detector);
    make_edge(n_detector, n_tracker);
    make_edge(n_tracker, drawer);
    make_edge(drawer, display);
    make_edge(drawer, video_streamer);
    make_edge(display, input_port<0>(limiter_joint));
    make_edge(video_streamer, input_port<1>(limiter_joint));
    make_edge(limiter_joint, limiter_fb);
    make_edge(limiter_fb, frame_limiter.decrement);
    
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
    
    WorldConfig world_config = WorldConfig(
        Line{
            cv::Point{0, 0},
            cv::Point{0, 0}
        },
        Line{
            cv::Point{0, 0},
            cv::Point{0, 0}
        },
        Line{
            cv::Point{0, 0},
            cv::Point{0, 0}
        },
        Line{
            cv::Point{0, 0},
            cv::Point{0, 0}
        }
    );
    
    std::string input = "/home/cv/code/samplevideos/pi3_20181213/2018-12-13--08-26-02--snippit-1.mp4";
    
    run_graph(input, net_config, world_config);
    
    return 0;
} 
