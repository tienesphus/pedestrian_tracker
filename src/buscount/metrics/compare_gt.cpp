#include "compare_gt.hpp"

#include <time_utils.hpp>
#include <string_utils.hpp>

#include <fstream>
#include <iostream>
#include <iomanip>

Bucket::Bucket(stoptime stop_time, StopData data)
    :stop_time(stop_time), data(data)
{}


std::vector<Bucket> read_gt(const std::string& location, bool front, const std::chrono::time_point<std::chrono::system_clock>& date) {
    std::ifstream source(location);
    if (!source.is_open()) {
        throw std::logic_error("Cannot open ground truth file");
    }

    std::vector<Bucket> gt;

    std::string line;

    // check the header
    std::getline(source, line);
    if ("Arrive,Depart,In_F,In_R,Out_F,Out_R\r" != line) {
        throw std::logic_error("CSV file in incorrect format");
    }

    // Read the rest of the data
    while (std::getline(source, line)) {
        // Split the line at commas
        std::stringstream ss_comma(line);
        std::vector<std::string> parts;
        while( ss_comma.good() )
        {
            std::string substr;
            getline( ss_comma, substr, ',' );
            trim(substr);
            parts.push_back( substr );
        }
        if (parts.size() != 6) {
            throw std::logic_error("Each line must have exactly six parts");
        }

        // pass the date
        auto arrive_time = date + string_to_time(parts[0], "%H:%M:%S");
        // ignore depart time (part[1])

        // pass the counts
        int in_f = parts[2].empty() ? 0 : std::stoi(parts[2]);
        int in_r = parts[3].empty() ? 0 : std::stoi(parts[3]);
        int out_f = parts[4].empty() ? 0 : std::stoi(parts[4]);
        int out_r = parts[5].empty() ? 0 : std::stoi(parts[5]);

        // store the value
        gt.emplace_back(arrive_time, front ? StopData{in_f, out_f} : StopData{in_r, out_r});
    }

    return gt;
}



stoptime closest(stoptime time, const std::vector<Bucket>& gt) {
    auto larger = std::lower_bound(gt.begin(), gt.end(), Bucket(time, {}));
    auto smaller = larger - 1;

    if (larger >= gt.end()) {
        return smaller->stop_time;
    } else if (smaller < gt.begin()) {
        return larger->stop_time;
    } else {

        if ((time - smaller->stop_time) < (larger->stop_time - time)) {
            return smaller->stop_time;
        } else {
            return larger->stop_time;
        }
    }
}

void append(stoptime closest, std::map<stoptime, StopData>& sut, Event e) {
    StopData& amount = sut[closest];
    switch (e) {
        case Event::COUNT_IN:
            amount.in_count += 1; break;
        case Event::COUNT_OUT:
            amount.out_count += 1; break;
        case Event::BACK_IN:
            amount.out_count -= 1; break;
        case Event::BACK_OUT:
            amount.in_count -= 1; break;
        default:
            throw std::logic_error("Unknown event");
    }
}

stoptime calctime(const std::string& filename, int frame_no) {
    int dir_sep = filename.find_last_of('/');
    int ext_sep = filename.find_first_of('.', dir_sep);
    std::string section = filename.substr(dir_sep + 1, ext_sep - dir_sep - 1);
    auto start = string_to_date(section, "%Y-%m-%d--%H-%M-%S");
    return start + std::chrono::seconds(frame_no / 25);
}

Error compute_error(const std::vector<Bucket>& gt, std::map<stoptime, StopData>& sut) {
    int total_errors = 0, total_exchanges = 0, num_stops = 0;
    float sum_error_in = 0, sum_error_out = 0;
    for (auto i = gt.begin(); i < gt.end(); i++) {
        auto time = i->stop_time;
        auto in_gt = i->data.in_count;
        auto out_gt = i->data.out_count;

        auto act = sut[time];
        auto in_sut = act.in_count;
        auto out_sut = act.out_count;

        auto in_err = std::abs(in_sut - in_gt);
        auto out_err = std::abs(out_sut - out_gt);

        auto in_max = std::max(in_gt, in_sut);
        auto out_max = std::max(out_gt, out_sut);

        sum_error_in += in_max == 0 ? 1 : static_cast<float>(in_err) / in_max;
        sum_error_out += out_max == 0 ? 1 : static_cast<float>(out_err) / out_max;

        total_errors += in_err + out_err;
        total_exchanges += in_gt + out_gt;
        num_stops += 1;
    }

    return Error {
        .in = sum_error_in / num_stops,
        .out = sum_error_out / num_stops,
        .total = static_cast<float>(total_errors) / total_exchanges,
        .total_errs = total_errors,
        .total_exchange = total_exchanges,
    };
}