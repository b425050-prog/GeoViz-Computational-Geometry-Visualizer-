#pragma once
// The engine deliberately has no raylib dependency. Coordinates here are doubles.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "scene.hpp"

namespace gv {
class GeometryKernel {
  public:
    static double cross(Point a, Point b, Point c);
    static double distance2(Point a, Point b);
    static double distance(Point a, Point b);
    static int orientation(Point a, Point b, Point c);
    static bool onSegment(Point p, Point a, Point b);
    static bool intersects(Point a, Point b, Point c, Point d);
    // -1 outside, 0 boundary, 1 inside.
    static int membership(Point q, const std::vector<Point> &polygon);
    static void validatePolygon(const std::vector<Point> &polygon);
};

} // namespace gv
