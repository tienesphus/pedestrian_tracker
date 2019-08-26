#ifndef BUS_COUNT_POSITION_AFFINITY_HPP
#define BUS_COUNT_POSITION_AFFINITY_HPP

#include "tracker_component.hpp"
#include <inference_engine.hpp>

class PositionData: public TrackData
{
    friend class PositionAffinity;

    cv::Rect2f loc;
    float xSpeed, ySpeed;

public:
    PositionData(cv::Rect2f loc, float xspeed, float yspeed);
    ~PositionData() override = default;
};

class PositionAffinity: public Affinity<PositionData> {
public:

    explicit PositionAffinity(float scale);

    ~PositionAffinity() override = default;

    std::unique_ptr<PositionData> init(const Detection& d, const cv::Mat& frame) const override;

    float affinity(const PositionData &detectionData, const PositionData &trackData) const override;

    void merge(const PositionData& detectionData, PositionData& trackData) const override;

private:
    float scale;

};


#endif //BUS_COUNT_POSITION_AFFINITY_HPP
