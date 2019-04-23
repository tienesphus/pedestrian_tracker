#ifndef WORLD_H
#define WORLD_H

#include "opencv2/core/cvstd.hpp"
#include "opencv2/core/mat.hpp"
#include "utils.hpp"

/**
 * The current global state of the world
 */
struct WorldState {
    cv::Ptr<cv::Mat> frame;
    int in_count;
    int out_count;
    
    WorldState(const cv::Ptr<cv::Mat>& frame, int in, int out);
    
    void draw(cv::Mat &img) const;
};

/**
 * Generic configuration data about a scene.
 */
struct WorldConfig
{
    Line inside;
    Line outside;
  
    Line inner_bounds_a;
    Line inner_bounds_b;
    
    WorldConfig(const Line &inside, const Line &outside, 
            const Line &inner_bounds_a, const Line &inner_bounds_b);
    
    /**
     * Tests if a point is inside the world boulds
     */
    bool in_bounds(const cv::Point& p) const;
    
    /**
     * Draws the config data to the given image
     * @param img buffer to draw config to
     */
    void draw(cv::Mat &img) const;
};

#endif
