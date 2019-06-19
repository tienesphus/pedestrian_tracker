#ifndef TRACKER_H
#define TRACKER_H

#include <opencv2/core/cvstd.hpp>
#include <opencv2/core/mat.hpp>

#include "detection.hpp"
#include "world.hpp"

/** 
 * A jumble of data that follows a detection result around 
 */
class Track;

/**
 * Persistent data needed to know how to track different objects across frames
 */
class Tracker
{
    
public:
    /**
     * Constructs a Tracker from the given data
     * @param config the world in/out lines
     * @param threshold the max distance before tracks are considered different
     */
    Tracker(WorldConfig config, float threshold);

    ~Tracker();
  
    /**
     * Processes some detections
     * @returns a snapshot of the new state of the world
     */
    WorldState process(const Detections &detections, const cv::Mat& frame);
    
    /**
     * Draws the current state
     */
    void draw(cv::Mat &img) const;
    
private:
    // tracker cannot be copied
    Tracker(const Tracker& t);
    Tracker& operator=(const Tracker& t);

    WorldConfig config;
    WorldState state;
    std::vector<Track*> tracks;
    int index_count;
    float threshold;
    
    void merge(const Detections &detections,  const cv::Size& frame_size);
    void update();
};

#endif
