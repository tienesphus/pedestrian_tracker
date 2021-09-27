
#define CATCH_CONFIG_MAIN
#include "catch.hpp"


#include <vector>
#include <stdexcept>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <unordered_map>
#include <random>
#include <iostream>
#include <sstream>
#include <algorithm>
///
/// \brief The DetectionLogExtra Entry struct
///
/// An entry describing the time of stay of tracked objects in a ROI
///
struct DetectionLogExtraEntry{
    int object_id;      ///< Tracked objects id
    uint64_t init_time; ///< The inital time when tracked object is in the roi (0 if N/A)
    int time_of_stay;   ///< The time of stay of tracked object in ROI 
    ///
    /// \brief DetectionLogExtraEntry default constructor.
    ///
    DetectionLogExtraEntry() : init_time(0), time_of_stay(0) {}

    ///
    /// \brief DetectionLogExtraEntry copy constructor.
    /// \param other Detection extra entry.
    ///
    DetectionLogExtraEntry(const DetectionLogExtraEntry &other)
        : object_id(other.object_id),
        init_time(other.init_time),
        time_of_stay(other.time_of_stay) {}

    ///
    /// \brief DetectionLogExtraEntry move constructor.
    /// \param other Detection extra entry.
    ///
    DetectionLogExtraEntry(DetectionLogExtraEntry &&other)
        : object_id(other.object_id),
        init_time(other.init_time),
        time_of_stay(other.time_of_stay) {}

    ///
    /// \brief Assignment operator.
    /// \param other Detection extra entry.
    /// \return Detection extra entry.
    ///
    DetectionLogExtraEntry &operator=(const DetectionLogExtraEntry &other) = default;

    ///
    /// \brief Move assignment operator.
    /// \param other Detection extra entry.
    /// \return Detection extra entry.
    ///
    DetectionLogExtraEntry &operator=(DetectionLogExtraEntry &&other) {
        if (this != &other) {
            object_id = other.object_id;
            init_time = other.init_time;
            time_of_stay = other.time_of_stay;
        }
        return *this;
    }
    bool operator== (const DetectionLogExtraEntry &rhs) const {
        return (object_id == rhs.object_id) 
        && (init_time == rhs.init_time) 
        && (time_of_stay == rhs.time_of_stay);
    }
};
/// Detection log extra is a vector of detection extra entries.
using DetectionLogExtra = std::unordered_map<int, DetectionLogExtraEntry>;

cv::Point2f GetBottomPoint(const cv::Rect box){

    cv::Point2f temp_pnt(1); 

    float width, height;
    width = box.width;
    height = box.height;
    temp_pnt = cv::Point2f((box.x + (width*0.5)), (box.y+height));	
		
	return temp_pnt;
}
template <typename T>
std::ostream &operator<<(std::ostream &os, const std::vector<T> &v) {
    os << "[\n";
    if (!v.empty()) {
        auto itr = v.begin();
        os << *itr;
        for (++itr; itr != v.end(); ++itr) os << ",\n" << *itr;
    }
    os << "\n]";
    return os;
}
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
            stream << entry.second.object_id << ',' << entry.second.init_time << ','
                    << (int) entry.second.time_of_stay/1000;
            stream << '\n';
        }
        
    }
}
}
void SaveDetectionLogToTrajFile(const std::string& path,
                                const DetectionLogExtra& log) {
    std::ofstream file;
    file.open(path.c_str(),std::ios_base::app);
    SaveDetectionExtraLogToStream(file, log);
}


TEST_CASE("calculate bottom center point given a rectangle","[calculate]"){

    std::vector<cv::Rect> testpoints {cv::Rect(278,330,34,65),cv::Rect(362,124,50,100),
        cv::Rect(519,133,75,20),
        cv::Rect(584,312,100,12),
        cv::Rect(372,365,43,29),
        cv::Rect(362,427,98,20),
        cv::Rect(481,431,30,120),
        cv::Rect(400,363,40,50)};
    std::vector<cv::Point2f> expected{cv::Point2f(295,395),
    cv::Point2f(387,224),
    cv::Point2f(556.5f,153),
    cv::Point2f(634,324),
    cv::Point2f(393.5,394),
    cv::Point2f(411,447),
    cv::Point2f(496,551),
    cv::Point2f(420,413),};
    std::vector<cv::Point2f> actual;
    for(unsigned int i=0; i< testpoints.size();i++){        
        actual.push_back(GetBottomPoint(testpoints[i]));
    }
    
    REQUIRE(expected == actual);
}
TEST_CASE("writing logs to a file", "[log]"){
    uint64_t cur_timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    DetectionLogExtra log;
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<> rando(1000,500000);
    std::vector<std::string> actual;
    std::vector<std::string> expected;
    for(int i=0;i<10;i++){
        DetectionLogExtraEntry entry;
        entry.object_id = i;
        entry.init_time = cur_timestamp;
        entry.time_of_stay = rando(mt);
        log.emplace(entry.object_id,entry);
        expected.push_back(std::to_string(entry.object_id) + ","+
                        std::to_string(entry.init_time) + "," +
                        std::to_string((int) entry.time_of_stay / 1000));
    } 
    SaveDetectionLogToTrajFile("/home/phu/Desktop/extra-log.csv",log);
    std::ifstream file("/home/phu/Desktop/extra-log.csv");
    std::string entry;
    if(!file.is_open()){
        throw std::runtime_error("cant open file");
    }
    std::string line;
    while (std::getline(file,line))
    {
        std::istringstream iss(line);
        std::string input;

        if(!(iss >>input)){
            break;
        }
        actual.push_back(input);
    }
    std::reverse(actual.begin(),actual.end());
    file.close();
    REQUIRE(expected == actual);

}