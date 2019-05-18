#include <iostream>
#include <tuple>
#include <vector>

#include "libbuscount.hpp"
#include "detection.hpp"
#include "world.hpp"

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
        //"../models/MobileNetSSD_IE/MobileNetSSD.xml", // config
        //"../models/MobileNetSSD_IE/MobileNetSSD.bin"  // model
        "../models/MobileNetSSD_caffe/MobileNetSSD.prototxt", // config
        "../models/MobileNetSSD_caffe/MobileNetSSD.caffemodel"  // model
    };
    
    WorldConfig world_config = WorldConfig::from_file("../config.csv");
    
    string input = "../../samplevideos/pi3_20181213/2018-12-13--08-26-02--snippit-1.mp4";
    cout << input << endl;

    cv::VideoCapture cap(input);
    function<BusCounter::src_cb_t> src = [&cap](cv::Ptr<cv::Mat> &frame) -> bool
    {
        cout << "START SRC" << endl;

        frame = cv::makePtr<cv::Mat>();

        // Probably no need to drop frames explicitly. Frames will be dropped through the pipeline
        // if the next stage has not used the relevant frame... (I think???)
        bool res = cap.read(*frame);
        cout << "END SRC (" << (res ? "true" : "false") << ")" << endl;

        return res;
    };

    // This callback is always run from whichever thread BusCounter.run is called on.
    function<BusCounter::dest_cb_t> dest = [](cv::Ptr<cv::Mat> frame) -> void
    {
        std::cout << "imshow()" << std::endl;
        imshow("output", *frame);
    };

    function<BusCounter::test_exit_t> test_exit = []() -> bool
    {
        // GUI stuff, so probably needs to be handled on the main thread as well
        //std::cout << "waitkey()" << std::endl;
        return cv::waitKey(20) == 'q';
    };

    BusCounter counter(net_config, world_config, src, dest, test_exit);
    counter.run(BusCounter::RUN_PARALLEL, true);

    return 0;
}
