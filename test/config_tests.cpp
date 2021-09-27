#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <vector>
#include <stdexcept>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
std::vector<cv::Point2f> ReadConfig(const std::string& path,const size_t& line_num){
    std::ifstream config_file(path);
    std::string line;
    std::vector<cv::Point2f>  points;
    if(!config_file.is_open()){
        throw std::runtime_error("Can't open config file (" +path+ ")");
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
    if(points.size() !=line_num){
        throw std::runtime_error("config file should have total size of "+std::to_string(line_num) + "(" +path+ ")");
    }
    return points;
}

void WriteConfig(const std::string &path, const std::vector<cv::Point2f> &points){
    std::ofstream config_file(path, std::ofstream::out | std::ofstream::trunc);

    if(!config_file.is_open()){
        throw std::runtime_error("Can't open camera config file (" +path+ ")");
    }
    for(const cv::Point2f &point :points){
        config_file << point.x << " " << point.y << std::endl;
    }
    config_file.close();
}


TEST_CASE("Reading a config file with correct format","[Reading]"){

   std::string path("/home/phu/Desktop/Distance-Cal-test-data/rw-1.txt");
   std::vector<cv::Point2f> points {cv::Point2f(278,330),cv::Point2f(362,124),
        cv::Point2f(519,133),
        cv::Point2f(584,312),
        cv::Point2f(372,365),
        cv::Point2f(362,427),
        cv::Point2f(481,431),
        cv::Point2f(400,363)};
    REQUIRE(ReadConfig(path,8) == points);
}

TEST_CASE("Reading a config file with wrong format","[Reading]"){
    std::string path("/home/phu/Desktop/Distance-Cal-test-data/rw-2.txt");
    std::vector<cv::Point2f> points {cv::Point2f(278,330),cv::Point2f(362,124),
        cv::Point2f(519,133),
        cv::Point2f(584,312),
        cv::Point2f(372,365),
        cv::Point2f(362,427),
        cv::Point2f(481,431),
        cv::Point2f(400,363)};
    REQUIRE_THROWS_AS(ReadConfig(path,8),std::runtime_error);
}

TEST_CASE("Reading an empty config file","[Reading]"){
    std::string path("/home/phu/Desktop/Distance-Cal-test-data/rw-3.txt");
    REQUIRE_THROWS_AS(ReadConfig(path,8),std::runtime_error);
}

TEST_CASE("Reading a config file with incorrect data","[Reading]"){
    std::string path("/home/phu/Desktop/Distance-Cal-test-data/rw-4.txt");
    REQUIRE_THROWS_AS(ReadConfig(path,8),std::runtime_error);
}
TEST_CASE("Reading a config file with surplus data","[Reading]"){
    std::string path("/home/phu/Desktop/Distance-Cal-test-data/rw-5.txt");
    REQUIRE_THROWS_AS(ReadConfig(path,8),std::runtime_error);
}
TEST_CASE("Writing to a config file", "[Writing]"){
    std::string path("/home/phu/Desktop/Distance-Cal-test-data/rw-6.txt");
    std::vector<cv::Point2f> points {cv::Point2f(278,330),cv::Point2f(362,124),
        cv::Point2f(519,133),
        cv::Point2f(584,312),
        cv::Point2f(372,365),
        cv::Point2f(362,427),
        cv::Point2f(481,431),
        cv::Point2f(400,363)};
    WriteConfig(path,points);

    REQUIRE(ReadConfig(path,8)==points);
}

TEST_CASE("Writing to an empty config file", "[Writing]"){
    std::string path("/home/phu/Desktop/Distance-Cal-test-data/rw-7.txt");
    std::vector<cv::Point2f> points {cv::Point2f(278,330),cv::Point2f(362,124),
        cv::Point2f(519,133),
        cv::Point2f(584,312),
        cv::Point2f(372,365),
        cv::Point2f(362,427),
        cv::Point2f(481,431),
        cv::Point2f(400,363)};
    WriteConfig(path,points);

    REQUIRE(ReadConfig(path,8)==points);
}
TEST_CASE("Writing to a nonexistent file","[Writing]"){
    std::string path("/home/phu/Desktop/Distance-Cal-test-data/asdasd.txt");
    REQUIRE_THROWS_AS(ReadConfig(path,8),std::runtime_error);
}