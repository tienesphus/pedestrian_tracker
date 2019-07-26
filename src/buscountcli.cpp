#include "libbuscount.hpp"
#include "feature_affinity.hpp"
#include "position_affinity.hpp"
#include "detector_openvino.hpp"
#include "detector_opencv.hpp"
#include "video_sync.hpp"

#include "server/server_client.hpp"

#include <opencv2/highgui.hpp>
#include <opencv2/dnn/dnn.hpp>

#include <sqlite3.h>

#include <iostream>
#include <zconf.h>
#include <unistd.h>


int main() {

    /*
    DetectorOpenCV::NetConfig net_config {
        0.5f,               // thresh
        15,                 // clazz
        cv::Size(300, 300), // size
        1, //2/255.0,            // scale
        cv::Scalar(1,1,1),//cv::Scalar(127.5, 127.5, 127.5),     // mean
        "../models/MobileNetSSD_IE/MobileNetSSD.xml", // config
        "../models/MobileNetSSD_IE/MobileNetSSD.bin", // model
        //"../models/MobileNetSSD_caffe/MobileNetSSD.prototxt", // config
        //"../models/MobileNetSSD_caffe/MobileNetSSD.caffemodel",  // model
        cv::dnn::DNN_BACKEND_INFERENCE_ENGINE,  // preferred backend
        cv::dnn::DNN_TARGET_MYRIAD,  // preferred device
    };
    */

    DetectorOpenVino::NetConfig net_config {
            0.6f,               // thresh
            15,                 // clazz
            std::string(SOURCE_DIR) + "/models/MobileNetSSD_IE/MobileNetSSD.xml", // config
            std::string(SOURCE_DIR) + "/models/MobileNetSSD_IE/MobileNetSSD.bin", // model
    };

    FeatureAffinity::NetConfig tracker_config {
            std::string(SOURCE_DIR) + "/models/Reidentify0031/person-reidentification-retail-0031.xml", // config
            std::string(SOURCE_DIR) + "/models/Reidentify0031/person-reidentification-retail-0031.bin", // model
            cv::Size(48, 96),    // input size
            0.6,                 // similarity thresh
    };

    //std::string input = std::string(SOURCE_DIR) + "/../samplevideos/pi3_20181213/2018-12-13--08-26-02--snippit-1.mp4";
    //VideoSync<cv::Mat> cap = VideoSync<cv::Mat>::from_video(input);
    auto cv_cap = std::make_shared<cv::VideoCapture>(0);
    cv_cap->set(cv::CAP_PROP_FRAME_WIDTH,640);
    cv_cap->set(cv::CAP_PROP_FRAME_HEIGHT,480);

    std::cout << "Loading plugin" << std::endl;
    InferenceEngine::InferencePlugin plugin = InferenceEngine::PluginDispatcher({""}).getPluginByDevice("MYRIAD");

    DetectorOpenVino detector(net_config, plugin);
    WorldConfig world_config = WorldConfig::from_file(std::string(SOURCE_DIR) + "/config.csv");
    TrackerComp tracker(world_config, 0.5);

    tracker.use<FeatureAffinity, FeatureData>(0.6, tracker_config, plugin);
    tracker.use<PositionAffinity, PositionData>(0.4, 0.7);

    sqlite3* db;
    std::string database_source = (std::string(SOURCE_DIR) + "/data/database.db");
    if (sqlite3_open(database_source.c_str(), &db) != SQLITE_OK) {
        throw std::logic_error("Cannot open database");
    }

    BusCounter counter(detector, tracker, world_config,
            //[&cap]() -> nonstd::optional<cv::Mat> { return cap.next(); },
            [&cv_cap]() -> nonstd::optional<cv::Mat> { cv::Mat frame; cv_cap->read(frame); return frame; },
            [](const cv::Mat& frame) { cv::imshow("output", frame); },
            []() { return cv::waitKey(20) == 'q'; },
            [&db](Event event) {
                std::cout << "EVENT: " << name(event) << std::endl;
                char* error = nullptr;
                std::string sql = "INSERT INTO Events(Name) VALUES ('" + name(event) + "')";
                if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &error) != SQLITE_OK) {
                    std::cout << "SQL ERROR: " << error << std::endl;
                    sqlite3_free(error);
                }
            }
    );


    std::atomic_bool running = { true };

    std::thread config_update([&running, &db, &world_config]() {
        std::string last_update;
        while (running) {
            char *error = nullptr;

            // fetch the latest update timestamp from the database
            std::string latest_update = "?";
            if (sqlite3_exec(db, "SELECT MAX(time) FROM ConfigUpdate",
                    [](void* latest_update, int argc, char** argv, char**) -> int {
                        if (argc != 1)
                            throw std::logic_error("Required one result");
                        *(std::string*)latest_update = argv[0];
                        return 0;
                    }, &latest_update, &error) != SQLITE_OK) {
                std::cout << "SELECT ERROR: " << error << std::endl;
                sqlite3_free(error);
                continue;
            }

            if (latest_update == "?") {
                throw std::logic_error("Latest update was not updated");
            }

            // fetch the config and update (if needed)
            if (last_update < latest_update) {
                if (sqlite3_exec(db, ("SELECT Config FROM ConfigUpdate WHERE Time='"+latest_update+"'").c_str(),
                        [](void* worldConfig, int argc, char** data, char**) -> int {
                            if (argc != 1)
                                throw std::logic_error("SELECT config muist return one column");
                            Json::Reader reader;
                            Json::Value json;
                            bool success = reader.parse(data[0], json);
                            if (!success)
                                throw std::logic_error("Cannot read json config from database");
                            nonstd::optional<OpenCVConfig> config = cvconfig_from_json(json);
                            if (!config)
                                throw std::logic_error("Cannot convert json config to config");
                            ((WorldConfig*)worldConfig)->crossing = config->crossing;
                            return 0;
                        }, &world_config,
                        &error) != SQLITE_OK) {
                    std::cout << "SELECT ERROR: " << error << std::endl;
                    sqlite3_free(error);
                    continue;
                } else {
                    last_update = std::move(latest_update);
                }
            }

            // Don't hog the CPU
            usleep(100 * 1000); // 100 ms
        }
    });

    counter.run(BusCounter::RUN_SERIAL, true);

    if (db) {
        std::cout << "Closing SQL" << std::endl;
        sqlite3_close(db);
    }

    running = false;
    config_update.join();

    return 0;
}
