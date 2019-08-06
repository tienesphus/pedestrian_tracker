#include "catch.hpp"

#include <libbuscount.hpp>
#include "testing_utils.hpp"

class DummyDetector: public Detector {
    Detections process(const cv::Mat& frame) override {
        REQUIRE(!frame.empty());
        std::vector<Detection> results;
        results.emplace_back(cv::Rect(1, 2, 3, 4), 0.5);
        return Detections(results);
    }
};

class DummyTracker: public Tracker {
    std::vector<Event> process(const Detections& detections, const cv::Mat& frame) override {
        REQUIRE(!frame.empty());
        REQUIRE(detections.get_detections().size() == 1);
        return std::vector<Event>();
    };
    void draw(cv::Mat&) const override {};
};

TEST_CASE( "Bus Counter runs in serial", "[libbuscount]" ) {

    DummyDetector detector;
    DummyTracker tracker;
    WorldConfig config = WorldConfig::from_file(SOURCE_DIR "/config.csv");

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
            },
            [](Event) {}
    );

    counter.run(BusCounter::RUN_SERIAL, false);
    // just get to the end without crashing
}


TEST_CASE( "Bus Counter runs in parallel", "[libbuscount]" ) {

    DummyDetector detector;
    DummyTracker tracker;
    WorldConfig config = WorldConfig::from_file(SOURCE_DIR "/config.csv");

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
                       },
                       [](Event) {}
    );

    counter.run(BusCounter::RUN_PARALLEL, false);
    // just get to the end without crashing
}