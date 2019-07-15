#include <utility>

#include <unistd.h>
#include <atomic>

#include <tracker_component.hpp>
#include <feature_affinity.hpp>
#include <tracker_reidentify.hpp>

#include "testing_utils.hpp"

#include <inference_engine.hpp>
#include <opencv2/imgcodecs.hpp>

#include "catch.hpp"

TEST_CASE( "Features_are_similar", "[affinity_features]" ) {

    InferenceEngine::InferencePlugin plugin = InferenceEngine::PluginDispatcher({""}).getPluginByDevice("MYRIAD");

    FeatureAffinity::NetConfig tracker_config {
            std::string(SOURCE_DIR) + "/models/Reidentify0031/person-reidentification-retail-0031.xml", // config
            std::string(SOURCE_DIR) + "/models/Reidentify0031/person-reidentification-retail-0031.bin", // model
            cv::Size(48, 96),    // input size
            0.3,                 // similarity thresh
    };

    FeatureAffinity affinity(tracker_config, plugin);

    cv::Mat frame = cv::imread(std::string(SOURCE_DIR)+"/tests/people.png");
    REQUIRE(!frame.empty());

    Detection d1(cv::Rect(7, 7, 89, 177), 1.0);
    Detection d2(cv::Rect(139, 7, 89, 177), 1.0); // same guy
    Detection d3(cv::Rect(809, 7, 89, 177), 1.0); // different guy

    auto t1 = affinity.init(d1, frame);
    auto t2 = affinity.init(d2, frame);
    auto t3 = affinity.init(d3, frame);

    float c1_1 = affinity.affinity(*t1, *t1);
    float c1_2 = affinity.affinity(*t1, *t2);
    float c1_3 = affinity.affinity(*t1, *t3);

    REQUIRE(c1_1 > 0.95); REQUIRE(c1_1 <= 1.01); // actually 1.00
    REQUIRE(c1_2 > 0.50); REQUIRE(c1_2 <= 0.60); // actually 0.56
    REQUIRE(c1_3 > 0.00); REQUIRE(c1_3 <= 0.10); // actually 0.05
}

TEST_CASE( "Reidentify_tracker_detects_okay", "[reidentify_tracker]" ) {

    InferenceEngine::InferencePlugin plugin = InferenceEngine::PluginDispatcher({""}).getPluginByDevice("MYRIAD");

    Tracker_RI::NetConfig tracker_config {
            std::string(SOURCE_DIR) + "/models/Reidentify0031/person-reidentification-retail-0031.xml", // config
            std::string(SOURCE_DIR) + "/models/Reidentify0031/person-reidentification-retail-0031.bin", // model
            cv::Size(48, 96),    // input size
            0.3,                 // similarity thresh
    };

    Tracker_RI track(tracker_config, WorldConfig::from_file(cv::Size(640, 480), std::string(SOURCE_DIR)+"/config.csv"), plugin);

    cv::Mat frame = cv::imread(std::string(SOURCE_DIR)+"/tests/people.png");
    REQUIRE(!frame.empty());

    cv::Rect b1(7, 7, 89, 177);
    cv::Rect b2(139, 7, 89, 177); // same guy
    cv::Rect b3(809, 7, 89, 177); // different guy

    auto f1 = track.identify(frame(b1));
    auto f2 = track.identify(frame(b2));
    auto f3 = track.identify(frame(b3));

    float c1_1 = cosine_similarity(f1, f1);
    float c1_2 = cosine_similarity(f1, f2);
    float c1_3 = cosine_similarity(f1, f3);

    REQUIRE(c1_1 > 0.95); REQUIRE(c1_1 <= 1.01); // actually 1.00
    REQUIRE(c1_2 > 0.50); REQUIRE(c1_2 <= 0.60); // actually 0.56
    REQUIRE(c1_3 > 0.00); REQUIRE(c1_3 <= 0.10); // actually 0.05
}