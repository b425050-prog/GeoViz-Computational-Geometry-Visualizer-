#pragma once

/**
 * @file PolygonAlgorithms.hpp
 * @brief Polygon, proximity, simplification, and enclosing-circle algorithms.
 */

#include "geoviz/animation/AlgorithmTrace.hpp"

#include <limits>
#include <optional>
#include <span>
#include <utility>
#include <vector>

namespace geoviz::algorithms {

struct PolygonTriangulationResult final {
    std::vector<Triangle> triangles;
    AlgorithmTrace trace;
};

struct ClosestPairResult final {
    std::optional<Segment> pair;
    double distance{std::numeric_limits<double>::infinity()};
    AlgorithmTrace trace;
};

struct SegmentIntersectionsResult final {
    std::vector<Point> intersections;
    AlgorithmTrace trace;
};

class EarClippingTriangulator final {
public:
    [[nodiscard]] PolygonTriangulationResult compute(std::span<const Point> polygon) const;
};

class ClosestPairSolver final {
public:
    [[nodiscard]] ClosestPairResult compute(std::span<const Point> points) const;
};

class SegmentIntersectionSolver final {
public:
    [[nodiscard]] SegmentIntersectionsResult compute(std::span<const Segment> segments) const;
};

/** @brief Sutherland-Hodgman clipping; the clip window must be convex and CCW. */
[[nodiscard]] std::vector<Point> clipPolygon(std::span<const Point> subject,
                                             std::span<const Point> convexClipWindow);

/** @brief Minkowski sum of two point sets, returned as a convex polygon. */
[[nodiscard]] std::vector<Point> minkowskiSum(std::span<const Point> first,
                                              std::span<const Point> second);

/** @brief Farthest pair of vertices on a convex CCW polygon via rotating calipers. */
[[nodiscard]] std::optional<Segment> convexDiameter(std::span<const Point> convexPolygon);

/** @brief Douglas-Peucker polyline simplification. */
[[nodiscard]] std::vector<Point> simplifyPolyline(std::span<const Point> polyline,
                                                  double tolerance);

/** @brief Deterministic incremental smallest enclosing circle. */
[[nodiscard]] std::optional<Circle> smallestEnclosingCircle(std::span<const Point> points);

}  // namespace geoviz::algorithms

