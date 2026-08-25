/**
 * @file test_main.cpp
 * @brief Dependency-free regression tests for GeoViz's mathematical core.
 *
 * A tiny custom harness keeps repository setup simple while still giving CI a
 * non-zero exit status, named test cases, and precise failure messages.
 */

#include "geoviz/algorithms/ConvexHull.hpp"
#include "geoviz/algorithms/Delaunay.hpp"
#include "geoviz/algorithms/HalfPlaneIntersection.hpp"
#include "geoviz/algorithms/PolygonAlgorithms.hpp"
#include "geoviz/algorithms/Predicates.hpp"
#include "geoviz/algorithms/Voronoi.hpp"
#include "geoviz/app/AlgorithmDemo.hpp"
#include "geoviz/app/Commands.hpp"
#include "geoviz/io/SceneSerializer.hpp"

#include <cmath>
#include <filesystem>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

using namespace geoviz;
using namespace geoviz::algorithms;

class TestSuite final {
public:
    void run(std::string_view name, const std::function<void()>& test) {
        try {
            test();
            ++passed_;
            std::cout << "[PASS] " << name << '\n';
        } catch (const std::exception& error) {
            ++failed_;
            std::cerr << "[FAIL] " << name << " -> " << error.what() << '\n';
        }
    }

    void expect(bool condition, std::string_view message) const {
        if (!condition) throw std::runtime_error(std::string(message));
    }

    void expectNear(double actual, double expected, double tolerance,
                    std::string_view message) const {
        if (std::abs(actual - expected) > tolerance) {
            throw std::runtime_error(std::string(message) + ": expected " +
                                     std::to_string(expected) + ", got " +
                                     std::to_string(actual));
        }
    }

    [[nodiscard]] int finish() const {
        std::cout << "\n" << passed_ << " passed, " << failed_ << " failed.\n";
        return failed_ == 0 ? 0 : 1;
    }

private:
    int passed_{0};
    int failed_{0};
};

[[nodiscard]] std::vector<Point> squareWithCentre() {
    return {{-1.0, -1.0}, {1.0, -1.0}, {1.0, 1.0}, {-1.0, 1.0}, {0.0, 0.0}};
}

}  // namespace

int main() {
    TestSuite suite;

    suite.run("Vector arithmetic and shape polymorphism", [&] {
        const Vector2D vector{3.0, 4.0};
        suite.expectNear(vector.length(), 5.0, 1.0e-12, "Vector length");
        suite.expectNear(vector.normalized().length(), 1.0, 1.0e-12, "Normalized length");

        std::vector<std::unique_ptr<Shape>> shapes;
        shapes.push_back(std::make_unique<Point>(1.0, 2.0));
        shapes.push_back(std::make_unique<Segment>(Point{0.0, 0.0}, Point{3.0, 4.0}));
        shapes.push_back(std::make_unique<Circle>(Point{0.0, 0.0}, 2.0));
        shapes.push_back(std::make_unique<Triangle>(Point{0.0, 0.0}, Point{2.0, 0.0}, Point{0.0, 2.0}));
        suite.expect(shapes[0]->kind() == ShapeKind::Point, "Virtual kind dispatch");
        suite.expectNear(shapes[1]->perimeter(), 10.0, 1.0e-12, "Segment perimeter convention");
        suite.expectNear(shapes[2]->area(), 4.0 * kPi, 1.0e-10, "Circle area");
        suite.expectNear(shapes[3]->area(), 2.0, 1.0e-12, "Triangle area");
        suite.expect(shapes[2]->clone()->contains(Point{1.0, 1.0}), "Polymorphic clone");
    });

    suite.run("Predicates, projections, and intersections", [&] {
        suite.expect(orientation({0.0, 0.0}, {1.0, 0.0}, {0.0, 1.0}) ==
                         Orientation::CounterClockwise,
                     "Counter-clockwise orientation");
        const Segment first({0.0, 0.0}, {4.0, 4.0});
        const Segment second({0.0, 4.0}, {4.0, 0.0});
        const IntersectionResult crossing = intersect(first, second);
        suite.expect(crossing.kind == IntersectionKind::Point, "Proper crossing kind");
        suite.expect(samePoint(crossing.points.front(), Point{2.0, 2.0}), "Crossing coordinate");
        suite.expectNear(distanceToSegment({2.0, 0.0}, {{0.0, 0.0}, {4.0, 0.0}}),
                         0.0, 1.0e-12, "Point on segment distance");
        suite.expect(!Segment{{1.0, 1.0}, {1.0, 1.0}}.contains(Point{1.0 + 1.0e-5, 1.0}),
                     "Degenerate segment uses linear, not squared, tolerance");
        const auto circleCrossing = intersect(Circle{{0.0, 0.0}, 2.0},
                                              Line{Point{-3.0, 0.0}, Point{3.0, 0.0}});
        suite.expect(circleCrossing.kind == IntersectionKind::TwoPoints,
                     "Line crosses circle twice");
    });

    suite.run("All four convex hull strategies", [&] {
        const std::vector<Point> points = squareWithCentre();
        const std::vector<HullAlgorithm> algorithms{HullAlgorithm::MonotonicChain,
                                                     HullAlgorithm::GrahamScan,
                                                     HullAlgorithm::JarvisMarch,
                                                     HullAlgorithm::QuickHull};
        for (const HullAlgorithm algorithm : algorithms) {
            const HullResult result = makeHullStrategy(algorithm)->compute(points);
            suite.expect(result.hull.size() == 4U, "Square hull has four vertices");
            suite.expectNear(std::abs(signedArea(result.hull)), 4.0, 1.0e-9,
                             "Square hull area");
            suite.expect(!result.trace.empty(), "Hull animation trace exists");
        }
    });

    suite.run("Bowyer-Watson Delaunay triangulation", [&] {
        const std::vector<Point> square{{-1.0, -1.0}, {1.0, -1.0},
                                        {1.0, 1.0}, {-1.0, 1.0}};
        const DelaunayResult result = DelaunayTriangulator{}.compute(square);
        suite.expect(result.points.size() == 4U, "Delaunay retains original sites");
        suite.expect(result.triangles.size() == 2U, "Square triangulates into two triangles");
        suite.expect(triangulationEdges(result).size() == 5U, "Square triangulation has five edges");
    });

    suite.run("Voronoi cells and Delaunay dual", [&] {
        const std::vector<Point> sites{{-2.0, 0.0}, {2.0, 0.0}};
        const VoronoiResult result = VoronoiBuilder{}.compute(sites, {-5.0, -4.0, 5.0, 4.0});
        suite.expect(result.cells.size() == 2U, "Two sites create two clipped cells");
        suite.expect(!result.edges.empty(), "Voronoi cell edges exist");
        suite.expectNear(std::abs(signedArea(result.cells[0].vertices)), 40.0, 1.0e-8,
                         "Bisector splits viewport equally");
    });

    suite.run("Half-plane intersection", [&] {
        const std::vector<HalfPlane> constraints{
            {Point{-1.0, -1.0}, Point{1.0, -1.0}},
            {Point{1.0, -1.0}, Point{1.0, 1.0}},
            {Point{1.0, 1.0}, Point{-1.0, 1.0}},
            {Point{-1.0, 1.0}, Point{-1.0, -1.0}}
        };
        const HalfPlaneIntersectionResult result =
            HalfPlaneIntersector{}.compute(constraints, {-5.0, -5.0, 5.0, 5.0});
        suite.expect(!result.empty, "Square constraints have a feasible region");
        suite.expect(result.polygon.size() == 4U, "Feasible region has four corners");
        suite.expectNear(std::abs(signedArea(result.polygon)), 4.0, 1.0e-9,
                         "Feasible square area");
    });

    suite.run("Ear-clipping triangulation", [&] {
        const std::vector<Point> polygon{{0.0, 0.0}, {4.0, 0.0}, {4.0, 3.0},
                                         {2.0, 1.5}, {0.0, 3.0}};
        const PolygonTriangulationResult result = EarClippingTriangulator{}.compute(polygon);
        suite.expect(result.triangles.size() == polygon.size() - 2U, "n - 2 triangles produced");
        double triangleArea = 0.0;
        for (const Triangle& triangle : result.triangles) triangleArea += triangle.area();
        suite.expectNear(triangleArea, std::abs(signedArea(polygon)), 1.0e-9,
                         "Triangle areas cover polygon exactly");
    });

    suite.run("Closest pair and segment intersections", [&] {
        const std::vector<Point> points{{0.0, 0.0}, {8.0, 8.0}, {2.0, 2.0},
                                        {2.25, 2.0}, {-4.0, 5.0}};
        const ClosestPairResult closest = ClosestPairSolver{}.compute(points);
        suite.expect(closest.pair.has_value(), "Closest pair exists");
        suite.expectNear(closest.distance, 0.25, 1.0e-12, "Closest distance");
        const ClosestPairResult duplicate = ClosestPairSolver{}.compute(
            std::vector<Point>{{1.0, 1.0}, {4.0, 5.0}, {1.0, 1.0}});
        suite.expectNear(duplicate.distance, 0.0, 1.0e-12,
                         "Duplicate sites have zero closest distance");

        const std::vector<Segment> segments{{{-2.0, 0.0}, {2.0, 0.0}},
                                             {{0.0, -2.0}, {0.0, 2.0}},
                                             {{3.0, 3.0}, {4.0, 4.0}}};
        const auto intersections = SegmentIntersectionSolver{}.compute(segments);
        suite.expect(intersections.intersections.size() == 1U, "One unique crossing");
        suite.expect(samePoint(intersections.intersections.front(), Point{0.0, 0.0}),
                     "Crossing is the origin");
    });

    suite.run("Advanced polygon utilities", [&] {
        const std::vector<Point> square{{-1.0, -1.0}, {1.0, -1.0},
                                        {1.0, 1.0}, {-1.0, 1.0}};
        const std::vector<Point> sum = minkowskiSum(square, square);
        suite.expect(sum.size() == 4U, "Square Minkowski sum is square");
        suite.expectNear(std::abs(signedArea(sum)), 16.0, 1.0e-9, "Minkowski sum area");

        const auto diameter = convexDiameter(square);
        suite.expect(diameter.has_value(), "Convex diameter exists");
        suite.expectNear(diameter->length(), std::sqrt(8.0), 1.0e-9, "Square diameter");

        const std::vector<Point> polyline{{0.0, 0.0}, {1.0, 0.01}, {2.0, -0.01}, {3.0, 0.0}};
        suite.expect(simplifyPolyline(polyline, 0.05).size() == 2U,
                     "Douglas-Peucker removes near-collinear vertices");

        const auto enclosing = smallestEnclosingCircle(square);
        suite.expect(enclosing.has_value(), "Enclosing circle exists");
        suite.expectNear(enclosing->radius(), std::sqrt(2.0), 1.0e-9,
                         "Square enclosing-circle radius");
    });

    suite.run("Command pattern undo and redo", [&] {
        app::SceneModel scene;
        app::CommandManager commands;
        commands.execute(std::make_unique<app::AddPointCommand>(Point{3.0, 4.0}), scene);
        suite.expect(scene.points().size() == 1U, "Command adds point");
        suite.expect(commands.undo(scene), "Undo available");
        suite.expect(scene.points().empty(), "Undo removes point");
        suite.expect(commands.redo(scene), "Redo available");
        suite.expect(scene.points().size() == 1U, "Redo restores point");
    });

    suite.run("Versioned scene serialization", [&] {
        const std::filesystem::path path =
            std::filesystem::temp_directory_path() / "geoviz_B425050_test.geoviz";
        const app::SceneSnapshot scene{{{1.25, -2.5}, {9.0, 3.0}},
                                       {HalfPlane{{0.0, 0.0}, Vector2D{1.0, 0.0}}}};
        io::SceneSerializer::save(scene, path);
        const app::SceneSnapshot loaded = io::SceneSerializer::load(path);
        suite.expect(loaded.points.size() == 2U, "Point count round-trips");
        suite.expect(loaded.halfPlanes.size() == 1U, "Half-plane count round-trips");
        suite.expect(samePoint(loaded.points[0], scene.points[0]), "Point values round-trip");
        std::filesystem::remove(path);
    });

    suite.run("Polymorphic demo registry", [&] {
        app::DemoRegistry registry;
        app::SceneModel scene;
        scene.setPoints(squareWithCentre());
        suite.expect(registry.size() == 14U, "Fourteen interactive labs registered");
        for (std::size_t index = 0U; index < registry.size(); ++index) {
            const AlgorithmTrace trace = registry.at(index).buildTrace(
                scene, {-10.0, -10.0, 10.0, 10.0});
            suite.expect(!trace.empty(), "Every demo produces a safe visual trace");
        }
    });

    return suite.finish();
}
