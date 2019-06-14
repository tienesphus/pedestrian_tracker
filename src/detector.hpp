#ifndef BUS_COUNT_DETECTOR_HPP
#define BUS_COUNT_DETECTOR_HPP

#include <future>
#include <opencv2/core/types.hpp>

#include "detection.hpp"

struct NetConfig {
    float thresh;           // confidence tothreshold positive detections at
    int clazz;              // class number of people
    cv::Size networkSize;   // The size to rescale the frame to when running inference
    float scale;            // how to scale input pixels (between 0-255) to. E.g.
    cv::Scalar mean;        // paramiter specific to how the model was trained
    std::string meta;       // path to the meta file (.prototxt, .xml)
    std::string model;      // path to the model file (.caffemodel, .bin)
    int preferableBackend;  // The prefered backend for inference (e.g. cv::dnn::DNN_BACKEND_INFERENCE_ENGINE)
    int preferableTarget;   // The prefered inference target (e.g. cv::dnn::DNN_TARGET_MYRIAD)
};

/**
 * A class that handles detection of things
 */
class Detector {
public:

    typedef std::shared_future<Detections> intermediate;

    Detector() = default;

    /**
     * Begins the process of running inference on an image.
     */
    intermediate start_async(const cv::Mat &frame);

    /**
     * Waits for the process to complete
     * @return some unintelligent raw output
     */
    Detections wait_async(const intermediate &request) const;

    /**
     * Runs detection for this frame and outputs the detected things.
     * @param frame the RGB input image
     * @return the detections
     */
    virtual Detections process(const cv::Mat &frame) = 0;

private:
    // disallow copying
    Detector(const Detector&);
    Detector& operator=(const Detector&);
};


#endif //BUS_COUNT_DETECTOR_HPP
