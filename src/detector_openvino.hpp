#ifndef DETECTION_OPENVINO_H
#define DETECTION_OPENVINO_H

#include "detection.hpp"


struct NetConfigOpenVino {
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

class DetectorOpenVino: public Detector<NetConfigOpenCV, InferenceEngine::InferRequest::Ptr> {

public:
    /**
     * Constructs a detector from the given NetConfig
     */
    DetectorOpenVino(const NetConfigOpenVino &config);
    
    cv::Ptr<cv::Mat> pre_process(const cv::Mat &frame) override;
    
    cv::Ptr<cv::Mat> process(const cv::Mat &blob) override;

    cv::Ptr<Detections> post_process(const cv::Mat &original, const cv::Mat &results) const override;
    
private:
    NetConfigOpenCV config;
};

#endif
