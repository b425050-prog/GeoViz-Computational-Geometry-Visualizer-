#pragma once

/**
 * @file Predicates.hpp
 * @brief Robust predicates, measurements, projections, and intersections.
 */

#include "geoviz/geometry/Primitives.hpp"

#include <optional>
#include <utility>
#include <vector>

namespace geoviz::algorithms {

enum class Orientation { Clockwise = -1, Collinear = 0, CounterClockwise = 1 };
enum class PointLocation { Outside, Boundary, Inside };
enum class IntersectionKind { None, Point, Overlap, TwoPoints };

/** @brief A uniform result type for line, segment, ray, and circle intersections. */
struct IntersectionResult final {
    IntersectionKind kind{IntersectionKind::None};
    std::vector<Point> points;

    [[nodiscard]] bool intersects() const noexcept { return kind != IntersectionKind::None; }
};

[[nodiscard]] bool almostEqual(double first, double second,
                               double epsilon = kEpsilon) noexcept;
[[nodiscard]] bool samePoint(const Point& first, const Point& second,
                             double epsilon = kEpsilon) noexcept;
[[nodiscard]] bool lexicographicLess(const Point& first, const Point& second,
                                     double epsilon = kEpsilon) noexcept;
[[nodiscard]] double dot(const Vector2D& first, const Vector2D& second) noexcept;
[[nodiscard]] double cross(const Vector2D& first, const Vector2D& second) noexcept;
[[nodiscard]] double cross(const Point& origin, const Point& first,
                           const Point& second) noexcept;
[[nodiscard]] Orientation orientation(const Point& first, const Point& second,
                                      const Point& third,
                                      double epsilon = kEpsilon) noexcept;
[[nodiscard]] double squaredDistance(const Point& first, const Point& second) noexcept;
[[nodiscard]] double distance(const Point& first, const Point& second) noexcept;
[[nodiscard]] double signedArea(const std::vector<Point>& polygon) noexcept;
[[nodiscard]] Point centroid(const std::vector<Point>& polygon);
[[nodiscard]] double distanceToLine(const Point& point, const Line& line) noexcept;
[[nodiscard]] double distanceToRay(const Point& point, const Ray& ray) noexcept;
[[nodiscard]] double distanceToSegment(const Point& point, const Segment& segment) noexcept;
[[nodiscard]] Point projectOntoLine(const Point& point, const Line& line);
[[nodiscard]] Point projectOntoSegment(const Point& point, const Segment& segment);
[[nodiscard]] PointLocation locatePointInPolygon(const Point& point,
                                                 const std::vector<Point>& polygon,
                                                 double epsilon = kEpsilon) noexcept;

[[nodiscard]] IntersectionResult intersect(const Line& first, const Line& second,
                                           double epsilon = kEpsilon);
[[nodiscard]] IntersectionResult intersect(const Segment& first, const Segment& second,
                                           double epsilon = kEpsilon);
[[nodiscard]] IntersectionResult intersect(const Ray& ray, const Segment& segment,
                                           double epsilon = kEpsilon);
[[nodiscard]] IntersectionResult intersect(const Circle& circle, const Line& line,
                                           double epsilon = kEpsilon);
[[nodiscard]] IntersectionResult intersect(const Circle& first, const Circle& second,
                                           double epsilon = kEpsilon);

[[nodiscard]] std::optional<Point> circumcenter(const Triangle& triangle,
                                                double epsilon = kEpsilon);
[[nodiscard]] bool circumcircleContains(const Triangle& triangle, const Point& point,
                                        double epsilon = kEpsilon) noexcept;
[[nodiscard]] Point rotate(const Point& point, const Point& pivot, double radians);
[[nodiscard]] Point reflectAcrossLine(const Point& point, const Line& mirror);

}  // namespace geoviz::algorithms

