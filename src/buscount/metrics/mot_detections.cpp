#include "mot_detections.hpp"

#include <string_utils.hpp>
#include <spdlog/spdlog.h>

#include <opencv2/core/mat.hpp>

#include <fstream>
#include <sstream>


DetectorMot::DetectorMot(float thresh, float conf_scale) :thresh(thresh), conf_scale(conf_scale)
{}

void DetectorMot::set_file(const std::string& detections_file)
{
    entries.clear();
    std::ifstream source(detections_file);
    if (!source.is_open()) {
        throw std::logic_error("Cannot detections file");
    }

    std::string line;
    while (std::getline(source, line)) {
        // skip empty lines
        if (line.empty())
            continue;

        // Split the line at commas
        std::stringstream ss_comma(line);
        std::vector<std::string> parts;
        while(ss_comma.good())
        {
            std::string substr;
            getline( ss_comma, substr, ',' );
            trim(substr);
            parts.push_back(substr);
        }
        if (parts.size() != 10) {
            throw std::logic_error("Each line must have exactly 10 parts");
        }

        int frame = std::stoi(parts[0]);
        // int id = std::stoi(parts[1]);
        double x = std::stod(parts[2]);
        double y = std::stod(parts[3]);
        double w = std::stod(parts[4]);
        double h = std::stod(parts[5]);
        double c = std::stod(parts[6]);
        // double x = std::stod(parts[7]);
        // double y = std::stod(parts[8]);
        // double z = std::stod(parts[9]);
        spdlog::trace("MOT det: f:{} {}% at {},{} {}x{})", frame, c, x, y, w, h);

        if (c < thresh*conf_scale)
            continue;

        entry_lock.lock();
        auto& detections = entries[frame];
        detections.emplace_back(cv::Rect2f(x, y, w, h), c);
        entry_lock.unlock();
    }
}

DetectorMot::~DetectorMot() = default;

Detections DetectorMot::process(const cv::Mat &frame, int frame_no) {
    std::lock_guard<std::mutex> lock(entry_lock);
    const auto& raw = entries[frame_no];

    // scale the detection to be between 0-1
    int w = frame.cols;
    int h = frame.rows;
    Detections actual;
    for (const Detection& d : raw.get_detections()) {
        actual.emplace_back(cv::Rect2f(d.box.x / w, d.box.y / h, d.box.width / w, d.box.height / h), d.confidence/conf_scale);
    }

    return actual;
}
