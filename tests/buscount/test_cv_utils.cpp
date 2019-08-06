#include "catch.hpp"

#include <cv_utils.hpp>

TEST_CASE( "intersection is correct", "[utils]" ) {

    cv::Rect2f a(1, 2, 6, 8);
    cv::Rect2f b(3, 4, 6, 4);
    cv::Rect2f actual = geom::intersection(a, b);

    cv::Rect2f expected(3, 4, 4, 4);

    REQUIRE(actual == expected);
}


TEST_CASE( "IoU is correct", "[utils]" ) {

    cv::Rect2f a(1, 2, 6, 8);
    cv::Rect2f b(3, 4, 6, 4);
    float actual = geom::IoU(a, b);

    float expected = 4*4.f / (6*8 + 6*4 - 4*4);

    REQUIRE(actual == expected);
}