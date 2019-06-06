#ifndef TRACKER_H
#define TRACKER_H

#include <opencv2/core/cvstd.hpp>
#include <opencv2/core/mat.hpp>

#include "detection.hpp"
#include "world.hpp"

/** 
 * A jumble of data that follows a detection result around 
 */
class Track
{
    friend class Tracker;

private:  
    /**
     * Initialises a Track
     * @param box the bounding box of where the track is
     * @param conf the confidence that the track still exists
     * @param index a unique index for the track
     */
    Track(cv::Rect box, float conf, int index);


    // Tracks cannot be copied
    Track(Track& t);

    /**
     * Updates the status of this Track. Updates the world count.
     */
    bool update(const WorldConfig& config, WorldState& world);

    /**
     * Draws the Track onto the given image
     */
    void draw(cv::Mat &img) const;

    cv::Rect box;
    float confidence;
    int index;
    
    bool been_inside = false;
    bool been_outside = false;
    bool counted_in = false;
    bool counted_out = false;
};

/**
 * Persistent data needed to know how to track different objects across frames
 */
class Tracker
{
    
public:
    explicit Tracker(WorldConfig config);

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
    
    void merge(const Detections &detections);
    void update();
};

#endif
