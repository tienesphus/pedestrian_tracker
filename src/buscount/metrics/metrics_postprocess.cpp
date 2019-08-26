#include "../libbuscount.hpp"
#include "../tracking/feature_affinity.hpp"
#include "../tracking/position_affinity.hpp"
#include "../detection/detector_openvino.hpp"
#include "../detection/detector_opencv.hpp"
#include "../video_sync.hpp"
#include "cached_detector.hpp"
#include "read_simulator.hpp"
#include "cached_features.hpp"
#include "timed_wrapper.hpp"

#include <data_fetch.hpp>

#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include <sqlite3.h>

#include <iostream>
#include <unistd.h>


int main() {

    FeatureAffinity::NetConfig tracker_config {
            SOURCE_DIR "/models/Reidentify0031/person-reidentification-retail-0031.xml", // config
            SOURCE_DIR "/models/Reidentify0031/person-reidentification-retail-0031.bin", // model
            cv::Size(48, 96),    // input size
            0.6,                 // similarity thresh
    };

    double time = 0;

    std::string input = std::string(SOURCE_DIR) + "/../samplevideos/pi3_20181213/2018-12-13--08-26-02--snippit-1.mp4";
    auto cap = ReadSimulator<>::from_video(input, &time);

    WorldConfig world_config = WorldConfig::from_file(SOURCE_DIR "/config.csv");

    InferenceEngine::InferencePlugin plugin = InferenceEngine::PluginDispatcher({""}).getPluginByDevice("MYRIAD");

    CacheDetections cache(SOURCE_DIR "/data/metrics.db", input);
    CachedDetectorReader cached_detector(cache, 0.6);
    TimedDetector timedDetector(cached_detector, &time, 1/11.0);

    TrackerComp tracker(world_config, 0.5);
    FeatureAffinity affinity(tracker_config, plugin);
    FeatureCache feature_cache(SOURCE_DIR "/data/metrics.db", input);
    CachedFeatures cached_features(affinity, feature_cache);
    tracker.use<TimedAffinity<FeatureData>, FeatureData>(0.5, cached_features, &time, 1/20.0);
    tracker.use<PositionAffinity, PositionData>(0.5, 0.7);

    BusCounter counter(timedDetector, tracker, world_config,
            [&cap]() -> nonstd::optional<cv::Mat> { return cap.next(); },
            [](const cv::Mat& frame) { cv::imshow("output", frame); },
            []() { return cv::waitKey(20) == 'q'; },
            [](Event event) {
                std::cout << "EVENT: " << name(event) << std::endl;
                // TODO store metric events for further processing
            }
    );

    counter.run(BusCounter::RUN_SERIAL, true);

    return 0;
}
