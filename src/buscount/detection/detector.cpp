#include "detector.hpp"
#include <opencv2/dnn.hpp>

// TODO Detector is just a wrapper around std::future. Perhaps the caller should handle that instead?

Detector::~Detector() = default;

Detector::intermediate Detector::start_async(const cv::Mat &frame, int frame_no)
{
    return std::async(
            [=]() -> auto {
                return this->process(frame, frame_no);
            }
    );
}

Detections Detector::wait_async(const Detector::intermediate &request) const
{
    return request.get();
}
