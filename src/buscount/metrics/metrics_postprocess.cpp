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
#include "compare_gt.hpp"
#include "../tracking/original_tracker.hpp"

#include <data_fetch.hpp>

#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include <sqlite3.h>

#include <unistd.h>
#include <fstream>
#include <iomanip>
#include <time_utils.hpp>
#include <spdlog/spdlog.h>
#include <file_utils.hpp>

int main_file(const std::string& input, Detector& detector, Tracker& tracker, WorldConfig& world_config,
              const std::vector<Bucket>& gt, std::map<stoptime, StopData>& sut, double* time,
              std::ofstream& output, bool draw) {

    int frames = cv::VideoCapture(input).get(cv::CAP_PROP_FRAME_COUNT);
    cv::Mat dummy(1, 1, CV_8U);
    auto cap_headless = ReadSimulator<>([&dummy, &frames]() -> nonstd::optional<cv::Mat> {
        --frames;
        if (frames >= 0)
            return dummy;
        else {
            return nonstd::nullopt;
        }
    }, 25, time);

    auto cap_drawing = ReadSimulator<>::from_video(input, time);
    //*time += 9*60 + 00; // skip to a specific point in the video

    BusCounter counter(detector, tracker, world_config,
            [draw, &cap_headless, &cap_drawing]() -> auto {
                if (draw)
                    return cap_drawing.next();
                else
                    return cap_headless.next();
            },
            [draw](const cv::Mat& frame) {
                if (draw)
                    cv::imshow("output", frame);
            },
            [draw]() {
                if (draw)
                    return cv::waitKey(0) == 'q';
                else
                    return false;
            },
            [&output, &gt, &sut, &input](Event event, const cv::Mat&, int frame_no) {
                spdlog::info("EVENT: {}, {}", name(event), frame_no);

                auto time = calctime(input, frame_no);
                spdlog::info("  OCCURED AT: {}", date_to_string(time, "%Y-%m-%d %H-%M-%S"));
                auto close = closest(time, gt);
                spdlog::info("  ASSIGNING : {}", date_to_string(close,  "%Y-%m-%d %H-%M-%S"));

                // record the event
                append(close, sut, event);

                // log it in results file
                //output << date_to_string(time, "%Y-%m-%d %H:%M:%S") << ", " << date_to_string(close, "%Y-%m-%d %H:%M:%S") << ", " << name(event) << std::endl;
                output << input << ", " << frame_no << ", " << name(event) << ", " << date_to_string(time, "%Y-%m-%d %H:%M:%S") << ", " << date_to_string(close, "%Y-%m-%d %H:%M:%S") << std::endl;
            }
    );

    counter.run(BusCounter::RUN_SERIAL, draw);

    return 0;
}


void process(const std::string& pi, bool front, const std::string& bus, const time_t& date, const std::string& video_folder, const std::string& gt_folder) {
    const std::string video_day = pi + "/" + date_to_string(date, "%Y%m%d");
    auto files = list_files_recursive(video_folder + "/" + video_day, ".*\\.mp4");
    std::sort(files.begin(), files.end());

    std::vector<Bucket> gt = read_gt(gt_folder + "/" + bus + "-" + date_to_string(date, "%Y%m%d") + ".csv", front, date);
    std::map<stoptime, StopData> sut;

    double time = 0;

    WorldConfig world_config = front ?
            WorldConfig::from_file(SOURCE_DIR "/data/configs/front.csv") :
            WorldConfig::from_file(SOURCE_DIR "/data/configs/rear.csv");

    DetectionCache detection_cache(SOURCE_DIR "/data/cache/detections.db", "uninitialised");
    CachedDetector cached_detector(detection_cache, /*detector, */0.2);
    TimedDetector timedDetector(cached_detector, &time, 0.1/25);

    TrackerComp tracker(world_config, 0.5, 0.08, 0.2);
    FeatureCache feature_cache(SOURCE_DIR "/data/cache/features.db", "uninitialised");
    CachedFeatures cached_features(feature_cache);
    tracker.use<TimedAffinity<FeatureData>, FeatureData>(1.0f, cached_features, &time, 1/100.0);
    //tracker.use<PositionAffinity, PositionData>(1.0f, 0.7);
    tracker.use<OriginalTracker, OriginalData>(1.0f, 60.0/300);

    std::ofstream output(SOURCE_DIR "/data/results/" + bus + "-" + (front ? "F" : "R") + "-" + date_to_string(date, "%Y%m%d") + ".csv");

    for (const std::string& file: files) {
        auto full_file = video_day + "/" + file;

        detection_cache.setTag(full_file);
        feature_cache.setTag(full_file);

        main_file(video_folder+"/"+full_file, timedDetector, tracker, world_config, gt, sut, &time, output, true);
    }

    output.close();

    Error err =  compute_error(gt, sut);
    std::cout << "ERROR CALCS: " << video_day << std::endl;
    std::cout << "         in: " << err.in << std::endl;
    std::cout << "        out: " << err.out << std::endl;
    std::cout << "      total: " << err.total << std::endl;
    std::cout << "     errors: " << err.total_errs << std::endl;
    std::cout << "  exchanges: " << err.total_exchange<< std::endl;
}


int main() {
    spdlog::set_level(spdlog::level::err);


    std::vector<std::tuple<std::string, bool, std::string, time_t>> deployments;
    deployments.emplace_back("pi4", false, "2031", date(2018, 12, 12));
    for (int i = 12; i <= 14; ++i) {
        deployments.emplace_back("pi1", true,  "2125", date(2018, 12, i));
        deployments.emplace_back("pi2", false, "2125", date(2018, 12, i));
        deployments.emplace_back("pi3", true,  "2031", date(2018, 12, i));
        deployments.emplace_back("pi4", false, "2031", date(2018, 12, i));
    }
    for (int i = 15; i <= 16; ++i) {
        deployments.emplace_back("pi1", true,  "2031", date(2018, 12, i));
        deployments.emplace_back("pi2", false, "2031", date(2018, 12, i));
        deployments.emplace_back("pi3", true,  "2125", date(2018, 12, i));
        deployments.emplace_back("pi4", false, "2125", date(2018, 12, i));
    }

    for (const auto& deploy : deployments) {
        process(
                std::get<0>(deploy),
                std::get<1>(deploy),
                std::get<2>(deploy),
                std::get<3>(deploy),
                "/home/matt/ml/bus_counting/bus_counting_cpp/video_data/",
                SOURCE_DIR "/data/gt"
        );
    }
}
