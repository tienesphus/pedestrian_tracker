#include "catch.hpp"

#include <detection/detection.hpp>

TEST_CASE( "Detection Rectangle Can be made", "[detection]" ) {

    Detection d(cv::Rect2f(.1f, .2f, .4f, .5f), .7f);

    REQUIRE( d.box == cv::Rect2f(.1f, .2f, .4f, .5f));
    REQUIRE( d.confidence == .7f);
}


TEST_CASE( "Vector of detections can be made", "[detection]" ) {

    
    Detection d(cv::Rect2f(.1f, .2f, .4f, .5f), .7f);

    REQUIRE( d.box == cv::Rect2f(.1f, .2f, .4f, .5f));
    REQUIRE( d.confidence == .7f);
}

TEST_CASE( "Detections equality", "[detections]")
{
    // same
    Detection d1(cv::Rect2f(.1f, .2f, .4f, .5f), .7f);
    Detection d2(cv::Rect2f(.10001f, 0.1996f, .4f, .5f), .7f);

    // not same
    Detection d3(cv::Rect2f(.2f, 0.1f, .4f, .5f), .7f);
    Detection d4(cv::Rect2f(.1f, 0.2f, .4f, .5f), .8f);


    REQUIRE(d1 == d2);
    REQUIRE(d1 != d3);
    REQUIRE(d1 != d4);
}