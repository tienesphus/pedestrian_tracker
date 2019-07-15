#ifndef DETECTION_H
#define DETECTION_H

#include <vector>
#include <opencv2/core/types.hpp>

/**
 * Represents a single detection output
 */
struct Detection {
    cv::Rect box;
    float confidence;

    Detection(cv::Rect  box, float confidence);
    
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
    explicit Detections(std::vector<Detection> detections);
    
    /**
     * Get the detections that occured
     */
    const std::vector<Detection>& get_detections() const;
    
    /**
     * Draws the results onto an image.
     */
    void draw(cv::Mat& display) const;
        
private:
    std::vector<Detection> detections;
};

#endif
