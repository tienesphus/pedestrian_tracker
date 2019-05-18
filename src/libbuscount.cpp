#include <iostream>
#include <vector>

#include <tbb/concurrent_queue.h>
#include <tbb/flow_graph.h>
#include <tbb/tick_count.h>

#include <opencv2/highgui.hpp>

#include "libbuscount.hpp"

using namespace std;
using namespace tbb;

class TickCounter
{
    cv::Ptr<float> s_per_frame;
    size_t avg_size = 10;

    long frame_count;
    tbb::tick_count prev;

public:
    TickCounter(const size_t avg_size): frame_count(0), prev(tbb::tick_count::now())
    {
        // Reset ms_per_frame. Default to 1 second...because that's a sensibly large value.
        s_per_frame = cv::Ptr<float>(new float[avg_size]);
        for(auto i = 0; i < avg_size; i++)
            *((float*)s_per_frame+i) = 1.0;
    }
    
    TickCounter(const TickCounter &other)
    {
        (*this) = other;
    }

    void operator=(const TickCounter &other)
    {
        this->prev = other.prev;
        this->frame_count = other.frame_count;
        this->avg_size = other.avg_size;
        this->s_per_frame = other.s_per_frame; 
    }

    void operator()(tbb::flow::continue_msg)
    {
        // This is called once per frame, so we only need to calculate the time between
        // this frame and the previous frame. No need to save the absolute time of every
        // frame.

        std::cout << "START TICK COUNTER" << std::endl;
        
        tbb::tick_count now = tbb::tick_count::now();
        s_per_frame[frame_count % avg_size] = (now - prev).seconds();

        frame_count++;
        prev = now;

        float secs = 0;
        for(auto i = 0; i < avg_size; i++)
            secs += s_per_frame[i];
        
        float fps = avg_size / secs;
        
        std::cout << "END TICK  frame: " << frame_count << " fps: " << fps << std::endl;
    }
};

BusCounter::BusCounter(
        NetConfigIR &nconf,
        WorldConfig &wconf,
        std::function<BusCounter::src_cb_t> src,
        std::function<BusCounter::dest_cb_t> dest,
        std::function<BusCounter::test_exit_t> test_exit
    ) :
        _detector(nconf),
        _world_config(wconf),
        _tracker(wconf),
        _src(src),
        _dest(dest),
        _test_exit(test_exit)
{
}

//
// Builds and runs a graph with the the given configeration options
//

// pre_detect: Preprocesses the image into a 'blob' so it is ready 
//             to feed into a detection algorithm
void BusCounter::pre_detect(const cv::Ptr<cv::Mat> input)
{
    cout << "START PRE DETECT" << endl;
    _detector.pre_process(*input);
    cout << "END PRE DETECT" << endl;
}

cv::Ptr<cv::Mat> BusCounter::detect(flow::continue_msg)
{
    std::cout << "START DETECT" << std::endl;
    auto detections = _detector.process();
    std::cout << "END DETECT" << std::endl;

    return detections;
}

cv::Ptr<Detections> BusCounter::post_detect(const std::tuple<cv::Ptr<cv::Mat>, cv::Ptr<cv::Mat>> input)
{
    std::cout << "START POST DETECT" << std::endl;
    cv::Ptr<cv::Mat> original = std::get<0>(input);
    cv::Ptr<cv::Mat> results  = std::get<1>(input);
    
    auto detections = _detector.post_process(original, *results);
    std::cout << "END POST DETECT" << std::endl;

    return detections;
}

std::tuple<cv::Ptr<WorldState>, cv::Ptr<Detections>> BusCounter::track(const cv::Ptr<Detections> input)
{
    std::cout << "START TRACK" << std::endl;
    cv::Ptr<WorldState> state = cv::makePtr<WorldState>(_tracker.process(*input));
    std::cout << "END TRACK" << std::endl;
    
    return std::tuple<cv::Ptr<WorldState>, cv::Ptr<Detections>>(state, input);
}

cv::Ptr<cv::Mat> BusCounter::draw(std::tuple<cv::Ptr<WorldState>, cv::Ptr<Detections>> input)
{
    std::cout << "START DRAW" << std::endl;

    cv::Ptr<WorldState> state = std::get<0>(input);
    cv::Ptr<Detections> detections = std::get<1>(input);

    cv::Ptr<cv::Mat> frame = detections->get_frame();
    
    detections->draw(*frame);
    _tracker.draw(*frame);
    state->draw(*frame);
    _world_config.draw(*frame);
    
    std::cout << "END DRAW" << std::endl;

    return frame;
}

cv::Ptr<cv::Mat> BusCounter::no_draw(std::tuple<cv::Ptr<WorldState>, cv::Ptr<Detections>> input)
{
    return std::get<1>(input)->get_frame();
}


void BusCounter::run(RunStyle style, bool draw)
{
    if(style == RUN_PARALLEL)
        run_parallel(draw);
    else
        run_serial(draw);
}


//
// Builds and runs a flow graph with the the given configeration options
//
void BusCounter::run_parallel(bool draw)
{
    namespace flow = tbb::flow;
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
    bool quit = false;
    
    std::cout << "Initialise Video" << std::endl;
    
    // src: Reads images from the input stream.
    //      It will continuously try to push more images down the stream
    std::cout << "Video inited" << std::endl;
    flow::source_node<Ptr<cv::Mat>>
        src_node(g, [this, &quit](Ptr<cv::Mat> &frame){ return !quit && _src(frame); }, false);

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
    flow::function_node<Ptr<cv::Mat>, flow::continue_msg>
        pre_detect_node(g, flow::serial, bind(&BusCounter::pre_detect, this, placeholders::_1));
    
    // detect: Takes the preprocessed blob
    flow::continue_node<Ptr<cv::Mat>>
        detect_node(g, flow::serial, bind(&BusCounter::detect, this, placeholders::_1));
    
    // joint: combines the results from the original image and the detection data
    flow::join_node<std::tuple<Ptr<cv::Mat>, Ptr<cv::Mat>>> joint_node(g);
    
    // post_detect: Takes the raw detection output, interperets it, and
    //              produces some meaningful results
    flow::function_node<std::tuple<Ptr<cv::Mat>, Ptr<cv::Mat>>, Ptr<Detections>>
        post_detect_node(g, flow::serial, bind(&BusCounter::post_detect, this, placeholders::_1));
    
    std::cout << "Init tracker" << std::endl;
    
    // track: Tracks detections
    flow::function_node<Ptr<Detections>, std::tuple<Ptr<WorldState>, Ptr<Detections>>>
        track_node(g, flow::serial, bind(&BusCounter::track, this, placeholders::_1));

    flow::function_node<std::tuple<Ptr<WorldState>, Ptr<Detections>>, Ptr<cv::Mat>>
        draw_node(g, flow::serial, bind(&BusCounter::draw, this, placeholders::_1));

    flow::function_node<std::tuple<Ptr<WorldState>, Ptr<Detections>>, Ptr<cv::Mat>>
        no_draw_node(g, flow::serial, bind(&BusCounter::no_draw, this, placeholders::_1));

    std::cout << "Init writter" << std::endl;
    
    // stream: Writes images to a stream
    tbb::concurrent_queue<Ptr<cv::Mat>> display_queue;
    flow::function_node<Ptr<cv::Mat>, flow::continue_msg> stream_node(g, flow::serial,
        [&display_queue](Ptr<cv::Mat> input) -> flow::continue_msg
        {
            display_queue.push(input);
            cout << "Pushed to queue" << endl;
            return flow::continue_msg();
        }
    );
    
    std::cout << "Init frame count" << std::endl;
    
    // monitor: Calculates the speed that the program is running at by moving average.
    TickCounter tickCounter(10);
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

    if (draw)
    {
        flow::make_edge(track_node, draw_node);
        flow::make_edge(draw_node, stream_node);
    }
    else
    {
        flow::make_edge(track_node, no_draw_node);
        flow::make_edge(no_draw_node, stream_node);
    }

    flow::make_edge(stream_node, tick_count_node);
    flow::make_edge(stream_node, throttle_node.decrement);
    
    // Begin running stuff
    std::cout << "Starting video" << std::endl;
    src_node.activate();
    std::cout << "Video started" << std::endl;
    
    // The display code _must_ be run on the main thread. Thus, we pass
    // display frames here through a queue
    while (!(quit || g.is_cancelled()))
    {
        Ptr<cv::Mat> display_img;
        if (display_queue.try_pop(display_img))
        {
            _dest(display_img);
        }

        quit = _test_exit();
    }
    
    // Allow the last few frames to pass through
    // If this is not done, invalid memory can occur
    g.wait_for_all();
}

//
// A single threaded version of run_parallel. Useful for debugging.
//
void BusCounter::run_serial(bool draw)
{
    
    using namespace cv;
    
    bool quit = false;
    
    /*
    std::cout << "Initialise Video" << std::endl;
    //VideoCapture cap(input);
    
    std::cout << "Init detector" << std::endl;
    //Detector detector(net_config);
    cv::dnn::Net net = net_config.make_network();
    
    std::cout << "Init tracker" << std::endl;
    //Tracker tracker(world_config);
    
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
    */
}
