#include "catch.hpp"

#include <time_utils.hpp>

TEST_CASE( "Time can be converted to a string", "[time]" )
{
    auto input = datetime(2018,6,10, 15,14,54);

    auto result = date_to_string(input, "%Y/%m/%d  %H.%M.%S");

    REQUIRE("2018/06/10  15.14.54" == result);
}

TEST_CASE( "Datetime is read from a string", "[time]" )
{
    auto input = "2019 03 10---05:34.10";

    auto result = string_to_date(input, "%Y %m %d---%H:%M.%S");
    auto result_s = date_to_string(result, "%Y %m %d---%H:%M.%S");

    REQUIRE(input == result_s);
}

TEST_CASE( "Times can be read and added", "[time]" )
{
    auto t = time(6, 5, 4);
    auto d = datetime(2019, 12, 11,  10, 9, 8);

    auto added = d + t;

    auto result = date_to_string(added, "%Y/%m/%d  %H:%M:%S");

    REQUIRE(result == "2019/12/11  16:14:12");
}