#include "catch.hpp"

#include <iostream>
#include <stdio.h>
#include <unistd.h>

#include "detector_opencv.hpp"
#include "testing_utils.hpp"

DetectorOpenCV::NetConfig load_test_config(int preferred_backend, int preferred_target) {
    return DetectorOpenCV::NetConfig {
            0.5f,               // thresh
            15,                 // clazz
            cv::Size(300, 300), // size
            2/255.0,            // scale
            cv::Scalar(127.5, 127.5, 127.5),     // mean
            SOURCE_DIR "/models/MobileNetSSD_caffe/MobileNetSSD.prototxt", // config
            SOURCE_DIR "/models/MobileNetSSD_caffe/MobileNetSSD.caffemodel",  // model
            preferred_backend,  // preferred backend
            preferred_target,   // preferred device
    };
}


TEST_CASE( "Sanity check - independent network detect", "[detector_opencv]" ) {

    // If this fails, either the config is wrong, or (more likely) you have not set up the libraries right

    cv::Mat image = load_test_image();
    auto config = load_test_config(cv::dnn::DNN_BACKEND_OPENCV, cv::dnn::DNN_TARGET_CPU);

    cv::Mat blob = cv::dnn::blobFromImage(image, config.scale, config.networkSize, config.mean);
    cv::dnn::Net net = cv::dnn::readNet(config.meta, config.model);
    net.setPreferableBackend(config.preferableBackend);
    net.setPreferableTarget(config.preferableTarget);

    // pass the network
    net.setInput(blob);
    cv::Mat result = net.forward();

    Detections results = static_post_process(result, config.clazz, config.thresh);
    require_detections_in_spec(results);
}

TEST_CASE( "Basic detection with Detector", "[detector_opencv]" ) {

    cv::Mat image = load_test_image();
    auto net_config = load_test_config(cv::dnn::DNN_BACKEND_OPENCV, cv::dnn::DNN_TARGET_CPU);
    DetectorOpenCV detector(net_config);

    Detections results = detector.process(image);

    require_detections_in_spec(results);
}

TEST_CASE( "Basic detection with Detector on Myraid", "[detector_opencv]" ) {

    // If this is crashing, make sure you have the NCS plugged in

    cv::Mat image = load_test_image();
    auto net_config = load_test_config(cv::dnn::DNN_BACKEND_INFERENCE_ENGINE, cv::dnn::DNN_TARGET_MYRIAD);
    DetectorOpenCV detector(net_config);

    Detections results = detector.process(image);

    require_detections_in_spec(results);
}

TEST_CASE( "Detection with aysnc and myraid", "[detector_opencv]" ) {

    cv::Mat image = load_test_image();
    auto net_config = load_test_config(cv::dnn::DNN_BACKEND_INFERENCE_ENGINE, cv::dnn::DNN_TARGET_MYRIAD);
    DetectorOpenCV detector(net_config);

    Detections results = detector.wait_async(detector.start_async(image));

    require_detections_in_spec(results);
}

TEST_CASE( "Multiprocessing with async and Myraid", "[detector_opencv]" ) {

    auto net_config = load_test_config(cv::dnn::DNN_BACKEND_INFERENCE_ENGINE, cv::dnn::DNN_TARGET_MYRIAD);
    DetectorOpenCV detector(net_config);

    const int iterations = 10;
    std::vector<Detector::intermediate> futures;

    for (int i = 0; i < iterations; i++) {
        cv::Mat image = load_test_image();
        futures.push_back(detector.start_async(image));
    }

    for (int i = 0; i < iterations; i++) {
        Detector::intermediate value = futures[i];
        Detections results = detector.wait_async(value);

        require_detections_in_spec(results);
    }

}