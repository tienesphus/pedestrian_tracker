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
    Detections(const cv::Ptr<cv::Mat> &frame, const std::vector<Detection> &detections);
    
    /**
     * Gets the image that these detections are for
     */
    cv::Ptr<cv::Mat> get_frame() const;
    
    /**
     * Get the detections that occured
     */
    std::vector<Detection> get_detections() const;
    
    /**
     * Draws the results onto an image.
     */
    void draw(cv::Mat& display) const;
        
private:
    cv::Ptr<cv::Mat> frame;
    std::vector<Detection> detections;
};

struct NetConfigIR {
    float thresh;
    int clazz;
    cv::Size networkSize;
    float scale;
    cv::Scalar mean;
    std::string xml;
    std::string bin;
    
    /**
     * Constructs a network object.
     * IMPORTANT NOTE:
     *  There seems to be a strange bug with Pi/OpenVino that requires
     *  the network to created in a specific way. I used to the Net 
     *  stored as a field in Detector, but calling net.setPreferableTarget 
     *  would cause seg fault. This only occurs on the Pi. Maybe has to
     *  do with the object getting copied??
     */
    cv::dnn::Net make_network() const;
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
     // TODO allow other models types to be constructed
    Detector(const NetConfigIR &config);

    /**
     * Constructs a network object.
     * Note: This simply calls make_network from the internal NetConfigIR object.
     */
    cv::dnn::Net make_network() const;
    
    /**
     * Preprocesses an image and loads it into the graph
     */
    void pre_process(const cv::Mat &frame, cv::dnn::Net& net);
    
    /**
     * Processes the loaded image
     * @return some unintelligent raw output
     */
    cv::Ptr<cv::Mat> process(cv::dnn::Net& net);
    
    /**
     * Takes raw processing output and makes sense of them
     * @return the detected things
     */
    cv::Ptr<Detections> post_process(const cv::Ptr<cv::Mat>& original, cv::Mat &results) const;
    
private:
    NetConfigIR config;
};

#endif
