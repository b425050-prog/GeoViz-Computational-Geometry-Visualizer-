/**
 * @file AlgorithmDemo.cpp
 * @brief Concrete strategy adapters for every item in the GeoViz sidebar.
 */

#include "geoviz/app/AlgorithmDemo.hpp"

#include "geoviz/algorithms/ConvexHull.hpp"
#include "geoviz/algorithms/Delaunay.hpp"
#include "geoviz/algorithms/HalfPlaneIntersection.hpp"
#include "geoviz/algorithms/PolygonAlgorithms.hpp"
#include "geoviz/algorithms/Predicates.hpp"
#include "geoviz/algorithms/Voronoi.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace geoviz::app {
namespace {

using algorithms::HullAlgorithm;

[[nodiscard]] AlgorithmTrace messageTrace(const DemoMetadata& metadata,
                                          const std::vector<Point>& points,
                                          std::string headline,
                                          std::string explanation) {
    AlgorithmTrace trace{metadata.title, metadata.complexity};
    VisualFrame frame;
    frame.headline = std::move(headline);
    frame.explanation = std::move(explanation);
    frame.sourcePoints = points;
    trace.addFrame(std::move(frame));
    return trace;
}

class PrimitiveDemo final : public AlgorithmDemo {
public:
    PrimitiveDemo() : metadata_{
        DemoId::PrimitiveLab, "Primitive Geometry Lab", "Point / Line / Ray",
        "FOUNDATIONS", "Explore points, vectors, segments, infinite lines, rays, circles, triangles, and polygons in one layered construction.",
        "O(1) per primitive", "Click to add points. Drag a point to see every dependent primitive update live.",
        InputStyle::PointSet, 1U, 0} {}

    [[nodiscard]] const DemoMetadata& metadata() const noexcept override { return metadata_; }

    [[nodiscard]] AlgorithmTrace buildTrace(const SceneModel& scene,
                                            const Aabb& viewBounds) const override {
        AlgorithmTrace trace{metadata_.title, metadata_.complexity};
        const std::vector<Point>& points = scene.points();
        VisualFrame pointFrame;
        pointFrame.headline = "Point objects";
        pointFrame.explanation = "A point stores a finite (x, y) position and participates in overloaded vector arithmetic.";
        pointFrame.sourcePoints = points;
        pointFrame.highlightedPoints = points;
        trace.addFrame(std::move(pointFrame));
        if (points.size() < 2U) return trace;

        const Segment segment(points[0], points[1]);
        const Vector2D direction = segment.direction().normalized();
        const double length = 2.0 * std::max(viewBounds.width(), viewBounds.height());
        VisualFrame linearFrame;
        linearFrame.headline = "Segment, line, and ray";
        linearFrame.explanation = "A segment is bounded; a line extends both ways; a ray begins at one endpoint and extends forward.";
        linearFrame.sourcePoints = points;
        linearFrame.acceptedEdges = {segment};
        linearFrame.candidateEdges.emplace_back(points[0] - direction * length,
                                                points[0] + direction * length);
        linearFrame.rejectedEdges.emplace_back(points[0], points[0] - direction * length);
        linearFrame.highlightedPoints = {segment.midpoint()};
        trace.addFrame(std::move(linearFrame));

        VisualFrame circleFrame;
        circleFrame.headline = "Distance and circle";
        circleFrame.explanation = "The endpoint distance becomes a radius; containment compares squared Euclidean distance.";
        circleFrame.sourcePoints = points;
        circleFrame.guideCircle.emplace(points[0], segment.length());
        circleFrame.acceptedEdges = {segment};
        trace.addFrame(std::move(circleFrame));
        if (points.size() < 3U) return trace;

        const Triangle triangle(points[0], points[1], points[2]);
        VisualFrame orientationFrame;
        orientationFrame.headline = "Orientation and triangle";
        const algorithms::Orientation turn = algorithms::orientation(points[0], points[1], points[2]);
        orientationFrame.explanation = turn == algorithms::Orientation::CounterClockwise
            ? "The signed cross product is positive: this is a counter-clockwise turn."
            : (turn == algorithms::Orientation::Clockwise
                   ? "The signed cross product is negative: this is a clockwise turn."
                   : "The cross product is approximately zero: the three points are collinear.");
        orientationFrame.sourcePoints = points;
        orientationFrame.triangles = {triangle};
        orientationFrame.highlightedPoints = triangle.vertices();
        if (const std::optional<Point> centre = algorithms::circumcenter(triangle); centre.has_value()) {
            orientationFrame.guideCircle.emplace(*centre, algorithms::distance(*centre, points[0]));
        }
        trace.addFrame(std::move(orientationFrame));

        algorithms::MonotonicChainHull hullStrategy;
        const auto hull = hullStrategy.compute(points).hull;
        if (hull.size() >= 3U) {
            VisualFrame polygonFrame;
            polygonFrame.headline = "Polygon abstraction";
            polygonFrame.explanation = "An ordered boundary provides area, perimeter, bounds, centroid, and point containment polymorphically.";
            polygonFrame.sourcePoints = points;
            polygonFrame.acceptedEdges = closedEdges(hull);
            polygonFrame.filledPolygons = {hull};
            polygonFrame.highlightedPoints = {algorithms::centroid(hull)};
            trace.addFrame(std::move(polygonFrame));
        }
        return trace;
    }

private:
    DemoMetadata metadata_;
};

class HullDemo final : public AlgorithmDemo {
public:
    HullDemo(DemoMetadata metadata, HullAlgorithm algorithm)
        : metadata_(std::move(metadata)), algorithm_(algorithm) {}
    [[nodiscard]] const DemoMetadata& metadata() const noexcept override { return metadata_; }
    [[nodiscard]] AlgorithmTrace buildTrace(const SceneModel& scene,
                                            const Aabb&) const override {
        return algorithms::makeHullStrategy(algorithm_)->compute(scene.points()).trace;
    }

private:
    DemoMetadata metadata_;
    HullAlgorithm algorithm_;
};

class DelaunayDemo final : public AlgorithmDemo {
public:
    DelaunayDemo() : metadata_{
        DemoId::Delaunay, "Delaunay Triangulation", "Delaunay",
        "TESSELLATION", "Incremental Bowyer-Watson triangulation satisfying the empty-circumcircle property.",
        "Expected O(n log n)", "Add non-collinear sites; avoid exact duplicates.",
        InputStyle::PointSet, 3U, 2} {}
    [[nodiscard]] const DemoMetadata& metadata() const noexcept override { return metadata_; }
    [[nodiscard]] AlgorithmTrace buildTrace(const SceneModel& scene, const Aabb&) const override {
        return algorithms::DelaunayTriangulator{}.compute(scene.points()).trace;
    }
private:
    DemoMetadata metadata_;
};

class VoronoiDemo final : public AlgorithmDemo {
public:
    VoronoiDemo() : metadata_{
        DemoId::Voronoi, "Voronoi Diagram", "Voronoi",
        "TESSELLATION", "Partitions the canvas into cells whose points are nearest to the same site, with the Delaunay dual overlaid.",
        "O(n^3) clipped-cell visualizer", "Add or drag sites; unbounded cells are clipped to the visible world.",
        InputStyle::PointSet, 2U, 3} {}
    [[nodiscard]] const DemoMetadata& metadata() const noexcept override { return metadata_; }
    [[nodiscard]] AlgorithmTrace buildTrace(const SceneModel& scene, const Aabb& bounds) const override {
        return algorithms::VoronoiBuilder{}.compute(scene.points(), bounds).trace;
    }
private:
    DemoMetadata metadata_;
};

[[nodiscard]] std::vector<HalfPlane> halfPlanesFromPairs(const std::vector<Point>& points) {
    std::vector<HalfPlane> halfPlanes;
    for (std::size_t index = 1U; index < points.size(); index += 2U) {
        if (!algorithms::samePoint(points[index - 1U], points[index])) {
            halfPlanes.emplace_back(points[index - 1U], points[index]);
        }
    }
    return halfPlanes;
}

class HalfPlaneDemo final : public AlgorithmDemo {
public:
    HalfPlaneDemo() : metadata_{
        DemoId::HalfPlaneIntersection, "Half-Plane Intersection", "Half-Plane Intersection",
        "CLIPPING", "Intersects oriented linear constraints into one convex feasible region.",
        "O(kv) visual clipping", "Every consecutive point pair defines an arrow; its LEFT side is feasible.",
        InputStyle::PointPairs, 4U, 4} {}
    [[nodiscard]] const DemoMetadata& metadata() const noexcept override { return metadata_; }
    [[nodiscard]] AlgorithmTrace buildTrace(const SceneModel& scene, const Aabb& bounds) const override {
        const std::vector<HalfPlane> derived = scene.halfPlanes().empty()
            ? halfPlanesFromPairs(scene.points()) : scene.halfPlanes();
        return algorithms::HalfPlaneIntersector{}.compute(derived, bounds).trace;
    }
private:
    DemoMetadata metadata_;
};

class EarClippingDemo final : public AlgorithmDemo {
public:
    EarClippingDemo() : metadata_{
        DemoId::EarClipping, "Polygon Ear Clipping", "Ear Clipping",
        "POLYGONS", "Triangulates a simple ordered polygon by repeatedly removing empty convex ears.",
        "O(n^2)", "Click vertices around the boundary in clockwise or counter-clockwise order.",
        InputStyle::OrderedPolygon, 3U, 5} {}
    [[nodiscard]] const DemoMetadata& metadata() const noexcept override { return metadata_; }
    [[nodiscard]] AlgorithmTrace buildTrace(const SceneModel& scene, const Aabb&) const override {
        return algorithms::EarClippingTriangulator{}.compute(scene.points()).trace;
    }
private:
    DemoMetadata metadata_;
};

class ClosestPairDemo final : public AlgorithmDemo {
public:
    ClosestPairDemo() : metadata_{
        DemoId::ClosestPair, "Closest Pair of Points", "Closest Pair",
        "PROXIMITY", "Divide-and-conquer search with a narrow merge strip around each recursive split.",
        "O(n log^2 n)", "Add any point cloud; the nearest two sites are highlighted.",
        InputStyle::PointSet, 2U, 1} {}
    [[nodiscard]] const DemoMetadata& metadata() const noexcept override { return metadata_; }
    [[nodiscard]] AlgorithmTrace buildTrace(const SceneModel& scene, const Aabb&) const override {
        return algorithms::ClosestPairSolver{}.compute(scene.points()).trace;
    }
private:
    DemoMetadata metadata_;
};

class SegmentDemo final : public AlgorithmDemo {
public:
    SegmentDemo() : metadata_{
        DemoId::SegmentIntersections, "Segment Intersections", "Segment Intersections",
        "INTERSECTIONS", "Uses robust orientation predicates to classify proper, endpoint, and collinear-overlap intersections.",
        "O(m^2)", "Every consecutive point pair forms one independent segment.",
        InputStyle::PointPairs, 4U, 6} {}
    [[nodiscard]] const DemoMetadata& metadata() const noexcept override { return metadata_; }
    [[nodiscard]] AlgorithmTrace buildTrace(const SceneModel& scene, const Aabb&) const override {
        std::vector<Segment> segments;
        for (std::size_t index = 1U; index < scene.points().size(); index += 2U) {
            segments.emplace_back(scene.points()[index - 1U], scene.points()[index]);
        }
        return algorithms::SegmentIntersectionSolver{}.compute(segments).trace;
    }
private:
    DemoMetadata metadata_;
};

class PointInPolygonDemo final : public AlgorithmDemo {
public:
    PointInPolygonDemo() : metadata_{
        DemoId::PointInPolygon, "Point in Polygon", "Point in Polygon",
        "POLYGONS", "Classifies a query as inside, outside, or on the boundary using ray crossing.",
        "O(n)", "The LAST point is the query; all earlier points form the ordered polygon.",
        InputStyle::PolygonWithQuery, 4U, 0} {}
    [[nodiscard]] const DemoMetadata& metadata() const noexcept override { return metadata_; }
    [[nodiscard]] AlgorithmTrace buildTrace(const SceneModel& scene, const Aabb& bounds) const override {
        if (scene.points().size() < 4U) {
            return messageTrace(metadata_, scene.points(), "Polygon plus query required", metadata_.inputHint);
        }
        std::vector<Point> polygon(scene.points().begin(), scene.points().end() - 1);
        const Point query = scene.points().back();
        const algorithms::PointLocation location = algorithms::locatePointInPolygon(query, polygon);
        AlgorithmTrace trace{metadata_.title, metadata_.complexity};
        VisualFrame frame;
        frame.headline = location == algorithms::PointLocation::Inside ? "Query is inside"
                         : (location == algorithms::PointLocation::Boundary ? "Query is on the boundary"
                                                                           : "Query is outside");
        frame.explanation = "A horizontal ray toggles parity at every valid boundary crossing.";
        frame.sourcePoints = scene.points();
        frame.highlightedPoints = {query};
        frame.acceptedEdges = closedEdges(polygon);
        frame.filledPolygons = {polygon};
        frame.candidateEdges.emplace_back(query, Point{bounds.maxX, query.y()});
        trace.addFrame(std::move(frame));
        return trace;
    }
private:
    DemoMetadata metadata_;
};

class EnclosingCircleDemo final : public AlgorithmDemo {
public:
    EnclosingCircleDemo() : metadata_{
        DemoId::SmallestEnclosingCircle, "Smallest Enclosing Circle", "Enclosing Circle",
        "PROXIMITY", "Finds the minimum-radius disk containing the entire point set.",
        "O(n^3) deterministic incremental", "Add sites; two or three boundary sites define the final circle.",
        InputStyle::PointSet, 1U, 3} {}
    [[nodiscard]] const DemoMetadata& metadata() const noexcept override { return metadata_; }
    [[nodiscard]] AlgorithmTrace buildTrace(const SceneModel& scene, const Aabb&) const override {
        AlgorithmTrace trace{metadata_.title, metadata_.complexity};
        VisualFrame frame;
        frame.headline = "Minimum enclosing disk";
        frame.explanation = "Whenever a site lies outside, it becomes a required boundary point of a new disk.";
        frame.sourcePoints = scene.points();
        frame.highlightedPoints = scene.points();
        frame.guideCircle = algorithms::smallestEnclosingCircle(scene.points());
        trace.addFrame(std::move(frame));
        return trace;
    }
private:
    DemoMetadata metadata_;
};

class DiameterDemo final : public AlgorithmDemo {
public:
    DiameterDemo() : metadata_{
        DemoId::RotatingCalipers, "Rotating Calipers Diameter", "Rotating Calipers",
        "CONVEX GEOMETRY", "Walks antipodal hull vertices to find the farthest pair without checking every pair.",
        "O(n log n) including hull", "Add a point cloud; GeoViz first extracts its convex hull.",
        InputStyle::PointSet, 2U, 2} {}
    [[nodiscard]] const DemoMetadata& metadata() const noexcept override { return metadata_; }
    [[nodiscard]] AlgorithmTrace buildTrace(const SceneModel& scene, const Aabb&) const override {
        algorithms::MonotonicChainHull hullBuilder;
        const algorithms::HullResult hull = hullBuilder.compute(scene.points());
        AlgorithmTrace trace{metadata_.title, metadata_.complexity};
        VisualFrame frame;
        frame.headline = "Antipodal diameter";
        frame.explanation = "Parallel support lines rotate around the convex hull; only antipodal pairs can be farthest.";
        frame.sourcePoints = scene.points();
        frame.acceptedEdges = closedEdges(hull.hull);
        frame.filledPolygons = {hull.hull};
        if (const auto diameter = algorithms::convexDiameter(hull.hull); diameter.has_value()) {
            frame.candidateEdges = {*diameter};
            frame.highlightedPoints = {diameter->start(), diameter->end()};
        }
        trace.addFrame(std::move(frame));
        return trace;
    }
private:
    DemoMetadata metadata_;
};

[[nodiscard]] DemoMetadata hullMetadata(DemoId id, std::string title,
                                        std::string sidebar, std::string complexity,
                                        std::string description, int accent) {
    return {id, std::move(title), std::move(sidebar), "CONVEX HULLS", std::move(description),
            std::move(complexity), "Click anywhere to add sites; right-click a site to remove it.",
            InputStyle::PointSet, 3U, accent};
}

}  // namespace

DemoRegistry::DemoRegistry() {
    demos_.push_back(std::make_unique<PrimitiveDemo>());
    demos_.push_back(std::make_unique<HullDemo>(
        hullMetadata(DemoId::HullMonotonic, "Convex Hull: Monotonic Chain", "Monotonic Chain",
                     "O(n log n)", "Two lexicographic scans construct lower and upper hull chains.", 0),
        HullAlgorithm::MonotonicChain));
    demos_.push_back(std::make_unique<HullDemo>(
        hullMetadata(DemoId::HullGraham, "Convex Hull: Graham Scan", "Graham Scan",
                     "O(n log n)", "Polar-angle sorting followed by a stack of counter-clockwise turns.", 1),
        HullAlgorithm::GrahamScan));
    demos_.push_back(std::make_unique<HullDemo>(
        hullMetadata(DemoId::HullJarvis, "Convex Hull: Jarvis March", "Jarvis March",
                     "O(nh)", "Gift wrapping selects one guaranteed extreme edge per hull vertex.", 5),
        HullAlgorithm::JarvisMarch));
    demos_.push_back(std::make_unique<HullDemo>(
        hullMetadata(DemoId::HullQuick, "Convex Hull: QuickHull", "QuickHull",
                     "Average O(n log n)", "Farthest-point partitioning recursively discards interior triangles.", 4),
        HullAlgorithm::QuickHull));
    demos_.push_back(std::make_unique<DelaunayDemo>());
    demos_.push_back(std::make_unique<VoronoiDemo>());
    demos_.push_back(std::make_unique<HalfPlaneDemo>());
    demos_.push_back(std::make_unique<EarClippingDemo>());
    demos_.push_back(std::make_unique<ClosestPairDemo>());
    demos_.push_back(std::make_unique<SegmentDemo>());
    demos_.push_back(std::make_unique<PointInPolygonDemo>());
    demos_.push_back(std::make_unique<EnclosingCircleDemo>());
    demos_.push_back(std::make_unique<DiameterDemo>());
}

const AlgorithmDemo& DemoRegistry::at(std::size_t index) const {
    if (index >= demos_.size()) throw std::out_of_range("Algorithm demo index is out of range.");
    return *demos_[index];
}

std::size_t DemoRegistry::indexOf(DemoId id) const {
    for (std::size_t index = 0U; index < demos_.size(); ++index) {
        if (demos_[index]->metadata().id == id) return index;
    }
    throw std::out_of_range("Requested algorithm demo is not registered.");
}

void TracePlayer::reset(std::size_t frameCount) noexcept {
    frameIndex_ = 0U;
    accumulator_ = 0.0F;
    playing_ = frameCount > 1U;
}

void TracePlayer::update(float deltaSeconds, std::size_t frameCount) noexcept {
    if (!playing_ || frameCount <= 1U) return;
    accumulator_ += deltaSeconds;
    const float interval = 1.0F / speed_.get();
    while (accumulator_ >= interval) {
        accumulator_ -= interval;
        if (frameIndex_ + 1U < frameCount) {
            ++frameIndex_;
        } else {
            playing_ = false;
            break;
        }
    }
}

void TracePlayer::last(std::size_t frameCount) noexcept {
    frameIndex_ = frameCount == 0U ? 0U : frameCount - 1U;
    accumulator_ = 0.0F;
}

void TracePlayer::next(std::size_t frameCount) noexcept {
    if (frameIndex_ + 1U < frameCount) ++frameIndex_;
    accumulator_ = 0.0F;
}

void TracePlayer::previous() noexcept {
    if (frameIndex_ > 0U) --frameIndex_;
    accumulator_ = 0.0F;
}

void TracePlayer::setSpeed(float framesPerSecond) noexcept {
    speed_.set(framesPerSecond);
}

}  // namespace geoviz::app
