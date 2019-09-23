#ifndef BUS_COUNT_TIME_UTILS_HPP
#define BUS_COUNT_TIME_UTILS_HPP

#include <chrono>
#include <string>

std::chrono::time_point<std::chrono::system_clock> string_to_date(const std::string& input, const std::string& format);

std::chrono::seconds string_to_time(const std::string& input, const std::string& format);

std::string date_to_string(std::chrono::time_point<std::chrono::system_clock> date, const std::string& format);

std::chrono::time_point<std::chrono::system_clock> datetime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second);

std::chrono::time_point<std::chrono::system_clock> date(uint16_t year, uint8_t month, uint8_t day);

std::chrono::seconds time(int64_t hour, int64_t minute, int64_t second);

#endif //BUS_COUNT_TIME_UTILS_HPP
