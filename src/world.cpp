#include "world.hpp"

WorldState::WorldState(const cv::Ptr<cv::Mat>& frame, int in, int out):
    in_count(in), out_count(out), frame(frame)
{
}

void WorldState::draw(cv::Mat &img) const
{
    // TODO remove magic numbers (position assumes a 300x300 img)
    std::string txt = 
            "IN: " + std::to_string(this->in_count) + "; " +
            "OUT:" + std::to_string(this->out_count);
        cv::putText(img, txt, 
                cv::Point(5, 280), cv::FONT_HERSHEY_SIMPLEX, 0.6, 
                cv::Scalar(255, 255, 255), 2);
}

WorldConfig::WorldConfig(const Line &inside, const Line &outside, 
        const Line &inner_bounds_a, const Line &inner_bounds_b):
    inside(inside), outside(outside), 
    inner_bounds_a(inner_bounds_a), inner_bounds_b(inner_bounds_b)
{
}

void WorldConfig::draw(cv::Mat &img) const
{
    inside.draw(img);
    outside.draw(img);
    
    inner_bounds_a.draw(img);
    inner_bounds_b.draw(img);
}
  
bool WorldConfig::in_bounds(const cv::Point &p) const
{
    return inner_bounds_a.side(p) && inner_bounds_b.side(p);
}

