#pragma once

/**
 * @file Delaunay.hpp
 * @brief Bowyer-Watson Delaunay triangulation with animation snapshots.
 */

#include "geoviz/animation/AlgorithmTrace.hpp"

#include <cstddef>
#include <span>
#include <vector>

namespace geoviz::algorithms {

/** @brief Triangle expressed as indices into DelaunayResult::points. */
struct IndexedTriangle final {
    std::size_t first{0U};
    std::size_t second{0U};
    std::size_t third{0U};

    [[nodiscard]] bool contains(std::size_t vertex) const noexcept {
        return first == vertex || second == vertex || third == vertex;
    }
};

struct DelaunayResult final {
    std::vector<Point> points;
    std::vector<IndexedTriangle> triangles;
    AlgorithmTrace trace;
};

/** @brief Incremental empty-circumcircle triangulator. */
class DelaunayTriangulator final {
public:
    [[nodiscard]] DelaunayResult compute(std::span<const Point> input) const;
};

[[nodiscard]] std::vector<Segment> triangulationEdges(const DelaunayResult& result);

}  // namespace geoviz::algorithms

