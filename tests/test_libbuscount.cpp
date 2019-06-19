#include "catch.hpp"

#include "libbuscount.hpp"
#include "detector.hpp"
#include "testing_utils.hpp"

class DummyDetector: public Detector {
    Detections process(const cv::Mat&) override {
        std::vector<Detection> results;
        results.emplace_back(cv::Rect(1, 2, 3, 4), 0.5);
        return Detections(results);
    }
};

TEST_CASE( "Bus Counter runs in serial", "[libbuscount]" ) {

    DummyDetector detector;

    WorldConfig config = WorldConfig::from_file(cv::Size(300, 300), std::string(SOURCE_DIR)+"/config.csv");
    Tracker tracker(config, 0.1);

    int count = 50;
    BusCounter counter(detector, tracker, config,
            []() -> nonstd::optional<cv::Mat> {
                return load_test_image();
            },
            [](const cv::Mat&) {
            },
            [&count]() -> bool {
                --count;
                return count < 0;
            }
    );

    counter.run(BusCounter::RUN_SERIAL, false);
    // just get to the end without crashing
}


TEST_CASE( "Bus Counter runs in parallel", "[libbuscount]" ) {

    DummyDetector detector;

    WorldConfig config = WorldConfig::from_file(cv::Size(300, 300), std::string(SOURCE_DIR)+"/config.csv");
    Tracker tracker(config, 0.1);

    int count = 50;
    BusCounter counter(detector, tracker, config,
                       []() -> nonstd::optional<cv::Mat> {
                           return load_test_image();
                       },
                       [](const cv::Mat&) {
                       },
                       [&count]() -> bool {
                           --count;
                           return count < 0;
                       }
    );

    counter.run(BusCounter::RUN_PARALLEL, false);
    // just get to the end without crashing
}