#include "catch.hpp"

#include <iostream>
#include <stdio.h>
#include <unistd.h>

#include <opencv2/core/types.hpp>
#include <opencv2/dnn.hpp>
#include <opencv2/highgui.hpp>

#include "testing_utils.hpp"
#include "detector_openvino.hpp"


TEST_CASE( "Basic detection with OpenVino", "[detector_openvino]" ) {

    cv::Mat image = load_test_image();
    NetConfig net_config = load_test_config(cv::dnn::DNN_BACKEND_OPENCV, cv::dnn::DNN_TARGET_CPU);
    DetectorOpenVino detector(net_config);

    Detections results = detector.process(image);

    require_detections_in_spec(results);
}
/*
TEST_CASE( "Basic detection with Detector on Myraid", "[detector_openvino]" ) {

    // If this is crashing, make sure you have the NCS plugged in

    cv::Mat image = load_test_image();
    NetConfigOpenCV net_config = load_test_config(cv::dnn::DNN_BACKEND_INFERENCE_ENGINE, cv::dnn::DNN_TARGET_MYRIAD);
    OpenCVDetector detector(net_config);

    Detections results = detector.process(image);

    require_detections_in_spec(results);
}

TEST_CASE( "Detection with aysnc and myraid", "[detector_openvino]" ) {

    cv::Mat image = load_test_image();
    NetConfigOpenCV net_config = load_test_config(cv::dnn::DNN_BACKEND_INFERENCE_ENGINE, cv::dnn::DNN_TARGET_MYRIAD);
    OpenCVDetector detector(net_config);

    Detections results = detector.post_process(detector.wait_async(detector.start_async(image)));

    require_detections_in_spec(results);
}

TEST_CASE( "Multiprocessing with async and Myraid", "[detector_openvino]" ) {

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
}*/