#include "world.hpp"

WorldState::WorldState(int in, int out, const cv::Ptr<cv::Mat>& display):
    in_count(in), out_count(out), display(display)
{
}

void WorldState::draw() const
{
    cv::Mat &img = *(this->display);
    
    // TODO remove magic numbers
    std::string txt = 
            "IN: " + std::to_string(this->in_count) + "; " +
            "OUT:" + std::to_string(this->out_count);
    cv::putText(img, txt, 
            cv::Point(5, 5), cv::FONT_HERSHEY_SIMPLEX, 0.6, 
            cv::Scalar(255, 255, 255), 2);
}

WorldConfig::WorldConfig(const Line &inside, const Line &outside, 
        const Line &inner_bounds_a, const Line &inner_bounds_b):
    inside(inside), outside(outside), 
    inner_bounds_a(inner_bounds_a), inner_bounds_b(inner_bounds_b)
{
}

WorldConfig WorldConfig::from_file(std::string fname)
{
    std::fstream config_file(fname);
    if (!config_file.is_open()) 
        throw "Cannot find config file: '" + fname + "'";
    
    int iax, iay, ibx, iby; // input line
    int oax, oay, obx, oby; // output line
    int b1ax, b1ay, b1bx, b1by; // bounds 1 line
    int b2ax, b2ay, b2bx, b2by; // bounds 2 line
    
    config_file >> iax >> iay >> ibx >> iby;
    config_file >> oax >> oay >> obx >> oby;
    config_file >> b1ax >> b1ay >> b1bx >> b1by;
    config_file >> b2ax >> b2ay >> b2bx >> b2by;
    
    config_file.close();

    // setup the world and persistent data
    return WorldConfig(
        // inside, outside:
        Line(cv::Point(iax, iay), cv::Point(ibx, iby)),
        Line(cv::Point(oax, oay), cv::Point(obx, oby)),

        // inner bounds:
        Line(cv::Point(b1ax, b1ay), cv::Point(b1bx, b1by)),
        Line(cv::Point(b2ax, b2ay), cv::Point(b2bx, b2by))
    );
}

void WorldConfig::draw(cv::Mat &img) const
{    
    inside.draw(img);
    outside.draw(img);
    
    inner_bounds_a.draw(img);
    inner_bounds_b.draw(img);
    
    std::cout << "DRAWED STUFF" << std::endl;
}
  
bool WorldConfig::in_bounds(const cv::Point &p) const
{
    return inner_bounds_a.side(p) && inner_bounds_b.side(p);
}

