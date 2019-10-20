#include <utility>
#include <unistd.h>
#include <atomic>

#include <tracking/tracker_component.hpp>
#include <tracking/feature_affinity.hpp>

#include "testing_utils.hpp"

#include <inference_engine.hpp>
#include <opencv2/imgcodecs.hpp>

#include "catch.hpp"

TEST_CASE( "Features_are_similar", "[affinity_features]" ) {

    InferenceEngine::InferencePlugin plugin = InferenceEngine::PluginDispatcher({""}).getPluginByDevice("MYRIAD");

    FeatureAffinity::NetConfig tracker_config {
            SOURCE_DIR "/models/Reidentify0031/person-reidentification-retail-0031.xml", // config
            SOURCE_DIR "/models/Reidentify0031/person-reidentification-retail-0031.bin", // model
            cv::Size(48, 96),    // input size
    };

    FeatureAffinity affinity(tracker_config, plugin);

    cv::Mat frame = cv::imread(SOURCE_DIR "/tests/buscount/people.png");
    REQUIRE(!frame.empty());

    float w = frame.cols;
    float h = frame.rows;

    Detection d1(cv::Rect2f(  7/w, 7/h, 89/w, 177/h), 1.0);
    Detection d2(cv::Rect2f(139/w, 7/h, 89/w, 177/h), 1.0); // same guy
    Detection d3(cv::Rect2f(809/w, 7/h, 89/w, 177/h), 1.0); // different guy

    auto t1 = affinity.init(d1, frame, 0);
    auto t2 = affinity.init(d2, frame, 1);
    auto t3 = affinity.init(d3, frame, 2);

    float c1_1 = affinity.affinity(*t1, *t1);
    float c1_2 = affinity.affinity(*t1, *t2);
    float c1_3 = affinity.affinity(*t1, *t3);

    REQUIRE(c1_1 > 0.95); REQUIRE(c1_1 <= 1.01); // actually 1.00
    REQUIRE(c1_2 > 0.50); REQUIRE(c1_2 <= 0.60); // actually 0.56
    REQUIRE(c1_3 > 0.00); REQUIRE(c1_3 <= 0.10); // actually 0.05
}

TEST_CASE( "Affinity handles floating point inaccuracies", "[affinity_features]" ) {

    // This test is because the below detection - if floating point intersection is used - will cause floating point
    // inaccuracies and cause the bounds to be slightly exceeded (by 0.00...05) and, thus, cause taking the Mat crop to
    // crash. Through this test, I ensure that integer cropping is done to remove any small errors.

    // The dangerous detection: cv::Rect2f(0.053201466, 0.218055576, 0.666138887, 0.786361217);
    // The bounds that it produces: cv::Rect2f(34.0489388,78.5000076, 426.328888,281.5);
    // Note: 78.5000076+281.5 > 360

    InferenceEngine::InferencePlugin plugin = InferenceEngine::PluginDispatcher({""}).getPluginByDevice("MYRIAD");

    FeatureAffinity::NetConfig tracker_config {
            SOURCE_DIR "/models/Reidentify0031/person-reidentification-retail-0031.xml", // config
            SOURCE_DIR "/models/Reidentify0031/person-reidentification-retail-0031.bin", // model
            cv::Size(48, 96),    // input size
    };

    FeatureAffinity affinity(tracker_config, plugin);

    // frame width/height
    int w = 640;
    int h = 360;
    cv::Mat frame(h, w, CV_8UC3); // don't care what is passed in
    Detection det(cv::Rect2f(0.053201466, 0.218055576, 0.666138887, 0.786361217), 0.1);

    // Dont crash
    affinity.init(det, frame, 0);
}