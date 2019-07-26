#ifndef BUS_COUNT_DETECTOR_OPENCV_HPP
#define BUS_COUNT_DETECTOR_OPENCV_HPP

#include "detector.hpp"

#include <opencv2/dnn.hpp>

class DetectorOpenCV: public Detector {

public:

    struct NetConfig {
        float thresh;           // confidence to threshold positive detections at (between 0-1)
        int clazz;              // class number of people (normally 0, 1, or 15)
        cv::Size networkSize;   // The size to rescale the frame to when running inference
        float scale;            // how to scale input pixels (between 0-255). Normally 1, 1/255 or 2/255;
        cv::Scalar mean;        // parameter specific to how the model was trained. Normally 0 or 255/2.
        std::string meta;       // path to the meta file (.prototxt, .xml)
        std::string model;      // path to the model file (.caffemodel, .bin)
        int preferableBackend;  // The preferred backend for inference (e.g. cv::dnn::DNN_BACKEND_INFERENCE_ENGINE)
        int preferableTarget;   // The preferred inference target (e.g. cv::dnn::DNN_TARGET_MYRIAD)
    };

    /**
     * Constructs a detector from the given NetConfig
     */
    explicit DetectorOpenCV(const NetConfig &config);
    virtual ~DetectorOpenCV();

    Detections process(const cv::Mat &frame) override;

private:
    // disallow copying
    DetectorOpenCV(const DetectorOpenCV&);
    DetectorOpenCV& operator=(const DetectorOpenCV&);

    NetConfig config;
    cv::dnn::Net net;
    std::mutex lock;
};

Detections static_post_process(const cv::Mat &data, int clazz, float thresh);

#endif //BUS_COUNT_DETECTOR_OPENCV_HPP
