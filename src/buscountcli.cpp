#include <iostream>
#include <tuple>
#include <vector>

#include <opencv2/videoio.hpp>

#include "libbuscount.hpp"
#include "detection.hpp"
#include "world.hpp"
#include "detector_opencv.hpp"
#include "video_sync.hpp"

using namespace std;

int main() {
    
    NetConfigOpenCV net_config {
        0.5f,               // thresh
        15,                 // clazz
        cv::Size(300, 300), // size
        2/255.0,            // scale
        cv::Scalar(127.5, 127.5, 127.5),     // mean
        //"../models/MobileNetSSD_IE/MobileNetSSD.xml", // config
        //"../models/MobileNetSSD_IE/MobileNetSSD.bin"  // model
        "../models/MobileNetSSD_caffe/MobileNetSSD.prototxt", // config
        "../models/MobileNetSSD_caffe/MobileNetSSD.caffemodel",  // model
        cv::dnn::DNN_BACKEND_INFERENCE_ENGINE,  // preferred backend
        cv::dnn::DNN_TARGET_MYRIAD,  // preferred device
    };

    //TODO I don't want to pass in the frame size
    // Also, it is not 50x50
    OpenCVDetector detector(net_config, cv::Size(50, 50));
    WorldConfig world_config = WorldConfig::from_file("../config.csv");
    Tracker tracker(world_config);

    string input = "../../samplevideos/pi3_20181213/2018-12-13--08-26-02--snippit-1.mp4";
    VideoSync cap(input);

    BusCounter counter(detector, tracker, world_config,
            [&cap]() -> std::optional<cv::Mat> { return cap.read(); },
            [](const cv::Mat& frame) { imshow("output", frame); },
            []() { return cv::waitKey(20) == 'q'; }
    );
    counter.run(BusCounter::RUN_SERIAL, true);

    return 0;
}
