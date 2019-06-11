#include "catch.hpp"

#include <iostream>
#include <stdio.h>
#include <unistd.h>

#include <opencv2/highgui.hpp>

#include "detector_opencv.hpp"

cv::Mat load_test_image() {
    cv::Mat image = cv::imread(std::string(SOURCE_DIR)+"/tests/skier.jpg");
    REQUIRE(!image.empty());
    return image;
}

NetConfigOpenCV load_test_config(int preferred_backend, int preferred_target) {
    return NetConfigOpenCV {
            0.5f,               // thresh
            15,                 // clazz
            cv::Size(300, 300), // size
            2/255.0,            // scale
            cv::Scalar(127.5, 127.5, 127.5),     // mean
            std::string(SOURCE_DIR)+"/models/MobileNetSSD_caffe/MobileNetSSD.prototxt", // config
            std::string(SOURCE_DIR)+"/models/MobileNetSSD_caffe/MobileNetSSD.caffemodel",  // model
            preferred_backend,  // preferred backend
            preferred_target,   // preferred device
    };
}

bool require_detections_in_spec(const Detections &result) {
    std::vector<Detection> detections = result.get_detections();
    REQUIRE(detections.size() == 1);
    float conf   = detections[0].confidence;
    cv::Rect box = detections[0].box;

    // actual results on my computer are:
    // conf: 0.999949
    // rect: (x1, y1) = (82,111)
    //       (w,h)    = (412,725)

    // I've allowed += 2px for error
    // (should be safe so long as same network is used)

    REQUIRE(conf > 0.98);
    REQUIRE(conf <= 1.001); // slightly over one to allow floating point weirdness
    REQUIRE(box.x > 80);
    REQUIRE(box.x < 85);
    REQUIRE(box.y > 109);
    REQUIRE(box.y < 113);
    REQUIRE(box.width > 410);
    REQUIRE(box.y < 415);
    REQUIRE(box.height > 722);
    REQUIRE(box.y < 727);

    return true;
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

    Detections results = static_post_process(result, config.clazz, config.thresh, cv::Size(image.cols, image.rows));
    require_detections_in_spec(results);
}

TEST_CASE( "Basic detection with Detector", "[detector_opencv]" ) {

    cv::Mat image = load_test_image();
    NetConfigOpenCV net_config = load_test_config(cv::dnn::DNN_BACKEND_OPENCV, cv::dnn::DNN_TARGET_CPU);
    OpenCVDetector detector(net_config, cv::Size(image.cols, image.rows));

    Detections results = detector.process(image);

    require_detections_in_spec(results);
}

TEST_CASE( "Basic detection with Detector on Myraid", "[detector_opencv]" ) {

    // If this is crashing, make sure you have the NCS plugged in

    cv::Mat image = load_test_image();
    NetConfigOpenCV net_config = load_test_config(cv::dnn::DNN_BACKEND_INFERENCE_ENGINE, cv::dnn::DNN_TARGET_MYRIAD);
    OpenCVDetector detector(net_config, cv::Size(image.cols, image.rows));

    Detections results = detector.process(image);

    require_detections_in_spec(results);
}

TEST_CASE( "Detection with aysnc and myraid", "[detector_opencv]" ) {

    cv::Mat image = load_test_image();
    NetConfigOpenCV net_config = load_test_config(cv::dnn::DNN_BACKEND_INFERENCE_ENGINE, cv::dnn::DNN_TARGET_MYRIAD);
    OpenCVDetector detector(net_config, cv::Size(image.cols, image.rows));

    Detections results = detector.post_process(detector.wait_async(detector.start_async(image)));

    require_detections_in_spec(results);
}

TEST_CASE( "Multiprocessing with async and Myraid", "[detector_opencv]" ) {

    NetConfigOpenCV net_config = load_test_config(cv::dnn::DNN_BACKEND_INFERENCE_ENGINE, cv::dnn::DNN_TARGET_MYRIAD);
    cv::Size image_size;
    {
        // separate scope so I don't accidentally reuse the input image in the loops below
        cv::Mat image = load_test_image();
        image_size = cv::Size(image.cols, image.rows);
    }
    OpenCVDetector detector(net_config, image_size);

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