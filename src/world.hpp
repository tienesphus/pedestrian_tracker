#ifndef WORLD_H
#define WORLD_H

#include <opencv2/core/mat.hpp>
#include "utils.hpp"

/**
 * The current global state of the world
 * TODO delete this WorldState class
 * (it is only used for the draw method which should get merged into libbuscount)
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
    utils::Line crossing;

    std::vector<utils::Line> bounds;

    /**
     * Reads configuration data from a csv file.
     * The csv file must use only whitespace to seperate values (no commas).
     * It's data must be ordered as:
     * - crossing line
     * - bounds 1 line
     * - bounds 2 line
     * - ...
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
    
    WorldConfig(const utils::Line& crossing, std::vector<utils::Line> bounds);

    /**
     * Tests if a point is on the IN side
     */
    bool inside(const utils::Point& p) const;

    /**
     * Tests if a point is on the OUT side
     */
    bool outside(const utils::Point& p) const;

    /**
     * Tests if a point is inside the world bounds
     */
    bool in_bounds(const utils::Point& p) const;
    
    /**
     * Draws the config data to the given image
     * @param img buffer to draw config to
     */
    void draw(cv::Mat &img) const;
};

#endif
