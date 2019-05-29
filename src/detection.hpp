#ifndef DETECTION_H
#define DETECTION_H

#include <opencv2/dnn/dnn.hpp>
#include <vector>

/**
 * Represents a single detection output
 */
struct Detection {
    cv::Rect2d box;
    float confidence;

    Detection(const cv::Rect2d& box, float confidence);
    
    /**
     * Draws this detection onto the image
     */
    void draw(cv::Mat &display) const;
};

/**
 * Keeps track of the detection results from a single frame 
 */
class Detections {
public:
    /**
     * Constructs a detection result
     * @param frame the image the detections were made on
     * @param detections a list of detections that were made
     */
    Detections(const std::vector<Detection> &detections);
    
    /**
     * Get the detections that occured
     */
    std::vector<Detection> get_detections() const;
    
    /**
     * Draws the results onto an image.
     */
    void draw(cv::Mat& display) const;
        
private:
    std::vector<Detection> detections;
};

/**
 * A class that handles detection of things
 */
class Detector {
public:

    /**
     * Preprocesses an image and loads it into the graph
     */
    virtual cv::Ptr<cv::Mat> pre_process(const cv::Mat &frame) = 0;
    
    /**
     * Processes the loaded image
     * @return some unintelligent raw output
     */
    virtual cv::Ptr<cv::Mat> process(const cv::Mat &blob) = 0;
    
    /**
     * Takes raw processing output and makes sense of them
     * @return the detected things
     */
    virtual cv::Ptr<Detections> post_process(const cv::Mat &original, const cv::Mat &results) const = 0;
    
};


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
    OpenCVDetector(const NetConfigOpenCV &config);
    
    cv::Ptr<cv::Mat> pre_process(const cv::Mat &frame) override;
    
    cv::Ptr<cv::Mat> process(const cv::Mat &blob) override;

    cv::Ptr<Detections> post_process(const cv::Mat &original, const cv::Mat &results) const override;
    
private:
    NetConfigOpenCV config;
    cv::dnn::Net net;
};

#endif
