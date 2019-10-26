// Standard includes
#include <iostream>
#include <tuple>
#include <vector>

// C++ includes
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <spdlog/spdlog.h>

// C includes
#include <unistd.h>
#include <sqlite3.h>

// Project libraries includes
#include <spdlog/spdlog.h>

// Project includes
#include "libbuscount.hpp"
#include "tracking/feature_affinity.hpp"
#include "tracking/position_affinity.hpp"
#include "detection/detector_openvino.hpp"
#include "detection/detector_opencv.hpp"
#include "video_sync.hpp"
#include "data_fetch.hpp"


int main() {
    std::cout << "STARTING" << std::endl;

    spdlog::default_logger()->set_level(spdlog::level::debug);

    /*
    DetectorOpenCV::NetConfig net_config {
        0.5f,               // thresh
        15,                 // clazz
        cv::Size(300, 300), // size
        1, //2/255.0,            // scale
        cv::Scalar(1,1,1),//cv::Scalar(127.5, 127.5, 127.5),     // mean
        SOURCE_DIR "/models/MobileNetSSD_IE/MobileNetSSD.xml", // config
        SOURCE_DIR "/models/MobileNetSSD_IE/MobileNetSSD.bin", // model
        // SOURCE_DIR "/models/MobileNetSSD_caffe/MobileNetSSD.prototxt", // config
        // SOURCE_DIR "/models/MobileNetSSD_caffe/MobileNetSSD.caffemodel",  // model
        cv::dnn::DNN_BACKEND_INFERENCE_ENGINE,  // preferred backend
        cv::dnn::DNN_TARGET_MYRIAD,  // preferred device
    };
    */

    DetectorOpenVino::NetConfig net_config {
            0.6f,               // thresh
            15,                 // clazz
            SOURCE_DIR "/models/MobileNetSSD_IE/MobileNetSSD.xml", // config
            SOURCE_DIR "/models/MobileNetSSD_IE/MobileNetSSD.bin", // model
    };

    FeatureAffinity::NetConfig tracker_config {
            SOURCE_DIR "/models/Reidentify0031/person-reidentification-retail-0031.xml", // config
            SOURCE_DIR "/models/Reidentify0031/person-reidentification-retail-0031.bin", // model
            cv::Size(48, 96),    // input size
    };

    std::string input = SOURCE_DIR "/../samplevideos/pi3_20181213/2018-12-13--08-26-02--snippit-1.mp4";
    VideoSync<cv::Mat> cap = VideoSync<cv::Mat>::from_video(input);
    //auto cv_cap = std::make_shared<cv::VideoCapture>(0);

    spdlog::info("Loading plugin");
    InferenceEngine::InferencePlugin plugin = InferenceEngine::PluginDispatcher({""}).getPluginByDevice("MYRIAD");

    DetectorOpenVino detector(net_config, plugin);
    WorldConfig world_config = WorldConfig::from_file(SOURCE_DIR "/config.csv");
    TrackerComp tracker(world_config, 0.5, 0.03, 0.2);

    tracker.use<FeatureAffinity, FeatureData>(1, tracker_config, plugin);
    tracker.use<PositionAffinity, PositionData>(1, 1);

    DataFetch data(SOURCE_DIR "/data/database.db");

    // Keep checking the database for any changes to the config
    std::atomic_bool running = { true };
    std::thread config_updater([&running, &data, &world_config]() {
        while (running) {

            data.check_config_update([&world_config](WorldConfig new_config) {
                world_config.crossing = new_config.crossing;
            });

            // Don't hog the CPU
            usleep(100 * 1000); // 100 ms
        }
    });

    BusCounter counter(detector, tracker, world_config,
            [&cap]() -> nonstd::optional<std::tuple<cv::Mat, int>> { return cap.next(); },
            /*[&cv_cap]() -> nonstd::optional<cv::Mat> {
                cv::Mat frame;
                cv_cap->read(frame);
                cv::resize(frame, frame, cv::Size(640, 480));
                return frame;
            },*/
            [](const cv::Mat& frame) { cv::imshow("output", frame); },
            []() { return cv::waitKey(1) == 'q'; },
            [&data](Event event, const cv::Mat&, int frame_no) {
                spdlog::info("EVENT: {}, {}", name(event), frame_no);
                data.enter_event(event);
            }
    );

    counter.run(BusCounter::RUN_PARALLEL, true);

    running = false;
    config_updater.join();

    return 0;
}
