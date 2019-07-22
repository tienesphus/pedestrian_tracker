#include <utility>

#include "position_affinity.hpp"

PositionData::PositionData(cv::Rect  loc, int xspeed, int yspeed)
    :loc(std::move(loc)), xspeed(xspeed), yspeed(yspeed)
{}

PositionAffinity::PositionAffinity(float scale): scale(scale)
{}

std::unique_ptr<PositionData> PositionAffinity::init(const Detection& d, const cv::Mat&) const
{
    return std::make_unique<PositionData>(d.box, 0, 0);
}

float PositionAffinity::affinity(const PositionData& detection, const PositionData& track) const
{
    // expand the region of interest
    int x_expected = track.loc.x + track.xspeed;
    int y_expected = track.loc.y + track.yspeed;

    int x_err = detection.loc.x - x_expected;
    int y_err = detection.loc.y - y_expected;
    int dist = x_err*x_err + y_err*y_err;

    float thresh = track.loc.width*track.loc.height*scale;
    float conf = (thresh - dist) / thresh;

    return conf;
}

void PositionAffinity::merge(const PositionData& detection, PositionData& track) const
{
    int xspeed = detection.loc.x - track.loc.x;
    int yspeed = detection.loc.y - track.loc.y;

    // use exponential smoothing function on the speeds
    float alpha = 0.5;
    track.xspeed = static_cast<int>(track.xspeed * alpha + xspeed * (1 - alpha));
    track.yspeed = static_cast<int>(track.yspeed * alpha + yspeed * (1 - alpha));

    // assume the other detection data is perfect
    track.loc = detection.loc;
}

void PositionAffinity::draw(const PositionData&, cv::Mat&) const
{
}

