#include <utility>
#include <unistd.h>
#include <atomic>

#include <tracking/tracker_component.hpp>
#include "testing_utils.hpp"

#include "catch.hpp"

// THIS TEST IS VERY MUCH SO UNFINISHED
/*
class TestData: public TrackData {
public:
    friend class TestTracker;

    int owner;
    Detection detection;
    cv::Mat frame;

    TestData(int owner, Detection detection, cv::Mat frame):
        owner(owner), detection(std::move(detection)), frame(std::move(frame))
    {}
};

class TestTracker: public Affinity<TestData> {
public:
    int number;

    std::vector<TestData> detector_calls;
    std::vector<std::tuple<int, int, int>> affinity_calls;
    std::vector<std::tuple<int, int>> merge_calls;

    TestTracker(int number): number(number)
    {}

    ~TestTracker() override = default;

    std::unique_ptr<TestData> init(const Detection &d, const cv::Mat &frame, int) const override {
        const_cast<TestTracker*>(this)->detector_calls.emplace_back(number, d, frame);
        return std::make_unique<TestData>(number, d, frame);
    }

    float affinity(const TestData &detectionData, const TestData &trackData) const override {
        REQUIRE(detectionData.owner == number);
        REQUIRE(trackData.owner == number);
        return number;
    }

    void merge(const TestData &detectionData, TestData &trackData) const override {
        REQUIRE(detectionData.owner == number);
        REQUIRE(trackData.owner == number);
    }

};

TEST_CASE( "Tracker gives correct data", "[tracker_comp]" ) {

    auto config = WorldConfig::from_file(std::string(SOURCE_DIR)+"/config.csv");
    TrackerComp tracker(config, 0.5, 0.05, 0.2);

    TestTracker* tracker_a = new TestTracker(1);
    TestTracker* tracker_b = new TestTracker(2);

    tracker.use<TestTracker, TestData>(0.8, std::unique_ptr<TestTracker>(tracker_a));
    tracker.use<TestTracker, TestData>(0.7, std::unique_ptr<TestTracker>(tracker_b));

    cv::Mat frame = load_test_image();

    tracker.process(Detections({
                Detection(cv::Rect2f(.1f, .2f, .3f, .4f), 0.5),
                Detection(cv::Rect2f(.6f, .7f, .8f, .9f), 0.10)
            }), frame, 0);

    REQUIRE(tracker_a->detector_calls.size() == 2);
    REQUIRE(tracker_b->detector_calls.size() == 2);
}
*/
