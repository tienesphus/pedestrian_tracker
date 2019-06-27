#include <cmath>

#include "utils.hpp"

#include <opencv2/imgproc.hpp>

//  ----------- LINE ---------------

Line::Line(const cv::Point &a, const cv::Point &b): 
    a(a), b(b)
{
    // calculate the normal
    int dx = b.x - a.x;
    int dy = b.y - a.y;

    // div 20 makes the drawn length look about right (FIXME)
    int ndx = dy / 20; 
    int ndy = -dx / 20;
    
    cv::Point center = na = (a+b)/2;

    // define the normal to be on the positive side
    nb = cv::Point(center.x+ndx, center.y+ndy);
    if (!side(nb))
        nb = cv::Point(center.x-ndx, center.y-ndy);
}

void Line::draw(cv::Mat &img) const {
    cv::line(img, a, b, cv::Scalar(180, 180, 180));
    cv::line(img, na, nb, cv::Scalar(0, 0, 255));
}

bool Line::side(const cv::Point &p) const {
    return ((p.x - a.x) * (b.y - a.y) - (p.y - a.y) * (b.x - a.x)) < 0;
}


//  ----------- HELPER METHODS ---------------

float IoU(const cv::Rect &a, const cv::Rect &b) {
  cv::Rect intersect = intersection(a, b);

  int i_area = intersect.area();

  if (intersect.area() <= 0) {
    return 0;
  }

  int u_area = a.area() + b.area() - i_area;
  
  return static_cast<float>(i_area) / u_area;
}


float cosine_similarity(const cv::Mat &a, const cv::Mat &b) {
    // See https://en.wikipedia.org/wiki/Cosine_similarity
    float dot = 0;
    float denom_a = 0;
    float denom_b = 0;

    // TODO this assumes a very specific mat type
    for (size_t i = 0; i < a.size[1]; i++) {
        float a_val = a.at<float>(0, i);
        float b_val = b.at<float>(0, i);
        dot     += a_val * b_val;
        denom_a += a_val * a_val;
        denom_b += b_val * b_val;
    }

    return dot / (std::sqrt(denom_a) * std::sqrt(denom_b) + 0.0001);

}

cv::Rect intersection(const cv::Rect& a, const cv::Rect& b) {
    int i_x1 = std::max(a.x, b.x);
    int i_y1 = std::max(a.y, b.y);
    int i_x2 = std::min(a.x+a.width,  b.x+b.width);
    int i_y2 = std::min(a.y+a.height, b.y+b.height);

    int w = std::max(i_x2-i_x1, 0);
    int h = std::max(i_y2-i_y1, 0);

    return { i_x1, i_y1, w, h };
}

cv::Scalar hsv2rgb(const cv::Scalar& hsv)
{
    cv::Mat m_rgb;
    cv::Mat m_hsv(1, 1, CV_8UC3, hsv);

    cvtColor(m_hsv, m_rgb, cv::COLOR_HSV2BGR);

    return {
            static_cast<double>(m_rgb.at<cv::Vec3b>(0, 0)[0]),
            static_cast<double>(m_rgb.at<cv::Vec3b>(0, 0)[1]),
            static_cast<double>(m_rgb.at<cv::Vec3b>(0, 0)[2])
    };
}