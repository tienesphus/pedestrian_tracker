#ifndef UTILS_H
#define UTILS_H

namespace geom {

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
        Line normal(const Point &p) const;

    };

}

#endif
