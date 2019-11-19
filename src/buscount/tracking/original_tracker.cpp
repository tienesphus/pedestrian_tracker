#include <utility>

#include "original_tracker.hpp"

OriginalData::OriginalData(cv::Rect2d loc)
    :loc(std::move(loc))
{}

OriginalTracker::OriginalTracker(double max_diff)
        :max_diff(max_diff)
{}

std::unique_ptr<OriginalData> OriginalTracker::init(const Detection &d, const cv::Mat&, int) const {
    return std::make_unique<OriginalData>(d.box);
}

float OriginalTracker::affinity(const OriginalData &a, const OriginalData &b) const {
    cv::Point2d center_a = cv::Point2d(a.loc.x + a.loc.width / 2, a.loc.y + a.loc.height / 4);
    cv::Point2d center_b = cv::Point2d(b.loc.x + b.loc.width / 2, b.loc.y + b.loc.height / 4);

    auto dx = center_a.x - center_b.x;
    auto dy = center_a.y - center_b.y;
    auto diff = dx*dx + dy*dy;

    return 1 - diff / (max_diff*max_diff);
}

void OriginalTracker::merge(const OriginalData &detectionData, OriginalData &trackData) const {
    trackData.loc = detectionData.loc;
}
