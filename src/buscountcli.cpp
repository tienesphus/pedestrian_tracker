#include <iostream>
#include <tuple>
#include <vector>

#include <opencv2/highgui.hpp>

#include "libbuscount.hpp"
#include "detection.hpp"
#include "world.hpp"
#include "detector_openvino.hpp"
#include "detector_opencv.hpp"
#include "video_sync.hpp"
#include "optional.hpp"


int main() {

    /*
    DetectorOpenCV::NetConfig net_config {
        0.5f,               // thresh
        15,                 // clazz
        cv::Size(300, 300), // size
        1, //2/255.0,            // scale
        cv::Scalar(1,1,1),//cv::Scalar(127.5, 127.5, 127.5),     // mean
        "../models/MobileNetSSD_IE/MobileNetSSD.xml", // config
        "../models/MobileNetSSD_IE/MobileNetSSD.bin", // model
        //"../models/MobileNetSSD_caffe/MobileNetSSD.prototxt", // config
        //"../models/MobileNetSSD_caffe/MobileNetSSD.caffemodel",  // model
        cv::dnn::DNN_BACKEND_INFERENCE_ENGINE,  // preferred backend
        cv::dnn::DNN_TARGET_MYRIAD,  // preferred device
    };
    */

    DetectorOpenVino::NetConfig net_config {
            0.6f,               // thresh
            15,                 // clazz
            "../models/MobileNetSSD_IE/MobileNetSSD.xml", // config
            "../models/MobileNetSSD_IE/MobileNetSSD.bin", // model
    };

    //std::string input = "../../samplevideos/pi3_20181213/2018-12-13--08-26-02--snippit-1.mp4";
    //VideoSync<cv::Mat> cap = VideoSync<cv::Mat>::from_video(input);
    auto cv_cap = std::make_shared<cv::VideoCapture>(0, cv::CAP_V4L2);
    cv_cap->set(cv::CAP_PROP_FRAME_WIDTH,640);
    cv_cap->set(cv::CAP_PROP_FRAME_HEIGHT,480);
    
    //VideoSync<cv::Mat> cap = VideoSync<cv::Mat>::from_capture(cv_cap);

    DetectorOpenVino detector(net_config);
    WorldConfig world_config = WorldConfig::from_file(cv::Size(640, 480), "../config.csv");
    Tracker tracker(world_config, 0.2);

    BusCounter counter(detector, tracker, world_config,
            //[&cap]() -> nonstd::optional<cv::Mat> { return cap.next(); },
            [&cv_cap]() -> nonstd::optional<cv::Mat> { cv::Mat frame; cv_cap->read(frame); return frame; },
            [](const cv::Mat& frame) { cv::imshow("output", frame); },
            []() { return cv::waitKey(20) == 'q'; }
    );
    counter.run(BusCounter::RUN_PARALLEL, true);

    return 0;
}
