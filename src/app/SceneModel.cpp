/**
 * @file SceneModel.cpp
 * @brief Bounds-checked observable scene mutation operations.
 */

#include "geoviz/app/SceneModel.hpp"

#include "geoviz/algorithms/Predicates.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace geoviz::app {

std::size_t SceneModel::addPoint(Point point) {
    points_.push_back(std::move(point));
    notify();
    return points_.size() - 1U;
}

void SceneModel::insertPoint(std::size_t index, Point point) {
    if (index > points_.size()) throw std::out_of_range("Point insertion index is out of range.");
    points_.insert(points_.begin() + static_cast<std::ptrdiff_t>(index), std::move(point));
    notify();
}

void SceneModel::setPoint(std::size_t index, Point point) {
    if (index >= points_.size()) throw std::out_of_range("Point update index is out of range.");
    points_[index] = std::move(point);
    notify();
}

Point SceneModel::removePoint(std::size_t index) {
    if (index >= points_.size()) throw std::out_of_range("Point removal index is out of range.");
    Point removed = points_[index];
    points_.erase(points_.begin() + static_cast<std::ptrdiff_t>(index));
    notify();
    return removed;
}

std::optional<std::size_t> SceneModel::nearestPoint(const Point& point,
                                                    double maximumDistance) const {
    std::optional<std::size_t> bestIndex;
    double bestSquared = maximumDistance * maximumDistance;
    for (std::size_t index = 0U; index < points_.size(); ++index) {
        const double candidate = algorithms::squaredDistance(point, points_[index]);
        if (candidate <= bestSquared) {
            bestSquared = candidate;
            bestIndex = index;
        }
    }
    return bestIndex;
}

void SceneModel::setPoints(std::vector<Point> points) {
    points_ = std::move(points);
    notify();
}

void SceneModel::setHalfPlanes(std::vector<HalfPlane> halfPlanes) {
    halfPlanes_ = std::move(halfPlanes);
    notify();
}

void SceneModel::replace(SceneSnapshot snapshot) {
    points_ = std::move(snapshot.points);
    halfPlanes_ = std::move(snapshot.halfPlanes);
    notify();
}

void SceneModel::clear() {
    points_.clear();
    halfPlanes_.clear();
    notify();
}

SceneModel::ObserverToken SceneModel::subscribe(Observer observer) {
    const ObserverToken token = nextObserverToken_++;
    observers_.emplace(token, std::move(observer));
    return token;
}

void SceneModel::unsubscribe(ObserverToken token) noexcept {
    observers_.erase(token);
}

void SceneModel::notify() {
    ++revision_;
    // Copy callbacks so an observer may safely unsubscribe itself while running.
    const auto observers = observers_;
    for (const auto& [token, observer] : observers) {
        static_cast<void>(token);
        if (observer) observer();
    }
}

}  // namespace geoviz::app

