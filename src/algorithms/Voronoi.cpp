/**
 * @file Voronoi.cpp
 * @brief Half-plane-clipping construction of bounded Voronoi cells.
 */

#include "geoviz/algorithms/Voronoi.hpp"

#include "geoviz/algorithms/Predicates.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <map>
#include <tuple>

namespace geoviz::algorithms {
namespace {

[[nodiscard]] double bisectorValue(const Point& candidate, const Point& site,
                                   const Point& other) noexcept {
    // Positive means candidate is at least as close to site as to other.
    return (other.x() * other.x() + other.y() * other.y()) -
           (site.x() * site.x() + site.y() * site.y()) -
           2.0 * (candidate.x() * (other.x() - site.x()) +
                  candidate.y() * (other.y() - site.y()));
}

[[nodiscard]] std::vector<Point> clipToCloserSide(const std::vector<Point>& polygon,
                                                  const Point& site, const Point& other) {
    std::vector<Point> output;
    if (polygon.empty()) return output;
    output.reserve(polygon.size() + 1U);

    for (std::size_t index = 0U; index < polygon.size(); ++index) {
        const Point& current = polygon[index];
        const Point& next = polygon[(index + 1U) % polygon.size()];
        const double currentValue = bisectorValue(current, site, other);
        const double nextValue = bisectorValue(next, site, other);
        const bool currentInside = currentValue >= -kEpsilon;
        const bool nextInside = nextValue >= -kEpsilon;

        if (currentInside) output.push_back(current);
        if (currentInside != nextInside) {
            const double denominator = currentValue - nextValue;
            if (std::abs(denominator) > kEpsilon) {
                const double parameter = std::clamp(currentValue / denominator, 0.0, 1.0);
                output.push_back(current + (next - current) * parameter);
            }
        }
    }
    return output;
}

struct QuantizedPoint final {
    std::int64_t x{0};
    std::int64_t y{0};
    [[nodiscard]] auto operator<=>(const QuantizedPoint&) const = default;
};

struct QuantizedEdge final {
    QuantizedPoint first;
    QuantizedPoint second;
    [[nodiscard]] auto operator<=>(const QuantizedEdge&) const = default;
};

[[nodiscard]] QuantizedPoint quantize(const Point& point) {
    constexpr double scale = 1.0e6;
    return {static_cast<std::int64_t>(std::llround(point.x() * scale)),
            static_cast<std::int64_t>(std::llround(point.y() * scale))};
}

[[nodiscard]] QuantizedEdge edgeKey(const Point& first, const Point& second) {
    QuantizedPoint a = quantize(first);
    QuantizedPoint b = quantize(second);
    if (b < a) std::swap(a, b);
    return {a, b};
}

}  // namespace

VoronoiResult VoronoiBuilder::compute(std::span<const Point> input, const Aabb& bounds) const {
    if (bounds.width() <= kEpsilon || bounds.height() <= kEpsilon ||
        !std::isfinite(bounds.minX) || !std::isfinite(bounds.minY) ||
        !std::isfinite(bounds.maxX) || !std::isfinite(bounds.maxY)) {
        throw GeometryError("Voronoi bounds must be finite and have positive area.");
    }

    DelaunayTriangulator triangulator;
    DelaunayResult delaunay = triangulator.compute(input);

    VoronoiResult result;
    result.trace = AlgorithmTrace{"Voronoi Diagram", "O(n^3) visual half-plane construction"};
    result.delaunayDual = triangulationEdges(delaunay);
    const std::vector<Point>& sites = delaunay.points;

    if (sites.empty()) {
        VisualFrame frame;
        frame.headline = "Add sites to begin";
        frame.explanation = "Each site owns the region closer to it than to any other site.";
        result.trace.addFrame(std::move(frame));
        return result;
    }

    const std::vector<Point> viewport{{bounds.minX, bounds.minY},
                                      {bounds.maxX, bounds.minY},
                                      {bounds.maxX, bounds.maxY},
                                      {bounds.minX, bounds.maxY}};
    std::size_t comparisons = 0U;
    for (std::size_t siteIndex = 0U; siteIndex < sites.size(); ++siteIndex) {
        std::vector<Point> cell = viewport;
        for (std::size_t otherIndex = 0U; otherIndex < sites.size(); ++otherIndex) {
            if (siteIndex == otherIndex) continue;
            ++comparisons;
            cell = clipToCloserSide(cell, sites[siteIndex], sites[otherIndex]);
            if (cell.empty()) break;
        }
        if (cell.size() >= 3U) {
            result.cells.push_back({sites[siteIndex], cell});
        }

        VisualFrame frame;
        frame.headline = "Clip cell for site " + std::to_string(siteIndex + 1U);
        frame.explanation = "Intersect the viewport with every closer-than-neighbour half-plane.";
        frame.sourcePoints = sites;
        frame.highlightedPoints = {sites[siteIndex]};
        for (const VoronoiCell& completed : result.cells) {
            frame.filledPolygons.push_back(completed.vertices);
        }
        frame.candidateEdges = result.delaunayDual;
        frame.comparisons = comparisons;
        frame.iteration = siteIndex + 1U;
        result.trace.addFrame(std::move(frame));
    }

    std::map<QuantizedEdge, Segment> uniqueEdges;
    for (const VoronoiCell& cell : result.cells) {
        for (std::size_t index = 0U; index < cell.vertices.size(); ++index) {
            const Point& first = cell.vertices[index];
            const Point& second = cell.vertices[(index + 1U) % cell.vertices.size()];
            uniqueEdges.insert_or_assign(edgeKey(first, second), Segment(first, second));
        }
    }
    result.edges.reserve(uniqueEdges.size());
    for (const auto& [key, edge] : uniqueEdges) {
        static_cast<void>(key);
        result.edges.push_back(edge);
    }

    VisualFrame finalFrame;
    finalFrame.headline = "Voronoi diagram complete";
    finalFrame.explanation = "Cell borders are equidistant from neighbouring sites; faint edges show the Delaunay dual.";
    finalFrame.sourcePoints = sites;
    finalFrame.acceptedEdges = result.edges;
    finalFrame.candidateEdges = result.delaunayDual;
    for (const VoronoiCell& cell : result.cells) {
        finalFrame.filledPolygons.push_back(cell.vertices);
    }
    finalFrame.comparisons = comparisons;
    finalFrame.iteration = result.trace.size();
    result.trace.addFrame(std::move(finalFrame));
    return result;
}

}  // namespace geoviz::algorithms

