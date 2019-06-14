#include "testing_utils.hpp"

#include "catch.hpp"

#include <opencv2/highgui.hpp>
#include <opencv2/dnn.hpp>

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

    // I've allowed += 2px for error
    // (should be safe so long as same network is used)

    REQUIRE(conf > 0.98);
    REQUIRE(conf <= 1.001); // slightly over one to allow floating point weirdness
    REQUIRE(box.x > 80);
    REQUIRE(box.x < 85);
    REQUIRE(box.y > 108);
    REQUIRE(box.y < 113);
    REQUIRE(box.width > 410);
    REQUIRE(box.y < 415);
    REQUIRE(box.height > 722);
    REQUIRE(box.y < 727);

    return true;
}