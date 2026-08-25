/**
 * @file HalfPlaneIntersection.cpp
 * @brief Sutherland-Hodgman style clipping against oriented infinite lines.
 */

#include "geoviz/algorithms/HalfPlaneIntersection.hpp"

#include "geoviz/algorithms/Predicates.hpp"

#include <algorithm>
#include <cmath>

namespace geoviz::algorithms {
namespace {

[[nodiscard]] double sideValue(const HalfPlane& halfPlane, const Point& point) noexcept {
    return cross(halfPlane.direction(), point - halfPlane.point());
}

[[nodiscard]] std::vector<Point> clip(const std::vector<Point>& polygon,
                                      const HalfPlane& halfPlane) {
    std::vector<Point> output;
    if (polygon.empty()) return output;
    output.reserve(polygon.size() + 1U);

    for (std::size_t index = 0U; index < polygon.size(); ++index) {
        const Point& current = polygon[index];
        const Point& next = polygon[(index + 1U) % polygon.size()];
        const double currentSide = sideValue(halfPlane, current);
        const double nextSide = sideValue(halfPlane, next);
        const bool currentInside = currentSide >= -kEpsilon;
        const bool nextInside = nextSide >= -kEpsilon;

        if (currentInside) output.push_back(current);
        if (currentInside != nextInside) {
            const double denominator = currentSide - nextSide;
            if (std::abs(denominator) > kEpsilon) {
                const double parameter = std::clamp(currentSide / denominator, 0.0, 1.0);
                output.push_back(current + (next - current) * parameter);
            }
        }
    }
    return output;
}

}  // namespace

HalfPlaneIntersectionResult HalfPlaneIntersector::compute(
    std::span<const HalfPlane> halfPlanes, const Aabb& bounds) const {
    if (!std::isfinite(bounds.minX) || !std::isfinite(bounds.minY) ||
        !std::isfinite(bounds.maxX) || !std::isfinite(bounds.maxY) ||
        bounds.width() <= kEpsilon || bounds.height() <= kEpsilon) {
        throw GeometryError("Half-plane view bounds must be finite and non-empty.");
    }

    HalfPlaneIntersectionResult result;
    result.trace = AlgorithmTrace{"Half-Plane Intersection", "O(kv) progressive convex clipping"};
    result.polygon = {{bounds.minX, bounds.minY}, {bounds.maxX, bounds.minY},
                      {bounds.maxX, bounds.maxY}, {bounds.minX, bounds.maxY}};

    VisualFrame initial;
    initial.headline = "Start with the visible universe";
    initial.explanation = "The canvas rectangle represents an initially unconstrained region.";
    initial.filledPolygons = {result.polygon};
    initial.acceptedEdges = closedEdges(result.polygon);
    result.trace.addFrame(std::move(initial));

    const double guideLength = 4.0 * std::max(bounds.width(), bounds.height());
    std::size_t comparisons = 0U;
    for (std::size_t index = 0U; index < halfPlanes.size(); ++index) {
        comparisons += result.polygon.size();
        result.polygon = clip(result.polygon, halfPlanes[index]);

        VisualFrame frame;
        frame.headline = "Apply constraint " + std::to_string(index + 1U);
        frame.explanation = "Discard the right side of the oriented boundary; retain its left side.";
        frame.candidateEdges.emplace_back(
            halfPlanes[index].point() - halfPlanes[index].direction() * guideLength,
            halfPlanes[index].point() + halfPlanes[index].direction() * guideLength);
        if (result.polygon.size() >= 3U) {
            frame.filledPolygons = {result.polygon};
            frame.acceptedEdges = closedEdges(result.polygon);
            frame.highlightedPoints = result.polygon;
        }
        frame.comparisons = comparisons;
        frame.iteration = index + 1U;
        result.trace.addFrame(std::move(frame));
        if (result.polygon.empty()) break;
    }

    result.empty = result.polygon.size() < 3U || std::abs(signedArea(result.polygon)) <= kEpsilon;
    VisualFrame finalFrame;
    finalFrame.headline = result.empty ? "The feasible region is empty" : "Feasible region complete";
    finalFrame.explanation = result.empty
        ? "No point satisfies every half-plane constraint simultaneously."
        : "Every point in the highlighted polygon satisfies every oriented constraint.";
    if (!result.empty) {
        finalFrame.filledPolygons = {result.polygon};
        finalFrame.acceptedEdges = closedEdges(result.polygon);
        finalFrame.highlightedPoints = result.polygon;
    }
    finalFrame.comparisons = comparisons;
    finalFrame.iteration = result.trace.size();
    result.trace.addFrame(std::move(finalFrame));
    return result;
}

}  // namespace geoviz::algorithms

