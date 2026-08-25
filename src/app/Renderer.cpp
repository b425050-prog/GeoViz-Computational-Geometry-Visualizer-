/**
 * @file Renderer.cpp
 * @brief Hardware-accelerated canvas rendering and coordinate conversion.
 */

#include "geoviz/app/Renderer.hpp"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace geoviz::app {

Vector2 CanvasTransform::worldToScreen(const Point& point) const noexcept {
    const float centreX = viewport_.x + 0.5F * viewport_.width;
    const float centreY = viewport_.y + 0.5F * viewport_.height;
    return {centreX + pan_.x + static_cast<float>(point.x()) * zoom_,
            centreY + pan_.y - static_cast<float>(point.y()) * zoom_};
}

Point CanvasTransform::screenToWorld(Vector2 screen) const {
    const float centreX = viewport_.x + 0.5F * viewport_.width;
    const float centreY = viewport_.y + 0.5F * viewport_.height;
    return {(screen.x - centreX - pan_.x) / zoom_,
            -(screen.y - centreY - pan_.y) / zoom_};
}

Aabb CanvasTransform::visibleBounds() const {
    const Point bottomLeft = screenToWorld({viewport_.x, viewport_.y + viewport_.height});
    const Point topRight = screenToWorld({viewport_.x + viewport_.width, viewport_.y});
    return {bottomLeft.x(), bottomLeft.y(), topRight.x(), topRight.y()};
}

void CanvasTransform::panBy(Vector2 screenDelta) noexcept {
    pan_.x += screenDelta.x;
    pan_.y += screenDelta.y;
}

void CanvasTransform::zoomAt(Vector2 screenPosition, float factor) noexcept {
    const Point anchoredWorld = screenToWorld(screenPosition);
    zoom_ = std::clamp(zoom_ * factor, 0.25F, 4.5F);
    const Vector2 movedScreen = worldToScreen(anchoredWorld);
    pan_.x += screenPosition.x - movedScreen.x;
    pan_.y += screenPosition.y - movedScreen.y;
}

void CanvasTransform::resetView() noexcept {
    pan_ = {0.0F, 0.0F};
    zoom_ = 1.0F;
}

void GeometryRenderer::drawGrid() const {
    const Rectangle viewport = transform_.viewport();
    BeginScissorMode(static_cast<int>(viewport.x), static_cast<int>(viewport.y),
                     static_cast<int>(viewport.width), static_cast<int>(viewport.height));

    DrawRectangleGradientV(static_cast<int>(viewport.x), static_cast<int>(viewport.y),
                           static_cast<int>(viewport.width), static_cast<int>(viewport.height),
                           theme_.backgroundRaised, theme_.background);

    const Aabb bounds = transform_.visibleBounds();
    double spacing = 50.0;
    while (spacing * transform_.zoom() < 38.0) spacing *= 2.0;
    while (spacing * transform_.zoom() > 92.0) spacing *= 0.5;

    const long long firstX = static_cast<long long>(std::floor(bounds.minX / spacing));
    const long long lastX = static_cast<long long>(std::ceil(bounds.maxX / spacing));
    const long long firstY = static_cast<long long>(std::floor(bounds.minY / spacing));
    const long long lastY = static_cast<long long>(std::ceil(bounds.maxY / spacing));
    for (long long index = firstX; index <= lastX; ++index) {
        const double worldX = static_cast<double>(index) * spacing;
        const Vector2 start = transform_.worldToScreen(Point{worldX, bounds.minY});
        const Vector2 end = transform_.worldToScreen(Point{worldX, bounds.maxY});
        const bool major = index % 5LL == 0LL;
        DrawLineEx(start, end, major ? 1.35F : 1.0F,
                   index == 0LL ? Fade(theme_.cyan, 0.38F)
                                : (major ? theme_.gridMajor : theme_.gridMinor));
    }
    for (long long index = firstY; index <= lastY; ++index) {
        const double worldY = static_cast<double>(index) * spacing;
        const Vector2 start = transform_.worldToScreen(Point{bounds.minX, worldY});
        const Vector2 end = transform_.worldToScreen(Point{bounds.maxX, worldY});
        const bool major = index % 5LL == 0LL;
        DrawLineEx(start, end, major ? 1.35F : 1.0F,
                   index == 0LL ? Fade(theme_.violet, 0.38F)
                                : (major ? theme_.gridMajor : theme_.gridMinor));
    }

    // A subtle radial glow visually anchors the editable canvas without a texture asset.
    const Vector2 centre{viewport.x + viewport.width * 0.5F,
                         viewport.y + viewport.height * 0.45F};
    DrawCircleGradient(static_cast<int>(centre.x), static_cast<int>(centre.y),
                       std::min(viewport.width, viewport.height) * 0.34F,
                       Fade(theme_.violet, 0.055F), BLANK);
    EndScissorMode();
}

void GeometryRenderer::drawPolygonFill(const std::vector<Point>& vertices, Color color) const {
    if (vertices.size() < 3U) return;
    std::vector<Vector2> screenVertices;
    screenVertices.reserve(vertices.size());
    for (const Point& vertex : vertices) screenVertices.push_back(transform_.worldToScreen(vertex));
    DrawTriangleFan(screenVertices.data(), static_cast<int>(screenVertices.size()), color);
}

void GeometryRenderer::drawSegment(const Segment& segment, float thickness, Color color) const {
    DrawLineEx(transform_.worldToScreen(segment.start()), transform_.worldToScreen(segment.end()),
               thickness, color);
}

void GeometryRenderer::drawPoint(const Point& point, float radius, Color fill, Color halo) const {
    const Vector2 screen = transform_.worldToScreen(point);
    DrawCircleV(screen, radius + 5.0F, Fade(halo, 0.16F));
    DrawCircleV(screen, radius + 2.0F, Fade(halo, 0.28F));
    DrawCircleV(screen, radius, fill);
    DrawCircleLines(static_cast<int>(screen.x), static_cast<int>(screen.y), radius, Fade(WHITE, 0.72F));
}

void GeometryRenderer::drawFrame(const VisualFrame& frame, Color accent) const {
    const Rectangle viewport = transform_.viewport();
    BeginScissorMode(static_cast<int>(viewport.x), static_cast<int>(viewport.y),
                     static_cast<int>(viewport.width), static_cast<int>(viewport.height));

    for (std::size_t index = 0U; index < frame.filledPolygons.size(); ++index) {
        const Color fill = theme_.accent(static_cast<int>(index) + 1);
        drawPolygonFill(frame.filledPolygons[index], Fade(fill, 0.13F));
    }
    for (const Triangle& triangle : frame.triangles) {
        const Vector2 first = transform_.worldToScreen(triangle.first());
        const Vector2 second = transform_.worldToScreen(triangle.second());
        const Vector2 third = transform_.worldToScreen(triangle.third());
        DrawTriangle(first, third, second, Fade(theme_.sky, 0.09F));
        DrawTriangleLines(first, second, third, Fade(theme_.sky, 0.55F));
    }
    if (frame.guideCircle.has_value()) {
        const Circle& circle = *frame.guideCircle;
        const Vector2 centre = transform_.worldToScreen(circle.centre());
        DrawCircleV(centre, static_cast<float>(circle.radius()) * transform_.zoom(),
                    Fade(theme_.violet, 0.055F));
        DrawCircleLines(static_cast<int>(centre.x), static_cast<int>(centre.y),
                        static_cast<float>(circle.radius()) * transform_.zoom(),
                        Fade(theme_.violet, 0.72F));
    }
    for (const Segment& edge : frame.rejectedEdges) drawSegment(edge, 3.0F, Fade(theme_.danger, 0.65F));
    for (const Segment& edge : frame.candidateEdges) drawSegment(edge, 2.0F, Fade(theme_.amber, 0.72F));
    for (const Segment& edge : frame.acceptedEdges) {
        drawSegment(edge, 6.0F, Fade(accent, 0.12F));
        drawSegment(edge, 2.4F, Fade(accent, 0.96F));
    }
    for (const Point& point : frame.sourcePoints) {
        drawPoint(point, 5.0F, theme_.text, accent);
    }
    for (const Point& point : frame.highlightedPoints) {
        drawPoint(point, 7.2F, accent, accent);
    }
    EndScissorMode();
}

void GeometryRenderer::drawShape(const Shape& shape) {
    shape.accept(*this);
}

void GeometryRenderer::visit(const Point& point) {
    drawPoint(point, 6.0F, visitorColor_, visitorColor_);
}

void GeometryRenderer::visit(const Line& line) {
    const Aabb bounds = transform_.visibleBounds();
    const double extension = 3.0 * std::max(bounds.width(), bounds.height());
    drawSegment(Segment(line.anchor() - line.direction() * extension,
                        line.anchor() + line.direction() * extension),
                2.0F, visitorColor_);
}

void GeometryRenderer::visit(const Ray& ray) {
    const Aabb bounds = transform_.visibleBounds();
    const double extension = 3.0 * std::max(bounds.width(), bounds.height());
    drawSegment(Segment(ray.origin(), ray.origin() + ray.direction() * extension),
                2.0F, visitorColor_);
}

void GeometryRenderer::visit(const Segment& segment) {
    drawSegment(segment, 2.5F, visitorColor_);
}

void GeometryRenderer::visit(const Circle& circle) {
    const Vector2 centre = transform_.worldToScreen(circle.centre());
    DrawCircleLines(static_cast<int>(centre.x), static_cast<int>(centre.y),
                    static_cast<float>(circle.radius()) * transform_.zoom(), visitorColor_);
}

void GeometryRenderer::visit(const Triangle& triangle) {
    const Vector2 first = transform_.worldToScreen(triangle.first());
    const Vector2 second = transform_.worldToScreen(triangle.second());
    const Vector2 third = transform_.worldToScreen(triangle.third());
    DrawTriangle(first, third, second, Fade(visitorColor_, 0.12F));
    DrawTriangleLines(first, second, third, visitorColor_);
}

void GeometryRenderer::visit(const Polygon& polygon) {
    drawPolygonFill(polygon.vertices(), Fade(visitorColor_, 0.12F));
    for (std::size_t index = 0U; index < polygon.size(); ++index) {
        drawSegment(polygon.edge(index), 2.0F, visitorColor_);
    }
}

}  // namespace geoviz::app

