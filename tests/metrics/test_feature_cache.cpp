#include <metrics/cached_detector.hpp>
#include <detection/detector_openvino.hpp>
#include <opencv2/core/mat.hpp>
#include <metrics/read_simulator.hpp>
#include <tracking/feature_affinity.hpp>
#include <metrics/cached_features.hpp>
#include "catch.hpp"
#include "../buscount/testing_utils.hpp"

TEST_CASE( "Features are cached and retrieved", "[feature_cache]" )
{
    FeatureCache cache(SOURCE_DIR "/data/metrics.db", "unit_tests_features");
    cache.clear();

    cache.store(FeatureData({1, 2, 3, 4}), 5, Detection(cv::Rect2d(0.1, 0.2, 0.3, 0.4), 0.5));

    auto data = cache.fetch(5, Detection(cv::Rect2d(0.1, 0.2, 0.3, 0.4), 0.5));

    REQUIRE(data.has_value());

    REQUIRE(data->features[0] == 1);
    REQUIRE(data->features[1] == 2);
    REQUIRE(data->features[2] == 3);
    REQUIRE(data->features[3] == 4);
}

TEST_CASE( "Two Features are cached and retrieved", "[feature_cache]" )
{
    FeatureCache cache(SOURCE_DIR "/data/metrics.db", "unit_tests_features");
    cache.clear();

    cache.store(FeatureData({1, 2, 3, 4}), 5, Detection(cv::Rect2d(0.1, 0.2, 0.3, 0.4), 0.5));
    cache.store(FeatureData({3, 4, 5, 6}), 5, Detection(cv::Rect2d(0.2, 0.3, 0.4, 0.5), 0.6));

    auto data_1 = cache.fetch(5, Detection(cv::Rect2d(0.1, 0.2, 0.3, 0.4), 0.5));
    auto data_2 = cache.fetch(5, Detection(cv::Rect2d(0.2, 0.3, 0.4, 0.5), 0.6));

    REQUIRE(data_1.has_value());
    REQUIRE(data_1->features[0] == 1);
    REQUIRE(data_1->features[1] == 2);
    REQUIRE(data_1->features[2] == 3);
    REQUIRE(data_1->features[3] == 4);

    REQUIRE(data_2.has_value());
    REQUIRE(data_2->features[0] == 3);
    REQUIRE(data_2->features[1] == 4);
    REQUIRE(data_2->features[2] == 5);
    REQUIRE(data_2->features[3] == 6);
}



TEST_CASE( "Features Cache Wrapper works okay", "[feature_cache]" )
{

    FeatureAffinity::NetConfig tracker_config {
            SOURCE_DIR "/models/Reidentify0031/person-reidentification-retail-0031.xml", // config
            SOURCE_DIR "/models/Reidentify0031/person-reidentification-retail-0031.bin", // model
            cv::Size(48, 96),    // input size
            0.6,                 // similarity thresh
    };

    FeatureCache cache(SOURCE_DIR "/data/metrics.db", "unit_tests_features");
    InferenceEngine::InferencePlugin plugin = InferenceEngine::PluginDispatcher({""}).getPluginByDevice("MYRIAD");
    FeatureAffinity affinity(tracker_config, plugin);
    CachedFeatures cached_features(affinity, cache);

    cache.clear();

    cv::Mat test_image = load_test_image();
    cv::Mat fake_image = cv::Mat(test_image.rows, test_image.cols, test_image.type());

    // put the same frame number in both images
    test_image.at<uint32_t>(0, 0) = 12;
    fake_image.at<uint32_t>(0, 0) = 12;

    Detection test_area_1(cv::Rect2f(0.1, 0.1, 0.2f, 0.5f), 0.6f);
    Detection test_area_2(cv::Rect2f(0.3, 0.6, 0.2f, 0.4f), 0.6f);

    auto data_test_1 = cached_features.init(test_area_1, test_image);
    auto data_test_2 = cached_features.init(test_area_2, test_image);

    auto data_fake_1 = cached_features.init(test_area_1, fake_image);
    auto data_fake_2 = cached_features.init(test_area_2, fake_image);


    // if the results are cached, then the fake_image should never have been queried,
    // so it shouldn't have noticed that the image was different. Thus, the features should
    // be exactly identical
    for (size_t i = 0; i < data_test_1->features.size(); i++) {
        REQUIRE(data_test_1->features[i] == data_fake_1->features[i]);
        REQUIRE(data_test_2->features[i] == data_fake_2->features[i]);
        REQUIRE(data_test_1->features[i] != data_test_2->features[i]);
    }
}