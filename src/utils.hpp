#ifndef UTILS_H
#define UTILS_H

#include <opencv2/core/types.hpp>

namespace utils {

    struct Point {
        float x, y;

        Point(float x, float y);

        Point operator+(const Point &other) const;

        Point &operator+=(const Point &other);

        Point operator-(const Point &other) const;

        Point &operator-=(const Point &other);

        Point operator*(float value) const;

        Point &operator*=(float value);

        Point operator/(float value) const;

        Point &operator/=(float value);

        bool operator==(const Point &other) const;
    };

    struct Line {
        Point a;
        Point b;

        Line(const Point &a, const Point &b);

        void draw(cv::Mat &img) const;

        /**
         * Tests if a point is on the "positive" side of the line.
         * The positive side is towards the right when standing at 'a' and looking towards 'b'
         */
        bool side(const Point &p) const;

        /**
         * Gets a normal to the line.
         * The first point will be at point 'p'. The second point will be on the "positive" side of the line.
         * The normal is of an undetermined length
         * @param p where to start the normal line from
         */
        Line normal(const Point& p) const;

    };

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
     * @param a rectangle a
     * @param b rectangle b
     * @return the intersection
     */
    cv::Rect2f intersection(const cv::Rect2f &a, const cv::Rect2f &b);

    /**
     * Converts a HSV scalar into a RGB scalar
     * @param hsv the hsv scalar
     * @return the rgb scalar
     */
    cv::Scalar hsv2rgb(const cv::Scalar &hsv);

}

#endif
