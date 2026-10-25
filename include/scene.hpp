#pragma once
// The engine deliberately has no raylib dependency. Coordinates here are doubles.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace gv {

constexpr double INF = std::numeric_limits<double>::infinity();
constexpr double TOL = 1e-9;
constexpr std::size_t MAX_POINTS = 256, MAX_EDGES = 4096, MAX_TRACE = 4096;
/**
 * Validated two-dimensional world coordinate.
 * Finite doubles are bounded at construction and mutation. The rendering layer
 * applies its own screen transform; geometry never operates in screen pixels.
 */
class Point {
    double x_ = 0;
    double y_ = 0;
    static double checked(double value) {
        if (!std::isfinite(value) || std::abs(value) > 1e7)
            throw std::invalid_argument("Coordinate must be finite and within +/-10000000.");
        return value;
    }

  public:
    Point() = default;
    Point(double x, double y) : x_(checked(x)), y_(checked(y)) {}
    double x() const noexcept {
        return x_;
    }
    void setX(double value) {
        x_ = checked(value);
    }
    double y() const noexcept {
        return y_;
    }
    void setY(double value) {
        y_ = checked(value);
    }
};
/**
 * Axis-aligned spatial bounds used for tree partitions and trace regions.
 * A zero-width or zero-height box is permitted: it represents a split line.
 */
class Box {
    double x0_ = 0;
    double y0_ = 0;
    double x1_ = 0;
    double y1_ = 0;

  public:
    Box() = default;
    Box(double x0, double y0, double x1, double y1) : x0_(x0), y0_(y0), x1_(x1), y1_(y1) {}
    double x0() const noexcept {
        return x0_;
    }
    void setX0(double value) {
        x0_ = value;
    }
    double y0() const noexcept {
        return y0_;
    }
    void setY0(double value) {
        y0_ = value;
    }
    double x1() const noexcept {
        return x1_;
    }
    void setX1(double value) {
        x1_ = value;
    }
    double y1() const noexcept {
        return y1_;
    }
    void setY1(double value) {
        y1_ = value;
    }
};
/**
 * An undirected weighted graph connection between input point indices.
 * Scene alone may mutate endpoints, cost and blocked status so topology changes
 * pass through its validation and revision-tracking operations.
 */
class Edge {
    friend class Scene;
    int a_ = 0;
    int b_ = 0;
    double weight_ = 1;
    bool blocked_ = false;
    void setA(int value) {
        a_ = value;
    }
    void setB(int value) {
        b_ = value;
    }
    void setWeight(double value) {
        weight_ = value;
    }
    void setBlocked(bool value) {
        blocked_ = value;
    }

  public:
    Edge() = default;
    Edge(int a, int b, double weight = 1, bool blocked = false)
        : a_(a), b_(b), weight_(weight), blocked_(blocked) {}
    int a() const noexcept {
        return a_;
    }
    int b() const noexcept {
        return b_;
    }
    double weight() const noexcept {
        return weight_;
    }
    bool blocked() const noexcept {
        return blocked_;
    }
};
/**
 * Aggregate input model and the sole owner of editable graph topology.
 * Const collection views prevent callers from bypassing edit operations. Each
 * successful change advances revision; replacing points clears dependent edges.
 */
class Scene {
    std::vector<Point> points_ = {};
    std::vector<Point> polygon_ = {};
    std::vector<Edge> edges_ = {};
    Point query_ = {300, 250};
    double radius_ = 75;
    int minPts_ = 4;
    int source_ = 0;
    int goal_ = 0;
    bool grid_ = false;
    std::uint64_t revision_ = 0;

  public:
    Scene() = default;
    const std::vector<Point> &points() const noexcept {
        return points_;
    }
    void setPoints(std::vector<Point> value);
    const std::vector<Point> &polygon() const noexcept {
        return polygon_;
    }
    void setPolygon(std::vector<Point> value);
    const std::vector<Edge> &edges() const noexcept {
        return edges_;
    }
    void setEdges(std::vector<Edge> value);
    const Point &query() const noexcept {
        return query_;
    }
    void setQuery(Point value) {
        query_ = value;
        touch();
    }
    double radius() const noexcept {
        return radius_;
    }
    void setRadius(double value) {
        if (!std::isfinite(value) || value <= 0 || value > 1e7)
            throw std::invalid_argument("Radius must be positive and at most 10000000.");
        radius_ = value;
        touch();
    }
    int minPts() const noexcept {
        return minPts_;
    }
    void setMinPts(int value) {
        if (value < 1 || value > static_cast<int>(MAX_POINTS))
            throw std::invalid_argument("MinPts must be between 1 and 256.");
        minPts_ = value;
        touch();
    }
    int source() const noexcept {
        return source_;
    }
    void setSource(int value) {
        if (value < 0 || (value != 0 && value >= static_cast<int>(points_.size())))
            throw std::out_of_range("Unknown source vertex.");
        source_ = value;
        touch();
    }
    int goal() const noexcept {
        return goal_;
    }
    void setGoal(int value) {
        if (value < 0 || (value != 0 && value >= static_cast<int>(points_.size())))
            throw std::out_of_range("Unknown goal vertex.");
        goal_ = value;
        touch();
    }
    bool grid() const noexcept {
        return grid_;
    }
    void setGrid(bool value) {
        grid_ = value;
        touch();
    }
    std::uint64_t revision() const noexcept {
        return revision_;
    }
    void touch() noexcept {
        ++revision_;
    }
    void addPoint(Point point);
    void movePoint(std::size_t index, Point point);
    void removePoint(std::size_t index);
    void addPolygonVertex(Point point);
    void movePolygonVertex(std::size_t index, Point point);
    void addEdge(Edge edge);
    void setEdgeWeight(std::size_t index, double weight);
    void setEdgeBlocked(std::size_t index, bool blocked);
};

} // namespace gv
