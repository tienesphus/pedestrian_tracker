#include <world.hpp>

#include "catch.hpp"

TEST_CASE( "World knows where bounds are", "[world]" ) {
    using namespace utils;
    std::vector<Line> bounds;
    // the direction of these lines is critical
    bounds.emplace_back(Point(0, 1), Point(0, 0));
    bounds.emplace_back(Point(1, 0), Point(1, 1));

    WorldConfig config(Line(Point(10, 10), Point(20, 20)), bounds);

    REQUIRE(config.in_bounds(Point(0.5f, 0.f)));

    REQUIRE_FALSE(config.in_bounds(Point(-0.5f, 0.f)));
    REQUIRE_FALSE(config.in_bounds(Point( 1.5f, 0.f)));
}

TEST_CASE( "World gets inside/outside", "[world]" ) {
    using namespace utils;
    WorldConfig config(Line(Point(0, 0), Point(0, 1)), {});

    REQUIRE(config.inside(Point(0, -1)));
    REQUIRE_FALSE(config.outside(Point(0, -1)));

    REQUIRE_FALSE(config.inside(Point(0, .5f)));
    REQUIRE_FALSE(config.outside(Point(0, .5f)));

    REQUIRE_FALSE(config.inside(Point(0, 2)));
    REQUIRE(config.outside(Point(0, 2)));
}

TEST_CASE( "No bounds is in_bounds", "[world]" ) {
    using namespace utils;
    WorldConfig config(Line(Point(0, 0), Point(0, 1)), {});

    REQUIRE(config.in_bounds(Point(1, 2)));
}