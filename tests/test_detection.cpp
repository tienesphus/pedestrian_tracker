#include "catch.hpp"

#include "detection.hpp"

#include <opencv2/core/types.hpp>

TEST_CASE( "Detection Rectangle Can be made", "[detection]" ) {

    Detection d(cv::Rect(1, 2, 4, 5), 0.7f);

    REQUIRE( d.box == cv::Rect(1, 2, 4, 5));
    REQUIRE( d.confidence == 0.7f);
}


TEST_CASE( "Vector of detections can be made", "[detection]" ) {

    
    Detection d(cv::Rect(1, 2, 4, 5), 0.7f);

    REQUIRE( d.box == cv::Rect(1, 2, 4, 5));
    REQUIRE( d.confidence == 0.7f);
}