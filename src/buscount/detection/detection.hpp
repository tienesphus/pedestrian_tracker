#ifndef DETECTION_H
#define DETECTION_H

#include <vector>
#include <opencv2/core/types.hpp>

/**
 * Represents a single detection output
 */
struct Detection {
private:
    static inline bool _approx_equals(float a, float b, float tolerance) {
        return (a > (b - tolerance)) && (a < (b + tolerance));
    }

public:

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

    inline bool operator==(const Detection& other) const  {
        return _approx_equals(box.x, other.box.x, 0.005)
               && _approx_equals(box.y, other.box.y, 0.005)
               && _approx_equals(box.width, other.box.width, 0.005)
               && _approx_equals(box.height, other.box.height, 0.005)
               && _approx_equals(confidence, other.confidence, 0.005);
    }

    inline bool operator!=(const Detection& other) const {
        return !(operator==(other));
    }
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
     * Creates an empty detection list
     */
    Detections();

    /**
     * Get the detections that occured
     */
    const std::vector<Detection>& get_detections() const;

    void push_back(const Detection& d);

    void emplace_back(const cv::Rect2f& r, float conf);

    /**
     * Draws the results onto an image.
     */
    void draw(cv::Mat& display) const;
        
private:
    std::vector<Detection> detections;
};

#endif
