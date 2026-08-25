#pragma once

/**
 * @file Renderer.hpp
 * @brief Canvas coordinate transform and ShapeVisitor-based raylib renderer.
 */

#include "geoviz/animation/AlgorithmTrace.hpp"
#include "geoviz/app/Theme.hpp"

#include <raylib.h>

namespace geoviz::app {

/** @brief Maps mathematical world coordinates to a pannable, zoomable viewport. */
class CanvasTransform final {
public:
    void setViewport(Rectangle viewport) noexcept { viewport_ = viewport; }
    [[nodiscard]] Rectangle viewport() const noexcept { return viewport_; }
    [[nodiscard]] Vector2 worldToScreen(const Point& point) const noexcept;
    [[nodiscard]] Point screenToWorld(Vector2 screen) const;
    [[nodiscard]] Aabb visibleBounds() const;
    void panBy(Vector2 screenDelta) noexcept;
    void zoomAt(Vector2 screenPosition, float factor) noexcept;
    void resetView() noexcept;
    [[nodiscard]] float zoom() const noexcept { return zoom_; }

private:
    Rectangle viewport_{0.0F, 0.0F, 1.0F, 1.0F};
    Vector2 pan_{0.0F, 0.0F};
    float zoom_{1.0F};
};

/**
 * @brief Renders every concrete Shape through double dispatch.
 *
 * Geometry classes know nothing about raylib.  Calling Shape::accept selects the
 * correct overload here, keeping presentation and domain responsibilities apart.
 */
class GeometryRenderer final : public ShapeVisitor {
public:
    GeometryRenderer(const CanvasTransform& transform, const Theme& theme)
        : transform_(transform), theme_(theme) {}

    void drawGrid() const;
    void drawFrame(const VisualFrame& frame, Color accent) const;
    void drawShape(const Shape& shape);

    void visit(const Point& point) override;
    void visit(const Line& line) override;
    void visit(const Ray& ray) override;
    void visit(const Segment& segment) override;
    void visit(const Circle& circle) override;
    void visit(const Triangle& triangle) override;
    void visit(const Polygon& polygon) override;

private:
    void drawSegment(const Segment& segment, float thickness, Color color) const;
    void drawPoint(const Point& point, float radius, Color fill, Color halo) const;
    void drawPolygonFill(const std::vector<Point>& vertices, Color color) const;

    const CanvasTransform& transform_;
    const Theme& theme_;
    Color visitorColor_{255, 255, 255, 255};
};

}  // namespace geoviz::app

