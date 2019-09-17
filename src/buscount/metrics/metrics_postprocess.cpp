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
#include <fstream>

int main_file(const std::string& input) {

    FeatureAffinity::NetConfig tracker_config {
            SOURCE_DIR "/models/Reidentify0031/person-reidentification-retail-0031.xml", // config
            SOURCE_DIR "/models/Reidentify0031/person-reidentification-retail-0031.bin", // model
            cv::Size(48, 96),    // input size
            0.6,                 // similarity thresh
    };
    DetectorOpenVino::NetConfig net_config {
            0.1f,               // thresh
            15,                 // clazz
            SOURCE_DIR "/models/MobileNetSSD_IE/MobileNetSSD.xml", // config
            SOURCE_DIR "/models/MobileNetSSD_IE/MobileNetSSD.bin", // model
    };

    double time = 0;

    //std::string input = std::string(SOURCE_DIR) + "/../samplevideos/pi3_20181213/2018-12-13--08-26-02--snippit-1.mp4";
    auto cap = ReadSimulator<>::from_video(input, &time);

    WorldConfig world_config = WorldConfig::from_file(SOURCE_DIR "/config.csv");

    InferenceEngine::InferencePlugin plugin = InferenceEngine::PluginDispatcher({""}).getPluginByDevice("MYRIAD");

    DetectorOpenVino detector(net_config, plugin);
    DetectionCache cache(SOURCE_DIR "/data/metrics.db", input);
    CachedDetector cached_detector(cache, detector, 0.6);
    TimedDetector timedDetector(cached_detector, &time, 1/11.0);

    TrackerComp tracker(world_config, 0.5);
    FeatureAffinity affinity(tracker_config, plugin);
    FeatureCache feature_cache(SOURCE_DIR "/data/metrics.db", input);
    CachedFeatures cached_features(affinity, feature_cache);
    tracker.use<TimedAffinity<FeatureData>, FeatureData>(0.5, cached_features, &time, 1/20.0);
    tracker.use<PositionAffinity, PositionData>(0.5, 0.7);

    std::ofstream output(input + ".new-results.csv");

    BusCounter counter(timedDetector, tracker, world_config,
            [&cap]() -> nonstd::optional<std::tuple<cv::Mat, int>> { return cap.next(); },
            [](const cv::Mat& frame) {
                cv::imshow("output", frame);
            },
            []() {
                return cv::waitKey(1) == 'q';
            },
            [&output](Event event, const cv::Mat&, int frame_no) {
                std::cout << "EVENT: " << name(event) << " " << frame_no << std::endl;
                output << frame_no << name(event) << std::endl;
            }
    );

    counter.run(BusCounter::RUN_SERIAL, true);

    output.close();

    return 0;
}

int main() {
    auto files = {
            "pi4/20181212/2018-12-12--07-45-41.mp4",
            "pi4/20181212/2018-12-12--15-25-53.mp4",
            "pi4/20181212/2018-12-12--08-15-42.mp4",
            "pi4/20181212/2018-12-12--11-15-47.mp4",
            "pi4/20181212/2018-12-12--09-25-44.mp4",
            "pi4/20181212/2018-12-12--16-05-54.mp4",
            "pi4/20181212/2018-12-12--10-55-46.mp4",
            "pi4/20181212/2018-12-12--08-55-43.mp4",
            "pi4/20181212/2018-12-12--11-55-48.mp4",
            "pi4/20181212/2018-12-12--10-15-45.mp4",
            "pi4/20181212/2018-12-12--14-35-52.mp4",
            "pi4/20181212/2018-12-12--13-25-50.mp4",
            "pi4/20181212/2018-12-12--12-45-49.mp4",
            "pi4/20181212/2018-12-12--10-35-46.mp4",
            "pi4/20181212/2018-12-12--09-35-44.mp4",
            "pi4/20181212/2018-12-12--09-45-45.mp4",
            "pi4/20181212/2018-12-12--15-45-54.mp4",
            "pi4/20181212/2018-12-12--15-55-54.mp4",
            "pi4/20181212/2018-12-12--07-55-42.mp4",
            "pi4/20181212/2018-12-12--15-15-53.mp4",
            "pi4/20181212/2018-12-12--09-15-44.mp4",
            "pi4/20181212/2018-12-12--08-35-43.mp4",
            "pi4/20181212/2018-12-12--13-35-50.mp4",
            "pi4/20181212/2018-12-12--14-25-51.mp4",
            "pi4/20181212/2018-12-12--12-55-49.mp4",
            "pi4/20181212/2018-12-12--07-35-41.mp4",
            "pi4/20181212/2018-12-12--10-45-46.mp4",
            "pi4/20181212/2018-12-12--10-25-46.mp4",
            "pi4/20181212/2018-12-12--12-35-49.mp4",
            "pi4/20181212/2018-12-12--07-25-41.mp4",
            "pi4/20181212/2018-12-12--10-05-45.mp4",
            "pi4/20181212/2018-12-12--11-25-47.mp4",
            "pi4/20181212/2018-12-12--11-35-47.mp4",
            "pi4/20181212/2018-12-12--08-05-42.mp4",
            "pi4/20181212/2018-12-12--08-25-42.mp4",
            "pi4/20181212/2018-12-12--13-15-50.mp4",
            "pi4/20181212/2018-12-12--08-45-43.mp4",
            "pi4/20181212/2018-12-12--14-05-51.mp4",
            "pi4/20181212/2018-12-12--11-45-47.mp4",
            "pi4/20181212/2018-12-12--15-05-53.mp4",
            "pi4/20181212/2018-12-12--14-45-52.mp4",
            "pi4/20181212/2018-12-12--11-05-46.mp4",
            "pi4/20181212/2018-12-12--12-15-48.mp4",
            "pi4/20181212/2018-12-12--09-05-43.mp4",
            "pi4/20181212/2018-12-12--14-55-52.mp4",
            "pi4/20181212/2018-12-12--15-35-53.mp4",
            "pi4/20181212/2018-12-12--13-45-51.mp4",
            "pi4/20181212/2018-12-12--14-15-51.mp4",
            "pi4/20181212/2018-12-12--13-05-50.mp4",
            "pi4/20181212/2018-12-12--12-05-48.mp4",
            "pi4/20181212/2018-12-12--12-25-48.mp4",
            "pi4/20181212/2018-12-12--09-55-45.mp4",
            "pi4/20181212/2018-12-12--13-55-51.mp4",

            "pi3/20181212/2018-12-12--08-08-32.mp4",
            "pi3/20181212/2018-12-12--11-38-38.mp4",
            "pi3/20181212/2018-12-12--10-28-36.mp4",
            "pi3/20181212/2018-12-12--13-28-40.mp4",
            "pi3/20181212/2018-12-12--11-28-37.mp4",
            "pi3/20181212/2018-12-12--14-38-42.mp4",
            "pi3/20181212/2018-12-12--14-28-42.mp4",
            "pi3/20181212/2018-12-12--09-18-34.mp4",
            "pi3/20181212/2018-12-12--09-58-35.mp4",
            "pi3/20181212/2018-12-12--12-38-39.mp4",
            "pi3/20181212/2018-12-12--10-38-36.mp4",
            "pi3/20181212/2018-12-12--09-48-35.mp4",
            "pi3/20181212/2018-12-12--13-48-41.mp4",
            "pi3/20181212/2018-12-12--09-08-34.mp4",
            "pi3/20181212/2018-12-12--11-18-37.mp4",
            "pi3/20181212/2018-12-12--11-58-38.mp4",
            "pi3/20181212/2018-12-12--10-18-36.mp4",
            "pi3/20181212/2018-12-12--13-08-40.mp4",
            "pi3/20181212/2018-12-12--14-18-42.mp4",
            "pi3/20181212/2018-12-12--08-58-34.mp4",
            "pi3/20181212/2018-12-12--10-48-36.mp4",
            "pi3/20181212/2018-12-12--07-28-31.mp4",
            "pi3/20181212/2018-12-12--07-48-32.mp4",
            "pi3/20181212/2018-12-12--10-58-37.mp4",
            "pi3/20181212/2018-12-12--11-48-38.mp4",
            "pi3/20181212/2018-12-12--14-08-41.mp4",
            "pi3/20181212/2018-12-12--12-48-39.mp4",
            "pi3/20181212/2018-12-12--10-08-35.mp4",
            "pi3/20181212/2018-12-12--12-58-40.mp4",
            "pi3/20181212/2018-12-12--12-18-39.mp4",
            "pi3/20181212/2018-12-12--08-48-33.mp4",
            "pi3/20181212/2018-12-12--09-38-35.mp4",
            "pi3/20181212/2018-12-12--08-38-33.mp4",
            "pi3/20181212/2018-12-12--07-58-32.mp4",
            "pi3/20181212/2018-12-12--11-08-37.mp4",
            "pi3/20181212/2018-12-12--12-28-39.mp4",
            "pi3/20181212/2018-12-12--08-28-33.mp4",
            "pi3/20181212/2018-12-12--13-18-40.mp4",
            "pi3/20181212/2018-12-12--08-18-32.mp4",
            "pi3/20181212/2018-12-12--13-38-41.mp4",
            "pi3/20181212/2018-12-12--13-58-41.mp4",
            "pi3/20181212/2018-12-12--12-08-38.mp4",
            "pi3/20181212/2018-12-12--07-38-31.mp4",
            "pi3/20181212/2018-12-12--09-28-34.mp4"
    };
    for (std::string file: files) {
        auto full_file = "/home/matt/ml/bus_counting/trials/" + file;
        main_file(full_file);
    }
}
