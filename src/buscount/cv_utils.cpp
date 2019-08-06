#include "cv_utils.hpp"

#include <opencv2/imgproc.hpp>


//  ----------- HELPER METHODS ---------------

namespace geom {

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

    void draw_line(cv::Mat &img, const Line& line, const cv::Scalar& clr) {
        int w = img.cols;
        int h = img.rows;
        cv::Point _a(static_cast<int>(line.a.x * w), static_cast<int>(line.a.y * h));
        cv::Point _b(static_cast<int>(line.b.x * w), static_cast<int>(line.b.y * h));
        cv::line(img, _a, _b, clr);
    }

    /**
     * Gets the point of intersection between two lines
     * Will crash if the two lines are parallel
     */
    Point getIntersection(Line l1, Line l2)
    {
        float dx1 = l1.b.x -l1.a.x;
        float dy1 = l1.b.y -l1.a.y;
        float dx2 = l2.b.x -l2.a.x;
        float dy2 = l2.b.y -l2.a.y;
        // ahh, maths...
        float s = (dx2 * (l1.a.y - l2.a.y) + dy2 * (l2.a.x - l1.a.x)) / (dx1 * dy2 - dx2 * dy1);
        return { l1.a.x + dx1 * s, l1.a.y + dy1 * s };
    }

    /**
     * Extends a line so that it extends to screen edges. Note: the lines may end up slightly
     * past the edge of the screen
     */
    Line extend(const Line& line)
    {
        // Choose whether to use top/bottom or left/right edges based on ratio of dx:dy
        // note that its okay if the points are outside the boundaries
        float dx = abs(line.a.x - line.b.x);
        float dy = abs(line.a.y - line.b.y);
        if (dx > dy) {
            // use left/right intercepts
            Point leftInt = getIntersection(line, Line(Point(0, 0), Point(0, 1)));
            Point rightInt = getIntersection(line, Line(Point(1, 0), Point(1, 1)));
            return { leftInt, rightInt };
        } else {
            // use top/bot intercepts
            Point topInt = getIntersection(line, Line(Point(0, 0), Point(1, 0)));
            Point botInt = getIntersection(line, Line(Point(0, 1), Point(1, 1)));
            return { topInt, botInt };
        }
    }

    void draw_world_config(cv::Mat &img, const WorldConfig& config)
    {
        // draw crossing line
        draw_line(img, config.crossing, cv::Scalar(180, 180, 180));

        // draw in/out lines (note, these will not be perfectly normal to the original line. They are normal when the
        // view is square - when the view is rectangular, the normal lines get distorted.
        draw_line(img, extend(config.crossing.normal(config.crossing.a)), cv::Scalar(220, 220, 220));
        draw_line(img, extend(config.crossing.normal(config.crossing.b)), cv::Scalar(220, 220, 220));

        for (const geom::Line& bound: config.bounds) {
            draw_line(img, bound, cv::Scalar(100, 100, 100));
            //extend(bound.normal((bound.a+bound.b)/2)).draw(img);
        }
    }

    void draw_world_count(cv::Mat& display, const int in_count, const int out_count)
    {
        // TODO remove magic numbers
        std::string txt =
                "IN: " + std::to_string(in_count) + "; " +
                "OUT:" + std::to_string(out_count);
        cv::putText(display, txt,
                    cv::Point(4, 24), cv::FONT_HERSHEY_SIMPLEX, 0.6,
                    cv::Scalar(255, 255, 255), 2);
    }




}