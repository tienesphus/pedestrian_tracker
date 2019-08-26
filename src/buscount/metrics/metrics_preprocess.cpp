#include "../libbuscount.hpp"
#include "../tracking/feature_affinity.hpp"
#include "../tracking/position_affinity.hpp"
#include "../detection/detector_openvino.hpp"
#include "../detection/detector_opencv.hpp"
#include "../video_sync.hpp"
#include "cached_detector.hpp"
#include "read_simulator.hpp"
#include "cached_features.hpp"

#include <data_fetch.hpp>

#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include <sqlite3.h>

#include <iostream>
#include <unistd.h>


int main() {

    DetectorOpenVino::NetConfig net_config {
            0.1f,               // thresh
            15,                 // clazz
            SOURCE_DIR "/models/MobileNetSSD_IE/MobileNetSSD.xml", // config
            SOURCE_DIR "/models/MobileNetSSD_IE/MobileNetSSD.bin", // model
    };

    std::string input = std::string(SOURCE_DIR) + "/../samplevideos/pi3_20181213/2018-12-13--08-26-02--snippit-1.mp4";
    auto cv_cap = std::make_shared<cv::VideoCapture>(input);

    std::cout << "Loading plugin" << std::endl;
    InferenceEngine::InferencePlugin plugin = InferenceEngine::PluginDispatcher({""}).getPluginByDevice("MYRIAD");

    WorldConfig world_config = WorldConfig::from_file(SOURCE_DIR "/config.csv");

    DetectorOpenVino detector(net_config, plugin);
    CacheDetections cache(SOURCE_DIR "/data/metrics.db", input);
    CachedDetectorWriter cachedDetector(cache, detector);

    TrackerComp tracker(world_config, 0.5);
    tracker.use<PositionAffinity, PositionData>(1, 0.7);

    BusCounter counter(cachedDetector, tracker, world_config,
            [&cv_cap]() -> nonstd::optional<cv::Mat> {
                cv::Mat frame;
                bool okay = cv_cap->read(frame);
                return okay ? nonstd::optional<cv::Mat>(frame) : nonstd::nullopt;
            },
            [](const cv::Mat& frame) { cv::imshow("output", frame); },
            []() { return cv::waitKey(20) == 'q'; },
            [](Event event) {
                std::cout << "EVENT: " << name(event) << std::endl;
            }
    );

    counter.run(BusCounter::RUN_PARALLEL, true);

    return 0;
}
