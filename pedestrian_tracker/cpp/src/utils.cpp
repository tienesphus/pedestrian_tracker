// Copyright (C) 2018-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#include "utils.hpp"

#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/core.hpp>

#include <fstream>
#include <string>
#include <sstream>

#include <algorithm>
#include <vector>
#include <map>
#include <string>
#include <set>
#include <memory>
#include <chrono>
#include <ctime>

using namespace InferenceEngine;
namespace {
template <typename StreamType>
void SaveDetectionLogToStream(StreamType& stream,
                              const DetectionLog& log,
                              const std::string& location) {
    struct tm * timestamp;
    time_t tTime;
    std::chrono::system_clock::time_point tPoint;
    std::string timeStr;
    
    for (const auto& entry : log) {
        std::vector<TrackedObject> objects(entry.objects.begin(),
                                           entry.objects.end());
        std::sort(objects.begin(), objects.end(),
                  [](const TrackedObject& a,
                     const TrackedObject& b)
                  { return a.object_id < b.object_id; });
        for (const auto& object : objects) {
            auto frame_idx_to_save = entry.frame_idx;
            stream << frame_idx_to_save << ',';
            //convert unix timestamp to local system time
            tPoint = std::chrono::system_clock::from_time_t(0) + std::chrono::milliseconds(object.timestamp);
            tTime = std::chrono::system_clock::to_time_t(tPoint);
            timestamp = localtime(&tTime);
            timeStr = asctime(timestamp);
            timeStr.pop_back();
            stream  << timeStr << ',' << object.object_id << ','
                << object.rect.x << ',' << object.rect.y << ','
                << object.rect.width << ',' << object.rect.height << ',' << object.confidence; 
            stream << "," << location;
            stream << '\n';
        }
    }
}
}  // anonymous namespace
namespace {
template <typename StreamType>
void SaveDetectionExtraLogToStream(StreamType& stream,
                              const DetectionLogExtra& log) {
    struct tm * timestamp;
    time_t tTime;
    std::chrono::system_clock::time_point tPoint;
    std::string timeStr;
    
    for (const auto& entry : log) {
        if(entry.second.time_of_stay ==0){
            continue;
        }else{
            tPoint = std::chrono::system_clock::from_time_t(0) + std::chrono::milliseconds(entry.second.init_time);
            tTime = std::chrono::system_clock::to_time_t(tPoint);
            timestamp = localtime(&tTime);
            timeStr = asctime(timestamp);
            timeStr.pop_back();
            stream << entry.second.object_id << ',' << timeStr << ','
                    << (float) entry.second.time_of_stay/1000;
            stream << '\n';
        }
        
    }
}
}  // anonymous namespace

void DrawPolyline(const std::vector<cv::Point>& polyline,
                  const cv::Scalar& color, cv::Mat* image, int lwd) {
    PT_CHECK(image);
    PT_CHECK(!image->empty());
    PT_CHECK_EQ(image->type(), CV_8UC3);
    PT_CHECK_GT(lwd, 0);
    PT_CHECK_LT(lwd, 20);

    for (size_t i = 1; i < polyline.size(); i++) {
        cv::line(*image, polyline[i - 1], polyline[i], color, lwd);
    }
}
cv::Point2f GetBottomPoint(const TrackedObject box){

    cv::Point2f temp_pnt(1); 

    float width, height;
    width = box.rect.width;
    height = box.rect.height;
    temp_pnt = cv::Point2f((box.rect.x + (width*0.5)), (box.rect.y+height));	
		
	return temp_pnt;
}
void MouseCallBack(int event, int x, int y, int flags, void* param)
{
    MouseParams* mp = (MouseParams *)param;
    cv::Mat *frame = ((cv::Mat *)mp->frame);
	if (event == cv::EVENT_LBUTTONDOWN)
	{
		if (mp->mouse_input.size() < 4) {
			cv::circle(*frame, cv::Point2f(x, y), 
            5, 
            cv::Scalar(0, 0, 255),
            4);
                
		}
		else
		{
			cv::circle(*frame, 
            cv::Point2f(x, y), 
            5, 
            cv::Scalar(255, 0, 0),
            4);
          
		}
		if (mp->mouse_input.size() >= 1 and mp->mouse_input.size() <= 3) {
            
			
			cv::line(*frame, cv::Point2f(x, y), 
            cv::Point2f(mp->mouse_input[mp->mouse_input.size() - 1].x,mp->mouse_input[mp->mouse_input.size() - 1].y),
            cv::Scalar(70, 70, 70), 
            2);
			if (mp->mouse_input.size() == 3) {
				line(*frame,  cv::Point2f(x, y),
                  cv::Point2f(mp->mouse_input[0].x, mp->mouse_input[0].y),
                    cv::Scalar(70, 70, 70), 
                   2);
			}

		}
        std::cout << "Point2f(" << x << ", " << y << ")," << std::endl;
		mp->mouse_input.push_back( cv::Point2f((float)x, (float)y));
        
        
	}
}

void SetCameraPoints(MouseParams *mp){

    for (;;) {
        cv::Mat* frame_copy = mp->frame;
        cv::imshow("image", *frame_copy);
        cv::waitKey(1);
        if (mp->mouse_input.size() == 8) {
            cv::destroyWindow("image");
            break;
            }
        
        }

}
std::vector<cv::Point2f> ReadConfig(const std::string& path){
    std::ifstream config_file(path);
    std::string line;
    std::vector<cv::Point2f>  points;
    if(!config_file.is_open()){
        throw std::runtime_error("Can't open camera config file (" +path+ ")");
    }
    if (config_file.peek() == std::ifstream::traits_type::eof()){
        throw std::runtime_error("config file is empty (" +path+ ")");
    }
    while (getline(config_file,line))
    {
        std::istringstream iss(line);
        std::string x_input,y_input;
        int x,y;
        if(!(iss >>x_input >> y_input)){
            break;
        }
        try{
            x =std::stoi(x_input);
            y =std::stoi(y_input);
            points.push_back(cv::Point2f(x,y));
        }catch(std::invalid_argument& e){
            throw std::runtime_error("config file is in a wrong format (" +path+ ")");
        }
        
    }
    if(points.size() !=8){
        throw std::runtime_error("config file should have total size of 8 (" +path+ ")");
    }
    return points;
}

void WriteConfig(const std::string &path, std::vector<cv::Point2f> points){
    std::ofstream config_file(path, std::ofstream::out | std::ofstream::trunc);

    if(!config_file.is_open()){
        throw std::runtime_error("Can't open camera config file (" +path+ ")");
    }
    for(const cv::Point2f &point :points){
        config_file << point.x << " " << point.y << std::endl;
    }
    config_file.close();
}


void SaveDetectionLogToTrajFile(const std::string& path,
                                const DetectionLog& log,
                                const std::string& location) {
    std::ofstream file(path.c_str());
    PT_CHECK(file.is_open());
    SaveDetectionLogToStream(file, log, location);
}
void SaveDetectionLogToTrajFile(const std::string& path,
                                const DetectionLogExtra& log) {
    std::ofstream file;
    file.open(path.c_str(),std::ios_base::app);
    SaveDetectionExtraLogToStream(file, log);
}

void PrintDetectionLog(const DetectionLog& log, const std::string& location) {
    SaveDetectionLogToStream(std::cout, log, location);
}

InferenceEngine::Core
LoadInferenceEngine(const std::vector<std::string>& devices,
                    const std::string& custom_cpu_library,
                    const std::string& custom_cldnn_kernels,
                    bool should_use_perf_counter) {
    std::set<std::string> loadedDevices;
    Core ie;

    for (const auto &device : devices) {
        if (loadedDevices.find(device) != loadedDevices.end()) {
            continue;
        }

        std::cout << "Loading device " << device << std::endl;
        std::cout << printable(ie.GetVersions(device)) << std::endl;

        /** Load extensions for the CPU device **/
        if ((device.find("CPU") != std::string::npos)) {
            if (!custom_cpu_library.empty()) {
                // CPU(MKLDNN) extensions are loaded as a shared library and passed as a pointer to base extension
                auto extension_ptr = std::make_shared<Extension>(custom_cpu_library);
                ie.AddExtension(std::static_pointer_cast<Extension>(extension_ptr), "CPU");
            }
        } else if (!custom_cldnn_kernels.empty()) {
            // Load Extensions for other plugins not CPU
            ie.SetConfig({{PluginConfigParams::KEY_CONFIG_FILE, custom_cldnn_kernels}}, "GPU");
        }

        if (should_use_perf_counter)
            ie.SetConfig({{PluginConfigParams::KEY_PERF_COUNT, PluginConfigParams::YES}});

        loadedDevices.insert(device);
    }

    return ie;
}
