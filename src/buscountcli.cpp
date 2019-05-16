#include <iostream>
#include <tuple>
#include <vector>

//#include <tbb/flow_graph.h>
//#include <tbb/tick_count.h>
//#include <tbb/concurrent_queue.h>

//#include <opencv2/videoio.hpp>
//#include <opencv2/highgui.hpp>

#include "libbuscount.hpp"
#include "detection.hpp"
#include "world.hpp"
//#include "tracker.hpp"
//#include "utils.hpp"

using namespace std;

tuple<cv::Ptr<WorldState>, cv::Ptr<cv::Mat>> dest(const cv::Ptr<WorldState>)
{
}

int main() {
    
    NetConfigIR net_config {
        0.5f,               // thresh
        15,                 // clazz
        cv::Size(300, 300), // size
        2/255.0,            // scale
        cv::Scalar(127.5, 127.5, 127.5),     // mean
        "../models/MobileNetSSD_IE/MobileNetSSD.xml", // xml
        "../models/MobileNetSSD_IE/MobileNetSSD.bin"  // bin
    };
    
    WorldConfig world_config = WorldConfig::from_file("../config.csv");
    
    string input = "../../samplevideos/pi3_20181213/2018-12-13--08-26-02--snippit-1.mp4";
    
    cout << input << endl;

    cv::VideoCapture cap(input);
    function<BusCounter::src_cb_t> src = [&cap](cv::Ptr<cv::Mat> frame) -> bool
    {
        cout << "START SRC" << endl;

        frame = cv::makePtr<cv::Mat>();

        // Probably no need to drop frames explicitly. Frames will be dropped through the pipeline
        // if the next stage has not used the relevant frame... (I think???)
        bool res = cap.read(*frame);
        cout << "SOURCE END" << endl;

        return res;
    };

    function<BusCounter::dest_cb_t> dest = [](cv::Ptr<cv::Mat> frame) -> void
    {
        // Drawing and streaming stuff goes here.
    };

    BusCounter counter(net_config, world_config, src, dest);
    counter.run(BusCounter::RUN_PARALLEL, true);
    
    //run_parallel(input, net_config, world_config, true);
    //run_series(input, net_config, world_config, true);
    
    return 0;
} 
