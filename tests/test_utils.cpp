#include <opencv2/core/types.hpp>
#include "catch.hpp"
#include <utils.hpp>

#include <opencv2/core/types.hpp>

TEST_CASE( "intersection is correct", "[utils]" ) {

    cv::Rect2f a(1, 2, 6, 8);
    cv::Rect2f b(3, 4, 6, 4);
    cv::Rect2f actual = utils::intersection(a, b);

    cv::Rect2f expected(3, 4, 4, 4);

    REQUIRE(actual == expected);
}


TEST_CASE( "IoU is correct", "[utils]" ) {

    cv::Rect2f a(1, 2, 6, 8);
    cv::Rect2f b(3, 4, 6, 4);
    float actual = utils::IoU(a, b);

    float expected = 4*4.f / (6*8 + 6*4 - 4*4);

    REQUIRE(actual == expected);
}

TEST_CASE("Line normals point in the correct direction", "[utils]") {
    using namespace utils;
    Line line(Point(0, 0), Point(4, 0));

    Line normal = line.normal(Point(2, 3));

    // must start at the given point
    REQUIRE(normal.a == Point(2, 3));
    // and should point directly up/down
    REQUIRE(normal.b.x == 2);
}

TEST_CASE("Positive side is to the right", "[utils]") {
    using namespace utils;

    // top to bot
    Line line(Point(0, 0), Point(10, 5));
    REQUIRE(line.side(Point(0, 5)));
    REQUIRE_FALSE(line.side(Point(10, 0)));

    // bot to top
    Line line2(Point(10, 5), Point(0, 0));
    REQUIRE_FALSE(line2.side(Point(0, 5)));
    REQUIRE(line2.side(Point(10, 0)));
}

TEST_CASE("Normal is created on positive side", "[utils]") {
    using namespace utils;

    // top to bot
    Line line(Point(10, 20), Point(30, 40));
    Line normal = line.normal(line.a);

    REQUIRE(line.a == normal.a);
    REQUIRE(line.side(normal.b));
}

