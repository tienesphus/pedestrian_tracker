#include "libbuscount.hpp"
#include "tick_counter.hpp"

#include <iostream>
#include <vector>
#include <unistd.h>

#include <tbb/concurrent_queue.h>
#include <tbb/flow_graph.h>
#include <tbb/mutex.h>
#include <tbb/tick_count.h>

#include <opencv2/highgui.hpp>

template <class Func>
auto log_lambda(const std::string& name, Func f) {
    return [name, f](auto ... args) -> decltype(f(args...)) {
        struct Logger {
            const std::string* name;
            explicit Logger(const std::string* name):name(name) { std::cout << "START " << name << std::endl; }
            ~Logger() { std::cout << "STOP " << name << std::endl; }
        };
        Logger log(&name);

        return f(args...);
    };
}

BusCounter::BusCounter(
        Detector& detector,
        Tracker& tracker,
        WorldConfig &wconf,
        std::function<BusCounter::src_cb_t> src,
        std::function<BusCounter::dest_cb_t> dest,
        std::function<BusCounter::test_exit_t> test_exit
    ) :
        _src(std::move(src)),
        _dest(std::move(dest)),
        _test_exit(std::move(test_exit)),
        _detector(detector),
        _tracker(tracker),
        _world_config(wconf)
{
}

void BusCounter::run(RunStyle style, bool draw)
{
    if(style == RUN_PARALLEL)
        run_parallel(draw);
    else
        run_serial(draw);
}

template <class T>
using ptr = std::shared_ptr<T>;

//
// Builds and runs a flow graph with the the given configuration options
//
void BusCounter::run_parallel(bool do_draw)
{
    typedef cv::Mat Mat;
    typedef cv::Ptr<Mat> PtrMat;
    namespace flow = tbb::flow;

    /*
     * This code below produces the following flow graph:
     * 
     *             src
     *              | Ptr<Mat>
     *              v
     *   +-->---throttle
     *   |          | Ptr<Mat>
     *   |          |
     *   |        +-+---> pre_detect
     *   |        |           | continue_msg
     *   |        |           v
     *   |        |         detect
     *   |        |           | Ptr<Mat>
     *   |        |           v
     *   |        |        post_detect
     *   |        |           | Ptr<Detections>
     *   |        |   +-------+
     *   |        v   v
     *   |        joint
     *   |          | tuple<Mat, Detections>
     *   |          v
     *   |        track
     *   |          | tuple<Mat, Detectons, WorldState>
     *   |       ...+....
     *   |       :      :
     *   |     draw    no_draw
     *   |       :      :
     *   |       :..+...:
     *   |          |  Ptr<Mat>
     *   |          v
     *   |        stream              
     *   |          | continue_msg
     *   |          v
     *   |        monitor
     *   |          | continue_msg
     *   +----------+
     */            
    
    // shared variables
    flow::graph g;
    std::atomic_bool stop = false;
    tbb::concurrent_queue<PtrMat> display_queue;

    std::cout << "Init functions" << std::endl;

    flow::source_node<PtrMat> src_node(g, log_lambda("SRC",
             [this, &stop](PtrMat &frame) -> bool {
                 std::optional<Mat> got_frame = this->_src();
                 if (got_frame.has_value()) {
                     frame = cv::makePtr<Mat>(*got_frame);
                 } else {
                     stop = true;
                 }
                 return !stop;
             }), false // false; don't start until we call src_node.activate below
    );


    std::cout << "Init limiter" << std::endl;

    // throttle: by default, infinite frames can be in the graph at one.
    //           This node will limit the graph so that only MAX_FRAMES
    //           will be processed at once. Without limiting, runnaway
    //           occurs (src will keep producing into an unlimited buffer)
    const int MAX_FRAMES = 2;
    flow::limiter_node<PtrMat> throttle_node(g, MAX_FRAMES);


    // The detect nodes all world together through the Detector object.
    // Note that the detect node is the bottle-neck in the pipeline, thus,
    // as much as possible should be moved into pre_detect or post_detect.
    
    // pre_detect: Preprocesses the image into a 'blob' so it is ready 
    //             to feed into a detection algorithm
    flow::function_node<PtrMat, std::shared_future<Mat>> start_detection(g, flow::serial, log_lambda("START DETECTION",
            [this](PtrMat mat) -> auto {
                return this->_detector.start_async(*mat);
            }
    ));

    // detect: Takes the preprocessed blob
    flow::function_node<std::shared_future<Mat>, PtrMat> wait_detection(g, flow::serial, log_lambda("WAIT DETECTION",
            [this](std::shared_future<Mat> request) -> PtrMat {
                return cv::makePtr<Mat>(this->_detector.wait_async(request));
            }
    ));

    // post_detect: Takes the raw detection output, interprets it, and
    //              produces some meaningful results
    flow::function_node<PtrMat, ptr<Detections>> post_detection(g, flow::serial, log_lambda("POST DETECT",
            [this](PtrMat input) -> auto {
                return std::make_shared<Detections>(this->_detector.post_process(*input));
            }
    ));


    // joint: combines the results from the original image and the detection data
    typedef std::tuple<PtrMat, ptr<Detections>> joint_output;
    flow::join_node<joint_output> joint_node(g);

    // track: Tracks detections
    typedef std::tuple<PtrMat, ptr<Detections>, ptr<WorldState>> track_output;
    flow::function_node<joint_output, track_output> track_node(g, flow::serial, log_lambda("TRACK",
            [this](joint_output input) -> auto {
                const auto& [frame, detections] = input;
                return std::make_tuple(frame, detections, std::make_shared<WorldState>(this->_tracker.process(*detections, *frame)));
            }
    ));

    flow::function_node<track_output, PtrMat> draw_node(g, flow::serial, log_lambda("DRAW",
            [this](track_output input) -> auto {
                auto& [frame, detections, state] = input;

                detections->draw(*frame);
                state->draw(*frame);
                // TODO possible concurrency issue of tracker updating before getting drawn
                this->_tracker.draw(*frame);
                this->_world_config.draw(*frame);

                return frame;
            }
    ));

    flow::function_node<track_output, PtrMat> no_draw_node(g, flow::serial, log_lambda("NO DRAW",
            [](track_output input) -> auto {
                return std::get<0>(input);
            }
    ));

    // stream: Writes images to a stream
    flow::function_node<PtrMat> stream_node(g, flow::serial, log_lambda("DISPLAY",
            [&display_queue](PtrMat input) -> void {
                display_queue.push(input);
            }
    ));
    
    std::cout << "making edges" << std::endl;

    // Setup flow dependencies
    flow::make_edge(src_node,         throttle_node);
    flow::make_edge(throttle_node,    start_detection);
    flow::make_edge(start_detection,  wait_detection);
    flow::make_edge(wait_detection,   post_detection);
    flow::make_edge(throttle_node,    flow::input_port<0>(joint_node));
    flow::make_edge(post_detection,   flow::input_port<1>(joint_node));
    flow::make_edge(joint_node,       track_node);

    if (do_draw)
    {
        flow::make_edge(track_node, draw_node);
        flow::make_edge(draw_node, stream_node);
    }
    else
    {
        flow::make_edge(track_node, no_draw_node);
        flow::make_edge(no_draw_node, stream_node);
    }

    flow::make_edge(stream_node, throttle_node.decrement);
    
    // Begin running stuff
    std::cout << "Starting video" << std::endl;
    src_node.activate();
    std::cout << "Video started" << std::endl;
    
    // The display code _must_ be run on the main thread. Thus, we pass
    // display frames here through a queue
    while (!stop)
    {
        PtrMat display_img;
        if (display_queue.try_pop(display_img))
            _dest(*display_img);

        // TODO stop display code hogging CPU
        // it actually doesn't currently because _test_exit() calls wait_key()
        // but that is very brittle.
        if (_test_exit()) {
            stop = true;
            break;
        }
    }
    
    // Allow the last few frames to pass through
    // This actually isn't needed since g's destructor will call it,
    // but I like to have it explicit.
    g.wait_for_all();
}

//
// A single threaded version of run_parallel. Useful for debugging non-threading related issues.
//
void BusCounter::run_serial(bool do_draw)
{
    while (true) {

        // Read at least once. Skip if source FPS is different from target FPS
        std::optional<cv::Mat> got_frame = _src();
        if (!got_frame)
            break;

        auto frame = *got_frame;
        auto detections = _detector.process(frame);
        auto state = _tracker.process(detections, frame);
        
        if (do_draw) {
            detections.draw(frame);
            _tracker.draw(frame);
            state.draw(frame);
            _world_config.draw(frame);
        }

        _dest(frame);
        if (_test_exit())
            break;
    }
}
