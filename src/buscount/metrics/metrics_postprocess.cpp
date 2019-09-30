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
                spdlog::info("  OCCURED AT: ", date_to_string(time, "%Y-%m-%d %H-%M-%S"));
                auto close = closest(time, gt);
                spdlog::info("  ASSIGNING : ", date_to_string(close,  "%Y-%m-%d %H-%M-%S"));

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

int main() {
    auto files = {
            //"test.mp4"
            //"test_2.mp4"
            "pi3/20181212/2018-12-12--07-28-31.mp4",
            "pi3/20181212/2018-12-12--07-38-31.mp4",
            "pi3/20181212/2018-12-12--07-48-32.mp4",
            "pi3/20181212/2018-12-12--07-58-32.mp4",
            "pi3/20181212/2018-12-12--08-08-32.mp4",
            "pi3/20181212/2018-12-12--08-18-32.mp4",
            "pi3/20181212/2018-12-12--08-28-33.mp4",
            "pi3/20181212/2018-12-12--08-38-33.mp4",
            "pi3/20181212/2018-12-12--08-48-33.mp4",
            "pi3/20181212/2018-12-12--08-58-34.mp4",
            "pi3/20181212/2018-12-12--09-08-34.mp4",
            "pi3/20181212/2018-12-12--09-18-34.mp4",
            "pi3/20181212/2018-12-12--09-28-34.mp4",
            "pi3/20181212/2018-12-12--09-38-35.mp4",
            "pi3/20181212/2018-12-12--09-48-35.mp4",
            "pi3/20181212/2018-12-12--09-58-35.mp4",
            "pi3/20181212/2018-12-12--10-08-35.mp4",
            "pi3/20181212/2018-12-12--10-18-36.mp4",
            "pi3/20181212/2018-12-12--10-28-36.mp4",
            "pi3/20181212/2018-12-12--10-38-36.mp4",
            "pi3/20181212/2018-12-12--10-48-36.mp4",
            "pi3/20181212/2018-12-12--10-58-37.mp4",
            "pi3/20181212/2018-12-12--11-08-37.mp4",
            "pi3/20181212/2018-12-12--11-18-37.mp4",
            "pi3/20181212/2018-12-12--11-28-37.mp4",
            "pi3/20181212/2018-12-12--11-38-38.mp4",
            "pi3/20181212/2018-12-12--11-48-38.mp4",
            "pi3/20181212/2018-12-12--11-58-38.mp4",
            "pi3/20181212/2018-12-12--12-08-38.mp4",
            "pi3/20181212/2018-12-12--12-18-39.mp4",
            "pi3/20181212/2018-12-12--12-28-39.mp4",
            "pi3/20181212/2018-12-12--12-38-39.mp4",
            "pi3/20181212/2018-12-12--12-48-39.mp4",
            "pi3/20181212/2018-12-12--12-58-40.mp4",
            "pi3/20181212/2018-12-12--13-08-40.mp4",
            "pi3/20181212/2018-12-12--13-18-40.mp4",
            "pi3/20181212/2018-12-12--13-28-40.mp4",
            "pi3/20181212/2018-12-12--13-38-41.mp4",
            "pi3/20181212/2018-12-12--13-48-41.mp4",
            "pi3/20181212/2018-12-12--13-58-41.mp4",
            "pi3/20181212/2018-12-12--14-08-41.mp4",
            "pi3/20181212/2018-12-12--14-18-42.mp4",
            "pi3/20181212/2018-12-12--14-28-42.mp4",
            "pi3/20181212/2018-12-12--14-38-42.mp4"

            /*"pi4/20181212/2018-12-12--15-25-53.mp4",
            "pi4/20181212/2018-12-12--07-45-41.mp4",
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
            "pi4/20181212/2018-12-12--13-55-51.mp4"*/
    };


    std::vector<Bucket> gt = read_gt(SOURCE_DIR "/data/2031 12.csv", true, date(2018,12,12));
    std::map<stoptime, StopData> sut;

    FeatureAffinity::NetConfig tracker_config {
            SOURCE_DIR "/models/Reidentify0031/person-reidentification-retail-0031.xml", // config
            SOURCE_DIR "/models/Reidentify0031/person-reidentification-retail-0031.bin", // model
            cv::Size(48, 96),    // input size
            0.6,                 // similarity thresh
    };
    DetectorOpenVino::NetConfig net_config {
            0.1f,               // thresh (to use for detection cache. Evaluation thresh is below)
            15,                 // clazz
            SOURCE_DIR "/models/MobileNetSSD_IE/MobileNetSSD.xml", // config
            SOURCE_DIR "/models/MobileNetSSD_IE/MobileNetSSD.bin", // model
    };

    double time = 0;

    WorldConfig world_config = WorldConfig::from_file(SOURCE_DIR "/config_2.csv");

    //InferenceEngine::InferencePlugin plugin = InferenceEngine::PluginDispatcher({""}).getPluginByDevice("MYRIAD");

    //DetectorOpenVino detector(net_config, plugin);
    DetectionCache detection_cache(SOURCE_DIR "/data/metrics.db", "uninitialised");
    CachedDetector cached_detector(detection_cache, /*detector, */0.3);
    TimedDetector timedDetector(cached_detector, &time, 3.0/25);

    TrackerComp tracker(world_config, 0.01, 0.05/3, 0.2);
    //FeatureAffinity affinity(tracker_config, plugin);
    //FeatureCache feature_cache(SOURCE_DIR "/data/metrics.db", "uninitialised");
    //CachedFeatures cached_features(/*affinity,*/ feature_cache);
    //tracker.use<TimedAffinity<FeatureData>, FeatureData>(0.5, cached_features, &time, 1/100.0);
    //tracker.use<PositionAffinity, PositionData>(1.0f, 0.7);
    tracker.use<OriginalTracker, OriginalData>(1.0f, 60.0/300);

    spdlog::set_level(spdlog::level::err);

    std::ofstream output(SOURCE_DIR "/data/results.csv");

    for (std::string file: files) {
        auto full_file = "/home/buscount/code/trials/" + file;

        detection_cache.setTag(full_file);
        //feature_cache.setTag(full_file);

        main_file(full_file, timedDetector, tracker, world_config, gt, sut, &time, output, true);
    }

    output.close();

    Error err =  compute_error(gt, sut);
    std::cout << "ERROR CALCS: " << std::endl;
    std::cout << "         in: " << err.in << std::endl;
    std::cout << "        out: " << err.out << std::endl;
    std::cout << "      total: " << err.total << std::endl;
    std::cout << "     errors: " << err.total_errs << std::endl;
    std::cout << "  exchanges: " << err.total_exchange<< std::endl;
}
