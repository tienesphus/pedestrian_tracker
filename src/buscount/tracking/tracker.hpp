#ifndef TRACKER_H
#define TRACKER_H

#include <opencv2/core/mat.hpp>

#include <config.hpp>
#include <event.hpp>
#include "../detection/detection.hpp"


/**
 * A class that tracks detections moving.
 *
 * Child implementations are very free to implement the system however they want.
 * All an implementation is REQUIRED to do is take some detections in and output any new in/out events each frame.
 */
class Tracker
{
    
public:

    Tracker() = default;

    virtual ~Tracker() = default;
  
    /**
     * Processes some detections
     * @returns all the events that occurred this frame
     */
    virtual std::vector<Event> process(const WorldConfig& config, const Detections &detections, const cv::Mat& frame, int frame_no) = 0;

    /**
     * Draws the current state
     */
    virtual void draw(cv::Mat &img) const = 0;
    
    // tracker cannot be copied
    Tracker(const Tracker& t) = delete;
    Tracker& operator=(const Tracker& t) = delete;
};

#endif
