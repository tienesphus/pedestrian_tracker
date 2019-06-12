#ifndef BUS_COUNT_DETECTOR_OPENCV_HPP
#define BUS_COUNT_DETECTOR_OPENCV_HPP

#include "detector.hpp"

#include <opencv2/dnn.hpp>

struct NetConfigOpenCV {
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

class OpenCVDetector: public Detector {

public:
    /**
     * Constructs a detector from the given NetConfig
     */
    explicit OpenCVDetector(const NetConfigOpenCV &config);

protected:
    cv::Mat run(const cv::Mat &frame) override;

private:
    // disallow copying
    OpenCVDetector(const OpenCVDetector&);
    OpenCVDetector& operator=(const OpenCVDetector&);

    NetConfigOpenCV config;
    cv::dnn::Net net;
    std::mutex lock;
};


#endif //BUS_COUNT_DETECTOR_OPENCV_HPP
