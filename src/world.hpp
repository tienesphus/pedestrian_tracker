#ifndef WORLD_H
#define WORLD_H

#include <opencv2/core/mat.hpp>
#include "utils.hpp"

/**
 * The current global state of the world
 */
struct WorldState {
    int in_count;
    int out_count;
    
    /**
     * Constructs the world state.
     * @param in the current in count
     * @param out the current out count
     */
    WorldState(int in, int out);
    
    /**
     * Draws the state to the display
     */
    void draw(cv::Mat& display) const;
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
    
    /**
     * Reads configuration data from a csv file.
     * The csv file must use only whitespace to seperate values (no commas).
     * It's data must be ordered as:
     * - inside line
     * - outside line
     * - bounds_a line
     * - bounds_a line
     * 
     * Each line should be ordered as:
     *  - x1
     *  - y1
     *  - x2
     *  - y2
     * 
     * Each data point should be scaled between 0-1
     * 
     * Ensure that the normals of inside/outside face away from each other
     * and the bounds lines face towards each other.
     *
     * @param the filename to read from
     * @returns a new WorldConfig
     */
    static WorldConfig from_file(const std::string& fname);
    
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
