#ifndef BUS_COUNT_ORIGINAL_TRACKER_HPP
#define BUS_COUNT_ORIGINAL_TRACKER_HPP

#include "tracker_component.hpp"

class OriginalData: public TrackData
{
    cv::Rect2d loc;

    friend class OriginalTracker;

public:
    explicit OriginalData(cv::Rect2d loc);
};

class OriginalTracker : public Affinity<OriginalData>
{
public:

    explicit OriginalTracker(double max_diff);

    ~OriginalTracker() override = default;

    std::unique_ptr<OriginalData> init(const Detection &d, const cv::Mat &frame, int frame_no) const override;

    float affinity(const OriginalData &detectionData, const OriginalData &trackData) const override;

    void merge(const OriginalData &detectionData, OriginalData &trackData) const override;

private:
    double max_diff;
};


#endif //BUS_COUNT_ORIGINAL_TRACKER_HPP
