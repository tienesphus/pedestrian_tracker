#include <utility>

#include "position_affinity.hpp"

PositionData::PositionData(cv::Rect2f loc, float xspeed, float yspeed)
    :loc(std::move(loc)), xSpeed(xspeed), ySpeed(yspeed)
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
    float x_expected = track.loc.x + track.xSpeed;
    float y_expected = track.loc.y + track.ySpeed;

    float x_err = detection.loc.x - x_expected;
    float y_err = detection.loc.y - y_expected;
    float dist = x_err*x_err + y_err*y_err;

    float thresh = track.loc.width*track.loc.height*scale;
    float conf = (thresh - dist) / thresh;

    return conf;
}

void PositionAffinity::merge(const PositionData& detection, PositionData& track) const
{
    float xSpeed = detection.loc.x - track.loc.x;
    float ySpeed = detection.loc.y - track.loc.y;

    // use exponential smoothing function on the speeds
    float alpha = 0.5;
    track.xSpeed = track.xSpeed * alpha + xSpeed * (1 - alpha);
    track.ySpeed = track.ySpeed * alpha + ySpeed * (1 - alpha);

    // assume the other detection data is perfect
    track.loc = detection.loc;
}


