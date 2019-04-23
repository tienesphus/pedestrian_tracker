#ifndef DETECTION_H
#define DETECTION_H

#include "opencv2/dnn/dnn.hpp"
#include <vector>

/**
 * Represents a single detection output
 */
struct Detection {
    cv::Rect2d box;
    float confidence;

    Detection(const cv::Rect2d& box, float confidence);
};

/**
 * Keeps track of the detection results from a single frame 
 */
class Detections {
public:
    // Constructs an no results detection
    Detections(const cv::Ptr<cv::Mat> &frame);
    
    // Constructs detection with results
    Detections(const cv::Ptr<cv::Mat> &frame, const std::vector<Detection> &detections);
    
    /**
     * Gets the image that these detections are for
     */
    cv::Ptr<cv::Mat> get_frame();
    
    /**
     * Get the detections that occured
     */
    std::vector<Detection> get_detections();
        
private:
    cv::Ptr<cv::Mat> frame;
    std::vector<Detection> detections;
};

struct NetConfigIR {
    float thresh;
    int clazz;
    cv::Size size;
    float scale;
    cv::Scalar mean;
    std::string xml;
    std::string bin;
};

/**
 * A class that handles detection of things
 */
class Detector {
public:

    /**
     * Constructs a detector from an Intermediate Representation 
     * (an OpenVino model)
     */
    Detector(const NetConfigIR &config);
    
    /**
     * Finds things in the image
     * @return the detected things
     */
    cv::Ptr<Detections> process(const cv::Ptr<cv::Mat> &frame);
    
private:
    cv::dnn::Net net;
    float thresh;
    int clazz;
    cv::Size size;
    float scale;
    cv::Scalar mean;
};

#endif
