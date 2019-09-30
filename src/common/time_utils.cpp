#include "time_utils.hpp"

#include <sstream>
#include <iomanip>

std::time_t string_to_date(const std::string& input, const std::string& format)
{
    std::tm tm = {};
    std::stringstream ss(input);
    ss >> std::get_time(&tm, format.c_str());
    return timegm(&tm);
}

std::time_t string_to_time(const std::string& input, const std::string& format)
{
    std::tm tm = {};
    std::stringstream ss_time(input);
    ss_time >> std::get_time(&tm, format.c_str());
    return time(tm.tm_hour, tm.tm_min, tm.tm_sec);
}

std::string date_to_string(std::time_t date, const std::string& format)
{
    auto tm = std::gmtime(&date);
    std::stringstream ss;
    ss << std::put_time(tm, format.c_str());
    return ss.str();
}


std::time_t datetime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second)
{
    std::tm expected_tm = {
            .tm_sec  = second,
            .tm_min  = minute,
            .tm_hour = hour,
            .tm_mday = day,
            .tm_mon  = month - 1,
            .tm_year = year - 1900,
    };
    return timegm(&expected_tm);
}

std::time_t date(uint16_t year, uint8_t month, uint8_t day)
{
    return datetime(year, month, day, 0, 0, 0);
}


std::time_t time(int64_t hour, int64_t minute, int64_t second)
{
    // let seconds/minutes/hours overflow into minutes/hours/days
    minute += second / 60;
    hour += minute / 60;
    uint8_t days = hour / 24;
    return datetime(1970, 1, days + 1, hour % 24, minute % 60, second % 60);
}