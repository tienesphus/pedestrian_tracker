#ifndef UTILS_H
#define UTILS_H

#include <opencv2/core/types.hpp>
#include <opencv2/imgproc.hpp>

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
float IoU(const cv::Rect &a, const cv::Rect &b);


/**
 * Computes the cosine difference between the two 'vectors'
 */
float cosine_similarity(const std::vector<float> &a, const std::vector<float> &b);

/**
 * Gets the intersection of two rectangles.
 * If the two rectangles do not overlap, then a zero size rectangle will be returned at a random location
 * @param a rectangle a
 * @param b rectangle b
 * @return the intersection
 */
cv::Rect intersection(const cv::Rect& a, const cv::Rect& b);

/**
 * Converts a HSV scalar into a RGB scalar
 * @param hsv the hsv scalar
 * @return the rgb scalar
 */
cv::Scalar hsv2rgb(const cv::Scalar& hsv);


#endif
