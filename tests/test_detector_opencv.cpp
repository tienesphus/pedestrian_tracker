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

bool require_detections_in_spec(const Detections &result) {
    std::vector<Detection> detections = result.get_detections();
    REQUIRE(detections.size() == 1);
    float conf   = detections[0].confidence;
    cv::Rect box = detections[0].box;

    // actual results on my computer are:
    // conf: 0.999949
    // rect: (x1, y1) = (82,111)
    //       (w,h)    = (412,725)

    // I've allowed += 10px for error

    REQUIRE(conf > 0.95);
    REQUIRE(conf < 1.0);
    REQUIRE(box.x > 70);
    REQUIRE(box.x < 90);
    REQUIRE(box.y > 100);
    REQUIRE(box.y < 120);
    REQUIRE(box.width > 400);
    REQUIRE(box.y < 420);
    REQUIRE(box.height > 715);
    REQUIRE(box.y < 735);

    return true;
}

TEST_CASE( "Sanity check - independent network detect", "[detector_opencv]" ) {

    cv::Mat image = load_test_image();

    cv::Mat blob = cv::dnn::blobFromImage(image, 2/255.0, cv::Size(300, 300), cv::Scalar(127.5, 127.5, 127.5));

    std::string config = std::string(SOURCE_DIR)+"/models/MobileNetSSD_caffe/MobileNetSSD.prototxt";
    std::string model  = std::string(SOURCE_DIR)+"/models/MobileNetSSD_caffe/MobileNetSSD.caffemodel";
    cv::dnn::Net net = cv::dnn::readNet(model, config);
    net.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
    net.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);

    // pass the network
    cv::Mat result;
    net.setInput(blob);
    net.forward(result);

    Detections results = static_post_process(result, 15, 0.5, cv::Size(image.cols, image.rows));
    require_detections_in_spec(results);
}

TEST_CASE( "Basic detection with Detector", "[detector_opencv]" ) {

    cv::Mat image = cv::imread(std::string(SOURCE_DIR)+"/tests/skier.jpg");
    REQUIRE(!image.empty());

    NetConfigOpenCV net_config {
            0.5f,               // thresh
            15,                 // clazz
            cv::Size(300, 300), // size
            2/255.0,            // scale
            cv::Scalar(127.5, 127.5, 127.5),     // mean
            std::string(SOURCE_DIR)+"/models/MobileNetSSD_caffe/MobileNetSSD.prototxt", // config
            std::string(SOURCE_DIR)+"/models/MobileNetSSD_caffe/MobileNetSSD.caffemodel",  // model
            cv::dnn::DNN_BACKEND_OPENCV,  // preferred backend
            cv::dnn::DNN_TARGET_CPU,  // preferred device
    };

    OpenCVDetector detector(net_config, cv::Size(image.cols, image.rows));

    Detections results = detector.process(image);
    require_detections_in_spec(results);
}




