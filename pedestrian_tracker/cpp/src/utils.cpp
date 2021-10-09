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

namespace
{
    template <typename StreamType>
    void SaveDetectionLogToStream(StreamType &stream,
                                  const DetectionLog &log,
                                  const std::string &location,
                                  const std::string &uuid)
    {
        struct tm *timestamp;
        time_t tTime;
        std::chrono::system_clock::time_point tPoint;
        std::string timeStr;

        for (const auto &entry : log)
        {
            std::vector<TrackedObject> objects(entry.objects.begin(),
                                               entry.objects.end());
            std::sort(objects.begin(), objects.end(),
                      [](const TrackedObject &a,
                         const TrackedObject &b)
                      { return a.object_id < b.object_id; });
            for (const auto &object : objects)
            {
                auto frame_idx_to_save = entry.frame_idx;
                stream << frame_idx_to_save << ',';
                //convert unix timestamp to local system time
                tPoint = std::chrono::system_clock::from_time_t(0) + std::chrono::milliseconds(object.timestamp);
                tTime = std::chrono::system_clock::to_time_t(tPoint);
                timestamp = localtime(&tTime);
                timeStr = asctime(timestamp);
                timeStr.pop_back();
                stream << timeStr << ',' << object.object_id << ','
                       << object.rect.x << ',' << object.rect.y << ','
                       << object.rect.width << ',' << object.rect.height << ',' << object.confidence;
                stream << "," << location;
                stream << "," << uuid;
                stream << '\n';
            }
        }
    }
} // anonymous namespace
namespace
{
    template <typename StreamType>
    void SaveDetectionExtraLogToStream(StreamType &stream,
                                       const DetectionLogExtra &log)
    {
        struct tm *timestamp;
        time_t tTime;
        std::chrono::system_clock::time_point tPoint;
        std::string timeStr;

        for (const auto &entry : log)
        {
            if (entry.second.time_of_stay == 0)
            {
                continue;
            }
            else
            {
                tPoint = std::chrono::system_clock::from_time_t(0) + std::chrono::milliseconds(entry.second.init_time);
                tTime = std::chrono::system_clock::to_time_t(tPoint);
                timestamp = localtime(&tTime);
                timeStr = asctime(timestamp);
                timeStr.pop_back();
                stream << entry.second.object_id << ',' << timeStr << ','
                       << (float)entry.second.time_of_stay / 1000 << ','
                       << entry.second.location;
                stream << '\n';
            }
        }
    }
} // anonymous namespace

void DrawPolyline(const std::vector<cv::Point> &polyline,
                  const cv::Scalar &color, cv::Mat *image, int lwd)
{
    PT_CHECK(image);
    PT_CHECK(!image->empty());
    PT_CHECK_EQ(image->type(), CV_8UC3);
    PT_CHECK_GT(lwd, 0);
    PT_CHECK_LT(lwd, 20);

    for (size_t i = 1; i < polyline.size(); i++)
    {
        cv::line(*image, polyline[i - 1], polyline[i], color, lwd);
    }
}

void DrawRoi(const std::vector<cv::Point2f> &polyline,
             const cv::Scalar &color, cv::Mat *image, int lwd)
{
    for (size_t i = 1; i < polyline.size(); i++)
    {
        cv::line(*image, polyline[i - 1], polyline[i], color, lwd);
    }
    cv::line(*image, polyline[0], polyline[polyline.size() - 1], color, lwd);
}
cv::Point2f GetBottomPoint(const cv::Rect box)
{

    cv::Point2f temp_pnt(1);
    float width, height;
    width = box.width;
    height = box.height;
    temp_pnt = cv::Point2f((box.x + (width * 0.5)), (box.y + height));

    return temp_pnt;
}
void MouseCallBack(int event, int x, int y, int flags, void *param)
{
    MouseParams *mp = (MouseParams *)param;
    cv::Mat *frame = ((cv::Mat *)mp->frame);
    if (event == cv::EVENT_LBUTTONDOWN)
    {
        if (mp->mouse_input.size() < 4)
        {
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
        if (mp->mouse_input.size() >= 1 and mp->mouse_input.size() <= 3)
        {

            cv::line(*frame, cv::Point2f(x, y),
                     cv::Point2f(mp->mouse_input[mp->mouse_input.size() - 1].x, mp->mouse_input[mp->mouse_input.size() - 1].y),
                     cv::Scalar(70, 70, 70),
                     2);
            if (mp->mouse_input.size() == 3)
            {
                line(*frame, cv::Point2f(x, y),
                     cv::Point2f(mp->mouse_input[0].x, mp->mouse_input[0].y),
                     cv::Scalar(70, 70, 70),
                     2);
            }
        }
        std::cout << "Point2f(" << x << ", " << y << ")," << std::endl;
        mp->mouse_input.push_back(cv::Point2f((float)x, (float)y));
    }
}

void SetPoints(MouseParams *mp, unsigned int point_num, std::string name)
{

    for (;;)
    {
        cv::Mat *frame_copy = mp->frame;
        cv::imshow(name, *frame_copy);
        cv::waitKey(1);
        if (mp->mouse_input.size() == point_num)
        {
            cv::destroyWindow(name);
            break;
        }
    }
}

//Read Camera and ROI configuration files
std::vector<cv::Point2f> ReadConfig(const std::string &path, const size_t &line_num)
{
    std::ifstream config_file(path);
    std::string line;
    std::vector<cv::Point2f> points;
    if (!config_file.is_open())
    {
        throw std::runtime_error("Can't open config file (" + path + "). Please ensure the folder/file exists");
    }
    if (config_file.peek() == std::ifstream::traits_type::eof())
    {
        throw std::runtime_error("config file is empty (" + path + ")");
    }
    while (getline(config_file, line))
    {
        std::istringstream iss(line);
        std::string x_input, y_input;
        int x, y;
        if (!(iss >> x_input >> y_input))
        {
            break;
        }
        try
        {
            x = std::stoi(x_input);
            y = std::stoi(y_input);
            points.push_back(cv::Point2f(x, y));
        }
        catch (std::invalid_argument &e)
        {
            throw std::runtime_error("config file is in a wrong format (" + path + ")");
        }
    }
    if (points.size() != line_num)
    {
        throw std::runtime_error("config file should have total size of " + std::to_string(line_num) + "(" + path + ")");
    }
    return points;
}
std::vector<std::string> SplitString(const std::string &s, char delim){
    std::vector<std::string> result;
    std::stringstream ss(s);
    std::string item;
    while (getline(ss,item,delim)){
        result.push_back(item);
    }
    return result;  
}
void WriteConfig(const std::string &path, const std::vector<cv::Point2f> &points){
    
    std::vector<std::string> temp = SplitString(path,'/');
    std::string folder_name = temp[0];
    //check whether the folder exists
    if(IsPathExist(folder_name)){
        std::ofstream config_file(path, std::ofstream::out | std::ofstream::trunc);
        if(!config_file.is_open()){
            WriteToNewFile(path,points);
            config_file.close();
        }else{
            for(const cv::Point2f &point :points){
                    config_file << point.x << " " << point.y << std::endl;
                }
            config_file.close();
        }
    }else{
        if(mkdir(folder_name.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1){
            throw std::runtime_error(strerror(errno));
        }
        WriteToNewFile(path,points);
    }
}
void WriteToNewFile(const std::string &file_name, const std::vector<cv::Point2f> &points){
    std::ofstream config_file(file_name);
    for(const cv::Point2f &point :points){
        config_file << point.x << " " << point.y << std::endl;
    }
    config_file.close();
}
float ToFloat(const std::string &str){
    std::istringstream iss(str);
    float temp;
    iss >> std::noskipws >> temp;
    //checking whether input is a valid number
    if (iss.eof() && !iss.fail()) {
        temp = std::stof(str);
    }
    else{
        throw std::runtime_error(str + " is not a valid input(numbers only with no white spaces)");
    }
    return temp;
}
void ReConfigWindow(const std::string &window_name, MouseParams *mp){
    
    //opening a window
    cv::namedWindow(window_name, 1);
    cv::setMouseCallback(window_name, MouseCallBack, (void *)mp);
}

//Triggers the reconfig sequence for both the roi or camera
void ReConfig(const std::string &input, MouseParams *mp)
{
    if (input == "cam")
    {
        std::string window_name = "Camera-Config";
        unsigned int num_points = 7; //number of points for re-config
        ReConfigWindow(window_name, mp);
        SetPoints(mp, num_points, window_name);
        WriteConfig(config_log_paths::PATHTOCAMCONFIG, mp->mouse_input);
    }
    else if (input == "roi")
    {
        std::string window_name = "ROI-Config";
        unsigned int num_points = 4; //number of points for re-config
        ReConfigWindow(window_name, mp);
        SetPoints(mp, num_points, window_name);
        WriteConfig(config_log_paths::PATHTOROICONFIG, mp->mouse_input);
    }
    else
    {
        throw std::runtime_error("invalid option (valid option: 'cam' or 'roi')");
    }
}
bool IsPathExist(const std::string &path){
    struct stat info;
    return (stat(path.c_str(),&info) == 0 && S_ISDIR(info.st_mode));
}
void SaveDetectionLogToTrajFile(const std::string& path,
                                const DetectionLog& log,
                                const std::string& location,
                                const std::string& uuid) {

    if(IsPathExist(config_log_paths::PATHTOLOG))
    {
        std::ofstream file(GetLogPath(path,"-peopletracker.csv").c_str());
        PT_CHECK(file.is_open());
        SaveDetectionLogToStream(file, log, location,uuid);
    }else{
        if(CreateDir(config_log_paths::PATHTOLOG.c_str())){
            std::ofstream file(GetLogPath(path,"-peopletracker.csv").c_str());
            PT_CHECK(file.is_open());
            SaveDetectionLogToStream(file, log, location,uuid);
        }  
    }
    
}
std::string GetLogPath(const std::string &file_name,const std::string &extension){
    std::vector<std::string> temp = SplitString(file_name,'.');  
    return config_log_paths::PATHTOLOG + temp[0] + extension;
}
//Save the detection log to a file excluding location
void SaveDetectionLogToTrajFile(const std::string &path,
                                const DetectionLogExtra &log)
{
    std::ofstream file;
    if(IsPathExist(config_log_paths::PATHTOLOG))
    {
        std::string temp = GetLogPath(path,"-roi.csv");
        file.open(GetLogPath(path,"-roi.csv").c_str(), std::ios_base::app);
        if(file.is_open()){
            SaveDetectionExtraLogToStream(file, log);
        }else{
            throw std::runtime_error(path + " is not a valid input(file name only)");
        }
       
    }else{
        if(CreateDir(config_log_paths::PATHTOLOG.c_str())){
            std::string temp = GetLogPath(path,"-roi.csv");
            file.open(GetLogPath(path,"-roi.csv").c_str(), std::ios_base::app);
            if(file.is_open()){
                SaveDetectionExtraLogToStream(file, log);
            }else{
                std::runtime_error(path + "is not a valid input(file name only)");
            }
        }
    }
    
}

void PrintDetectionLog(const DetectionLog &log, const std::string &location, const std::string &uuid)
{
    SaveDetectionLogToStream(std::cout, log, location,uuid);
}
void GetIpAddress(){
    struct ifaddrs * ifAddrStruct=NULL;
    struct ifaddrs * ifa=NULL;
    void * tmpAddrPtr=NULL;

    getifaddrs(&ifAddrStruct);
    printf("Streaming on\n");
    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) {
            continue;
        }
        if (ifa->ifa_addr->sa_family == AF_INET) { // check it is IP4
            // is a valid IP4 Address
            tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
            
            printf("%s IP Address %s:8080/bgr\n", ifa->ifa_name, addressBuffer);  
        }
    }
    if (ifAddrStruct!=NULL) freeifaddrs(ifAddrStruct);
}
bool CreateDir(const std::string &path){
    if(mkdir(config_log_paths::PATHTOLOG.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1){
        throw std::runtime_error(strerror(errno));
    }
    return true;
}
std::map<int, std::string> LocateDirection(std::vector<LogInformation> &logList)
{
    //if Y is going down then the user is walking up
    //if X is going lower they are going towards the left
    //This log will log the user ID and the loction left or right that they are moving
    std::map<int, int> yMap;
    std::map<int, int> xMap;
    std::map<int, std::string> direction;

    for (std::vector<LogInformation>::iterator it = logList.begin(); it != logList.end(); ++it)
    {

        //If the user is not in the HashMap at the X,Y Chords with the Unique ID as the key
        //Users starting location
        if (yMap.count(it->uniqueID) == 0)
        {
            yMap[it->uniqueID] = it->y_Location;
            xMap[it->uniqueID] = it->x_Location;
        }
        else
        {
            //larger of the two numbers will be the direction that the user is moving
            int maxXValue = abs(xMap[it->uniqueID] - it->x_Location);
            int maxYValue = abs(yMap[it->uniqueID] - it->y_Location);

            if (maxXValue > maxYValue)
            {
                if (xMap[it->uniqueID] < it->x_Location)
                    direction[it->uniqueID] = "LEFT";

                else
                    direction[it->uniqueID] = "RIGHT";
            }
            else
            {
                if (yMap[it->uniqueID] > it->y_Location)
                    direction[it->uniqueID] = "FORWARD";
                else
                    direction[it->uniqueID] = "BACKWARD";
            }
        }
    }
    return direction;
}

//
void WriteDirectionLog(const std::string &path)
{

    //Define relivant private parameters
    std::string file_name = GetLogPath(path,"-peopletracker.csv");
    std::cout << "FileName is: " << file_name << std::endl;
    std::ifstream logFile(file_name);
    std::vector<LogInformation> logList;
    std::map<int, std::string> dateMap;

    //std::string newFileName = file_name.insert(file_name.length() - 4, "-direction");
    std::string newFileName = GetLogPath(path,"-directions.csv");
    std::string line;

    std::cout << newFileName << std::endl;

    while (std::getline(logFile, line))
    {
        std::stringstream iss(line);
        LogInformation log(iss);
        logList.push_back(log);
        //std::cout << iss.str() << std::endl;
        // process pair (a,b)
    }

    //Save the date and time of the person exiting the frame
    for (std::vector<LogInformation>::iterator it = logList.begin(); it != logList.end(); ++it)
    {
          if (dateMap.count(it->uniqueID) == 0)
        {
            dateMap[it->uniqueID] = it->dateTime;

        }
    }

    std::map<int, std::string> directionList = LocateDirection(logList);

    // Write out to external file
    std::ofstream directionFile;
    directionFile.open (newFileName);
    for (const auto &p : directionList)
    {

        directionFile << dateMap[p.first] << "," << p.first << "," << p.second << "," << logList[0].location << std::endl;
    }
    directionFile.close(); 
}

std::string GenUuid(){
    srand(time(NULL));
    char struuid[20];

    sprintf(struuid, "%x%x%x",                   
        ((rand() & 0x0fff) | 0x4000),   // Generates a 32-bit Hex number of the form 4xxx (4 indicates the UUID version)
        rand() % 0x3fff + 0x8000,       // Generates a 32-bit Hex number in the range [0x8000, 0xbfff]
        rand()); 
    std::string uuid;
    uuid.append(struuid);
    return uuid;
}

InferenceEngine::Core
LoadInferenceEngine(const std::vector<std::string> &devices,
                    const std::string &custom_cpu_library,
                    const std::string &custom_cldnn_kernels,
                    bool should_use_perf_counter)
{
    std::set<std::string> loadedDevices;
    Core ie;

    for (const auto &device : devices)
    {
        if (loadedDevices.find(device) != loadedDevices.end())
        {
            continue;
        }

        std::cout << "Loading device " << device << std::endl;
        std::cout << printable(ie.GetVersions(device)) << std::endl;

        /** Load extensions for the CPU device **/
        if ((device.find("CPU") != std::string::npos))
        {
            if (!custom_cpu_library.empty())
            {
                // CPU(MKLDNN) extensions are loaded as a shared library and passed as a pointer to base extension
                auto extension_ptr = std::make_shared<Extension>(custom_cpu_library);
                ie.AddExtension(std::static_pointer_cast<Extension>(extension_ptr), "CPU");
            }
        }
        else if (!custom_cldnn_kernels.empty())
        {
            // Load Extensions for other plugins not CPU
            ie.SetConfig({{PluginConfigParams::KEY_CONFIG_FILE, custom_cldnn_kernels}}, "GPU");
        }

        if (should_use_perf_counter)
            ie.SetConfig({{PluginConfigParams::KEY_PERF_COUNT, PluginConfigParams::YES}});

        loadedDevices.insert(device);
    }

    return ie;
}
