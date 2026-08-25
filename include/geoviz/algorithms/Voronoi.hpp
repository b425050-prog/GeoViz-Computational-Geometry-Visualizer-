#pragma once

/**
 * @file Voronoi.hpp
 * @brief Viewport-clipped Voronoi cells and their Delaunay dual.
 */

#include "geoviz/algorithms/Delaunay.hpp"

#include <span>
#include <vector>

namespace geoviz::algorithms {

struct VoronoiCell final {
    Point site;
    std::vector<Point> vertices;
};

struct VoronoiResult final {
    std::vector<VoronoiCell> cells;
    std::vector<Segment> edges;
    std::vector<Segment> delaunayDual;
    AlgorithmTrace trace;
};

/**
 * @brief Builds exact clipped cells by intersecting perpendicular-bisector half-planes.
 *
 * This formulation remains stable for unbounded cells because the caller
 * supplies the visible canvas bounds.  Delaunay edges are also returned to make
 * the geometric duality directly visible in the UI.
 */
class VoronoiBuilder final {
public:
    [[nodiscard]] VoronoiResult compute(std::span<const Point> sites,
                                        const Aabb& bounds) const;
};

}  // namespace geoviz::algorithms

