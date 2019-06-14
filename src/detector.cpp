#include "detector.hpp"
#include <opencv2/dnn.hpp>
#include <iostream>

// TODO Detector is just wrapper around std::future. Perhaps the caller should handle that instead?

Detector::intermediate Detector::start_async(const cv::Mat &frame)
{
    return std::async(
            [=]() -> auto {
                return this->process(frame);
            }
    );
}

Detections Detector::wait_async(const Detector::intermediate &request) const
{
    return request.get();
}