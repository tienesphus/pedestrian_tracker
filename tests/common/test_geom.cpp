#include "catch.hpp"

#include <geom.hpp>

TEST_CASE("Line normals point in the correct direction", "[utils]") {
    using namespace geom;
    Line line(Point(0, 0), Point(4, 0));

    Line normal = line.normal(Point(2, 3));

    // must start at the given point
    REQUIRE(normal.a == Point(2, 3));
    // and should point directly up/down
    REQUIRE(normal.b.x == 2);
}

TEST_CASE("Positive side is to the right", "[utils]") {
    using namespace geom;

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
    using namespace geom;

    // top to bot
    Line line(Point(10, 20), Point(30, 40));
    Line normal = line.normal(line.a);

    REQUIRE(line.a == normal.a);
    REQUIRE(line.side(normal.b));
}

