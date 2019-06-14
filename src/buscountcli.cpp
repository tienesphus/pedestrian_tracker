#include <iostream>
#include <tuple>
#include <vector>

#include "libbuscount.hpp"
#include "detection.hpp"
#include "world.hpp"
#include "detector_openvino.hpp"
#include "video_sync.hpp"

using namespace std;

int main() {
    
    NetConfig net_config {
        0.5f,               // thresh
        15,                 // clazz
        cv::Size(300, 300), // size
        2/255.0,            // scale
        cv::Scalar(127.5, 127.5, 127.5),     // mean
        "../models/MobileNetSSD_IE/MobileNetSSD.xml", // config
        "../models/MobileNetSSD_IE/MobileNetSSD.bin", // model
        //"../models/MobileNetSSD_caffe/MobileNetSSD.prototxt", // config
        //"../models/MobileNetSSD_caffe/MobileNetSSD.caffemodel",  // model
        cv::dnn::DNN_BACKEND_INFERENCE_ENGINE,  // preferred backend
        cv::dnn::DNN_TARGET_MYRIAD,  // preferred device
    };

    string input = "../../samplevideos/pi3_20181213/2018-12-13--08-26-02--snippit-1.mp4";
    VideoSync<cv::Mat> cap = VideoSync<cv::Mat>::from_video(input);

    DetectorOpenVino detector(net_config);
    WorldConfig world_config = WorldConfig::from_file("../config.csv");
    Tracker tracker(world_config);

    BusCounter counter(detector, tracker, world_config,
            [&cap]() -> std::optional<cv::Mat> { return cap.next(); },
            [](const cv::Mat& frame) { imshow("output", frame); },
            []() { return cv::waitKey(20) == 'q'; }
    );
    counter.run(BusCounter::RUN_PARALLEL, true);

    return 0;
}
