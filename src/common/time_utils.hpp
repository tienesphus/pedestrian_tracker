#ifndef BUS_COUNT_TIME_UTILS_HPP
#define BUS_COUNT_TIME_UTILS_HPP

#include <string>
#include <ctime>

std::time_t string_to_date(const std::string& input, const std::string& format);

std::time_t string_to_time(const std::string& input, const std::string& format);

std::string date_to_string(std::time_t date, const std::string& format);

std::time_t datetime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second);

std::time_t date(uint16_t year, uint8_t month, uint8_t day);

std::time_t time(int64_t hour, int64_t minute, int64_t second);

#endif //BUS_COUNT_TIME_UTILS_HPP
