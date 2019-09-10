#include "libbuscount.hpp"
#include "tracking/feature_affinity.hpp"
#include "tracking/position_affinity.hpp"
#include "detection/detector_openvino.hpp"
#include "detection/detector_opencv.hpp"
#include "video_sync.hpp"

#include <data_fetch.hpp>

#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include <sqlite3.h>

#include <iostream>
#include <unistd.h>

#include <string>
using namespace std;

#include <fstream>
#include <json/json.h>

int main() {

	// set variables to default values
	// they will be used if the file fails to open
	
	Json::Value root;
	std::cin >> root;
	
	std::ifstream config_doc("config.json", std::ifstream::binary);
	config_doc >> root;

	int optionA = root.get("optionA", "1" ).asInt();
	float thresh = root.get("thresh", "0.6" ).asFloat();
	int clazz = root.get("clazz", "15" ).asInt();
	string openVinoConfigDirectory = root.get("openVinoConfigDirectory", "/models/MobileNetSSD_IE/MobileNetSSD.xml" ).asString();
	string openVinoModelDirectory = root.get("openVinoModelDirectory", "/models/MobileNetSSD_IE/MobileNetSSD.bin" ).asString();
	string featureAffinityConfigDirectory = root.get("featureAffinityConfigDirectory", "/models/Reidentify0031/person-reidentification-retail-0031.xml" ).asString();
	string featureAffinityModelDirectory = root.get("featureAffinityModelDirectory", "/models/Reidentify0031/person-reidentification-retail-0031.bin" ).asString();
	int cvSizeA = root.get("cvSizeA", "48" ).asInt();
	int cvSizeB = root.get("cvSizeB", "96" ).asInt();
	int scale = root.get("scale", "1" ).asInt();
	int scaleA = root.get("scaleA", "1" ).asInt();
	int scaleB = root.get("scaleB", "1" ).asInt();
	int scaleC = root.get("scaleC", "1" ).asInt();
	float similarityThresh = root.get("similarityThresh", "0.6" ).asFloat();
	string worldConfigDirectory = root.get("worldConfigDirectory", "/config.csv" ).asString();
	float trackerCompVariable = root.get("trackerCompVariable", "0.5" ).asFloat();
	float trackerVariableA = root.get("trackerVariableA", "0.6" ).asFloat();
	float trackerVariableB = root.get("trackerVariableB", "0.4" ).asFloat();
	float trackerVariableC = root.get("trackerVariableC", "0.7" ).asFloat();
	string databaseDirectory = root.get("databaseDirectory", "/data/database.db" ).asString();
	int sleepA = root.get("sleepA", "100" ).asInt();
	int sleepB = root.get("sleepB", "1000" ).asInt();
	int cvReSizeA = root.get("cvReSizeA", "640" ).asInt();
	int cvReSizeB = root.get("cvReSizeB", "480" ).asInt();
	int waitTime = root.get("waitTime", "20" ).asInt();


    if (optionA == 0) {
		DetectorOpenCV::NetConfig net_config {
			thresh,               // thresh
			clazz,                 // clazz
			cv::Size(cvSizeA, cvSizeB), // size
			scale, //2/255.0,            // scale
			cv::Scalar(scaleA,scaleB,scaleC),//cv::Scalar(127.5, 127.5, 127.5),     // mean
			string (SOURCE_DIR) + openVinoConfigDirectory, // config
			string (SOURCE_DIR) + openVinoModelDirectory, // model
			// SOURCE_DIR "/models/MobileNetSSD_caffe/MobileNetSSD.prototxt", // config
			// SOURCE_DIR "/models/MobileNetSSD_caffe/MobileNetSSD.caffemodel",  // model
			cv::dnn::DNN_BACKEND_INFERENCE_ENGINE,  // preferred backend
			cv::dnn::DNN_TARGET_MYRIAD,  // preferred device
		};
    }
	else {
		
		DetectorOpenVino::NetConfig net_config {
				thresh,               // thresh
				clazz,                 // clazz
				string (SOURCE_DIR) + openVinoConfigDirectory, // config
				string (SOURCE_DIR) + openVinoModelDirectory, // model
		};

		FeatureAffinity::NetConfig tracker_config {
				string (SOURCE_DIR) + featureAffinityConfigDirectory, // config
				string (SOURCE_DIR) + featureAffinityModelDirectory, // model
				cv::Size(cvSizeA, cvSizeB),    // input size
				similarityThresh,                 // similarity thresh
		};
	}
	
    //std::string input = std::string(SOURCE_DIR) + "/../samplevideos/pi3_20181213/2018-12-13--08-26-02--snippit-1.mp4";
    //VideoSync<cv::Mat> cap = VideoSync<cv::Mat>::from_video(input);
    auto cv_cap = std::make_shared<cv::VideoCapture>(0);

    std::cout << "Loading plugin" << std::endl;
    InferenceEngine::InferencePlugin plugin = InferenceEngine::PluginDispatcher({""}).getPluginByDevice("MYRIAD");

    DetectorOpenVino detector(net_config, plugin);
    WorldConfig world_config = WorldConfig::from_file(string (SOURCE_DIR) + worldConfigDirectory);
    TrackerComp tracker(world_config, trackerCompVariable);

    tracker.use<FeatureAffinity, FeatureData>(trackerVariableA, tracker_config, plugin);
    tracker.use<PositionAffinity, PositionData>(trackerVariableB, trackerVariableC);
	
    DataFetch data(string (SOURCE_DIR) + databaseDirectory);

    // Keep checking the database for any changes to the config
    std::atomic_bool running = { true };
    std::thread config_updater([&running, &data, &world_config, &sleepA, &sleepB]() {
        while (running) {

            data.check_config_update([&world_config](WorldConfig new_config) {
                world_config.crossing = new_config.crossing;
            });

            // Don't hog the CPU
            usleep(sleepA * sleepB); // 100 ms
        }
    });

    BusCounter counter(detector, tracker, world_config,
            //[&cap]() -> nonstd::optional<cv::Mat> { return cap.next(); },
            [&cv_cap, &cvReSizeA, &cvReSizeB]() -> nonstd::optional<cv::Mat> {
                cv::Mat frame;
                cv_cap->read(frame);
                cv::resize(frame, frame, cv::Size(cvReSizeA, cvReSizeB));
                return frame;
            },
            [](const cv::Mat& frame) { cv::imshow("output", frame); },
            [&waitTime]() { return cv::waitKey(waitTime) == 'q'; },
            [&data](Event event) {
                std::cout << "EVENT: " << name(event) << std::endl;
                data.enter_event(event);
            }
    );

    counter.run(BusCounter::RUN_PARALLEL, true);

    running = false;
    config_updater.join();

    return 0;
}