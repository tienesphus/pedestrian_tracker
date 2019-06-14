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
    NetConfig net_config = load_test_config_ie();
    DetectorOpenVino detector(net_config);

    Detections results = detector.process(image);

    require_detections_in_spec(results);
}

TEST_CASE( "Detection with aysnc", "[detector_openvino]" ) {

    cv::Mat image = load_test_image();
    NetConfig net_config = load_test_config_ie();
    DetectorOpenVino detector(net_config);

    Detections results = detector.wait_async(detector.start_async(image));

    require_detections_in_spec(results);
}

TEST_CASE( "Multiprocessing with async", "[detector_openvino]" ) {

    NetConfig net_config = load_test_config_ie();
    DetectorOpenVino detector(net_config);

    const int iterations = 10;
    std::vector<Detector::intermediate> futures;

    for (int i = 0; i < iterations; i++) {
        cv::Mat image = load_test_image();
        futures.push_back(detector.start_async(image));
    }

    std::vector<Detections> results;
    for (int i = 0; i < iterations; i++) {
        Detector::intermediate value = futures[i];
        results.push_back(detector.wait_async(value));
    }

    for (int i = 0; i < iterations; i++) {
        require_detections_in_spec(results[i]);
    }
}