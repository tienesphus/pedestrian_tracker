#ifndef BUS_COUNT_DETECTOR_OPENCV_HPP
#define BUS_COUNT_DETECTOR_OPENCV_HPP

#include "detector.hpp"

#include <opencv2/dnn.hpp>

class OpenCVDetector: public Detector {

public:
    /**
     * Constructs a detector from the given NetConfig
     */
    explicit OpenCVDetector(const NetConfig &config);

    Detections process(const cv::Mat &frame) override;

private:
    // disallow copying
    OpenCVDetector(const OpenCVDetector&);
    OpenCVDetector& operator=(const OpenCVDetector&);

    NetConfig config;
    cv::dnn::Net net;
    std::mutex lock;
};

Detections static_post_process(const cv::Mat &data, int clazz, float thresh, const cv::Size &image_size);

#endif //BUS_COUNT_DETECTOR_OPENCV_HPP
