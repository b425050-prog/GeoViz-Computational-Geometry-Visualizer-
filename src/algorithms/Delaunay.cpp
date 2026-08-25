/**
 * @file Delaunay.cpp
 * @brief Animated Bowyer-Watson triangulation implementation.
 */

#include "geoviz/algorithms/Delaunay.hpp"

#include "geoviz/algorithms/Predicates.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <set>
#include <tuple>
#include <utility>

namespace geoviz::algorithms {
namespace {

struct EdgeKey final {
    std::size_t first{0U};
    std::size_t second{0U};

    EdgeKey(std::size_t a, std::size_t b)
        : first(std::min(a, b)), second(std::max(a, b)) {}

    [[nodiscard]] auto operator<=>(const EdgeKey&) const = default;
};

[[nodiscard]] std::vector<Point> uniqueSortedPoints(std::span<const Point> input) {
    std::vector<Point> points(input.begin(), input.end());
    std::sort(points.begin(), points.end(), [](const Point& first, const Point& second) {
        return lexicographicLess(first, second);
    });
    points.erase(std::unique(points.begin(), points.end(), [](const Point& first, const Point& second) {
        return samePoint(first, second);
    }), points.end());
    return points;
}

[[nodiscard]] Triangle materialize(const IndexedTriangle& triangle,
                                   const std::vector<Point>& points) {
    return {points[triangle.first], points[triangle.second], points[triangle.third]};
}

[[nodiscard]] bool inCircumcircleInclusive(const IndexedTriangle& indexed,
                                           const std::vector<Point>& points,
                                           const Point& candidate) {
    const Triangle triangle = materialize(indexed, points);
    const std::optional<Point> centre = circumcenter(triangle);
    if (!centre.has_value()) {
        return false;
    }
    const double radiusSquared = squaredDistance(*centre, triangle.first());
    const double candidateSquared = squaredDistance(*centre, candidate);
    const double tolerance = 1.0e-8 * std::max(1.0, radiusSquared);
    return candidateSquared <= radiusSquared + tolerance;
}

[[nodiscard]] std::vector<Triangle> materializeAll(
    const std::vector<IndexedTriangle>& triangles, const std::vector<Point>& points,
    const std::set<std::size_t>& omitted = {}) {
    std::vector<Triangle> result;
    result.reserve(triangles.size());
    for (std::size_t index = 0; index < triangles.size(); ++index) {
        if (!omitted.contains(index)) {
            result.push_back(materialize(triangles[index], points));
        }
    }
    return result;
}

}  // namespace

DelaunayResult DelaunayTriangulator::compute(std::span<const Point> input) const {
    DelaunayResult result;
    result.points = uniqueSortedPoints(input);
    result.trace = AlgorithmTrace{"Bowyer-Watson Delaunay", "Expected O(n log n), O(n^2) naive search"};

    if (result.points.size() < 3U) {
        VisualFrame frame;
        frame.headline = "More sites required";
        frame.explanation = "Delaunay triangulation requires at least three non-collinear sites.";
        frame.sourcePoints = result.points;
        result.trace.addFrame(std::move(frame));
        return result;
    }

    const std::size_t originalCount = result.points.size();
    double minX = result.points.front().x();
    double maxX = minX;
    double minY = result.points.front().y();
    double maxY = minY;
    for (const Point& point : result.points) {
        minX = std::min(minX, point.x());
        maxX = std::max(maxX, point.x());
        minY = std::min(minY, point.y());
        maxY = std::max(maxY, point.y());
    }
    const double centreX = 0.5 * (minX + maxX);
    const double centreY = 0.5 * (minY + maxY);
    const double extent = std::max(1.0, std::max(maxX - minX, maxY - minY));

    // The oversized super-triangle contains the complete data set.  Its three
    // vertices are removed from the public result after incremental insertion.
    result.points.emplace_back(centreX - 24.0 * extent, centreY - 2.0 * extent);
    result.points.emplace_back(centreX, centreY + 24.0 * extent);
    result.points.emplace_back(centreX + 24.0 * extent, centreY - 2.0 * extent);
    const std::size_t superFirst = originalCount;
    const std::size_t superSecond = originalCount + 1U;
    const std::size_t superThird = originalCount + 2U;

    // Store every working triangle counter-clockwise.  The screen renderer then
    // performs one predictable y-axis inversion for all triangles.
    std::vector<IndexedTriangle> working{{superFirst, superThird, superSecond}};
    VisualFrame initial;
    initial.headline = "Create a super-triangle";
    initial.explanation = "A temporary enclosing triangle makes every insertion local.";
    initial.sourcePoints.assign(result.points.begin(), result.points.begin() +
                                                       static_cast<std::ptrdiff_t>(originalCount));
    initial.triangles.push_back(materialize(working.front(), result.points));
    result.trace.addFrame(std::move(initial));

    std::size_t comparisons = 0U;
    for (std::size_t pointIndex = 0U; pointIndex < originalCount; ++pointIndex) {
        const Point& inserted = result.points[pointIndex];
        std::set<std::size_t> badTriangleIndices;
        for (std::size_t triangleIndex = 0U; triangleIndex < working.size(); ++triangleIndex) {
            ++comparisons;
            if (inCircumcircleInclusive(working[triangleIndex], result.points, inserted)) {
                badTriangleIndices.insert(triangleIndex);
            }
        }

        std::map<EdgeKey, std::pair<std::size_t, std::size_t>> edgeOccurrences;
        std::map<EdgeKey, std::size_t> edgeCounts;
        for (std::size_t badIndex : badTriangleIndices) {
            const IndexedTriangle& triangle = working[badIndex];
            const std::array<std::pair<std::size_t, std::size_t>, 3> edges{{
                {triangle.first, triangle.second},
                {triangle.second, triangle.third},
                {triangle.third, triangle.first}
            }};
            for (const auto& [first, second] : edges) {
                const EdgeKey key(first, second);
                ++edgeCounts[key];
                edgeOccurrences[key] = {first, second};
            }
        }

        VisualFrame cavity;
        cavity.headline = "Remove violated circumcircles";
        cavity.explanation = "Triangles whose circumcircles contain the new site form a cavity.";
        cavity.sourcePoints.assign(result.points.begin(), result.points.begin() +
                                                          static_cast<std::ptrdiff_t>(originalCount));
        cavity.highlightedPoints = {inserted};
        cavity.triangles = materializeAll(working, result.points);
        if (!badTriangleIndices.empty()) {
            const Triangle bad = materialize(working[*badTriangleIndices.begin()], result.points);
            if (const std::optional<Point> centre = circumcenter(bad); centre.has_value()) {
                cavity.guideCircle.emplace(*centre, distance(*centre, bad.first()));
            }
        }
        for (const auto& [key, count] : edgeCounts) {
            if (count == 1U) {
                const auto [first, second] = edgeOccurrences.at(key);
                cavity.acceptedEdges.emplace_back(result.points[first], result.points[second]);
            }
        }
        cavity.comparisons = comparisons;
        cavity.iteration = pointIndex + 1U;
        result.trace.addFrame(std::move(cavity));

        std::vector<IndexedTriangle> retained;
        retained.reserve(working.size() + edgeCounts.size());
        for (std::size_t index = 0U; index < working.size(); ++index) {
            if (!badTriangleIndices.contains(index)) retained.push_back(working[index]);
        }

        for (const auto& [key, count] : edgeCounts) {
            if (count != 1U) continue;
            auto [first, second] = edgeOccurrences.at(key);
            const Orientation turn = orientation(result.points[first], result.points[second], inserted);
            if (turn == Orientation::Collinear) continue;
            if (turn == Orientation::Clockwise) std::swap(first, second);
            retained.push_back({first, second, pointIndex});
        }
        working = std::move(retained);

        VisualFrame retriangulated;
        retriangulated.headline = "Retriangulate the cavity";
        retriangulated.explanation = "Connect the inserted site to every boundary edge.";
        retriangulated.sourcePoints.assign(result.points.begin(), result.points.begin() +
                                                                 static_cast<std::ptrdiff_t>(originalCount));
        retriangulated.highlightedPoints = {inserted};
        retriangulated.triangles = materializeAll(working, result.points);
        retriangulated.comparisons = comparisons;
        retriangulated.iteration = pointIndex + 1U;
        result.trace.addFrame(std::move(retriangulated));
    }

    for (const IndexedTriangle& triangle : working) {
        if (!triangle.contains(superFirst) && !triangle.contains(superSecond) &&
            !triangle.contains(superThird)) {
            result.triangles.push_back(triangle);
        }
    }
    result.points.resize(originalCount);

    VisualFrame finalFrame;
    finalFrame.headline = "Delaunay triangulation complete";
    finalFrame.explanation = "No site lies strictly inside any retained triangle's circumcircle.";
    finalFrame.sourcePoints = result.points;
    for (const IndexedTriangle& triangle : result.triangles) {
        finalFrame.triangles.push_back(materialize(triangle, result.points));
    }
    finalFrame.acceptedEdges = triangulationEdges(result);
    finalFrame.comparisons = comparisons;
    finalFrame.iteration = result.trace.size();
    result.trace.addFrame(std::move(finalFrame));
    return result;
}

std::vector<Segment> triangulationEdges(const DelaunayResult& result) {
    std::set<EdgeKey> uniqueEdges;
    for (const IndexedTriangle& triangle : result.triangles) {
        uniqueEdges.emplace(triangle.first, triangle.second);
        uniqueEdges.emplace(triangle.second, triangle.third);
        uniqueEdges.emplace(triangle.third, triangle.first);
    }
    std::vector<Segment> edges;
    edges.reserve(uniqueEdges.size());
    for (const EdgeKey& edge : uniqueEdges) {
        edges.emplace_back(result.points[edge.first], result.points[edge.second]);
    }
    return edges;
}

}  // namespace geoviz::algorithms
