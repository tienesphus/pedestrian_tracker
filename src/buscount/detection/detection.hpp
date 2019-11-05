#ifndef DETECTION_H
#define DETECTION_H

#include <vector>
#include <opencv2/core/types.hpp>

/**
 * Represents a single detection output
 */
struct Detection {
    /**
     * The detection bounds. Positions are scaled between 0 and 1
     */
    cv::Rect2f box;
    /**
     * The confidence of a detection (between 0 and 1)
     */
    float confidence;

    /**
     * Constructs a new Detection box
     */
    Detection(cv::Rect2f box, float confidence);
    
    /**
     * Draws this detection onto the image.
     */
    void draw(cv::Mat &display) const;
};

/**
 * Keeps track of the detection results from a single frame. Effectively just a Vector of Detections
 * TODO Detections class is useless?
 */
class Detections {
public:
    /**
     * Constructs a detection result
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
