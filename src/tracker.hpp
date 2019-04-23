#ifndef  TRACKER_H
#define TRACKER_H

#include "opencv2/core/cvstd.hpp"
#include "opencv2/core/mat.hpp"
#include "detection.hpp"
#include "world.hpp"

/** 
 * A jumble of data that follows a detection result around 
 */
class Track
{

public:  
    /**
     * Initialises a Track
     * @param box the bounding box of where the track is
     * @param conf the confidence that the track still exists
     * @param index a unique index for the track
     */
    Track(const cv::Rect2d &box, float conf, int index);

    /**
     * Updates the status of this Track. Updates the world count.
     */
    bool update(const WorldConfig& config, WorldState& world);

    /**
     * Draws the Track onto the given image
     */
    void draw(cv::Mat &img) const;

private:
    cv::Rect2d box;
    float confidence;
    int index;
    
    bool been_inside = false;
    bool been_outside = false;
    bool counted_in = false;
    bool counted_out = false;
    
    friend class Tracker;
};

/**
 * Persistent data needed to know how to track different objects across frames
 */
class Tracker
{
    
public:
    Tracker(WorldConfig config);

    ~Tracker();
  
    /**
     * Processes some detections
     * @returns the new state of the world
     */
    cv::Ptr<WorldState> process(Detections &detections);

    /**
     * Updates the status of this Track. Updates the world count.
     */
    bool update(const WorldConfig& config, WorldState& world);

    /**
     * Draws the current state
     */
    void draw(cv::Mat &img) const;
    
private:
    WorldConfig config;
    WorldState state;
    std::vector<Track*> tracks;
    int index_count;
};

#endif
