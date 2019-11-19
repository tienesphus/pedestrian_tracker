#include <metrics/compare_gt.hpp>
#include <time_utils.hpp>

#include "catch.hpp"

TEST_CASE( "Video time can be taken from filename", "[gt_compare]" )
{
    std::string filename = "/blah/blah/2018-06-10--15-14-54.blah.blah";

    auto expected = datetime(2018,06,10, 15,14,54);

    auto actual = calctime(filename, 0);

    auto expected_s = date_to_string(expected, "%Y/%m/%d %H:%M:%S");
    auto actual_s = date_to_string(actual, "%Y/%m/%d %H:%M:%S");

    REQUIRE(expected_s == actual_s);
}

TEST_CASE( "Calc time gets frame offset correct", "[gt_compare]" )
{
    std::string filename = "/blah/blah/2018-06-10--15-20-10.blah.blah";

    auto expected = datetime(2018,06,10, 15,27,54);

    auto actual = calctime(filename, 25 * ((27-20)*60 + (54-10)));

    auto expected_s = date_to_string(expected, "%Y/%m/%d %H:%M:%S");
    auto actual_s = date_to_string(actual, "%Y/%m/%d %H:%M:%S");

    REQUIRE(expected_s == actual_s);
}

TEST_CASE( "Computes accuracy correct", "[gt_compare]" )
{
    auto t1 = datetime(2019, 02, 05,  3, 8, 0);
    auto t2 = t1 + time(0, 2, 0);
    auto t3 = t1 + time(0, 5, 0);
    auto t4 = t1 + time(0, 7, 0);

    std::vector<Bucket> gt = {
            Bucket(t1, {1, 2}, false),
            Bucket(t2, {3, 4}, false),
            Bucket(t3, {5, 6}, false),
            Bucket(t4, {0, 0}, false),
    };

    std::map<stoptime, StopData> sut = {
            {t1, {1, 2}},
            {t2, {4, 3}},
            // missed completely
            // correctly didn't detect anything
    };

    float in_expected = (0.0f + std::abs(4.0f-3)/std::max(4, 3) + 1.0f + 0.0f) / 4;
    float out_expected = (0.0f + std::abs(4.0f-3)/std::max(4, 3) + 1.0f + 0.0f) / 4;
    int total_err_expect = 0 + 0 + 1 + 1 + 5 + 6 + 0 + 0;
    int total_exchange_expect = 1 + 2 + 3 + 4 + 5 + 6 + 0 + 0;
    float total_expect = static_cast<float>(total_err_expect) / total_exchange_expect;

    Error errors = compute_error(gt, sut);

    REQUIRE(errors.in == in_expected);
    REQUIRE(errors.out == out_expected);
    REQUIRE(errors.total == total_expect);
    REQUIRE(errors.total_errs == total_err_expect);
    REQUIRE(errors.total_exchange == total_exchange_expect);

}

