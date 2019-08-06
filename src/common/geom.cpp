#include "geom.hpp"

#include <cmath>

namespace geom {

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

    bool Line::side(const Point &p) const {
        // magic maths
        return ((p.x - a.x) * (b.y - a.y) - (p.y - a.y) * (b.x - a.x)) < 0;
    }

    Line Line::normal(const Point &p) const {
        float dx = b.x - a.x;
        float dy = b.y - a.y;
        return { Point(p.x, p.y), Point(p.x - dy, p.y + dx) };
    }

}