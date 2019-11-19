#include "../libbuscount.hpp"
#include "../tracking/feature_affinity.hpp"
#include "../tracking/position_affinity.hpp"
#include "../detection/detector_openvino.hpp"
#include "../detection/detector_opencv.hpp"
#include "cached_detector.hpp"
#include "cached_features.hpp"
#include "mot_detections.hpp"
#include "file_utils.hpp"
#include <string_utils.hpp>

#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include <sqlite3.h>

#include <unistd.h>
#include <spdlog/spdlog.h>
#include <dirent.h>
#include <fstream>


int main_file(const std::string& set_name, const std::vector<std::string>& files,
        Detector& detector, Tracker& tracker, WorldConfig& config, bool draw)
{

    uint32_t frameno = 0;

    if (draw)
        cv::namedWindow(set_name);

    cv::Mat buffer = cv::imread(files.empty() ? "" : files[0]);

    BusCounter counter(detector, tracker, config,
            [&files, &frameno, draw, &buffer]() -> nonstd::optional<std::tuple<cv::Mat, int>> {
                if (frameno < files.size()) {
                    if (draw) {
                        const auto &filename = files[frameno];
                        cv::Mat frame = cv::imread(filename);
                        // pre-increment frameno so that 1 is the start frameno (to matches MOT standard)
                        return std::make_tuple(frame, ++frameno);
                    } else {
                        return std::make_tuple(buffer, ++frameno);
                    }
                } else {
                    return nonstd::nullopt;
                }
            },
            [&set_name, draw](const cv::Mat& frame) {
                if (draw) {
                    cv::Mat output;
                    cv::resize(frame, output, cv::Size(640, 640 * frame.rows / frame.cols));
                    cv::imshow(set_name, output);
                }
            },
            [draw]() {
                if (draw)
                    return cv::waitKey(1) == 'q';
                else
                    return false;
            },
            [](Event event, const cv::Mat&, int) {
                spdlog::debug("EVENT: {}", name(event));
            }
    );

    counter.run(BusCounter::RUN_PARALLEL, draw);

    if (draw)
        cv::destroyWindow(set_name);

    return 0;
}


std::vector<std::tuple<std::string, std::vector<std::string>>> list_image_files(const std::string &trials_dir) {
    std::vector<std::tuple<std::string, std::vector<std::string>>> files;
    const auto sets = list_files(trials_dir);
    for (const auto& set : sets) {
        const auto files_relative = list_files_recursive(trials_dir + "/" + set, "img1/[^/]*\\.jpg");
        if (files_relative.empty())
            continue;
        std::vector<std::string> files_absolute;
        for (const auto& f : files_relative) {
            files_absolute.push_back(trials_dir + "/" + set + "/" + f);
        }
        std::sort(files_absolute.begin(), files_absolute.end());
        files.emplace_back(set, files_absolute);
    }
    return files;
}


int main() {

    spdlog::set_level(spdlog::level::info);

    // find all files
    std::string trials_folder = "/home/matt/ml/data/2DMOT2015/train/";
    auto files = list_image_files(trials_folder);

    // load detector and feature affinity
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
    //InferenceEngine::InferencePlugin plugin = InferenceEngine::PluginDispatcher({""}).getPluginByDevice("MYRIAD");

    for (const auto& set: files) {

        const std::string& set_name = std::get<0>(set);
        //if (set_name != std::string("ADL-Rundle-8"))
        //    continue;
        const std::vector<std::string>& set_files = std::get<1>(set);
        spdlog::info("Processing {}", set_name);

        //DetectorOpenVino detectorOV(net_config, plugin);
        DetectorMot detector(0.9f, 1);
        DetectionCache detection_cache(SOURCE_DIR "/data/detections_mot_train_mbnet.db", "uninitialised");
        CachedDetector cachedDetector(detection_cache, /*detectorOV,*/ 0.6f);

        //detector.set_file("/home/matt/Downloads/sort-master/data/" + set_name + "/det.txt");
        detection_cache.setTag(set_name);

        std::ofstream results(std::string("results/") + set_name + ".txt");

        TrackerComp tracker(0.2f, 0.03, 0.2,
                [&results](const cv::Mat& frame, uint32_t frameno, int index, const cv::Rect2f& box, float conf) {
                    // Write the MOT tracking results
                    int w = frame.cols;
                    int h = frame.rows;
                    results << frameno << "," << index <<"," << (box.x * w) << "," << (box.y * h) << ","
                            << (box.width *w) << "," << (box.height *h) << "," << (conf*100) << ",-1,-1,-1" << std::endl;
        });
        //FeatureAffinity affinity(tracker_config, plugin);
        //FeatureCache feature_cache(SOURCE_DIR "/data/features_mot_train_mbnet.db", "uninitialised");
        //tracker.use<CachedFeatures, FeatureData>(1.0f, /*affinity, */feature_cache);
        tracker.use<PositionAffinity, PositionData>(1.0f, 2.0f);

        //feature_cache.setTag(set_name);

        main_file(set_name, set_files, cachedDetector, tracker, world_config, false);

        results.close();
    }
}
