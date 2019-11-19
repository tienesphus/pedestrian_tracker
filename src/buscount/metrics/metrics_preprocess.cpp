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

#include <unistd.h>
#include <spdlog/spdlog.h>
#include <dirent.h>
#include <string_utils.hpp>


int main_file(const std::string& full_filename, bool flip, Detector& detector, Tracker& tracker, WorldConfig& config) {

    auto cv_cap = std::make_shared<cv::VideoCapture>(full_filename);

    cv::namedWindow(full_filename);

    uint32_t frameno = 0;

    BusCounter counter(detector, tracker, config,
            [&cv_cap, &frameno, flip]() -> nonstd::optional<std::tuple<cv::Mat, int>> {
                cv::Mat frame;
                bool okay = cv_cap->read(frame);
                if (okay) {
                    if (flip)  {
                        cv::flip(frame, frame, 0);
                    }
                    return std::make_tuple(frame, frameno++);
                } else {
                    return nonstd::nullopt;
                }
            },
            [&full_filename](const cv::Mat& frame) { cv::imshow(full_filename, frame); },
            []() { return cv::waitKey(1) == 'q'; },
            [](Event event, const cv::Mat&, int) {
                spdlog::debug("EVENT: {}", name(event));
            }
    );

    counter.run(BusCounter::RUN_PARALLEL, true);

    cv::destroyWindow(full_filename);
    return 0;
}


std::vector<std::string> list_video_files(const std::string& trials_dir_name) {
    // list out all the video files
    std::vector<std::string> files;
    DIR *trials_dir;
    if ((trials_dir = opendir(trials_dir_name.c_str())) != nullptr) {
        struct dirent *trials_ent;
        while ((trials_ent  = readdir (trials_dir)) != nullptr) {
            // pi_dir_name will be pi1, pi2, etc
            const std::string pi_dir_name = trials_ent->d_name;
            DIR  *pi_dir;
            if ((pi_dir = opendir((trials_dir_name + "/" + pi_dir_name).c_str())) != nullptr) {
                struct dirent *pi_ent;
                while ((pi_ent = readdir (pi_dir)) != nullptr) {
                    // dirname will be e.g. 20181212
                    const std::string date_dir_name = pi_ent->d_name;
                    DIR  *date_dir;
                    if ((date_dir = opendir((trials_dir_name + "/" + pi_dir_name + "/" + date_dir_name).c_str())) != nullptr) {
                        struct dirent *date_ent;
                        while ((date_ent = readdir (date_dir)) != nullptr) {
                            // file will be something like '2018-12-12--15-25-53.mp4'
                            std::string video_file = date_ent->d_name;
                            if (ends_with(video_file, ".mp4")) {
                                files.push_back(pi_dir_name + "/" + date_dir_name + "/" + video_file);
                            }
                        }
                        closedir(date_dir);
                    }
                }
                closedir(pi_dir);
            }
        }
        closedir(trials_dir);
    }

    std::sort(files.begin(), files.end());
    return files;
}


int main() {

    spdlog::set_level(spdlog::level::trace);

    // find all files
    std::string trials_folder = SOURCE_DIR "/../video_data/";
    auto files = list_video_files(trials_folder);

    // load detector and feature affity
    DetectorOpenVino::NetConfig net_config {
            0.1f,               // thresh (limit to store in database)
            15,                 // person class
            SOURCE_DIR "/models/MobileNetSSD_IE/MobileNetSSD.xml", // config
            SOURCE_DIR "/models/MobileNetSSD_IE/MobileNetSSD.bin", // model
    };

    FeatureAffinity::NetConfig tracker_config {
            SOURCE_DIR "/models/Reidentify0031/person-reidentification-retail-0031.xml", // config
            SOURCE_DIR "/models/Reidentify0031/person-reidentification-retail-0031.bin", // model
            cv::Size(48, 96),    // input size
    };

    // load any random world config (really doesn't matter which)
    WorldConfig world_config = WorldConfig::from_file(SOURCE_DIR "/config.csv");

    spdlog::info("Loading plugin");
    InferenceEngine::InferencePlugin plugin = InferenceEngine::PluginDispatcher({""}).getPluginByDevice("MYRIAD");

    DetectorOpenVino detector(net_config, plugin);
    DetectionCache detection_cache(SOURCE_DIR "/data/detections.db", "unitialised");
    CachedDetector cachedDetector(detection_cache, detector, 0.2f);

    TrackerComp tracker(0.5, 0.05/3, 0.2);
    FeatureAffinity affinity(tracker_config, plugin);
    FeatureCache feature_cache(SOURCE_DIR "/data/features.db", "unitialised");
    tracker.use<CachedFeatures, FeatureData>(1.0f, affinity, feature_cache);


    for (const std::string& file: files) {
        auto full_file = trials_folder + file;
        spdlog::info("{} -> {}", file, full_file);
        bool flip = file.find("flip") != std::string::npos;

        detection_cache.setTag(file);
        feature_cache.setTag(file);

        main_file(full_file, flip, cachedDetector, tracker, world_config);
    }
}
