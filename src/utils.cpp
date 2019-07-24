#include "utils.hpp"

#include <cmath>

#include <opencv2/imgproc.hpp>

namespace utils {

//  ----------- POINT ---------------

    Point::Point(float x, float y) :
            x(x), y(y) {}

    Point &Point::operator+=(const Point &other) {
        this->x += other.x;
        this->y += other.y;
        return *this;
    }

    Point Point::operator+(const Point &other) const {
        return Point(*this) += other;
    }

    Point &Point::operator-=(const Point &other) {
        this->x -= other.x;
        this->y -= other.y;
        return *this;
    }

    Point Point::operator-(const Point &other) const {
        return Point(*this) -= other;
    }

    Point &Point::operator*=(float value) {
        this->x *= value;
        this->y *= value;
        return *this;
    }

    Point Point::operator*(float value) const {
        return Point(*this) *= value;
    }

    Point &Point::operator/=(float value) {
        this->x /= value;
        this->y /= value;
        return *this;
    }

    Point Point::operator/(float value) const {
        return Point(*this) /= value;
    }

    bool Point::operator==(const Point &other) const {
        return x == other.x && y == other.y;
    }

//  ----------- LINE ---------------

    Line::Line(const Point &a, const Point &b) :
            a(a), b(b) {
    }

    void Line::draw(cv::Mat &img) const {
        int w = img.cols;
        int h = img.rows;
        cv::Point _a(static_cast<int>(a.x * w), static_cast<int>(a.y * h));
        cv::Point _b(static_cast<int>(b.x * w), static_cast<int>(b.y * h));
        cv::line(img, _a, _b, cv::Scalar(180, 180, 180));
    }

    bool Line::side(const Point &p) const {
        // magic maths
        return ((p.x - a.x) * (b.y - a.y) - (p.y - a.y) * (b.x - a.x)) < 0;
    }

    Line Line::normal(const Point &p) const {
        float dx = b.x - a.x;
        float dy = b.y - a.y;
        return Line(Point(p.x, p.y), Point(p.x-dy, p.y+dx));
    }


//  ----------- HELPER METHODS ---------------

    float IoU(const cv::Rect2f &a, const cv::Rect2f &b) {
        cv::Rect2f intersect = intersection(a, b);

        float i_area = intersect.area();

        if (intersect.area() <= 0) {
            return 0;
        }

        float u_area = a.area() + b.area() - i_area;

        return i_area / u_area;
    }


    float cosine_similarity(const std::vector<float> &a, const std::vector<float> &b) {
        // See https://en.wikipedia.org/wiki/Cosine_similarity
        float dot = 0;
        float denom_a = 0;
        float denom_b = 0;

        for (size_t i = 0; i < a.size(); i++) {
            float a_val = a[i];
            float b_val = b[i];
            dot += a_val * b_val;
            denom_a += a_val * a_val;
            denom_b += b_val * b_val;
        }

        return dot / (std::sqrt(denom_a) * std::sqrt(denom_b) + 0.0001);

    }

    cv::Rect2f intersection(const cv::Rect2f &a, const cv::Rect2f &b) {
        float i_x1 = std::max(a.x, b.x);
        float i_y1 = std::max(a.y, b.y);
        float i_x2 = std::min(a.x + a.width, b.x + b.width);
        float i_y2 = std::min(a.y + a.height, b.y + b.height);

        float w = std::max(i_x2 - i_x1, 0.0f);
        float h = std::max(i_y2 - i_y1, 0.0f);

        return {i_x1, i_y1, w, h};
    }

    cv::Scalar hsv2rgb(const cv::Scalar &hsv) {
        cv::Mat m_rgb;
        cv::Mat m_hsv(1, 1, CV_8UC3, hsv);

        cvtColor(m_hsv, m_rgb, cv::COLOR_HSV2BGR);

        return {
                static_cast<double>(m_rgb.at<cv::Vec3b>(0, 0)[0]),
                static_cast<double>(m_rgb.at<cv::Vec3b>(0, 0)[1]),
                static_cast<double>(m_rgb.at<cv::Vec3b>(0, 0)[2])
        };
    }
}