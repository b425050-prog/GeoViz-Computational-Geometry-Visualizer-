#include "geometry.hpp"
#include "algorithms.hpp"
#include "trace.hpp"
#include <iomanip>
#include <sstream>
namespace gv {

// Replace point identities as one transaction; old edges cannot refer to the new points.
void Scene::setPoints(std::vector<Point> value) {
    if (value.size() > MAX_POINTS)
        throw std::length_error("Too many points.");
    points_ = std::move(value);
    edges_.clear();
    grid_ = false;
    source_ = 0;
    goal_ = points_.empty() ? 0 : static_cast<int>(points_.size() - 1);
    touch();
}
// An unfinished polygon is allowed while editing; execution and reload validate it.
void Scene::setPolygon(std::vector<Point> value) {
    if (value.size() > MAX_POINTS)
        throw std::length_error("Too many polygon vertices.");
    polygon_ = std::move(value);
    touch();
}
// Validate a candidate copy first so rejected topology cannot corrupt the current scene.
void Scene::setEdges(std::vector<Edge> value) {
    Scene candidate = *this;
    candidate.edges_ = std::move(value);
    Engine::validate(candidate);
    edges_ = std::move(candidate.edges_);
    touch();
}
// Enforce the common scene bound before changing the point collection.
void Scene::addPoint(Point point) {
    if (points_.size() >= MAX_POINTS)
        throw std::length_error("Too many points.");
    points_.push_back(std::move(point));
    touch();
}
// Moving a vertex invalidates the regular-grid assumption used by its heuristic.
void Scene::movePoint(std::size_t index, Point point) {
    points_.at(index) = std::move(point);
    grid_ = false;
    touch();
}
// Delete incident links, then remap every surviving endpoint above the removed index.
void Scene::removePoint(std::size_t index) {
    if (index >= points_.size())
        throw std::out_of_range("Unknown point.");
    const int id = static_cast<int>(index);
    points_.erase(points_.begin() + id);
    edges_.erase(
        std::remove_if(edges_.begin(), edges_.end(),
                       [id](const Edge &edge) { return edge.a() == id || edge.b() == id; }),
        edges_.end());
    for (auto &edge : edges_) {
        if (edge.a() > id)
            edge.setA(edge.a() - 1);
        if (edge.b() > id)
            edge.setB(edge.b() - 1);
    }
    source_ = 0;
    goal_ = points_.empty() ? 0 : static_cast<int>(points_.size() - 1);
    grid_ = false;
    touch();
}
// Append in boundary order; the UI may hold an incomplete polygon until Run.
void Scene::addPolygonVertex(Point point) {
    if (polygon_.size() >= MAX_POINTS)
        throw std::length_error("Too many polygon vertices.");
    polygon_.push_back(std::move(point));
    touch();
}
// Keep polygon identity stable while changing its geometric position.
void Scene::movePolygonVertex(std::size_t index, Point point) {
    polygon_.at(index) = std::move(point);
    touch();
}
// Reject invalid endpoints, duplicate undirected pairs and non-finite or negative costs.
void Scene::addEdge(Edge edge) {
    if (edges_.size() >= MAX_EDGES)
        throw std::length_error("Too many edges.");
    if (edge.a() < 0 || edge.b() < 0 || edge.a() == edge.b() ||
        edge.a() >= static_cast<int>(points_.size()) ||
        edge.b() >= static_cast<int>(points_.size()) || !std::isfinite(edge.weight()) ||
        edge.weight() < 0 || edge.weight() > 1e12)
        throw std::invalid_argument("Invalid edge endpoints or cost.");
    for (const auto &existing : edges_)
        if ((edge.a() == existing.a() && edge.b() == existing.b()) ||
            (edge.a() == existing.b() && edge.b() == existing.a()))
            throw std::invalid_argument("Duplicate undirected edge.");
    edges_.push_back(std::move(edge));
    touch();
}
// Validate before mutation so a failed edit preserves the previous usable weight.
void Scene::setEdgeWeight(std::size_t index, double weight) {
    if (!std::isfinite(weight) || weight < 0 || weight > 1e12)
        throw std::invalid_argument("Edge cost must be finite and non-negative.");
    edges_.at(index).setWeight(weight);
    touch();
}
// Blocked links remain in the scene for display but are excluded from route search.
void Scene::setEdgeBlocked(std::size_t index, bool blocked) {
    edges_.at(index).setBlocked(blocked);
    touch();
}

// Use fixed precision for display; file serialization separately uses 17 significant digits.
std::string number(double x, int precision) {
    if (!std::isfinite(x))
        return "INF";
    std::ostringstream out;
    if (x != 0 && std::abs(x) < std::pow(10.0, -precision))
        out << std::scientific;
    else
        out << std::fixed;
    out << std::setprecision(precision) << x;
    return out.str();
}
// Check scoped-enum values before indexing the display-name table.
std::string modeName(Mode m) {
    static const char *names[] = {"Convex hull",    "Geofence",         "Dijkstra",   "A*",
                                  "DBSCAN scan",    "Closest pair",     "KD nearest", "KD radius",
                                  "DBSCAN indexed", "Segment collision"};
    const int index = static_cast<int>(m);
    if (index < 0 || index >= 10)
        throw std::invalid_argument("Unknown algorithm mode.");
    return names[index];
}
// A manual step pauses the clock and never leaves the available frame interval.
void Playback::step(int direction, std::size_t count) {
    const auto target = direction < 0 ? (cursor_ ? cursor_ - 1 : 0) : cursor_ + 1;
    seek(target, count);
}
// Cap a delayed frame contribution to avoid skipping a long trace after a UI pause.
void Playback::update(double dt, std::size_t n) {
    if (!playing_ || !n)
        return;
    clock_ += std::min(dt, 0.25);
    while (clock_ >= seconds_ && cursor_ + 1 < n) {
        clock_ -= seconds_;
        ++cursor_;
    }
    if (cursor_ + 1 >= n) {
        playing_ = false;
        clock_ = 0;
    }
}
// Signed twice-area: positive means the third point lies left of the directed first edge.
double GeometryKernel::cross(Point a, Point b, Point c) {
    return (b.x() - a.x()) * (c.y() - a.y()) - (b.y() - a.y()) * (c.x() - a.x());
}
// Squared distance avoids unnecessary square roots in comparisons and radius queries.
double GeometryKernel::distance2(Point a, Point b) {
    double dx = a.x() - b.x(), dy = a.y() - b.y();
    return dx * dx + dy * dy;
}
// Convert squared distance to the world unit used by the scene.
double GeometryKernel::distance(Point a, Point b) {
    return std::sqrt(distance2(a, b));
}
int GeometryKernel::orientation(Point a, Point b, Point c) {
    double v = cross(a, b, c);
    // Scale-aware cross-product tolerance; sorting NEVER uses this tolerance.
    double scale = std::max(1.0, std::hypot(b.x() - a.x(), b.y() - a.y()) *
                                     std::hypot(c.x() - a.x(), c.y() - a.y()));
    return v > TOL * scale ? 1 : (v < -TOL * scale ? -1 : 0);
}
// Combine collinearity with padded endpoint bounds, including zero-length segments.
bool GeometryKernel::onSegment(Point p, Point a, Point b) {
    double pad = TOL * std::max(1.0, distance(a, b));
    return orientation(a, b, p) == 0 && p.x() >= std::min(a.x(), b.x()) - pad &&
           p.x() <= std::max(a.x(), b.x()) + pad && p.y() >= std::min(a.y(), b.y()) - pad &&
           p.y() <= std::max(a.y(), b.y()) + pad;
}
// Opposite orientations detect proper crossings; endpoint tests add touches and overlap.
bool GeometryKernel::intersects(Point a, Point b, Point c, Point d) {
    int x = orientation(a, b, c), y = orientation(a, b, d), z = orientation(c, d, a),
        w = orientation(c, d, b);
    return (x * y < 0 && z * w < 0) || (!x && onSegment(c, a, b)) || (!y && onSegment(d, a, b)) ||
           (!z && onSegment(a, c, d)) || (!w && onSegment(b, c, d));
}
// A half-open vertical test counts a polygon vertex once; boundary takes precedence.
int GeometryKernel::membership(Point q, const std::vector<Point> &p) {
    bool inside = false;
    for (std::size_t i = 0; i < p.size(); ++i) {
        Point a = p[i], b = p[(i + 1) % p.size()];
        if (onSegment(q, a, b))
            return 0;
        if ((a.y() > q.y()) != (b.y() > q.y())) {
            double x = a.x() + (q.y() - a.y()) * (b.x() - a.x()) / (b.y() - a.y());
            if (q.x() < x)
                inside = !inside;
        }
    }
    return inside ? 1 : -1;
}
// Reject zero area, repeated adjacent vertices, folded edges and self-intersections.
void GeometryKernel::validatePolygon(const std::vector<Point> &p) {
    if (p.size() < 3)
        throw std::runtime_error("A polygon needs at least 3 vertices.");
    long double area = 0, compensation = 0;
    const long double ox = p.front().x(), oy = p.front().y();
    for (std::size_t i = 0; i < p.size(); ++i) {
        if (distance2(p[i], p[(i + 1) % p.size()]) < TOL * TOL)
            throw std::runtime_error("Repeated adjacent polygon vertex.");
        // Adjacent edges may meet at their shared vertex, but must not fold back/overlap.
        Point a = p[i], b = p[(i + 1) % p.size()], c = p[(i + 2) % p.size()];
        if (orientation(a, b, c) == 0 &&
            ((a.x() - b.x()) * (c.x() - b.x()) + (a.y() - b.y()) * (c.y() - b.y())) > 0)
            throw std::runtime_error("Adjacent polygon edges overlap.");
        const long double term =
            (static_cast<long double>(a.x()) - ox) * (static_cast<long double>(b.y()) - oy) -
            (static_cast<long double>(a.y()) - oy) * (static_cast<long double>(b.x()) - ox);
        const long double corrected = term - compensation;
        const long double next = area + corrected;
        compensation = (next - area) - corrected;
        area = next;
        for (std::size_t j = i + 1; j < p.size(); ++j) {
            if (j == (i + 1) % p.size() || i == (j + 1) % p.size())
                continue;
            if (intersects(p[i], p[(i + 1) % p.size()], p[j], p[(j + 1) % p.size()]))
                throw std::runtime_error("Polygon self-intersects.");
        }
    }
    if (std::abs(area) < TOL)
        throw std::runtime_error("Polygon has zero area.");
}
} // namespace gv
