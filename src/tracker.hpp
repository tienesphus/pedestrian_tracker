#ifndef TRACKER_H
#define TRACKER_H

#include <opencv2/core/mat.hpp>

#include "world.hpp"
#include "detection.hpp"


/**
 * A class that tracks detections moving
 */
class Tracker
{
    
public:

    Tracker() = default;

    virtual ~Tracker() = default;
  
    /**
     * Processes some detections
     * @returns a snapshot of the new state of the world
     */
    virtual WorldState process(const Detections &detections, const cv::Mat& frame) = 0;
    
    /**
     * Draws the current state
     */
    virtual void draw(cv::Mat &img) const = 0;
    
private:
    // tracker cannot be copied
    Tracker(const Tracker& t);
    Tracker& operator=(const Tracker& t);
};

#endif
