#include "utils.hpp"

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


float IoU(const cv::Rect2d &a, const cv::Rect2d &b) {
  int i_x1 = std::max(a.x, b.x);
  int i_y1 = std::max(a.y, b.y);
  int i_x2 = std::min(a.x+a.width,  b.x+b.width);
  int i_y2 = std::min(a.y+a.height, b.y+b.height);
  
  if ((i_x2 < i_x1) || (i_y2 < i_y1)) {
    return 0;
  }
  
  cv::Rect2d intersect(
      cv::Point2d(i_x1, i_y1),
      cv::Point2d(i_x2, i_y2)
  );
  
  float i_area = intersect.area();
  float u_area = a.area() + b.area() - i_area;
  
  return i_area / u_area;
}
