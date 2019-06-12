#include "catch.hpp"

#include <iostream>
#include <stdio.h>
#include <unistd.h>

#include "detector_opencv.hpp"
#include "testing_utils.hpp"



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

    Detections results = static_post_process(result, config.clazz, config.thresh, cv::Size(image.cols, image.rows));
    require_detections_in_spec(results);
}

TEST_CASE( "Basic detection with Detector", "[detector_opencv]" ) {

    cv::Mat image = load_test_image();
    NetConfigOpenCV net_config = load_test_config(cv::dnn::DNN_BACKEND_OPENCV, cv::dnn::DNN_TARGET_CPU);
    OpenCVDetector detector(net_config);

    Detections results = detector.process(image);

    require_detections_in_spec(results);
}

TEST_CASE( "Basic detection with Detector on Myraid", "[detector_opencv]" ) {

    // If this is crashing, make sure you have the NCS plugged in

    cv::Mat image = load_test_image();
    NetConfigOpenCV net_config = load_test_config(cv::dnn::DNN_BACKEND_INFERENCE_ENGINE, cv::dnn::DNN_TARGET_MYRIAD);
    OpenCVDetector detector(net_config);

    Detections results = detector.process(image);

    require_detections_in_spec(results);
}

TEST_CASE( "Detection with aysnc and myraid", "[detector_opencv]" ) {

    cv::Mat image = load_test_image();
    NetConfigOpenCV net_config = load_test_config(cv::dnn::DNN_BACKEND_INFERENCE_ENGINE, cv::dnn::DNN_TARGET_MYRIAD);
    OpenCVDetector detector(net_config);

    Detections results = detector.post_process(detector.wait_async(detector.start_async(image)));

    require_detections_in_spec(results);
}

TEST_CASE( "Multiprocessing with async and Myraid", "[detector_opencv]" ) {

    NetConfigOpenCV net_config = load_test_config(cv::dnn::DNN_BACKEND_INFERENCE_ENGINE, cv::dnn::DNN_TARGET_MYRIAD);
    OpenCVDetector detector(net_config);

    const int iterations = 10;
    std::vector<std::shared_future<Detections>> futures;

    for (int i = 0; i < iterations; i++) {
        cv::Mat image = load_test_image();
        // note: the pass image by VALUE is very important
        std::shared_future<Detections> a = std::async([image, &detector]() -> Detections {
            auto async = detector.start_async(image);
            auto intermediate = detector.wait_async(async);
            return detector.post_process(intermediate);
        });
        futures.push_back(a);
    }

    std::vector<Detections> results;
    for (int i = 0; i < iterations; i++) {
        auto value = futures[i];
        results.push_back(value.get());
    }

    for (int i = 0; i < iterations; i++) {
        require_detections_in_spec(results[i]);
    }
}