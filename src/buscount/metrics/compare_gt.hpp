#ifndef BUS_COUNT_COMPARE_GT_HPP
#define BUS_COUNT_COMPARE_GT_HPP

#include <event.hpp>

#include <chrono>
#include <vector>
#include <map>

typedef std::chrono::time_point<std::chrono::system_clock> stoptime;

struct StopData {
    int in_count = 0, out_count = 0;
};

struct Bucket {
    stoptime stop_time;
    StopData data;

    Bucket(stoptime stop_time, StopData data);

    inline bool operator > (const Bucket& other) const {
        return stop_time > other.stop_time;
    }

    inline bool operator == (const Bucket& other) const {
        return stop_time == other.stop_time;
    }

    inline bool operator < (const Bucket& other) const {
        return stop_time < other.stop_time;
    }

    inline bool operator >= (const Bucket& other) const {
        return stop_time >= other.stop_time;
    }

    inline bool operator <= (const Bucket& other) const {
        return stop_time <= other.stop_time;
    }
};


std::vector<Bucket> read_gt(const std::string& location, bool front, const std::chrono::time_point<std::chrono::system_clock>& date);

stoptime closest(stoptime time, const std::vector<Bucket>& gt);

void append(stoptime closest, std::map<stoptime, StopData>& sut, Event e);

stoptime calctime(const std::string& filename, int frame_no);

struct Error {
    float in, out, total;
    int total_errs, total_exchange;
};

Error compute_error(const std::vector<Bucket>& gt, std::map<stoptime, StopData>& sut);

#endif //BUS_COUNT_COMPARE_GT_HPP
