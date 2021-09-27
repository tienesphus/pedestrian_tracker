#define CATCH_CONFIG_MAIN
#include "catch.hpp"


#include <vector>
#include <stdexcept>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
float CalculateDist(cv::Point2f point_1, cv::Point2f point_2){
    float dist;
	float x = 0.0, y = 0.0;
	x = pow((point_1.x - point_2.x), 2);
	y = pow((point_1.y - point_2.y), 2);

	dist = sqrt(x + y);
	return dist;
}
float ToFloat(const std::string &str){
    std::istringstream iss(str);
    float temp;
    iss >> std::noskipws >> temp;
    if(iss.eof() && !iss.fail()){
        temp = std::stof(str);            
    }
    else{
        throw std::runtime_error(str +" is not a valid input(numbers only)");
    }
    return temp;
}
TEST_CASE("calculate distance between two given points","[calculate]"){

    std::vector<cv::Point2f> testpoints {cv::Point2f(278,330),cv::Point2f(362,124),
        cv::Point2f(519,133),
        cv::Point2f(584,312),
        cv::Point2f(372,365),
        cv::Point2f(362,427),
        cv::Point2f(481,431),
        cv::Point2f(400,363)};
    std::vector<float> expected{222.467975,190.436341,62.801274,105.75916};
    std::vector<float> actual;
    int j=1;
    int k=0;
    for(int i=0; i<4;i++){        
        actual.push_back(CalculateDist(testpoints[k],testpoints[j]));
        k+=2;
        j+=2;
    }
    
    REQUIRE(expected == actual);
}
TEST_CASE("converting string to float","[coverting]"){
    std::vector<std::string> test_data = {"1.5","2.0","0.0","123.0","4893.0","50.0","1230.92929","999999.9999","1111129.23","0.999999999999"};
    std::vector<float> expected = {1.5f,2.0f,0.0f,123.0f,4893.0f,50.0f,1230.92929f,999999.9999f,1111129.23f,0.999999999999f};
    std::vector<float> actual;
    for(const auto &test:test_data){
        actual.push_back(ToFloat(test));
    }
    REQUIRE(expected == actual);
}
TEST_CASE("converting string(all letters) to float","[coverting]"){
    std::string test = "asd";
    std::vector<float> actual;
    REQUIRE_THROWS_AS(ToFloat(test),std::runtime_error);
}
TEST_CASE("converting string(letters with number) to float","[coverting]"){
    std::string test = "asd2";
    std::vector<float> actual;
    REQUIRE_THROWS_AS(ToFloat(test),std::runtime_error);
}
TEST_CASE("converting an empty string to float","[coverting]"){
    std::string test = "";
    std::vector<float> actual;
    REQUIRE_THROWS_AS(ToFloat(test),std::runtime_error);
}