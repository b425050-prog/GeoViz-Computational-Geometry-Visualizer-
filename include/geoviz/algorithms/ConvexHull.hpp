#pragma once

/**
 * @file ConvexHull.hpp
 * @brief Four interchangeable convex-hull strategy implementations.
 */

#include "geoviz/animation/AlgorithmTrace.hpp"

#include <memory>
#include <span>
#include <string_view>
#include <vector>

namespace geoviz::algorithms {

enum class HullAlgorithm { MonotonicChain, GrahamScan, JarvisMarch, QuickHull };

struct HullResult final {
    std::vector<Point> hull;
    AlgorithmTrace trace;
};

/**
 * @brief Strategy interface for convex hull computation.
 *
 * The UI owns this abstraction rather than a concrete algorithm, demonstrating
 * runtime polymorphism and allowing the algorithm to be changed at run time.
 */
class ConvexHullStrategy {
public:
    virtual ~ConvexHullStrategy() = default;
    [[nodiscard]] virtual std::string_view name() const noexcept = 0;
    [[nodiscard]] virtual std::string_view complexity() const noexcept = 0;
    [[nodiscard]] virtual HullResult compute(std::span<const Point> points) const = 0;
};

class MonotonicChainHull final : public ConvexHullStrategy {
public:
    [[nodiscard]] std::string_view name() const noexcept override { return "Monotonic Chain"; }
    [[nodiscard]] std::string_view complexity() const noexcept override { return "O(n log n)"; }
    [[nodiscard]] HullResult compute(std::span<const Point> points) const override;
};

class GrahamScanHull final : public ConvexHullStrategy {
public:
    [[nodiscard]] std::string_view name() const noexcept override { return "Graham Scan"; }
    [[nodiscard]] std::string_view complexity() const noexcept override { return "O(n log n)"; }
    [[nodiscard]] HullResult compute(std::span<const Point> points) const override;
};

class JarvisMarchHull final : public ConvexHullStrategy {
public:
    [[nodiscard]] std::string_view name() const noexcept override { return "Jarvis March"; }
    [[nodiscard]] std::string_view complexity() const noexcept override { return "O(nh)"; }
    [[nodiscard]] HullResult compute(std::span<const Point> points) const override;
};

class QuickHull final : public ConvexHullStrategy {
public:
    [[nodiscard]] std::string_view name() const noexcept override { return "QuickHull"; }
    [[nodiscard]] std::string_view complexity() const noexcept override {
        return "Average O(n log n), worst O(n^2)";
    }
    [[nodiscard]] HullResult compute(std::span<const Point> points) const override;
};

[[nodiscard]] std::unique_ptr<ConvexHullStrategy> makeHullStrategy(HullAlgorithm algorithm);

}  // namespace geoviz::algorithms

