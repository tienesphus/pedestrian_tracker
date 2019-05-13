#include "libbuscount.hpp"

BusCounter::BusCounter(
  std::function<bool(Ptr<Mat>)> src,
  std::function<flow::tuple<Ptr<WorldState>, Ptr<Mat>>(const Ptr<WorldState>)> dest
);

/**
 * Builds and runs a graph with the the given configeration options
 */
void BusCounter::run(const std::string& input, const NetConfigIR &net_config, const WorldConfig world_config, bool draw)
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
     *   |       track
     *   |         | Ptr<WorldState>
     *   |         v
     *   |       draw              
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
    flow::function_node<Ptr<Mat>, flow::continue_msg> pre_detect(g, flow::serial,
        [&detector](const Ptr<Mat> input) -> flow::continue_msg {
            std::cout << "START PRE DETECT" << std::endl;
            detector.pre_process(*input);
            std::cout << "END PRE DETECT" << std::endl;
            return flow::continue_msg();
        }
    );
    
    // detect: Takes the preprocessed blob
    flow::function_node<flow::continue_msg, Ptr<Mat>> detect(g, flow::serial,
        [&detector](flow::continue_msg _) -> Ptr<Mat> {
            std::cout << "START DETECT" << std::endl;
            auto detections = detector.process();
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

     flow::function_node<Ptr<flow::tuple<Ptr<WorldState>>, flow::tuple<Ptr<WorldState>, Ptr<Mat>>> draw(g, flow::serial,
        [&tracker, &world_config, &display_queue](const Ptr<WorldState> state) -> flow::tuple<Ptr<WorldState>, Ptr<Mat>>
	{
            // TODO figure out how to put this into it's own node
            // Trouble is that Tracker's data is temporary, so, if Tracker::draw()
            // is called latter, it may or may not be okay.
            Ptr<Mat> display;
	    display = Ptr<Mat>(new cv::Mat());
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

