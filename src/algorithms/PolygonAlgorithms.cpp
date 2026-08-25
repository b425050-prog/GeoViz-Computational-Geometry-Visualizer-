/**
 * @file PolygonAlgorithms.cpp
 * @brief Animated ear clipping, closest pair, and additional advanced routines.
 */

#include "geoviz/algorithms/PolygonAlgorithms.hpp"

#include "geoviz/algorithms/ConvexHull.hpp"
#include "geoviz/algorithms/Predicates.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <numeric>
#include <set>

namespace geoviz::algorithms {
namespace {

[[nodiscard]] bool pointBlocksEar(const Point& point, const Triangle& triangle) {
    // For ear clipping, a vertex on the candidate diagonal must also block the
    // ear; accepting it would create overlapping triangles in polygons with
    // collinear or nearly collinear diagonals.
    return triangle.contains(point);
}

[[nodiscard]] std::vector<Point> uniqueInput(std::span<const Point> input) {
    std::vector<Point> result;
    for (const Point& point : input) {
        if (std::none_of(result.begin(), result.end(), [&](const Point& existing) {
                return samePoint(point, existing);
            })) {
            result.push_back(point);
        }
    }
    return result;
}

[[nodiscard]] Point lineIntersection(const Point& segmentStart, const Point& segmentEnd,
                                     const Point& clipStart, const Point& clipEnd) {
    const Vector2D segmentDirection = segmentEnd - segmentStart;
    const Vector2D clipDirection = clipEnd - clipStart;
    const double denominator = cross(segmentDirection, clipDirection);
    if (std::abs(denominator) <= kEpsilon) return segmentEnd;
    const double parameter = cross(clipStart - segmentStart, clipDirection) / denominator;
    return segmentStart + segmentDirection * parameter;
}

[[nodiscard]] Circle circleFromDiameter(const Point& first, const Point& second) {
    const Point centre{0.5 * (first.x() + second.x()), 0.5 * (first.y() + second.y())};
    return {centre, distance(centre, first)};
}

[[nodiscard]] Circle circleThroughThree(const Point& first, const Point& second,
                                        const Point& third) {
    const Triangle triangle(first, second, third);
    if (const std::optional<Point> centre = circumcenter(triangle); centre.has_value()) {
        return {*centre, distance(*centre, first)};
    }
    // Collinear points are enclosed by the diameter of their farthest pair.
    const double firstSecond = squaredDistance(first, second);
    const double firstThird = squaredDistance(first, third);
    const double secondThird = squaredDistance(second, third);
    if (firstSecond >= firstThird && firstSecond >= secondThird) return circleFromDiameter(first, second);
    if (firstThird >= secondThird) return circleFromDiameter(first, third);
    return circleFromDiameter(second, third);
}

}  // namespace

PolygonTriangulationResult EarClippingTriangulator::compute(std::span<const Point> input) const {
    PolygonTriangulationResult result;
    result.trace = AlgorithmTrace{"Ear-Clipping Triangulation", "O(n^2)"};
    std::vector<Point> polygon = uniqueInput(input);
    if (polygon.size() < 3U) {
        VisualFrame frame;
        frame.headline = "A polygon needs three vertices";
        frame.explanation = "Add vertices in boundary order to begin ear clipping.";
        frame.sourcePoints = polygon;
        result.trace.addFrame(std::move(frame));
        return result;
    }
    if (signedArea(polygon) < 0.0) std::reverse(polygon.begin(), polygon.end());

    std::vector<std::size_t> active(polygon.size());
    std::iota(active.begin(), active.end(), 0U);
    std::size_t comparisons = 0U;
    std::size_t safety = 0U;
    while (active.size() > 3U && safety++ < polygon.size() * polygon.size()) {
        bool earFound = false;
        for (std::size_t activeIndex = 0U; activeIndex < active.size(); ++activeIndex) {
            const std::size_t previous = active[(activeIndex + active.size() - 1U) % active.size()];
            const std::size_t current = active[activeIndex];
            const std::size_t next = active[(activeIndex + 1U) % active.size()];
            ++comparisons;
            if (orientation(polygon[previous], polygon[current], polygon[next]) !=
                Orientation::CounterClockwise) {
                continue;
            }

            const Triangle candidate(polygon[previous], polygon[current], polygon[next]);
            bool containsVertex = false;
            for (std::size_t index : active) {
                if (index == previous || index == current || index == next) continue;
                ++comparisons;
                if (pointBlocksEar(polygon[index], candidate)) {
                    containsVertex = true;
                    break;
                }
            }

            VisualFrame inspect;
            inspect.headline = containsVertex ? "Reject blocked ear" : "Clip a valid ear";
            inspect.explanation = containsVertex
                ? "Another active vertex lies inside this candidate triangle."
                : "The vertex is convex and its ear triangle contains no other vertex.";
            inspect.sourcePoints = polygon;
            inspect.highlightedPoints = candidate.vertices();
            inspect.triangles = result.triangles;
            inspect.triangles.push_back(candidate);
            std::vector<Point> activePolygon;
            for (std::size_t index : active) activePolygon.push_back(polygon[index]);
            inspect.acceptedEdges = closedEdges(activePolygon);
            inspect.comparisons = comparisons;
            inspect.iteration = result.trace.size();
            result.trace.addFrame(std::move(inspect));

            if (!containsVertex) {
                result.triangles.push_back(candidate);
                active.erase(active.begin() + static_cast<std::ptrdiff_t>(activeIndex));
                earFound = true;
                break;
            }
        }
        if (!earFound) {
            throw GeometryError("Ear clipping failed: the polygon may be self-intersecting.");
        }
    }
    if (active.size() == 3U) {
        result.triangles.emplace_back(polygon[active[0]], polygon[active[1]], polygon[active[2]]);
    }

    VisualFrame finalFrame;
    finalFrame.headline = "Polygon triangulated";
    finalFrame.explanation = "A simple n-vertex polygon is partitioned into exactly n - 2 triangles.";
    finalFrame.sourcePoints = polygon;
    finalFrame.triangles = result.triangles;
    finalFrame.acceptedEdges = closedEdges(polygon);
    finalFrame.comparisons = comparisons;
    finalFrame.iteration = result.trace.size();
    result.trace.addFrame(std::move(finalFrame));
    return result;
}

ClosestPairResult ClosestPairSolver::compute(std::span<const Point> input) const {
    ClosestPairResult result;
    result.trace = AlgorithmTrace{"Closest Pair", "O(n log^2 n) divide and conquer"};
    // Duplicates are intentionally retained: two identical input points make
    // the mathematically correct closest distance exactly zero.
    std::vector<Point> points(input.begin(), input.end());
    std::sort(points.begin(), points.end(), [](const Point& first, const Point& second) {
        return lexicographicLess(first, second);
    });
    if (points.size() < 2U) {
        VisualFrame frame;
        frame.headline = "At least two points are required";
        frame.sourcePoints = points;
        result.trace.addFrame(std::move(frame));
        return result;
    }

    std::size_t comparisons = 0U;
    std::function<std::pair<double, Segment>(std::size_t, std::size_t)> solve;
    solve = [&](std::size_t left, std::size_t right) -> std::pair<double, Segment> {
        if (right - left <= 3U) {
            double best = std::numeric_limits<double>::infinity();
            Segment bestPair(points[left], points[left + 1U]);
            for (std::size_t first = left; first < right; ++first) {
                for (std::size_t second = first + 1U; second < right; ++second) {
                    ++comparisons;
                    const double candidate = distance(points[first], points[second]);
                    if (candidate < best) {
                        best = candidate;
                        bestPair = Segment(points[first], points[second]);
                    }
                }
            }
            return {best, bestPair};
        }

        const std::size_t middle = left + (right - left) / 2U;
        const double dividingX = points[middle].x();
        auto leftResult = solve(left, middle);
        auto rightResult = solve(middle, right);
        auto bestResult = leftResult.first <= rightResult.first ? leftResult : rightResult;

        std::vector<Point> strip;
        for (std::size_t index = left; index < right; ++index) {
            if (std::abs(points[index].x() - dividingX) < bestResult.first) {
                strip.push_back(points[index]);
            }
        }
        std::sort(strip.begin(), strip.end(), [](const Point& first, const Point& second) {
            return first.y() < second.y();
        });
        for (std::size_t first = 0U; first < strip.size(); ++first) {
            for (std::size_t second = first + 1U;
                 second < strip.size() && strip[second].y() - strip[first].y() < bestResult.first;
                 ++second) {
                ++comparisons;
                const double candidate = distance(strip[first], strip[second]);
                if (candidate < bestResult.first) {
                    bestResult = {candidate, Segment(strip[first], strip[second])};
                }
            }
        }

        VisualFrame frame;
        frame.headline = "Merge across a dividing strip";
        frame.explanation = "Only points within the current best distance of the split can improve it.";
        frame.sourcePoints = points;
        frame.highlightedPoints = strip;
        frame.acceptedEdges = {bestResult.second};
        frame.candidateEdges.emplace_back(Point{dividingX, -10000.0}, Point{dividingX, 10000.0});
        frame.comparisons = comparisons;
        frame.iteration = result.trace.size();
        result.trace.addFrame(std::move(frame));
        return bestResult;
    };

    auto [bestDistance, bestPair] = solve(0U, points.size());
    result.distance = bestDistance;
    result.pair = bestPair;
    VisualFrame finalFrame;
    finalFrame.headline = "Closest pair found";
    finalFrame.explanation = "The highlighted segment has minimum Euclidean length.";
    finalFrame.sourcePoints = points;
    finalFrame.highlightedPoints = {bestPair.start(), bestPair.end()};
    finalFrame.acceptedEdges = {bestPair};
    finalFrame.comparisons = comparisons;
    finalFrame.iteration = result.trace.size();
    result.trace.addFrame(std::move(finalFrame));
    return result;
}

SegmentIntersectionsResult SegmentIntersectionSolver::compute(
    std::span<const Segment> segments) const {
    SegmentIntersectionsResult result;
    result.trace = AlgorithmTrace{"Segment Intersections", "O(m^2) exact pairwise test"};
    std::size_t comparisons = 0U;
    for (std::size_t first = 0U; first < segments.size(); ++first) {
        for (std::size_t second = first + 1U; second < segments.size(); ++second) {
            ++comparisons;
            const IntersectionResult intersection = intersect(segments[first], segments[second]);
            for (const Point& point : intersection.points) {
                if (std::none_of(result.intersections.begin(), result.intersections.end(),
                                 [&](const Point& existing) { return samePoint(point, existing); })) {
                    result.intersections.push_back(point);
                }
            }

            VisualFrame frame;
            frame.headline = intersection.intersects() ? "Intersection detected" : "Disjoint pair";
            frame.explanation = "Orientation tests classify the two endpoints against the opposite segment.";
            frame.acceptedEdges.assign(segments.begin(), segments.end());
            frame.candidateEdges = {segments[first], segments[second]};
            frame.highlightedPoints = result.intersections;
            frame.comparisons = comparisons;
            frame.iteration = result.trace.size();
            result.trace.addFrame(std::move(frame));
        }
    }
    if (segments.size() < 2U) {
        VisualFrame frame;
        frame.headline = "Draw at least two segments";
        frame.explanation = "In this demo, consecutive point pairs form independent segments.";
        result.trace.addFrame(std::move(frame));
    }
    return result;
}

std::vector<Point> clipPolygon(std::span<const Point> subject,
                               std::span<const Point> convexClipWindow) {
    std::vector<Point> output(subject.begin(), subject.end());
    if (output.empty() || convexClipWindow.size() < 3U) return {};
    std::vector<Point> clipWindow(convexClipWindow.begin(), convexClipWindow.end());
    if (signedArea(clipWindow) < 0.0) std::reverse(clipWindow.begin(), clipWindow.end());

    for (std::size_t edgeIndex = 0U; edgeIndex < clipWindow.size(); ++edgeIndex) {
        const Point clipStart = clipWindow[edgeIndex];
        const Point clipEnd = clipWindow[(edgeIndex + 1U) % clipWindow.size()];
        const std::vector<Point> input = output;
        output.clear();
        if (input.empty()) break;
        for (std::size_t index = 0U; index < input.size(); ++index) {
            const Point& current = input[index];
            const Point& next = input[(index + 1U) % input.size()];
            const bool currentInside = cross(clipStart, clipEnd, current) >= -kEpsilon;
            const bool nextInside = cross(clipStart, clipEnd, next) >= -kEpsilon;
            if (currentInside) output.push_back(current);
            if (currentInside != nextInside) {
                output.push_back(lineIntersection(current, next, clipStart, clipEnd));
            }
        }
    }
    return output;
}

std::vector<Point> minkowskiSum(std::span<const Point> first, std::span<const Point> second) {
    if (first.empty() || second.empty()) return {};
    std::vector<Point> pairwiseSums;
    pairwiseSums.reserve(first.size() * second.size());
    for (const Point& a : first) {
        for (const Point& b : second) {
            pairwiseSums.emplace_back(a.x() + b.x(), a.y() + b.y());
        }
    }
    MonotonicChainHull hull;
    return hull.compute(pairwiseSums).hull;
}

std::optional<Segment> convexDiameter(std::span<const Point> input) {
    if (input.size() < 2U) return std::nullopt;
    if (input.size() == 2U) return Segment(input[0], input[1]);
    std::vector<Point> polygon(input.begin(), input.end());
    if (signedArea(polygon) < 0.0) std::reverse(polygon.begin(), polygon.end());

    std::size_t opposite = 1U;
    double bestDistance = 0.0;
    Segment best(polygon[0], polygon[1]);
    for (std::size_t index = 0U; index < polygon.size(); ++index) {
        const std::size_t next = (index + 1U) % polygon.size();
        while (std::abs(cross(polygon[index], polygon[next],
                             polygon[(opposite + 1U) % polygon.size()])) >
               std::abs(cross(polygon[index], polygon[next], polygon[opposite])) + kEpsilon) {
            opposite = (opposite + 1U) % polygon.size();
        }
        for (const std::size_t endpoint : {index, next}) {
            const double candidate = squaredDistance(polygon[endpoint], polygon[opposite]);
            if (candidate > bestDistance) {
                bestDistance = candidate;
                best = Segment(polygon[endpoint], polygon[opposite]);
            }
        }
    }
    return best;
}

std::vector<Point> simplifyPolyline(std::span<const Point> polyline, double tolerance) {
    if (tolerance < 0.0) throw GeometryError("Simplification tolerance cannot be negative.");
    if (polyline.size() <= 2U) return {polyline.begin(), polyline.end()};

    std::vector<bool> keep(polyline.size(), false);
    keep.front() = true;
    keep.back() = true;
    std::function<void(std::size_t, std::size_t)> simplify;
    simplify = [&](std::size_t first, std::size_t last) {
        double maximumDistance = -1.0;
        std::size_t farthest = first;
        const Segment baseline(polyline[first], polyline[last]);
        for (std::size_t index = first + 1U; index < last; ++index) {
            const double candidate = distanceToSegment(polyline[index], baseline);
            if (candidate > maximumDistance) {
                maximumDistance = candidate;
                farthest = index;
            }
        }
        if (maximumDistance > tolerance) {
            keep[farthest] = true;
            simplify(first, farthest);
            simplify(farthest, last);
        }
    };
    simplify(0U, polyline.size() - 1U);

    std::vector<Point> result;
    for (std::size_t index = 0U; index < polyline.size(); ++index) {
        if (keep[index]) result.push_back(polyline[index]);
    }
    return result;
}

std::optional<Circle> smallestEnclosingCircle(std::span<const Point> input) {
    if (input.empty()) return std::nullopt;
    const std::vector<Point> points = uniqueInput(input);
    Circle circle(points.front(), 0.0);
    for (std::size_t first = 0U; first < points.size(); ++first) {
        if (circle.contains(points[first], 1.0e-7)) continue;
        circle = Circle(points[first], 0.0);
        for (std::size_t second = 0U; second < first; ++second) {
            if (circle.contains(points[second], 1.0e-7)) continue;
            circle = circleFromDiameter(points[first], points[second]);
            for (std::size_t third = 0U; third < second; ++third) {
                if (!circle.contains(points[third], 1.0e-7)) {
                    circle = circleThroughThree(points[first], points[second], points[third]);
                }
            }
        }
    }
    return circle;
}

}  // namespace geoviz::algorithms
