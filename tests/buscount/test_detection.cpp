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