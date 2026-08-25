#pragma once

/**
 * @file SceneModel.hpp
 * @brief Observable application model containing editable geometric input.
 */

#include "geoviz/geometry/Primitives.hpp"

#include <cstddef>
#include <functional>
#include <map>
#include <optional>
#include <span>
#include <vector>

namespace geoviz::app {

/** @brief Snapshot used for file I/O and transactional scene replacement. */
struct SceneSnapshot final {
    std::vector<Point> points;
    std::vector<HalfPlane> halfPlanes;
};

/**
 * @brief Encapsulates mutable scene state and notifies observers after changes.
 *
 * Algorithms receive only const views of this class, which prevents the
 * presentation layer from accidentally changing mathematical input.
 */
class SceneModel final {
public:
    using Observer = std::function<void()>;
    using ObserverToken = std::size_t;

    [[nodiscard]] const std::vector<Point>& points() const noexcept { return points_; }
    [[nodiscard]] const std::vector<HalfPlane>& halfPlanes() const noexcept { return halfPlanes_; }
    [[nodiscard]] std::size_t revision() const noexcept { return revision_; }
    [[nodiscard]] SceneSnapshot snapshot() const { return {points_, halfPlanes_}; }

    [[nodiscard]] std::size_t addPoint(Point point);
    void insertPoint(std::size_t index, Point point);
    void setPoint(std::size_t index, Point point);
    [[nodiscard]] Point removePoint(std::size_t index);
    [[nodiscard]] std::optional<std::size_t> nearestPoint(const Point& point,
                                                          double maximumDistance) const;
    void setPoints(std::vector<Point> points);
    void setHalfPlanes(std::vector<HalfPlane> halfPlanes);
    void replace(SceneSnapshot snapshot);
    void clear();

    [[nodiscard]] ObserverToken subscribe(Observer observer);
    void unsubscribe(ObserverToken token) noexcept;

private:
    void notify();

    std::vector<Point> points_;
    std::vector<HalfPlane> halfPlanes_;
    std::map<ObserverToken, Observer> observers_;
    ObserverToken nextObserverToken_{1U};
    std::size_t revision_{0U};
};

}  // namespace geoviz::app

