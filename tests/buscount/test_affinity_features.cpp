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
            0.3,                 // similarity thresh
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