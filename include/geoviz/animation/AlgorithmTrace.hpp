#pragma once

/**
 * @file AlgorithmTrace.hpp
 * @brief Framework-independent snapshots used to animate geometry algorithms.
 *
 * Algorithms never draw directly.  Instead, each meaningful decision is stored
 * as a VisualFrame.  The desktop UI can play, pause, scrub, or replay these
 * immutable frames, while tests can inspect the final mathematical result.
 */

#include "geoviz/geometry/Primitives.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace geoviz {

/** @brief One self-contained visual state in an algorithm animation. */
struct VisualFrame final {
    std::string headline;
    std::string explanation;
    std::vector<Point> sourcePoints;
    std::vector<Point> highlightedPoints;
    std::vector<Segment> acceptedEdges;
    std::vector<Segment> candidateEdges;
    std::vector<Segment> rejectedEdges;
    std::vector<Triangle> triangles;
    std::vector<std::vector<Point>> filledPolygons;
    std::optional<Circle> guideCircle;
    std::size_t comparisons{0U};
    std::size_t iteration{0U};
};

/** @brief Metadata and ordered frames produced by one algorithm run. */
class AlgorithmTrace final {
public:
    AlgorithmTrace() = default;
    AlgorithmTrace(std::string algorithmName, std::string complexity)
        : algorithmName_(std::move(algorithmName)), complexity_(std::move(complexity)) {}

    void addFrame(VisualFrame frame) { frames_.push_back(std::move(frame)); }
    void reserve(std::size_t count) { frames_.reserve(count); }

    [[nodiscard]] const std::string& algorithmName() const noexcept { return algorithmName_; }
    [[nodiscard]] const std::string& complexity() const noexcept { return complexity_; }
    [[nodiscard]] const std::vector<VisualFrame>& frames() const noexcept { return frames_; }
    [[nodiscard]] bool empty() const noexcept { return frames_.empty(); }
    [[nodiscard]] std::size_t size() const noexcept { return frames_.size(); }

private:
    std::string algorithmName_;
    std::string complexity_;
    std::vector<VisualFrame> frames_;
};

/** @brief Converts a closed vertex chain into individual renderable edges. */
[[nodiscard]] inline std::vector<Segment> closedEdges(const std::vector<Point>& vertices) {
    std::vector<Segment> result;
    if (vertices.size() < 2U) {
        return result;
    }
    result.reserve(vertices.size());
    for (std::size_t index = 0; index < vertices.size(); ++index) {
        result.emplace_back(vertices[index], vertices[(index + 1U) % vertices.size()]);
    }
    return result;
}

/** @brief Converts an open point chain into its consecutive edges. */
[[nodiscard]] inline std::vector<Segment> openEdges(const std::vector<Point>& vertices) {
    std::vector<Segment> result;
    if (vertices.size() < 2U) {
        return result;
    }
    result.reserve(vertices.size() - 1U);
    for (std::size_t index = 1U; index < vertices.size(); ++index) {
        result.emplace_back(vertices[index - 1U], vertices[index]);
    }
    return result;
}

}  // namespace geoviz

