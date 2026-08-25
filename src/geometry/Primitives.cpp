/**
 * @file Primitives.cpp
 * @brief Validated implementations of GeoViz's geometry object hierarchy.
 */

#include "geoviz/geometry/Primitives.hpp"

#include <algorithm>
#include <iomanip>
#include <limits>
#include <ostream>

namespace geoviz {
namespace {

[[nodiscard]] bool isFinite(double value) noexcept {
    return std::isfinite(value);
}

[[nodiscard]] double cross(const Vector2D& first, const Vector2D& second) noexcept {
    return first.x() * second.y() - first.y() * second.x();
}

[[nodiscard]] double dot(const Vector2D& first, const Vector2D& second) noexcept {
    return first.x() * second.x() + first.y() * second.y();
}

[[nodiscard]] double pointDistance(const Point& first, const Point& second) noexcept {
    return (first - second).length();
}

}  // namespace

Vector2D Vector2D::normalized(double epsilon) const {
    const double magnitude = length();
    if (magnitude <= epsilon) {
        throw GeometryError("Cannot normalize a zero-length vector.");
    }
    return *this / magnitude;
}

Vector2D Vector2D::operator/(double scalar) const {
    if (std::abs(scalar) <= kEpsilon) {
        throw GeometryError("Vector division by zero is undefined.");
    }
    return {x_ / scalar, y_ / scalar};
}

Point::Point(double x, double y) {
    set(x, y);
}

void Point::set(double x, double y) {
    if (!isFinite(x) || !isFinite(y)) {
        throw GeometryError("Point coordinates must be finite numbers.");
    }
    x_ = x;
    y_ = y;
}

Point Point::operator+(const Vector2D& displacement) const {
    return {x_ + displacement.x(), y_ + displacement.y()};
}

Point Point::operator-(const Vector2D& displacement) const {
    return {x_ - displacement.x(), y_ - displacement.y()};
}

Vector2D Point::operator-(const Point& other) const noexcept {
    return {x_ - other.x_, y_ - other.y_};
}

bool Point::contains(const Point& point, double epsilon) const noexcept {
    return pointDistance(*this, point) <= epsilon;
}

std::unique_ptr<Shape> Point::clone() const {
    return std::make_unique<Point>(*this);
}

void Point::accept(ShapeVisitor& visitor) const {
    visitor.visit(*this);
}

std::ostream& operator<<(std::ostream& output, const Point& point) {
    output << std::fixed << std::setprecision(3) << '(' << point.x_ << ", " << point.y_ << ')';
    return output;
}

Line::Line(Point anchor, Vector2D direction)
    : anchor_(std::move(anchor)), direction_(direction.normalized()) {}

Line::Line(const Point& first, const Point& second)
    : Line(first, second - first) {}

Aabb Line::bounds() const noexcept {
    const double infinity = std::numeric_limits<double>::infinity();
    return {-infinity, -infinity, infinity, infinity};
}

bool Line::contains(const Point& point, double epsilon) const noexcept {
    return std::abs(cross(direction_, point - anchor_)) <= epsilon;
}

std::unique_ptr<Shape> Line::clone() const {
    return std::make_unique<Line>(*this);
}

void Line::accept(ShapeVisitor& visitor) const {
    visitor.visit(*this);
}

Ray::Ray(Point origin, Vector2D direction)
    : origin_(std::move(origin)), direction_(direction.normalized()) {}

Ray::Ray(const Point& origin, const Point& through)
    : Ray(origin, through - origin) {}

Point Ray::pointAt(double parameter) const {
    if (parameter < -kEpsilon) {
        throw GeometryError("A ray cannot be evaluated at a negative parameter.");
    }
    return origin_ + direction_ * std::max(0.0, parameter);
}

Aabb Ray::bounds() const noexcept {
    const double infinity = std::numeric_limits<double>::infinity();
    return {
        direction_.x() < 0.0 ? -infinity : origin_.x(),
        direction_.y() < 0.0 ? -infinity : origin_.y(),
        direction_.x() > 0.0 ? infinity : origin_.x(),
        direction_.y() > 0.0 ? infinity : origin_.y()
    };
}

bool Ray::contains(const Point& point, double epsilon) const noexcept {
    const Vector2D relative = point - origin_;
    return std::abs(cross(direction_, relative)) <= epsilon &&
           dot(direction_, relative) >= -epsilon;
}

std::unique_ptr<Shape> Ray::clone() const {
    return std::make_unique<Ray>(*this);
}

void Ray::accept(ShapeVisitor& visitor) const {
    visitor.visit(*this);
}

Segment::Segment(Point start, Point end)
    : start_(std::move(start)), end_(std::move(end)) {}

double Segment::length() const noexcept {
    return pointDistance(start_, end_);
}

Point Segment::midpoint() const {
    return {0.5 * (start_.x() + end_.x()), 0.5 * (start_.y() + end_.y())};
}

Aabb Segment::bounds() const noexcept {
    return {
        std::min(start_.x(), end_.x()),
        std::min(start_.y(), end_.y()),
        std::max(start_.x(), end_.x()),
        std::max(start_.y(), end_.y())
    };
}

bool Segment::contains(const Point& point, double epsilon) const noexcept {
    const Vector2D segment = end_ - start_;
    const Vector2D relative = point - start_;
    if (segment.squaredLength() <= epsilon * epsilon) {
        return pointDistance(start_, point) <= epsilon;
    }
    if (std::abs(cross(segment, relative)) > epsilon * std::max(1.0, segment.length())) {
        return false;
    }
    return dot(relative, point - end_) <= epsilon;
}

std::unique_ptr<Shape> Segment::clone() const {
    return std::make_unique<Segment>(*this);
}

void Segment::accept(ShapeVisitor& visitor) const {
    visitor.visit(*this);
}

Circle::Circle(Point centre, double radius)
    : centre_(std::move(centre)), radius_(radius) {
    if (!isFinite(radius_) || radius_ < 0.0) {
        throw GeometryError("A circle radius must be finite and non-negative.");
    }
}

Aabb Circle::bounds() const noexcept {
    return {
        centre_.x() - radius_, centre_.y() - radius_,
        centre_.x() + radius_, centre_.y() + radius_
    };
}

bool Circle::contains(const Point& point, double epsilon) const noexcept {
    return pointDistance(centre_, point) <= radius_ + epsilon;
}

std::unique_ptr<Shape> Circle::clone() const {
    return std::make_unique<Circle>(*this);
}

void Circle::accept(ShapeVisitor& visitor) const {
    visitor.visit(*this);
}

Triangle::Triangle(Point first, Point second, Point third)
    : first_(std::move(first)), second_(std::move(second)), third_(std::move(third)) {}

Aabb Triangle::bounds() const noexcept {
    return {
        std::min({first_.x(), second_.x(), third_.x()}),
        std::min({first_.y(), second_.y(), third_.y()}),
        std::max({first_.x(), second_.x(), third_.x()}),
        std::max({first_.y(), second_.y(), third_.y()})
    };
}

double Triangle::signedArea() const noexcept {
    return 0.5 * cross(second_ - first_, third_ - first_);
}

double Triangle::perimeter() const noexcept {
    return pointDistance(first_, second_) + pointDistance(second_, third_) +
           pointDistance(third_, first_);
}

bool Triangle::contains(const Point& point, double epsilon) const noexcept {
    const double firstCross = cross(second_ - first_, point - first_);
    const double secondCross = cross(third_ - second_, point - second_);
    const double thirdCross = cross(first_ - third_, point - third_);
    const bool hasNegative = firstCross < -epsilon || secondCross < -epsilon || thirdCross < -epsilon;
    const bool hasPositive = firstCross > epsilon || secondCross > epsilon || thirdCross > epsilon;
    return !(hasNegative && hasPositive);
}

std::unique_ptr<Shape> Triangle::clone() const {
    return std::make_unique<Triangle>(*this);
}

void Triangle::accept(ShapeVisitor& visitor) const {
    visitor.visit(*this);
}

Polygon::Polygon(std::vector<Point> vertices)
    : vertices_(std::move(vertices)) {
    if (vertices_.size() < 3U) {
        throw GeometryError("A polygon requires at least three vertices.");
    }
}

Segment Polygon::edge(std::size_t index) const {
    if (index >= vertices_.size()) {
        throw std::out_of_range("Polygon edge index is out of range.");
    }
    return {vertices_[index], vertices_[(index + 1U) % vertices_.size()]};
}

Aabb Polygon::bounds() const noexcept {
    Aabb result{vertices_.front().x(), vertices_.front().y(),
                vertices_.front().x(), vertices_.front().y()};
    for (const Point& vertex : vertices_) {
        result.minX = std::min(result.minX, vertex.x());
        result.minY = std::min(result.minY, vertex.y());
        result.maxX = std::max(result.maxX, vertex.x());
        result.maxY = std::max(result.maxY, vertex.y());
    }
    return result;
}

double Polygon::signedArea() const noexcept {
    double doubledArea = 0.0;
    for (std::size_t index = 0; index < vertices_.size(); ++index) {
        const Point& current = vertices_[index];
        const Point& next = vertices_[(index + 1U) % vertices_.size()];
        doubledArea += current.x() * next.y() - current.y() * next.x();
    }
    return 0.5 * doubledArea;
}

double Polygon::perimeter() const noexcept {
    double result = 0.0;
    for (std::size_t index = 0; index < vertices_.size(); ++index) {
        result += pointDistance(vertices_[index], vertices_[(index + 1U) % vertices_.size()]);
    }
    return result;
}

bool Polygon::contains(const Point& point, double epsilon) const noexcept {
    bool inside = false;
    for (std::size_t index = 0, previous = vertices_.size() - 1U;
         index < vertices_.size(); previous = index++) {
        const Point& current = vertices_[index];
        const Point& before = vertices_[previous];

        Segment boundary(before, current);
        if (boundary.contains(point, epsilon)) {
            return true;
        }

        const bool crossesScanline = (current.y() > point.y()) != (before.y() > point.y());
        if (crossesScanline) {
            const double crossingX = (before.x() - current.x()) *
                                         (point.y() - current.y()) /
                                         (before.y() - current.y()) + current.x();
            if (point.x() < crossingX) {
                inside = !inside;
            }
        }
    }
    return inside;
}

std::unique_ptr<Shape> Polygon::clone() const {
    return std::make_unique<Polygon>(*this);
}

void Polygon::accept(ShapeVisitor& visitor) const {
    visitor.visit(*this);
}

HalfPlane::HalfPlane(Point point, Vector2D direction)
    : point_(std::move(point)), direction_(direction.normalized()) {}

HalfPlane::HalfPlane(const Point& from, const Point& to)
    : HalfPlane(from, to - from) {}

bool HalfPlane::contains(const Point& candidate, double epsilon) const noexcept {
    return cross(direction_, candidate - point_) >= -epsilon;
}

}  // namespace geoviz
