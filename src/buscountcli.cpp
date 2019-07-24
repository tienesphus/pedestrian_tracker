#include <iostream>

#include <opencv2/highgui.hpp>

#include "libbuscount.hpp"
#include "feature_affinity.hpp"
#include "position_affinity.hpp"
#include "detector_openvino.hpp"
#include "detector_opencv.hpp"
#include "video_sync.hpp"

#include "server/server.hpp"

#include <opencv2/dnn/dnn.hpp>

int main() {

    /*DetectorOpenCV::NetConfig net_config {
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
    };*/

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

    std::string input = std::string(SOURCE_DIR) + "/../samplevideos/pi3_20181213/2018-12-13--08-26-02--snippit-1.mp4";
    VideoSync<cv::Mat> cap = VideoSync<cv::Mat>::from_video(input);
    //auto cv_cap = std::make_shared<cv::VideoCapture>(0);
    //cv_cap->set(cv::CAP_PROP_FRAME_WIDTH,640);
    //cv_cap->set(cv::CAP_PROP_FRAME_HEIGHT,480);
    
    //VideoSync<cv::Mat> cap = VideoSync<cv::Mat>::from_capture(cv_cap);

    std::cout << "Loading plugin" << std::endl;
    InferenceEngine::InferencePlugin plugin = InferenceEngine::PluginDispatcher({""}).getPluginByDevice("MYRIAD");

    DetectorOpenVino detector(net_config, plugin);
    WorldConfig world_config = WorldConfig::from_file(std::string(SOURCE_DIR) + "/config.csv");
    TrackerComp tracker(world_config, 0.5);

    tracker.use<FeatureAffinity, FeatureData>(0.6, tracker_config, plugin);
    tracker.use<PositionAffinity, PositionData>(0.4, 0.7);

    BusCounter counter(detector, tracker, world_config,
            [&cap]() -> nonstd::optional<cv::Mat> { return cap.next(); },
            //[&cv_cap]() -> nonstd::optional<cv::Mat> { cv::Mat frame; cv_cap->read(frame); return frame; },
            [](const cv::Mat& frame) { cv::imshow("output", frame); },
            []() { return cv::waitKey(20) == 'q'; },
            [](Event event) {std::cout << "EVENT: " << name(event) << std::endl;}
    );


    server::init_master();
    server::init_slave(
            [&world_config]() -> server::Config {
                std::vector<server::Feed> feeds;
                feeds.emplace_back("test", "/test");
                feeds.emplace_back("live", "/live");
                return server::Config(
                        server::OpenCVConfig(
                                server::Line(
                                        server::Point(world_config.crossing.a.x, world_config.crossing.a.y),
                                        server::Point(world_config.crossing.b.x, world_config.crossing.b.y)
                                )
                        ),
                        feeds);
            },
            [&world_config](server::OpenCVConfig config) {
                // TODO ugly conversion between server::Line and utils::Line
                world_config.crossing.a.x = config.crossing.a.x;
                world_config.crossing.a.y = config.crossing.a.y;
                world_config.crossing.b.x = config.crossing.b.x;
                world_config.crossing.b.y = config.crossing.b.y;
            }
    );

    //std::thread server_thread(server::start);

    counter.run(BusCounter::RUN_SERIAL, true);

    return 0;
}
