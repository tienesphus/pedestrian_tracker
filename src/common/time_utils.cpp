#include "time_utils.hpp"

#include <sstream>
#include <iomanip>

std::chrono::time_point<std::chrono::system_clock> string_to_date(const std::string& input, const std::string& format)
{
    std::tm tm = {};
    std::stringstream ss(input);
    ss >> std::get_time(&tm, format.c_str());
    tm.tm_zone = "AEDT";
    tm.tm_isdst = 1;
    tm.tm_gmtoff = 39600;
    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

std::chrono::seconds string_to_time(const std::string& input, const std::string& format)
{
    std::tm tm = {};
    std::stringstream ss_time(input);
    ss_time >> std::get_time(&tm, format.c_str());
    return time(tm.tm_hour, tm.tm_min, tm.tm_sec);
}

std::string date_to_string(std::chrono::time_point<std::chrono::system_clock> date, const std::string& format)
{
    auto in_time_t = std::chrono::system_clock::to_time_t(date);
    auto tm = std::localtime(&in_time_t);
    std::stringstream ss;
    ss << std::put_time(tm, format.c_str());
    return ss.str();
}


std::chrono::time_point<std::chrono::system_clock> datetime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second)
{
    std::tm expected_tm = {
            .tm_sec  = second,
            .tm_min  = minute,
            .tm_hour = hour,
            .tm_mday = day,
            .tm_mon  = month - 1,
            .tm_year = year - 1900,
    };
    // TODO how to get rid of hacked in timezone
    expected_tm.tm_zone = "AEDT";
    expected_tm.tm_isdst = 0;
    expected_tm.tm_gmtoff = 39600;
    return std::chrono::system_clock::from_time_t(std::mktime(&expected_tm));
}

std::chrono::time_point<std::chrono::system_clock> date(uint16_t year, uint8_t month, uint8_t day)
{
    return datetime(year, month, day, 0, 0, 0);
}


std::chrono::seconds time(int64_t hour, int64_t minute, int64_t second)
{
    int64_t seconds = hour*60*60 + minute*60 + second;
    return std::chrono::seconds(seconds);
}