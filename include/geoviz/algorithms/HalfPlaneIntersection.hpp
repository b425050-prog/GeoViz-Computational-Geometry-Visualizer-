#pragma once

/**
 * @file HalfPlaneIntersection.hpp
 * @brief Bounded half-plane intersection by progressive convex clipping.
 */

#include "geoviz/animation/AlgorithmTrace.hpp"

#include <span>
#include <vector>

namespace geoviz::algorithms {

struct HalfPlaneIntersectionResult final {
    std::vector<Point> polygon;
    bool empty{true};
    AlgorithmTrace trace;
};

/**
 * @brief Intersects left-sided half-plane constraints inside finite view bounds.
 *
 * Bounding the computation is intentional: it makes unbounded mathematical
 * regions representable on screen and gives the visualizer a well-defined
 * polygon at every animation step.
 */
class HalfPlaneIntersector final {
public:
    [[nodiscard]] HalfPlaneIntersectionResult compute(
        std::span<const HalfPlane> halfPlanes, const Aabb& bounds) const;
};

}  // namespace geoviz::algorithms

