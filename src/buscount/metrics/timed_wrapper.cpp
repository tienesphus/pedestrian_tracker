//
// Created by matt on 26/08/19.
//

#include "timed_wrapper.hpp"

TimedDetector::TimedDetector(Detector &delegate, double *time, double detection_time):
    delegate(delegate), time(time), detection_time(detection_time)
{
}

TimedDetector::~TimedDetector() = default;

Detections TimedDetector::process(const cv::Mat &frame, int frame_no) {
    *time += detection_time;
    return delegate.process(frame, frame_no);
}
