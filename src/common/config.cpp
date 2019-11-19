#include "config.hpp"

#include "optional.hpp"

#include <utility>
#include <fstream>

//  ----------- WORLD CONFIG ---------------

WorldConfig::WorldConfig(const geom::Line& crossing, std::vector<geom::Line> bounds):
    crossing(crossing), bounds(std::move(bounds))
{
}

nonstd::optional<geom::Line> read_line(std::fstream& stream)
{
    float ax, ay, bx, by;
    if (stream >> ax >> ay >> bx >> by)
        return geom::Line(geom::Point(ax, ay), geom::Point(bx, by));
    else
        return nonstd::nullopt;
}

WorldConfig WorldConfig::from_file(const std::string& fname)
{
    std::fstream config_file(fname);
    if (!config_file.is_open()) 
        throw std::logic_error("Cannot find config file: '" + fname + "'");

    // read the crossing line
    nonstd::optional<geom::Line> line = read_line(config_file);
    if (!line)
        throw std::logic_error("File does not contain a crossing line");
    geom::Line crossing = *line;

    // Read the bounds
    std::vector<geom::Line> bounds;
    while ((line = read_line(config_file))) {
        bounds.push_back(*line);
    }

    config_file.close();

    return WorldConfig(crossing, bounds);
}

  
bool WorldConfig::in_bounds(const geom::Point &p) const
{
    for (const geom::Line& line : bounds) {
        if (!line.side(p))
            return false;
    }
    return true;
}

bool WorldConfig::inside(const geom::Point &p) const {

    geom::Line norm = crossing.normal(crossing.a);
    return norm.side(p);
}

bool WorldConfig::outside(const geom::Point &p) const {
    geom::Line norm = crossing.normal(crossing.b);
    return !norm.side(p);
}

