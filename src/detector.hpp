#ifndef BUS_COUNT_DETECTOR_HPP
#define BUS_COUNT_DETECTOR_HPP

#include <future>
#include <opencv2/core/types.hpp>

#include "detection.hpp"

/**
 * A class that handles detection of things
 */
class Detector {
public:
    Detector(float thresh, int clazz);

    /**
     * Runs detection for this frame
     * @param frame
     * @return
     */
    virtual cv::Mat run(const cv::Mat &frame) = 0;

    /**
     * Pre-processes an image and starts the inference on it.
     */
    std::future<cv::Mat> run_async(const cv::Mat &frame);

    /**
     * Waits for the process to complete
     * @return some unintelligent raw output
     */
    cv::Mat process(std::future<cv::Mat> &request) const;

    /**
     * Takes raw processing output from 'process' and makes sense of it
     * @return the detected things
     */
    cv::Ptr<Detections> post_process(const cv::Mat &original, const cv::Mat &data) const;

    /**
     * Runs the inference from start to end. Does no multi-threading.
     * @param frame the input frame
     * @return the the final detections
     */
    cv::Ptr<Detections> process(const cv::Mat &frame);

private:
    const float thresh;
    const int clazz;
};


#endif //BUS_COUNT_DETECTOR_HPP
