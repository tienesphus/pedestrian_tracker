#ifndef WORLD_H
#define WORLD_H

#include "geom.hpp"

#include <vector>
#include <string>

/**
 * Generic configuration data about a scene.
 */
struct WorldConfig
{
    geom::Line crossing;

    std::vector<geom::Line> bounds;

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
    
    WorldConfig(const geom::Line& crossing, std::vector<geom::Line> bounds);

    /**
     * Tests if a point is on the IN side
     */
    bool inside(const geom::Point& p) const;

    /**
     * Tests if a point is on the OUT side
     */
    bool outside(const geom::Point& p) const;

    /**
     * Tests if a point is inside the world bounds
     */
    bool in_bounds(const geom::Point& p) const;
};

#endif
