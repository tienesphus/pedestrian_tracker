#include "detection.hpp"

#include <utility>
#include <iostream>

#include <opencv2/core/mat.hpp>
#include <opencv2/core/cvstd.hpp>
#include <opencv2/imgproc.hpp>


//  ----------- DETECTION ---------------

Detection::Detection(cv::Rect2d  box, float confidence):
    box(std::move(box)), confidence(confidence)
{}

void Detection::draw(cv::Mat& display) const
{
    cv::rectangle(display, this->box, cv::Scalar(0, 0, this->confidence*255), 3);
}


//  ----------- DETECTIONS ---------------

Detections::Detections(std::vector<Detection> detections):
    detections(std::move(detections))
{
}

std::vector<Detection> Detections::get_detections() const
{
    return this->detections;
}

void Detections::draw(cv::Mat& display) const
{
    for (const Detection& d : this->detections)
        d.draw(display);
}