#include <metrics/read_simulator.hpp>

#include "catch.hpp"

TEST_CASE( "Read simulator puts frame number into video", "[read_sim]" )
{
    double time = 0;
    ReadSimulator<> sym([]() -> cv::Mat {return cv::Mat(2, 2, CV_32F); }, 20, &time);

    for (int i = 0; i < 10; i++) {
        cv::Mat frame = *sym.next();
        int frame_no = frame.at<int32_t>(0, 0);
        time += 1.0/20;
        REQUIRE(frame_no == i);
    }
}

TEST_CASE( "Read simulator tracks time adjusted time correctly", "[read_sim]" )
{
    double time = 0;
    ReadSimulator<> sym([]() -> cv::Mat {return cv::Mat(2, 2, CV_32F); }, 10, &time);

    int frame_no = sym.next()->at<int32_t>(0, 0);
    REQUIRE(frame_no == 0);

    time += 1;

    frame_no = sym.next()->at<int32_t>(0, 0);
    REQUIRE(frame_no == 10);

    time += 0.5;

    frame_no = sym.next()->at<int32_t>(0, 0);
    REQUIRE(frame_no == 15);

}