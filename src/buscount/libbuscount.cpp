#include <utility>

#include "libbuscount.hpp"
#include "tick_counter.hpp"
#include "cv_utils.hpp"

#include <iostream>
#include <vector>
#include <unistd.h>

#include <tbb/concurrent_queue.h>
#include <tbb/flow_graph.h>
#include <tbb/mutex.h>
#include <tbb/tick_count.h>

#include <opencv2/highgui.hpp>

/**
 * A simple logging class that logs when it is created and when it is destroyed.
 */
class ScopeLog {
public:
    explicit ScopeLog(std::string tag):tag(std::move(tag)) {
        std::cout << "START " << this->tag << std::endl;
    }
    ~ScopeLog() {
        std::cout << "END " << this->tag << std::endl;
    }

private:
    std::string tag;
};

BusCounter::BusCounter(
        Detector& detector,
        Tracker& tracker,
        WorldConfig &wconf,
        std::function<BusCounter::src_cb_t> src,
        std::function<BusCounter::dest_cb_t> dest,
        std::function<BusCounter::test_exit_t> test_exit,
        std::function<BusCounter::event_handle_t > event_handle
    ) :
        _src(std::move(src)),
        _dest(std::move(dest)),
        _test_exit(std::move(test_exit)),
        _event_handle(std::move(event_handle)),
        _detector(detector),
        _tracker(tracker),
        _world_config(wconf),
        inside_count(0),
        outside_count(0)
{
}

void BusCounter::run(RunStyle style, bool draw)
{
    if(style == RUN_PARALLEL)
        run_parallel(draw);
    else
        run_serial(draw);
}

// abbreviation
template <class T>
using ptr = std::shared_ptr<T>;

void BusCounter::handle_events(const std::vector<Event>& events)
{
    for (Event e : events) {
        // handle the events internally
        switch (e) {
            case COUNT_IN:  inside_count++;  break;
            case COUNT_OUT: outside_count++; break;
            case BACK_IN:   outside_count--; break;
            case BACK_OUT:  inside_count--;  break;
            default:
                throw std::logic_error("Unknown event type");
        }

        // handle the event externally
        _event_handle(e);
    }
}

//
// Builds and runs a flow graph with the the given configuration options
//
void BusCounter::run_parallel(bool do_draw)
{
    typedef cv::Mat Mat;
    typedef ptr<Mat> PtrMat;
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
     *   |        +-+---> start_detection
     *   |        |           | Detector::intermediate
     *   |        |           v
     *   |        |       wait_detection
     *   |        |           | Ptr<Detections>
     *   |        |   +-------+
     *   |        v   v
     *   |        joint
     *   |          | tuple<Mat, Detections>
     *   |          v
     *   |        track
     *   |          | tuple<Mat, Detections, WorldState>
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
    std::atomic_bool stop(false);
    tbb::concurrent_queue<PtrMat> display_queue;
    TickCounter<> counter;

    std::cout << "Init functions" << std::endl;

    flow::source_node<PtrMat> src_node(g,
            [this, &stop](PtrMat &frame) -> bool {
                ScopeLog log("SRC");
                nonstd::optional<Mat> got_frame = this->_src();
                if (got_frame.has_value()) {
                    frame = cv::makePtr<Mat>(*got_frame);
                } else {
                    stop = true;
                }
                return !stop;
            }, false // false; don't start until we call src_node.activate below
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
    flow::function_node<PtrMat, Detector::intermediate> start_detection(g, flow::serial,
            [this](PtrMat mat) -> auto {
                ScopeLog log("DETECTION");
                return this->_detector.start_async(*mat);
            }
    );

    // detect: Takes the preprocessed blob
    flow::function_node<Detector::intermediate, ptr<Detections>> wait_detection(g, flow::serial,
            [this](Detector::intermediate request) -> ptr<Detections> {
                ScopeLog log("WAIT DETECTION");
                return cv::makePtr<Detections>(this->_detector.wait_async(request));
            }
    );

    // joint: combines the results from the original image and the detection data
    typedef std::tuple<PtrMat, ptr<Detections>> joint_output;
    flow::join_node<joint_output> joint_node(g);

    // track: Tracks detections
    typedef std::tuple<PtrMat, ptr<Detections>> track_output;
    flow::function_node<joint_output, track_output> track_node(g, flow::serial,
            [this](joint_output input) -> auto {
                ScopeLog log("TRACK");
                auto frame = std::get<0>(input);
                auto detections = std::get<1>(input);
                auto events = this->_tracker.process(*detections, *frame);
                handle_events(events);
                return input;
            }
    );

    flow::function_node<track_output, PtrMat> draw_node(g, flow::serial,
            [this](track_output input) -> auto {
                ScopeLog log("DRAW");
                auto frame = std::get<0>(input);
                auto detections = std::get<1>(input);
                detections->draw(*frame);
                geom::draw_world_count(*frame, inside_count, outside_count);
                // TODO possible concurrency issue of tracker updating before getting drawn
                this->_tracker.draw(*frame);
                geom::draw_world_config(*frame, _world_config);

                return frame;
            }
    );

    flow::function_node<track_output, PtrMat> no_draw_node(g, flow::serial,
            [](track_output input) -> auto {
                ScopeLog log("NO DRAW");
                return std::get<0>(input);
            }
    );

    // stream: Writes images to a stream
    flow::function_node<PtrMat> stream_node(g, flow::serial,
            [&display_queue](PtrMat input) -> void {
                ScopeLog log("DISPLAY");
                display_queue.push(input);
            }
    );

    // stream: Writes images to a stream
    flow::function_node<flow::continue_msg> fps_node(g, flow::serial,
            [&counter](flow::continue_msg) -> void {
                ScopeLog log("TICK");
                auto fps = counter.process_tick();
                std::cout << "FPS: " << (fps ? *fps : -1) << std::endl;
            }
    );
    
    std::cout << "making edges" << std::endl;

    // Setup flow dependencies
    flow::make_edge(src_node,         throttle_node);
    flow::make_edge(throttle_node,    start_detection);
    flow::make_edge(start_detection,  wait_detection);
    flow::make_edge(throttle_node,    flow::input_port<0>(joint_node));
    flow::make_edge(wait_detection,   flow::input_port<1>(joint_node));
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
    flow::make_edge(stream_node,  fps_node);
    flow::make_edge(fps_node,     throttle_node.decrement);
    
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

    TickCounter<> counter;
    while (true) {

        // Read at least once. Skip if source FPS is different from target FPS
        nonstd::optional<cv::Mat> got_frame = _src();
        if (!got_frame)
            break;

        auto frame = *got_frame;
        auto detections = _detector.process(frame);
        auto events = _tracker.process(detections, frame);

        handle_events(events);

        if (do_draw) {
            detections.draw(frame);
            _tracker.draw(frame);
            geom::draw_world_count(frame, inside_count, outside_count);
            geom::draw_world_config(frame, _world_config);
        }

        auto fps = counter.process_tick();
        std::cout << "FPS: " << (fps ? *fps : -1) << std::endl;

        _dest(frame);
        if (_test_exit())
            break;
    }
}
