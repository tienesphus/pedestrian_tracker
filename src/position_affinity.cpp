#include "position_affinity.hpp"

PositionAffinity::PositionAffinity()
{

}

std::unique_ptr<PositionData> PositionAffinity::init(const Detection&, const cv::Mat&) const
{
    return std::make_unique<PositionData>();
}

float PositionAffinity::affinity(const PositionData&, const PositionData&) const
{
    return 1;
}

void PositionAffinity::merge(const PositionData&, PositionData&) const
{
}

void PositionAffinity::draw(const PositionData&, cv::Mat&) const
{
}

