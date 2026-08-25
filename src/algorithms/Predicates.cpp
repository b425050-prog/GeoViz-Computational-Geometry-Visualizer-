/**
 * @file Predicates.cpp
 * @brief Implementations of core computational-geometry building blocks.
 */

#include "geoviz/algorithms/Predicates.hpp"

#include <algorithm>
#include <cmath>

namespace geoviz::algorithms {

bool almostEqual(double first, double second, double epsilon) noexcept {
    const double scale = std::max({1.0, std::abs(first), std::abs(second)});
    return std::abs(first - second) <= epsilon * scale;
}

bool samePoint(const Point& first, const Point& second, double epsilon) noexcept {
    return squaredDistance(first, second) <= epsilon * epsilon;
}

bool lexicographicLess(const Point& first, const Point& second, double epsilon) noexcept {
    // std::sort requires a strict weak ordering.  Introducing a fuzzy epsilon
    // into a comparator can violate transitivity, so ordering is deliberately
    // exact; tolerance is applied later by duplicate-removal predicates.
    static_cast<void>(epsilon);
    if (first.x() != second.x()) return first.x() < second.x();
    return first.y() < second.y();
}

double dot(const Vector2D& first, const Vector2D& second) noexcept {
    return first.x() * second.x() + first.y() * second.y();
}

double cross(const Vector2D& first, const Vector2D& second) noexcept {
    return first.x() * second.y() - first.y() * second.x();
}

double cross(const Point& origin, const Point& first, const Point& second) noexcept {
    return cross(first - origin, second - origin);
}

Orientation orientation(const Point& first, const Point& second, const Point& third,
                        double epsilon) noexcept {
    const double determinant = cross(first, second, third);
    const double scale = std::max({1.0, (second - first).length(), (third - first).length()});
    if (std::abs(determinant) <= epsilon * scale * scale) {
        return Orientation::Collinear;
    }
    return determinant > 0.0 ? Orientation::CounterClockwise : Orientation::Clockwise;
}

double squaredDistance(const Point& first, const Point& second) noexcept {
    return (first - second).squaredLength();
}

double distance(const Point& first, const Point& second) noexcept {
    return std::sqrt(squaredDistance(first, second));
}

double signedArea(const std::vector<Point>& polygon) noexcept {
    if (polygon.size() < 3U) {
        return 0.0;
    }
    double twiceArea = 0.0;
    for (std::size_t index = 0; index < polygon.size(); ++index) {
        const Point& current = polygon[index];
        const Point& next = polygon[(index + 1U) % polygon.size()];
        twiceArea += current.x() * next.y() - current.y() * next.x();
    }
    return 0.5 * twiceArea;
}

Point centroid(const std::vector<Point>& polygon) {
    if (polygon.empty()) {
        throw GeometryError("Cannot calculate the centroid of an empty point set.");
    }

    const double area = signedArea(polygon);
    if (std::abs(area) <= kEpsilon) {
        double sumX = 0.0;
        double sumY = 0.0;
        for (const Point& point : polygon) {
            sumX += point.x();
            sumY += point.y();
        }
        return {sumX / static_cast<double>(polygon.size()),
                sumY / static_cast<double>(polygon.size())};
    }

    double weightedX = 0.0;
    double weightedY = 0.0;
    for (std::size_t index = 0; index < polygon.size(); ++index) {
        const Point& current = polygon[index];
        const Point& next = polygon[(index + 1U) % polygon.size()];
        const double weight = current.x() * next.y() - next.x() * current.y();
        weightedX += (current.x() + next.x()) * weight;
        weightedY += (current.y() + next.y()) * weight;
    }
    const double divisor = 6.0 * area;
    return {weightedX / divisor, weightedY / divisor};
}

double distanceToLine(const Point& point, const Line& line) noexcept {
    return std::abs(cross(line.direction(), point - line.anchor()));
}

double distanceToRay(const Point& point, const Ray& ray) noexcept {
    const Vector2D relative = point - ray.origin();
    if (dot(relative, ray.direction()) < 0.0) {
        return distance(point, ray.origin());
    }
    return std::abs(cross(ray.direction(), relative));
}

double distanceToSegment(const Point& point, const Segment& segment) noexcept {
    const Vector2D direction = segment.direction();
    const double denominator = direction.squaredLength();
    if (denominator <= kEpsilon * kEpsilon) {
        return distance(point, segment.start());
    }
    const double parameter = std::clamp(dot(point - segment.start(), direction) / denominator,
                                        0.0, 1.0);
    return distance(point, segment.start() + direction * parameter);
}

Point projectOntoLine(const Point& point, const Line& line) {
    const double parameter = dot(point - line.anchor(), line.direction());
    return line.pointAt(parameter);
}

Point projectOntoSegment(const Point& point, const Segment& segment) {
    const Vector2D direction = segment.direction();
    const double denominator = direction.squaredLength();
    if (denominator <= kEpsilon * kEpsilon) {
        return segment.start();
    }
    const double parameter = std::clamp(dot(point - segment.start(), direction) / denominator,
                                        0.0, 1.0);
    return segment.start() + direction * parameter;
}

PointLocation locatePointInPolygon(const Point& point, const std::vector<Point>& polygon,
                                   double epsilon) noexcept {
    if (polygon.size() < 3U) {
        return PointLocation::Outside;
    }

    bool inside = false;
    for (std::size_t index = 0, previous = polygon.size() - 1U;
         index < polygon.size(); previous = index++) {
        const Segment edge(polygon[previous], polygon[index]);
        if (edge.contains(point, epsilon)) {
            return PointLocation::Boundary;
        }
        const Point& first = polygon[index];
        const Point& second = polygon[previous];
        const bool straddles = (first.y() > point.y()) != (second.y() > point.y());
        if (straddles) {
            const double xAtY = (second.x() - first.x()) * (point.y() - first.y()) /
                                    (second.y() - first.y()) + first.x();
            if (point.x() < xAtY) {
                inside = !inside;
            }
        }
    }
    return inside ? PointLocation::Inside : PointLocation::Outside;
}

IntersectionResult intersect(const Line& first, const Line& second, double epsilon) {
    const double denominator = cross(first.direction(), second.direction());
    const Vector2D offset = second.anchor() - first.anchor();
    if (std::abs(denominator) <= epsilon) {
        return std::abs(cross(offset, first.direction())) <= epsilon
                   ? IntersectionResult{IntersectionKind::Overlap, {}}
                   : IntersectionResult{};
    }
    const double parameter = cross(offset, second.direction()) / denominator;
    return {IntersectionKind::Point, {first.pointAt(parameter)}};
}

IntersectionResult intersect(const Segment& first, const Segment& second, double epsilon) {
    const Vector2D r = first.direction();
    const Vector2D s = second.direction();
    const Vector2D offset = second.start() - first.start();
    const double denominator = cross(r, s);
    const double collinearity = cross(offset, r);

    if (std::abs(denominator) <= epsilon && std::abs(collinearity) <= epsilon) {
        const double rSquared = r.squaredLength();
        if (rSquared <= epsilon * epsilon) {
            return second.contains(first.start(), epsilon)
                       ? IntersectionResult{IntersectionKind::Point, {first.start()}}
                       : IntersectionResult{};
        }
        double firstParameter = dot(offset, r) / rSquared;
        double secondParameter = firstParameter + dot(s, r) / rSquared;
        if (firstParameter > secondParameter) {
            std::swap(firstParameter, secondParameter);
        }
        const double overlapStart = std::max(0.0, firstParameter);
        const double overlapEnd = std::min(1.0, secondParameter);
        if (overlapStart > overlapEnd + epsilon) {
            return {};
        }
        const Point startPoint = first.start() + r * overlapStart;
        if (almostEqual(overlapStart, overlapEnd, epsilon)) {
            return {IntersectionKind::Point, {startPoint}};
        }
        return {IntersectionKind::Overlap,
                {startPoint, first.start() + r * overlapEnd}};
    }
    if (std::abs(denominator) <= epsilon) {
        return {};
    }

    const double firstParameter = cross(offset, s) / denominator;
    const double secondParameter = cross(offset, r) / denominator;
    if (firstParameter >= -epsilon && firstParameter <= 1.0 + epsilon &&
        secondParameter >= -epsilon && secondParameter <= 1.0 + epsilon) {
        return {IntersectionKind::Point, {first.start() + r * firstParameter}};
    }
    return {};
}

IntersectionResult intersect(const Ray& ray, const Segment& segment, double epsilon) {
    const Vector2D r = ray.direction();
    const Vector2D s = segment.direction();
    const Vector2D offset = segment.start() - ray.origin();
    const double denominator = cross(r, s);
    if (std::abs(denominator) <= epsilon) {
        if (std::abs(cross(offset, r)) > epsilon) {
            return {};
        }
        std::vector<Point> overlapPoints;
        if (ray.contains(segment.start(), epsilon)) overlapPoints.push_back(segment.start());
        if (ray.contains(segment.end(), epsilon)) overlapPoints.push_back(segment.end());
        if (overlapPoints.empty()) return {};
        return {overlapPoints.size() == 1U ? IntersectionKind::Point : IntersectionKind::Overlap,
                std::move(overlapPoints)};
    }

    const double rayParameter = cross(offset, s) / denominator;
    const double segmentParameter = cross(offset, r) / denominator;
    if (rayParameter >= -epsilon && segmentParameter >= -epsilon &&
        segmentParameter <= 1.0 + epsilon) {
        return {IntersectionKind::Point, {ray.origin() + r * rayParameter}};
    }
    return {};
}

IntersectionResult intersect(const Circle& circle, const Line& line, double epsilon) {
    const Vector2D fromCentre = line.anchor() - circle.centre();
    const double projection = dot(fromCentre, line.direction());
    const double constant = fromCentre.squaredLength() - circle.radius() * circle.radius();
    const double discriminant = projection * projection - constant;
    if (discriminant < -epsilon) {
        return {};
    }
    if (std::abs(discriminant) <= epsilon) {
        return {IntersectionKind::Point, {line.pointAt(-projection)}};
    }
    const double root = std::sqrt(discriminant);
    return {IntersectionKind::TwoPoints,
            {line.pointAt(-projection - root), line.pointAt(-projection + root)}};
}

IntersectionResult intersect(const Circle& first, const Circle& second, double epsilon) {
    const double centreDistance = distance(first.centre(), second.centre());
    if (centreDistance <= epsilon && almostEqual(first.radius(), second.radius(), epsilon)) {
        return {IntersectionKind::Overlap, {}};
    }
    if (centreDistance > first.radius() + second.radius() + epsilon ||
        centreDistance < std::abs(first.radius() - second.radius()) - epsilon ||
        centreDistance <= epsilon) {
        return {};
    }

    const Vector2D direction = (second.centre() - first.centre()) / centreDistance;
    const double along = (first.radius() * first.radius() - second.radius() * second.radius() +
                          centreDistance * centreDistance) / (2.0 * centreDistance);
    const Point base = first.centre() + direction * along;
    const double heightSquared = first.radius() * first.radius() - along * along;
    if (heightSquared <= epsilon) {
        return {IntersectionKind::Point, {base}};
    }
    const Vector2D offset = direction.perpendicularLeft() * std::sqrt(heightSquared);
    return {IntersectionKind::TwoPoints, {base + offset, base - offset}};
}

std::optional<Point> circumcenter(const Triangle& triangle, double epsilon) {
    const Point& a = triangle.first();
    const Point& b = triangle.second();
    const Point& c = triangle.third();
    const double divisor = 2.0 * cross(a, b, c);
    if (std::abs(divisor) <= epsilon) {
        return std::nullopt;
    }
    const double aSquared = a.x() * a.x() + a.y() * a.y();
    const double bSquared = b.x() * b.x() + b.y() * b.y();
    const double cSquared = c.x() * c.x() + c.y() * c.y();
    const double x = (aSquared * (b.y() - c.y()) + bSquared * (c.y() - a.y()) +
                      cSquared * (a.y() - b.y())) / divisor;
    const double y = (aSquared * (c.x() - b.x()) + bSquared * (a.x() - c.x()) +
                      cSquared * (b.x() - a.x())) / divisor;
    return Point{x, y};
}

bool circumcircleContains(const Triangle& triangle, const Point& point, double epsilon) noexcept {
    const double ax = triangle.first().x() - point.x();
    const double ay = triangle.first().y() - point.y();
    const double bx = triangle.second().x() - point.x();
    const double by = triangle.second().y() - point.y();
    const double cx = triangle.third().x() - point.x();
    const double cy = triangle.third().y() - point.y();

    const double determinant = (ax * ax + ay * ay) * (bx * cy - cx * by) -
                               (bx * bx + by * by) * (ax * cy - cx * ay) +
                               (cx * cx + cy * cy) * (ax * by - bx * ay);
    const double sign = triangle.signedArea() >= 0.0 ? determinant : -determinant;
    return sign > epsilon;
}

Point rotate(const Point& point, const Point& pivot, double radians) {
    const double cosine = std::cos(radians);
    const double sine = std::sin(radians);
    const Vector2D relative = point - pivot;
    return pivot + Vector2D{relative.x() * cosine - relative.y() * sine,
                            relative.x() * sine + relative.y() * cosine};
}

Point reflectAcrossLine(const Point& point, const Line& mirror) {
    const Point projection = projectOntoLine(point, mirror);
    return point + (projection - point) * 2.0;
}

}  // namespace geoviz::algorithms
