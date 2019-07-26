#include "world.hpp"
#include "optional.hpp"

#include <utility>
#include <fstream>
#include <iostream>

#include <opencv2/imgproc.hpp>

//  ----------- WORLD STATE ---------------

WorldState::WorldState(int in, int out):
    in_count(in), out_count(out)
{
}

void WorldState::draw(cv::Mat& display) const
{
    // TODO remove magic numbers
    std::string txt = 
            "IN: " + std::to_string(this->in_count) + "; " +
            "OUT:" + std::to_string(this->out_count);
    cv::putText(display, txt, 
            cv::Point(4, 24), cv::FONT_HERSHEY_SIMPLEX, 0.6, 
            cv::Scalar(255, 255, 255), 2);
}

//  ----------- WORLD CONFIG ---------------

WorldConfig::WorldConfig(const utils::Line& crossing, std::vector<utils::Line> bounds):
    crossing(crossing), bounds(std::move(bounds))
{
}

nonstd::optional<utils::Line> read_line(std::fstream& stream)
{
    float ax, ay, bx, by;
    if (stream >> ax >> ay >> bx >> by)
        return utils::Line(utils::Point(ax, ay), utils::Point(bx, by));
    else
        return nonstd::nullopt;
}

WorldConfig WorldConfig::from_file(const std::string& fname)
{
    std::fstream config_file(fname);
    if (!config_file.is_open()) 
        throw std::logic_error("Cannot find config file: '" + fname + "'");

    // read the crossing line
    nonstd::optional<utils::Line> line = read_line(config_file);
    if (!line)
        throw std::logic_error("File does not contain a crossing line");
    utils::Line crossing = *line;

    // Read the bounds
    std::vector<utils::Line> bounds;
    while ((line = read_line(config_file))) {
        bounds.push_back(*line);
    }

    config_file.close();

    return WorldConfig(crossing, bounds);
}

/**
 * Gets the point of intersection between two lines
 * Will crash if the two lines are parallel
 */
utils::Point getIntersection(utils::Line l1, utils::Line l2)
{
    float dx1 = l1.b.x -l1.a.x;
    float dy1 = l1.b.y -l1.a.y;
    float dx2 = l2.b.x -l2.a.x;
    float dy2 = l2.b.y -l2.a.y;
    // ahh, maths...
    float s = (dx2 * (l1.a.y - l2.a.y) + dy2 * (l2.a.x - l1.a.x)) / (dx1 * dy2 - dx2 * dy1);
    return utils::Point(l1.a.x + dx1 * s, l1.a.y + dy1 * s);
}

/**
    * Extends a line so that it extends to screen edges. Note: the lines may end up slightly
    * past the edge of the screen
*/
utils::Line extend(const utils::Line& line)
{
    using namespace utils;

    // Choose whether to use top/bottom or left/right edges based on ratio of dx:dy
    // note that its okay if the points are outside the boundaries
    float dx = abs(line.a.x - line.b.x);
    float dy = abs(line.a.y - line.b.y);
    if (dx > dy) {
        // use left/right intercepts
        Point leftInt = getIntersection(line, Line(Point(0, 0), Point(0, 1)));
        Point rightInt = getIntersection(line, Line(Point(1, 0), Point(1, 1)));
        return Line(leftInt, rightInt);
    } else {
        // use top/bot intercepts
        Point topInt = getIntersection(line, Line(Point(0, 0), Point(1, 0)));
        Point botInt = getIntersection(line, Line(Point(0, 1), Point(1, 1)));
        return Line(topInt, botInt);
    }
}


void WorldConfig::draw(cv::Mat &img) const
{
    crossing.draw(img);

    extend(crossing.normal(crossing.a)).draw(img);
    extend(crossing.normal(crossing.b)).draw(img);

    for (const utils::Line& bound: bounds) {
        bound.draw(img);
        //extend(bound.normal((bound.a+bound.b)/2)).draw(img);
    }

    std::cout << "DRAWN STUFF" << std::endl;
}

  
bool WorldConfig::in_bounds(const utils::Point &p) const
{
    for (const utils::Line& line : bounds) {
        if (!line.side(p))
            return false;
    }
    return true;
}

bool WorldConfig::inside(const utils::Point &p) const {

    utils::Line norm = crossing.normal(crossing.a);
    return norm.side(p);
}

bool WorldConfig::outside(const utils::Point &p) const {
    utils::Line norm = crossing.normal(crossing.b);
    return !norm.side(p);
}

