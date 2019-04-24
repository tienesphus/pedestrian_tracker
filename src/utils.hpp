#ifndef UTILS_H
#define UTILS_H

#include <opencv2/opencv.hpp>

struct Line {
    cv::Point a;
    cv::Point b;

    // The normal of the line
    // TODO move normals to rendering code
    cv::Point na;
    cv::Point nb;

    Line(const cv::Point& a, const cv::Point& b);
    
    void draw(cv::Mat& img) const;

    bool side(const cv::Point& p) const;
    
};

/**
 * Caclulates the Intersection Over Union of two boxes
 * 
 * IoU is area(intersection)/area(union)
 */
float IoU(const cv::Rect2d &a, const cv::Rect2d &b);

#endif
