#include "catch.hpp"

#include <iostream>
#include <stdio.h>
#include <unistd.h>

#include <opencv2/core/types.hpp>
#include <opencv2/dnn.hpp>
#include <inference_engine.hpp>

#include "testing_utils.hpp"
#include "detection/detector_openvino.hpp"

DetectorOpenVino::NetConfig load_test_config_ie() {
    return DetectorOpenVino::NetConfig {
            0.5f,               // thresh
            15,                 // clazz
            SOURCE_DIR "/models/MobileNetSSD_IE/MobileNetSSD.xml", // config
            SOURCE_DIR "/models/MobileNetSSD_IE/MobileNetSSD.bin",  // model
    };
}


TEST_CASE( "Basic detection with OpenVino", "[detector_openvino]" ) {

    cv::Mat image = load_test_image();
    auto net_config = load_test_config_ie();
    InferenceEngine::InferencePlugin plugin = InferenceEngine::PluginDispatcher({""}).getPluginByDevice("MYRIAD");
    DetectorOpenVino detector(net_config, plugin);

    Detections results = detector.process(image);

    require_detections_in_spec(results);
}

TEST_CASE( "Detection with aysnc", "[detector_openvino]" ) {

    cv::Mat image = load_test_image();
    auto net_config = load_test_config_ie();
    InferenceEngine::InferencePlugin plugin = InferenceEngine::PluginDispatcher({""}).getPluginByDevice("MYRIAD");
    DetectorOpenVino detector(net_config, plugin);

    Detections results = detector.wait_async(detector.start_async(image));

    require_detections_in_spec(results);
}

TEST_CASE( "Multiprocessing with async", "[detector_openvino]" ) {

    auto net_config = load_test_config_ie();
    InferenceEngine::InferencePlugin plugin = InferenceEngine::PluginDispatcher({""}).getPluginByDevice("MYRIAD");
    DetectorOpenVino detector(net_config, plugin);

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