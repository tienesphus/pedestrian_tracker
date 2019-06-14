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
    Detector(float thresh, int clazz);

    // TODO it seems kinda strange to split this up into three steps.
    //  Are you sure it's not possible to do the entire process in one step?

    /**
     * Pre-processes an image and starts the inference on it.
     */
    std::shared_future<cv::Mat> start_async(const cv::Mat &frame);

    /**
     * Waits for the process to complete
     * @return some unintelligent raw output
     */
    cv::Mat wait_async(const std::shared_future<cv::Mat> &request) const;

    /**
     * Takes raw processing output from 'process' and makes sense of it
     * @return the detected things
     */
    virtual Detections post_process(const cv::Mat &data) const;

    /**
     * Runs the inference from start to end. Does no multi-threading.
     * @param frame the input frame
     * @return the the final detections
     */
    Detections process(const cv::Mat &frame);

protected:
    /**
     * Runs detection for this frame
     * @param frame the RGB input image
     * @return some data (should be passed to Detector::post_process)
     */
    virtual cv::Mat run(const cv::Mat &frame) = 0;

private:
    // disallow copying
    Detector(const Detector&);
    Detector& operator=(const Detector&);

    const float thresh;
    const int clazz;
    cv::Size input_size;
};

Detections static_post_process(const cv::Mat &data, int clazz, float thresh, const cv::Size &image_size);


#endif //BUS_COUNT_DETECTOR_HPP
