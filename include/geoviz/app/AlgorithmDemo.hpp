#pragma once

/**
 * @file AlgorithmDemo.hpp
 * @brief Polymorphic adapters that connect scene input to algorithm traces.
 */

#include "geoviz/animation/AlgorithmTrace.hpp"
#include "geoviz/app/SceneModel.hpp"
#include "geoviz/core/Utility.hpp"

#include <cstddef>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace geoviz::app {

enum class DemoId {
    PrimitiveLab,
    HullMonotonic,
    HullGraham,
    HullJarvis,
    HullQuick,
    Delaunay,
    Voronoi,
    HalfPlaneIntersection,
    EarClipping,
    ClosestPair,
    SegmentIntersections,
    PointInPolygon,
    SmallestEnclosingCircle,
    RotatingCalipers
};

enum class InputStyle { PointSet, OrderedPolygon, PointPairs, PolygonWithQuery };

struct DemoMetadata final {
    DemoId id{DemoId::PrimitiveLab};
    std::string title;
    std::string sidebarLabel;
    std::string category;
    std::string description;
    std::string complexity;
    std::string inputHint;
    InputStyle inputStyle{InputStyle::PointSet};
    std::size_t minimumPoints{0U};
    int accentIndex{0};
};

/** @brief First pure interface: supplies user-facing component metadata. */
class DescribedComponent {
public:
    virtual ~DescribedComponent() = default;
    [[nodiscard]] virtual const DemoMetadata& metadata() const noexcept = 0;
};

/** @brief Second pure interface: transforms a scene into visual frames. */
class TraceProducer {
public:
    virtual ~TraceProducer() = default;
    [[nodiscard]] virtual AlgorithmTrace buildTrace(const SceneModel& scene,
                                                    const Aabb& viewBounds) const = 0;
};

/**
 * @brief Common demo abstraction using multiple inheritance of pure interfaces.
 *
 * Interface-only multiple inheritance composes independent capabilities without
 * shared state or the ambiguity of a diamond hierarchy.
 */
class AlgorithmDemo : public DescribedComponent, public TraceProducer {
public:
    ~AlgorithmDemo() override = default;
};

/** @brief Factory-owned collection used by the navigation sidebar. */
class DemoRegistry final {
public:
    DemoRegistry();

    [[nodiscard]] std::size_t size() const noexcept { return demos_.size(); }
    [[nodiscard]] const AlgorithmDemo& at(std::size_t index) const;
    [[nodiscard]] std::size_t indexOf(DemoId id) const;

private:
    std::vector<std::unique_ptr<AlgorithmDemo>> demos_;
};

/** @brief State machine for deterministic play/pause/step animation control. */
class TracePlayer final {
public:
    void reset(std::size_t frameCount) noexcept;
    void update(float deltaSeconds, std::size_t frameCount) noexcept;
    void toggle() noexcept { playing_ = !playing_; }
    void setPlaying(bool value) noexcept { playing_ = value; }
    void first() noexcept { frameIndex_ = 0U; accumulator_ = 0.0F; }
    void last(std::size_t frameCount) noexcept;
    void next(std::size_t frameCount) noexcept;
    void previous() noexcept;
    void setSpeed(float framesPerSecond) noexcept;

    [[nodiscard]] std::size_t frameIndex() const noexcept { return frameIndex_; }
    [[nodiscard]] bool playing() const noexcept { return playing_; }
    [[nodiscard]] float speed() const noexcept { return speed_.get(); }

private:
    std::size_t frameIndex_{0U};
    float accumulator_{0.0F};
    core::BoundedValue<float> speed_{2.0F, 0.5F, 12.0F};
    bool playing_{true};
};

}  // namespace geoviz::app
