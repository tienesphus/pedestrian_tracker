#ifndef BUS_COUNT_TIMED_WRAPPER_HPP
#define BUS_COUNT_TIMED_WRAPPER_HPP

#include "../detection/detector.hpp"
#include "../tracking/tracker_component.hpp"

class TimedDetector : public Detector
{

public:
    TimedDetector(Detector& delegate, double* time, double detection_time);

    ~TimedDetector() override;

    Detections process(const cv::Mat &frame, int frame_no) override;

private:
    Detector& delegate;
    double* time;
    double detection_time;
};


template <typename  T>
class TimedAffinity : public Affinity<T>
{
public:
    TimedAffinity(Affinity<T>& delegate, double* time, double process_time)
        :delegate(delegate), time(time), detection_time(process_time)
    {}

    std::unique_ptr<T> init(const Detection &d, const cv::Mat &frame, int frame_no) const override {
        *time += detection_time;
        return delegate.init(d, frame, frame_no);
    }

    float affinity(const T &detectionData, const T &trackData) const override {
        return delegate.affinity(detectionData, trackData);
    }

    void merge(const T &detectionData, T &trackData) const override {
        delegate.merge(detectionData, trackData);
    }

private:
    Affinity<T>& delegate;
    double* time;
    double detection_time;
};


#endif //BUS_COUNT_TIMED_WRAPPER_HPP
