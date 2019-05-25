#include <iostream>
#include <vector>

#include <tbb/concurrent_queue.h>
#include <tbb/flow_graph.h>
#include <tbb/mutex.h>
#include <tbb/tick_count.h>

#include <opencv2/highgui.hpp>

#include "libbuscount.hpp"

using namespace std;
using namespace tbb;

class TickCounterImpl
{
    size_t avg_size;
    cv::Ptr<float> s_per_frame;

    float fps;
    long frame_count;
    tick_count prev;

    tbb::mutex fps_mutex;

public:
    TickCounterImpl(const size_t avg_size):
        avg_size(avg_size),
        s_per_frame(cv::Ptr<float>(new float[avg_size])),
        fps(30.0),
        frame_count(0),
        prev(tick_count::now())
    {
        // Reset ms_per_frame. Default to 1 second...because that's a sensibly large value.
        for(size_t i = 0; i < avg_size; i++)
            *((float*)s_per_frame+i) = 1.0 / 30.0;
    }
    
    TickCounterImpl(const TickCounterImpl &other)
    {
        (*this) = other;
    }

    float getFps()
    {
        tbb::mutex::scoped_lock lock(fps_mutex);
        return fps;
    }

    TickCounterImpl& operator=(const TickCounterImpl &other)
    {
        this->prev = other.prev;
        this->frame_count = other.frame_count;
        this->avg_size = other.avg_size;
        this->s_per_frame = other.s_per_frame; 

        return *this;
    }

    void operator()(flow::continue_msg)
    {
        // This is called once per frame, so we only need to calculate the time between
        // this frame and the previous frame. No need to save the absolute time of every
        // frame.

        std::cout << "START TICK COUNTER" << std::endl;
        
        tick_count now = tick_count::now();
        s_per_frame[frame_count % avg_size] = (now - prev).seconds();

        frame_count++;
        prev = now;

        double secs = 0;
        for(size_t i = 0; i < avg_size; i++)
            secs += s_per_frame[i];
        
        {
            tbb::mutex::scoped_lock lock(fps_mutex);
            fps = avg_size / secs;
        }

        std::cout << "END TICK  frame: " << frame_count << " fps: " << fps << std::endl;
    }
};

class TickCounter
{
    cv::Ptr<TickCounterImpl> impl;
public:
    TickCounter(const size_t avg_size): impl(cv::makePtr<TickCounterImpl>(avg_size))
    {
    }

    TickCounter(const TickCounter &other)
    {
        (*this) = other;
    }

    float getFps() const
    {
        return impl->getFps();
    }

    void operator()(flow::continue_msg _)
    {
        (*impl)(_);
    }
};

BusCounter::BusCounter(
        NetConfigIR &nconf,
        WorldConfig &wconf,
        std::function<BusCounter::src_cb_t> src,
        std::function<BusCounter::dest_cb_t> dest,
        std::function<BusCounter::test_exit_t> test_exit
    ) :
        _src(src),
        _dest(dest),
        _test_exit(test_exit),
        _detector(nconf),
        _world_config(wconf),
        _tracker(wconf)
{
}

//
// Builds and runs a graph with the the given configeration options
//

// pre_detect: Preprocesses the image into a 'blob' so it is ready 
//             to feed into a detection algorithm
cv::Ptr<cv::Mat> BusCounter::pre_detect(const cv::Ptr<cv::Mat> frame)
{
    cout << "START PRE DETECT" << endl;
    auto blob = _detector.pre_process(*frame);
    cout << "END PRE DETECT" << endl;

    return blob;
}

cv::Ptr<cv::Mat> BusCounter::detect(const cv::Ptr<cv::Mat> blob)
{
    std::cout << "START DETECT" << std::endl;
    auto results = _detector.process(*blob);
    std::cout << "END DETECT" << std::endl;

    return results;
}

std::tuple<cv::Ptr<cv::Mat>, cv::Ptr<Detections>> BusCounter::post_detect(const std::tuple<cv::Ptr<cv::Mat>, cv::Ptr<cv::Mat>> input)
{
    std::cout << "START POST DETECT" << std::endl;
    cv::Ptr<cv::Mat> original = std::get<0>(input);
    cv::Ptr<cv::Mat> results  = std::get<1>(input);
    
    auto detections = _detector.post_process(*original, *results);
    std::cout << "END POST DETECT" << std::endl;

    return std::tuple<cv::Ptr<cv::Mat>, cv::Ptr<Detections>>(original, detections);
}

std::tuple<cv::Ptr<WorldState>, cv::Ptr<cv::Mat>, cv::Ptr<Detections>> BusCounter::track(std::tuple<cv::Ptr<cv::Mat> , cv::Ptr<Detections>> input)
{
    std::cout << "START TRACK" << std::endl;
    cv::Ptr<cv::Mat> frame = std::get<0>(input);
    cv::Ptr<Detections> detections  = std::get<1>(input);

    cv::Ptr<WorldState> state = cv::makePtr<WorldState>(_tracker.process(*detections));
    std::cout << "END TRACK" << std::endl;
    
    return std::tuple<cv::Ptr<WorldState>, cv::Ptr<cv::Mat>, cv::Ptr<Detections>>(state, frame, detections);
}

cv::Ptr<cv::Mat> BusCounter::draw(std::tuple<cv::Ptr<WorldState>, cv::Ptr<cv::Mat>, cv::Ptr<Detections>> input)
{
    std::cout << "START DRAW" << std::endl;

    cv::Ptr<WorldState> state = std::get<0>(input);
    cv::Ptr<cv::Mat> frame = std::get<1>(input);
    cv::Ptr<Detections> detections = std::get<2>(input);

    detections->draw(*frame);
    _tracker.draw(*frame);
    state->draw(*frame);
    _world_config.draw(*frame);
    
    std::cout << "END DRAW" << std::endl;

    return frame;
}

cv::Ptr<cv::Mat> BusCounter::no_draw(std::tuple<cv::Ptr<WorldState>, cv::Ptr<cv::Mat>, cv::Ptr<Detections>> input)
{
    return std::get<1>(input);
}


void BusCounter::run(RunStyle style, double src_fps, bool draw)
{
    if(style == RUN_PARALLEL)
        run_parallel(src_fps, draw);
    else
        run_serial(src_fps, draw);
}


//
// Builds and runs a flow graph with the the given configeration options
//
void BusCounter::run_parallel(double src_fps, bool do_draw)
{
    using namespace cv;
    
    /*
     * This code below produces the following flow graph:
     * 
     *             src
     *              | Ptr<cv::Mat>
     *              v
     *   +-->---throttle
     *   |          | Ptr<cv::Mat>
     *   |          |
     *   |        +-+---> pre_detect
     *   |        |           | continue_msg
     *   |        |           v
     *   |        |         detect
     *   |        |           | Ptr<cv::Mat>
     *   |        |   +-------+
     *   |        |   |
     *   |        v   v
     *   |        joint
     *   |          | tuple<cv::Mat, cv::Mat>
     *   |          v
     *   |     post_detect
     *   |          | Ptr<Detections>
     *   |          v
     *   |        track
     *   |          | tuple<Ptr<WorldState> Ptr<cv::Mat>>
     *   |       ...+....
     *   |       :      :
     *   |     draw    no_draw
     *   |       :      :
     *   |       :..+...:
     *   |          |  Ptr<cv::Mat>
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
    TickCounter tickCounter(10);
    tbb::mutex quit_mutex;
    bool quit = false;
    
    std::cout << "Initialise Video" << std::endl;
    
    // src: Reads images from the input stream.
    //      It will continuously try to push more images down the stream
    std::cout << "Video inited" << std::endl;
    flow::source_node<Ptr<cv::Mat>> src_node(g,
            [this, &quit_mutex, src_fps, &tickCounter, &quit](Ptr<cv::Mat> &frame)
            {
                bool frame_returned = _src(frame);
                int skip;

                skip = src_fps / tickCounter.getFps();

                for (int i = 0; i <= skip; i++)
                {
                    if (!(frame_returned = _src(frame)))
                        break;
                }

                {
                    tbb::mutex::scoped_lock lock(quit_mutex);
                    return !(quit = (quit || !frame_returned));
                }

            }, false);

    std::cout << "Init limiter" << std::endl;
    
    // throttle: by default, infinite frames can be in the graph at one.
    //           This node will limit the graph so that only MAX_FRAMES
    //           will be processed at once. Without limiting, runnaway
    //           occurs (src will keep producing into an unlimited buffer)
    const int MAX_FRAMES = 2;
    flow::limiter_node<Ptr<cv::Mat>> throttle_node(g, MAX_FRAMES);
    
    std::cout << "Init detector" << std::endl;
    
    // The detect nodes all world together through the Detector object.
    // Note that the detect node is the bottle-neck in the pipeline, thus,
    // as much as possible should be moved into pre_detect or post_detect.
    
    // pre_detect: Preprocesses the image into a 'blob' so it is ready 
    //             to feed into a detection algorithm
    flow::function_node<Ptr<cv::Mat>, Ptr<cv::Mat>>
        pre_detect_node(g, flow::serial, bind(&BusCounter::pre_detect, this, placeholders::_1));
    
    // detect: Takes the preprocessed blob
    flow::function_node<Ptr<cv::Mat>, cv::Ptr<cv::Mat>>
        detect_node(g, flow::serial, bind(&BusCounter::detect, this, placeholders::_1));
    
    // joint: combines the results from the original image and the detection data
    flow::join_node<std::tuple<Ptr<cv::Mat>, Ptr<cv::Mat>>> joint_node(g);
    
    // post_detect: Takes the raw detection output, interperets it, and
    //              produces some meaningful results
    flow::function_node<tuple<Ptr<Mat>, Ptr<Mat>>, tuple<Ptr<Mat>, Ptr<Detections>>>
        post_detect_node(g, flow::serial, bind(&BusCounter::post_detect, this, placeholders::_1));
    
    std::cout << "Init tracker" << std::endl;
    
    // track: Tracks detections
    flow::function_node<tuple<Ptr<Mat>, Ptr<Detections>>, tuple<Ptr<WorldState>, Ptr<Mat>, Ptr<Detections>>>
        track_node(g, flow::serial, bind(&BusCounter::track, this, placeholders::_1));

    flow::function_node<std::tuple<Ptr<WorldState>, Ptr<Mat>, Ptr<Detections>>, Ptr<Mat>>
        draw_node(g, flow::serial, bind(&BusCounter::draw, this, placeholders::_1));

    flow::function_node<std::tuple<Ptr<WorldState>, Ptr<Mat>, Ptr<Detections>>, Ptr<cv::Mat>>
        no_draw_node(g, flow::serial, bind(&BusCounter::no_draw, this, placeholders::_1));

    std::cout << "Init writter" << std::endl;
    
    // stream: Writes images to a stream
    concurrent_queue<Ptr<cv::Mat>> display_queue;
    flow::function_node<Ptr<cv::Mat>> stream_node(g, flow::serial,
        [&display_queue](Ptr<cv::Mat> input) -> void
        {
            display_queue.push(input);
            cout << "Pushed to queue" << endl;
        }
    );
    
    std::cout << "Init frame count" << std::endl;
    
    // monitor: Calculates the speed that the program is running at by moving average.
    flow::continue_node<flow::continue_msg> tick_count_node(g, flow::serial, tickCounter);
        
    std::cout << "making edges" << std::endl;
        
    // Setup flow dependencis
    flow::make_edge(src_node,         throttle_node);
    flow::make_edge(throttle_node,    pre_detect_node);
    flow::make_edge(pre_detect_node,  detect_node);
    flow::make_edge(throttle_node,    flow::input_port<0>(joint_node));
    flow::make_edge(detect_node,      flow::input_port<1>(joint_node));
    flow::make_edge(joint_node,       post_detect_node);
    flow::make_edge(post_detect_node, track_node);

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

    // Performance isn't great when adding this here. For some
    // strange reason, adding manual calls to "try_put" within
    // the loop below doubles the frame rate.
    //flow::make_edge(stream_node, tick_count_node);
    //flow::make_edge(stream_node, throttle_node.decrement);
    
    // Begin running stuff
    std::cout << "Starting video" << std::endl;
    src_node.activate();
    std::cout << "Video started" << std::endl;
    
    // The display code _must_ be run on the main thread. Thus, we pass
    // display frames here through a queue
    while (!quit)
    {
        Ptr<cv::Mat> display_img;
        if (display_queue.try_pop(display_img))
        {
            // Calling try_put manually seems to double framerate. Will
            // keep it this way for now.
            tick_count_node.try_put(flow::continue_msg());
            throttle_node.decrement.try_put(flow::continue_msg());

            _dest(display_img);
        }

        bool should_exit = _test_exit();

        {
            tbb::mutex::scoped_lock lock(quit_mutex);
            quit = quit || should_exit;
        }
    }
    
    // Allow the last few frames to pass through
    // If this is not done, invalid memory can occur
    g.wait_for_all();
}

//
// A single threaded version of run_parallel. Useful for debugging non-threading related issues.
//
void BusCounter::run_serial(double src_fps, bool do_draw)
{
    
    using namespace cv;
    
    cout << "Init tracker" << endl;
    
    cout << "Init frame count" << endl;
    TickCounter tickCounter(10);
    
    while (true) {

        Ptr<Mat> frame = makePtr<Mat>();

        // Read at least once. Skip if source FPS is different from target FPS
        bool got_frame = true;
        int skip = src_fps / tickCounter.getFps();
        for (int i = 0; i <= skip; i++)
        {
            got_frame =  _src(frame);
        }

        if (!got_frame) break;
    
        auto blob = pre_detect(frame);

        auto net_result = detect(blob);
        auto detections = post_detect(std::tuple<Ptr<Mat>&, Ptr<Mat>&>(frame, net_result));
        auto state = track(detections);
        
        if (do_draw)
        {
            frame = draw(state);
        }
        else
        {
            frame = no_draw(state);
        }
        
        cout << "START DISPLAY" << endl;
        _dest(frame);
        cout << "END DISPLAY" << endl;

        tickCounter(flow::continue_msg());

        if (_test_exit())
        {
            break;
        }
    }
}
