#pragma once

/**
 * @file Primitives.hpp
 * @brief Object-oriented definitions of GeoViz's fundamental 2-D geometry types.
 *
 * The geometry module deliberately has no dependency on the graphical user
 * interface.  This keeps every algorithm deterministic, testable, and reusable
 * in another C++ program.  The abstract Shape interface and ShapeVisitor show
 * runtime polymorphism and the Visitor design pattern without coupling the
 * mathematical model to raylib.
 */

#include <cmath>
#include <cstddef>
#include <iosfwd>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

namespace geoviz {

inline constexpr double kEpsilon = 1.0e-9;
inline constexpr double kPi = 3.141592653589793238462643383279502884;

class Point;
class Line;
class Ray;
class Segment;
class Circle;
class Triangle;
class Polygon;

/** @brief Describes the runtime type of a polymorphic shape. */
enum class ShapeKind {
    Point,
    Line,
    Ray,
    Segment,
    Circle,
    Triangle,
    Polygon
};

/** @brief Axis-aligned bounding box used by spatial and rendering code. */
struct Aabb final {
    double minX{0.0};
    double minY{0.0};
    double maxX{0.0};
    double maxY{0.0};

    [[nodiscard]] double width() const noexcept { return maxX - minX; }
    [[nodiscard]] double height() const noexcept { return maxY - minY; }
    [[nodiscard]] bool contains(double x, double y, double epsilon = kEpsilon) const noexcept {
        return x >= minX - epsilon && x <= maxX + epsilon &&
               y >= minY - epsilon && y <= maxY + epsilon;
    }
};

/**
 * @brief A checked exception raised when a geometric object is invalid.
 *
 * Keeping validation failures distinct from generic runtime errors makes error
 * handling in the application layer explicit and easy to explain in a viva.
 */
class GeometryError final : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

/**
 * @brief A two-dimensional displacement with overloaded vector arithmetic.
 *
 * The coordinates are encapsulated and are immutable after construction.  A
 * vector is a value object, so copying it is inexpensive and intentional.
 */
class Vector2D final {
public:
    constexpr Vector2D() noexcept = default;
    constexpr Vector2D(double x, double y) noexcept : x_(x), y_(y) {}

    [[nodiscard]] constexpr double x() const noexcept { return x_; }
    [[nodiscard]] constexpr double y() const noexcept { return y_; }
    [[nodiscard]] constexpr double squaredLength() const noexcept { return x_ * x_ + y_ * y_; }
    [[nodiscard]] double length() const noexcept { return std::sqrt(squaredLength()); }
    [[nodiscard]] Vector2D normalized(double epsilon = kEpsilon) const;
    [[nodiscard]] constexpr Vector2D perpendicularLeft() const noexcept { return {-y_, x_}; }
    [[nodiscard]] constexpr Vector2D perpendicularRight() const noexcept { return {y_, -x_}; }

    [[nodiscard]] constexpr Vector2D operator+() const noexcept { return *this; }
    [[nodiscard]] constexpr Vector2D operator-() const noexcept { return {-x_, -y_}; }
    [[nodiscard]] constexpr Vector2D operator+(const Vector2D& other) const noexcept {
        return {x_ + other.x_, y_ + other.y_};
    }
    [[nodiscard]] constexpr Vector2D operator-(const Vector2D& other) const noexcept {
        return {x_ - other.x_, y_ - other.y_};
    }
    [[nodiscard]] constexpr Vector2D operator*(double scalar) const noexcept {
        return {x_ * scalar, y_ * scalar};
    }
    [[nodiscard]] Vector2D operator/(double scalar) const;

private:
    double x_{0.0};
    double y_{0.0};
};

[[nodiscard]] constexpr Vector2D operator*(double scalar, const Vector2D& vector) noexcept {
    return vector * scalar;
}

/**
 * @brief Double-dispatch interface used by the renderer.
 *
 * Adding a new operation over every shape requires one visitor implementation,
 * while the core shape classes remain closed for modification.
 */
class ShapeVisitor {
public:
    virtual ~ShapeVisitor() = default;
    virtual void visit(const Point& point) = 0;
    virtual void visit(const Line& line) = 0;
    virtual void visit(const Ray& ray) = 0;
    virtual void visit(const Segment& segment) = 0;
    virtual void visit(const Circle& circle) = 0;
    virtual void visit(const Triangle& triangle) = 0;
    virtual void visit(const Polygon& polygon) = 0;
};

/** @brief Abstract base class shared by every drawable geometric shape. */
class Shape {
public:
    virtual ~Shape() = default;

    [[nodiscard]] virtual ShapeKind kind() const noexcept = 0;
    [[nodiscard]] virtual std::string_view typeName() const noexcept = 0;
    [[nodiscard]] virtual Aabb bounds() const noexcept = 0;
    [[nodiscard]] virtual double area() const noexcept = 0;
    [[nodiscard]] virtual double perimeter() const noexcept = 0;
    [[nodiscard]] virtual bool contains(const Point& point,
                                        double epsilon = kEpsilon) const noexcept = 0;
    [[nodiscard]] virtual std::unique_ptr<Shape> clone() const = 0;
    virtual void accept(ShapeVisitor& visitor) const = 0;
};

/** @brief A finite Cartesian point and the simplest concrete Shape. */
class Point final : public Shape {
public:
    Point() noexcept = default;
    Point(double x, double y);

    [[nodiscard]] double x() const noexcept { return x_; }
    [[nodiscard]] double y() const noexcept { return y_; }
    void set(double x, double y);

    [[nodiscard]] Point operator+(const Vector2D& displacement) const;
    [[nodiscard]] Point operator-(const Vector2D& displacement) const;
    [[nodiscard]] Vector2D operator-(const Point& other) const noexcept;

    [[nodiscard]] ShapeKind kind() const noexcept override { return ShapeKind::Point; }
    [[nodiscard]] std::string_view typeName() const noexcept override { return "Point"; }
    [[nodiscard]] Aabb bounds() const noexcept override { return {x_, y_, x_, y_}; }
    [[nodiscard]] double area() const noexcept override { return 0.0; }
    [[nodiscard]] double perimeter() const noexcept override { return 0.0; }
    [[nodiscard]] bool contains(const Point& point,
                                double epsilon = kEpsilon) const noexcept override;
    [[nodiscard]] std::unique_ptr<Shape> clone() const override;
    void accept(ShapeVisitor& visitor) const override;

    friend std::ostream& operator<<(std::ostream& output, const Point& point);

private:
    double x_{0.0};
    double y_{0.0};
};

/** @brief An infinite line represented by an anchor and a non-zero direction. */
class Line final : public Shape {
public:
    Line(Point anchor, Vector2D direction);
    Line(const Point& first, const Point& second);

    [[nodiscard]] const Point& anchor() const noexcept { return anchor_; }
    [[nodiscard]] const Vector2D& direction() const noexcept { return direction_; }
    [[nodiscard]] Point pointAt(double parameter) const { return anchor_ + direction_ * parameter; }

    [[nodiscard]] ShapeKind kind() const noexcept override { return ShapeKind::Line; }
    [[nodiscard]] std::string_view typeName() const noexcept override { return "Line"; }
    [[nodiscard]] Aabb bounds() const noexcept override;
    [[nodiscard]] double area() const noexcept override { return 0.0; }
    [[nodiscard]] double perimeter() const noexcept override { return 0.0; }
    [[nodiscard]] bool contains(const Point& point,
                                double epsilon = kEpsilon) const noexcept override;
    [[nodiscard]] std::unique_ptr<Shape> clone() const override;
    void accept(ShapeVisitor& visitor) const override;

private:
    Point anchor_;
    Vector2D direction_;
};

/** @brief A half-infinite line beginning at an origin. */
class Ray final : public Shape {
public:
    Ray(Point origin, Vector2D direction);
    Ray(const Point& origin, const Point& through);

    [[nodiscard]] const Point& origin() const noexcept { return origin_; }
    [[nodiscard]] const Vector2D& direction() const noexcept { return direction_; }
    [[nodiscard]] Point pointAt(double parameter) const;

    [[nodiscard]] ShapeKind kind() const noexcept override { return ShapeKind::Ray; }
    [[nodiscard]] std::string_view typeName() const noexcept override { return "Ray"; }
    [[nodiscard]] Aabb bounds() const noexcept override;
    [[nodiscard]] double area() const noexcept override { return 0.0; }
    [[nodiscard]] double perimeter() const noexcept override { return 0.0; }
    [[nodiscard]] bool contains(const Point& point,
                                double epsilon = kEpsilon) const noexcept override;
    [[nodiscard]] std::unique_ptr<Shape> clone() const override;
    void accept(ShapeVisitor& visitor) const override;

private:
    Point origin_;
    Vector2D direction_;
};

/** @brief A closed finite segment between two endpoints. */
class Segment final : public Shape {
public:
    Segment(Point start, Point end);

    [[nodiscard]] const Point& start() const noexcept { return start_; }
    [[nodiscard]] const Point& end() const noexcept { return end_; }
    [[nodiscard]] Vector2D direction() const noexcept { return end_ - start_; }
    [[nodiscard]] double length() const noexcept;
    [[nodiscard]] Point midpoint() const;

    [[nodiscard]] ShapeKind kind() const noexcept override { return ShapeKind::Segment; }
    [[nodiscard]] std::string_view typeName() const noexcept override { return "Segment"; }
    [[nodiscard]] Aabb bounds() const noexcept override;
    [[nodiscard]] double area() const noexcept override { return 0.0; }
    [[nodiscard]] double perimeter() const noexcept override { return 2.0 * length(); }
    [[nodiscard]] bool contains(const Point& point,
                                double epsilon = kEpsilon) const noexcept override;
    [[nodiscard]] std::unique_ptr<Shape> clone() const override;
    void accept(ShapeVisitor& visitor) const override;

private:
    Point start_;
    Point end_;
};

/** @brief A circle defined by a centre and a non-negative radius. */
class Circle final : public Shape {
public:
    Circle(Point centre, double radius);

    [[nodiscard]] const Point& centre() const noexcept { return centre_; }
    [[nodiscard]] double radius() const noexcept { return radius_; }

    [[nodiscard]] ShapeKind kind() const noexcept override { return ShapeKind::Circle; }
    [[nodiscard]] std::string_view typeName() const noexcept override { return "Circle"; }
    [[nodiscard]] Aabb bounds() const noexcept override;
    [[nodiscard]] double area() const noexcept override { return kPi * radius_ * radius_; }
    [[nodiscard]] double perimeter() const noexcept override { return 2.0 * kPi * radius_; }
    [[nodiscard]] bool contains(const Point& point,
                                double epsilon = kEpsilon) const noexcept override;
    [[nodiscard]] std::unique_ptr<Shape> clone() const override;
    void accept(ShapeVisitor& visitor) const override;

private:
    Point centre_;
    double radius_{0.0};
};

/** @brief A triangle whose vertices may be queried independently. */
class Triangle final : public Shape {
public:
    Triangle(Point first, Point second, Point third);

    [[nodiscard]] const Point& first() const noexcept { return first_; }
    [[nodiscard]] const Point& second() const noexcept { return second_; }
    [[nodiscard]] const Point& third() const noexcept { return third_; }
    [[nodiscard]] std::vector<Point> vertices() const { return {first_, second_, third_}; }

    [[nodiscard]] ShapeKind kind() const noexcept override { return ShapeKind::Triangle; }
    [[nodiscard]] std::string_view typeName() const noexcept override { return "Triangle"; }
    [[nodiscard]] Aabb bounds() const noexcept override;
    [[nodiscard]] double signedArea() const noexcept;
    [[nodiscard]] double area() const noexcept override { return std::abs(signedArea()); }
    [[nodiscard]] double perimeter() const noexcept override;
    [[nodiscard]] bool contains(const Point& point,
                                double epsilon = kEpsilon) const noexcept override;
    [[nodiscard]] std::unique_ptr<Shape> clone() const override;
    void accept(ShapeVisitor& visitor) const override;

private:
    Point first_;
    Point second_;
    Point third_;
};

/** @brief A simple polygon stored as an ordered sequence of at least three vertices. */
class Polygon final : public Shape {
public:
    explicit Polygon(std::vector<Point> vertices);

    [[nodiscard]] const std::vector<Point>& vertices() const noexcept { return vertices_; }
    [[nodiscard]] std::size_t size() const noexcept { return vertices_.size(); }
    [[nodiscard]] Segment edge(std::size_t index) const;

    [[nodiscard]] ShapeKind kind() const noexcept override { return ShapeKind::Polygon; }
    [[nodiscard]] std::string_view typeName() const noexcept override { return "Polygon"; }
    [[nodiscard]] Aabb bounds() const noexcept override;
    [[nodiscard]] double signedArea() const noexcept;
    [[nodiscard]] double area() const noexcept override { return std::abs(signedArea()); }
    [[nodiscard]] double perimeter() const noexcept override;
    [[nodiscard]] bool contains(const Point& point,
                                double epsilon = kEpsilon) const noexcept override;
    [[nodiscard]] std::unique_ptr<Shape> clone() const override;
    void accept(ShapeVisitor& visitor) const override;

private:
    std::vector<Point> vertices_;
};

/**
 * @brief An oriented line whose feasible side is the line's left half-plane.
 *
 * HalfPlane is intentionally a small value object rather than a Shape because
 * it represents a constraint used by an algorithm, not a directly owned scene
 * entity.
 */
class HalfPlane final {
public:
    HalfPlane(Point point, Vector2D direction);
    HalfPlane(const Point& from, const Point& to);

    [[nodiscard]] const Point& point() const noexcept { return point_; }
    [[nodiscard]] const Vector2D& direction() const noexcept { return direction_; }
    [[nodiscard]] double angle() const noexcept { return std::atan2(direction_.y(), direction_.x()); }
    [[nodiscard]] bool contains(const Point& candidate,
                                double epsilon = kEpsilon) const noexcept;

private:
    Point point_;
    Vector2D direction_;
};

}  // namespace geoviz

