#ifndef BUS_COUNT_CV_UTILS_HPP
#define BUS_COUNT_CV_UTILS_HPP

#include <opencv2/core/types.hpp>

#include <geom.hpp>
#include <config.hpp>

namespace geom {
    /**
     * Calculates the Intersection Over Union of two boxes
     *
     * IoU is area(intersection)/area(union)
     */
    float IoU(const cv::Rect2f &a, const cv::Rect2f &b);


    /**
     * Computes the cosine difference between the two 'vectors'
     */
    float cosine_similarity(const std::vector<float> &a, const std::vector<float> &b);

    /**
     * Gets the intersection of two rectangles.
     * If the two rectangles do not overlap, then a zero size rectangle will be returned at a random location
     * Note: Due to floating point inaccuracies, the intersection may be slightly outside the actual intersection.
     * If this is a problem, do not use this method (try using the integer version instead)
     * @param a rectangle a
     * @param b rectangle b
     * @return the intersection
     */
    cv::Rect2f intersection(const cv::Rect2f &a, const cv::Rect2f &b);

    /**
     * Gets the intersection of two rectangles.
     * If the two rectangles do not overlap, then a zero size rectangle will be returned at a random location
     * @param a rectangle a
     * @param b rectangle b
     * @return the intersection
     */
    cv::Rect2i intersection(const cv::Rect2i &a, const cv::Rect2i &b);

    /**
     * Converts a HSV scalar into a RGB scalar
     * @param hsv the hsv scalar
     * @return the rgb scalar
     */
    cv::Scalar hsv2rgb(const cv::Scalar &hsv);


    /**
     * Draws the line to the given image in the given color
     * @param img the image to draw
     * @param line the line to draw
     * @param clr the color to draw in
     */
    void draw_line(cv::Mat &img, const Line& line, const cv::Scalar& clr);

    /**
     * Draws the config data to the given image
     * @param img buffer to draw config to
     */
    void draw_world_config(cv::Mat &img, const WorldConfig& config);

    /**
    * Draws the count to the display
    */
    void draw_world_count(cv::Mat &img, const int in_count, const int out_count);

}

#endif //BUS_COUNT_CV_UTILS_HPP
