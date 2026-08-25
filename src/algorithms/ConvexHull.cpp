/**
 * @file ConvexHull.cpp
 * @brief Animated implementations of Monotonic Chain, Graham, Jarvis, and QuickHull.
 */

#include "geoviz/algorithms/ConvexHull.hpp"

#include "geoviz/algorithms/Predicates.hpp"

#include <algorithm>
#include <functional>
#include <limits>
#include <sstream>
#include <utility>

namespace geoviz::algorithms {
namespace {

[[nodiscard]] std::vector<Point> uniquePoints(std::span<const Point> input) {
    std::vector<Point> points(input.begin(), input.end());
    std::sort(points.begin(), points.end(), [](const Point& first, const Point& second) {
        return lexicographicLess(first, second);
    });
    points.erase(std::unique(points.begin(), points.end(), [](const Point& first, const Point& second) {
        return samePoint(first, second);
    }), points.end());
    return points;
}

[[nodiscard]] HullResult trivialResult(std::vector<Point> points,
                                       std::string algorithmName,
                                       std::string complexity) {
    AlgorithmTrace trace(std::move(algorithmName), std::move(complexity));
    VisualFrame frame;
    frame.headline = "Trivial hull";
    frame.explanation = points.empty()
        ? "No points are available. Add points to the canvas."
        : "Fewer than three distinct points already form their own convex hull.";
    frame.sourcePoints = points;
    frame.highlightedPoints = points;
    frame.acceptedEdges = points.size() == 2U
                              ? std::vector<Segment>{Segment(points[0], points[1])}
                              : std::vector<Segment>{};
    trace.addFrame(std::move(frame));
    return {std::move(points), std::move(trace)};
}

void addFinalHullFrame(AlgorithmTrace& trace, const std::vector<Point>& points,
                       const std::vector<Point>& hull, std::size_t comparisons) {
    VisualFrame frame;
    frame.headline = "Convex hull complete";
    frame.explanation = "Every input point is on or inside the highlighted boundary.";
    frame.sourcePoints = points;
    frame.highlightedPoints = hull;
    frame.acceptedEdges = closedEdges(hull);
    frame.filledPolygons.push_back(hull);
    frame.comparisons = comparisons;
    frame.iteration = trace.size();
    trace.addFrame(std::move(frame));
}

[[nodiscard]] std::string pointLabel(const Point& point) {
    std::ostringstream text;
    text << point;
    return text.str();
}

}  // namespace

HullResult MonotonicChainHull::compute(std::span<const Point> input) const {
    std::vector<Point> points = uniquePoints(input);
    if (points.size() < 3U) {
        return trivialResult(std::move(points), std::string(name()), std::string(complexity()));
    }

    AlgorithmTrace trace{std::string(name()), std::string(complexity())};
    trace.reserve(points.size() * 4U);
    VisualFrame sortedFrame;
    sortedFrame.headline = "Lexicographic ordering";
    sortedFrame.explanation = "Sort by x, then y. The scan will build lower and upper chains.";
    sortedFrame.sourcePoints = points;
    trace.addFrame(std::move(sortedFrame));

    std::size_t comparisons = 0U;
    auto buildChain = [&](auto begin, auto end, std::string_view chainName) {
        std::vector<Point> chain;
        for (auto iterator = begin; iterator != end; ++iterator) {
            const Point candidate = *iterator;
            VisualFrame inspect;
            inspect.headline = std::string("Inspecting ") + pointLabel(candidate);
            inspect.explanation = "The next vertex must keep a counter-clockwise turn on the " +
                                  std::string(chainName) + " chain.";
            inspect.sourcePoints = points;
            inspect.highlightedPoints = {candidate};
            inspect.acceptedEdges = openEdges(chain);
            if (!chain.empty()) inspect.candidateEdges.emplace_back(chain.back(), candidate);
            inspect.comparisons = comparisons;
            inspect.iteration = trace.size();
            trace.addFrame(std::move(inspect));

            while (chain.size() >= 2U) {
                ++comparisons;
                const Orientation turn = orientation(chain[chain.size() - 2U], chain.back(), candidate);
                if (turn == Orientation::CounterClockwise) {
                    break;
                }
                VisualFrame remove;
                remove.headline = "Discarding a non-left turn";
                remove.explanation = "The middle vertex cannot be an extreme hull vertex.";
                remove.sourcePoints = points;
                remove.highlightedPoints = {chain[chain.size() - 2U], chain.back(), candidate};
                remove.acceptedEdges = openEdges(chain);
                remove.rejectedEdges.emplace_back(chain[chain.size() - 2U], chain.back());
                remove.candidateEdges.emplace_back(chain.back(), candidate);
                remove.comparisons = comparisons;
                remove.iteration = trace.size();
                trace.addFrame(std::move(remove));
                chain.pop_back();
            }
            chain.push_back(candidate);
        }
        return chain;
    };

    std::vector<Point> lower = buildChain(points.begin(), points.end(), "lower");
    std::vector<Point> upper = buildChain(points.rbegin(), points.rend(), "upper");
    lower.pop_back();
    upper.pop_back();
    lower.insert(lower.end(), upper.begin(), upper.end());

    addFinalHullFrame(trace, points, lower, comparisons);
    return {std::move(lower), std::move(trace)};
}

HullResult GrahamScanHull::compute(std::span<const Point> input) const {
    std::vector<Point> points = uniquePoints(input);
    if (points.size() < 3U) {
        return trivialResult(std::move(points), std::string(name()), std::string(complexity()));
    }

    const auto pivotIterator = std::min_element(points.begin(), points.end(),
        [](const Point& first, const Point& second) {
            return first.y() < second.y() ||
                   (almostEqual(first.y(), second.y()) && first.x() < second.x());
        });
    std::iter_swap(points.begin(), pivotIterator);
    const Point pivot = points.front();
    std::sort(points.begin() + 1, points.end(), [&](const Point& first, const Point& second) {
        const Orientation turn = orientation(pivot, first, second);
        if (turn == Orientation::Collinear) {
            return squaredDistance(pivot, first) < squaredDistance(pivot, second);
        }
        return turn == Orientation::CounterClockwise;
    });

    AlgorithmTrace trace{std::string(name()), std::string(complexity())};
    VisualFrame orderFrame;
    orderFrame.headline = "Polar-angle ordering";
    orderFrame.explanation = "Choose the lowest pivot and sort all other points around it.";
    orderFrame.sourcePoints = points;
    orderFrame.highlightedPoints = {pivot};
    for (std::size_t index = 1U; index < points.size(); ++index) {
        orderFrame.candidateEdges.emplace_back(pivot, points[index]);
    }
    trace.addFrame(std::move(orderFrame));

    std::vector<Point> stack;
    std::size_t comparisons = 0U;
    for (const Point& candidate : points) {
        while (stack.size() >= 2U) {
            ++comparisons;
            if (orientation(stack[stack.size() - 2U], stack.back(), candidate) ==
                Orientation::CounterClockwise) {
                break;
            }
            VisualFrame popFrame;
            popFrame.headline = "Pop from the Graham stack";
            popFrame.explanation = "A clockwise or collinear turn would bend into the hull.";
            popFrame.sourcePoints = points;
            popFrame.highlightedPoints = {stack[stack.size() - 2U], stack.back(), candidate};
            popFrame.acceptedEdges = openEdges(stack);
            popFrame.rejectedEdges.emplace_back(stack[stack.size() - 2U], stack.back());
            popFrame.candidateEdges.emplace_back(stack.back(), candidate);
            popFrame.comparisons = comparisons;
            popFrame.iteration = trace.size();
            trace.addFrame(std::move(popFrame));
            stack.pop_back();
        }
        stack.push_back(candidate);
        VisualFrame pushFrame;
        pushFrame.headline = "Push an extreme candidate";
        pushFrame.explanation = "The current stack maintains only left turns.";
        pushFrame.sourcePoints = points;
        pushFrame.highlightedPoints = {candidate};
        pushFrame.acceptedEdges = openEdges(stack);
        pushFrame.comparisons = comparisons;
        pushFrame.iteration = trace.size();
        trace.addFrame(std::move(pushFrame));
    }

    addFinalHullFrame(trace, points, stack, comparisons);
    return {std::move(stack), std::move(trace)};
}

HullResult JarvisMarchHull::compute(std::span<const Point> input) const {
    std::vector<Point> points = uniquePoints(input);
    if (points.size() < 3U) {
        return trivialResult(std::move(points), std::string(name()), std::string(complexity()));
    }

    AlgorithmTrace trace{std::string(name()), std::string(complexity())};
    const std::size_t start = static_cast<std::size_t>(std::distance(
        points.begin(), std::min_element(points.begin(), points.end(),
            [](const Point& first, const Point& second) {
                return lexicographicLess(first, second);
            })));

    std::vector<Point> hull;
    std::size_t current = start;
    std::size_t comparisons = 0U;
    do {
        hull.push_back(points[current]);
        std::size_t candidate = (current + 1U) % points.size();
        for (std::size_t probe = 0U; probe < points.size(); ++probe) {
            if (probe == current) continue;
            ++comparisons;
            const Orientation turn = orientation(points[current], points[candidate], points[probe]);
            if (turn == Orientation::Clockwise ||
                (turn == Orientation::Collinear &&
                 squaredDistance(points[current], points[probe]) >
                     squaredDistance(points[current], points[candidate]))) {
                candidate = probe;
            }

            VisualFrame frame;
            frame.headline = "Gift-wrapping the next edge";
            frame.explanation = "Rotate the candidate edge until every point lies on one side.";
            frame.sourcePoints = points;
            frame.highlightedPoints = {points[current], points[probe]};
            frame.acceptedEdges = openEdges(hull);
            frame.candidateEdges.emplace_back(points[current], points[candidate]);
            frame.comparisons = comparisons;
            frame.iteration = trace.size();
            trace.addFrame(std::move(frame));
        }
        current = candidate;
    } while (current != start && hull.size() <= points.size());

    addFinalHullFrame(trace, points, hull, comparisons);
    return {std::move(hull), std::move(trace)};
}

HullResult QuickHull::compute(std::span<const Point> input) const {
    std::vector<Point> points = uniquePoints(input);
    if (points.size() < 3U) {
        return trivialResult(std::move(points), std::string(name()), std::string(complexity()));
    }

    AlgorithmTrace trace{std::string(name()), std::string(complexity())};
    const Point left = points.front();
    const Point right = points.back();
    std::vector<Point> upper;
    std::vector<Point> lower;
    for (const Point& point : points) {
        const double side = cross(left, right, point);
        if (side > kEpsilon) upper.push_back(point);
        if (side < -kEpsilon) lower.push_back(point);
    }

    VisualFrame partition;
    partition.headline = "Initial QuickHull partition";
    partition.explanation = "The extreme x-coordinates divide the input into two independent sets.";
    partition.sourcePoints = points;
    partition.highlightedPoints = {left, right};
    partition.candidateEdges.emplace_back(left, right);
    trace.addFrame(std::move(partition));

    std::vector<Point> hull{left};
    std::size_t comparisons = 0U;
    std::function<void(const Point&, const Point&, const std::vector<Point>&)> expand;
    expand = [&](const Point& first, const Point& second, const std::vector<Point>& candidates) {
        if (candidates.empty()) {
            hull.push_back(second);
            return;
        }

        auto farthest = candidates.begin();
        double largestArea = -1.0;
        for (auto iterator = candidates.begin(); iterator != candidates.end(); ++iterator) {
            ++comparisons;
            const double area = std::abs(cross(first, second, *iterator));
            if (area > largestArea) {
                largestArea = area;
                farthest = iterator;
            }
        }
        const Point pivot = *farthest;

        VisualFrame split;
        split.headline = "Choose the farthest point";
        split.explanation = "The farthest point forms a triangle; points inside it cannot be extreme.";
        split.sourcePoints = points;
        split.highlightedPoints = {first, pivot, second};
        split.candidateEdges = {Segment(first, pivot), Segment(pivot, second)};
        split.rejectedEdges.emplace_back(first, second);
        split.triangles.emplace_back(first, pivot, second);
        split.comparisons = comparisons;
        split.iteration = trace.size();
        trace.addFrame(std::move(split));

        std::vector<Point> firstSide;
        std::vector<Point> secondSide;
        for (const Point& point : candidates) {
            if (samePoint(point, pivot)) continue;
            if (cross(first, pivot, point) > kEpsilon) firstSide.push_back(point);
            if (cross(pivot, second, point) > kEpsilon) secondSide.push_back(point);
        }
        expand(first, pivot, firstSide);
        expand(pivot, second, secondSide);
    };

    expand(left, right, upper);
    // The lower set is on the left of the reversed baseline.
    if (!hull.empty() && samePoint(hull.back(), right)) {
        // right is already the connector between the upper and lower recursions.
    }
    expand(right, left, lower);
    if (hull.size() > 1U && samePoint(hull.front(), hull.back())) hull.pop_back();

    addFinalHullFrame(trace, points, hull, comparisons);
    return {std::move(hull), std::move(trace)};
}

std::unique_ptr<ConvexHullStrategy> makeHullStrategy(HullAlgorithm algorithm) {
    switch (algorithm) {
        case HullAlgorithm::MonotonicChain: return std::make_unique<MonotonicChainHull>();
        case HullAlgorithm::GrahamScan: return std::make_unique<GrahamScanHull>();
        case HullAlgorithm::JarvisMarch: return std::make_unique<JarvisMarchHull>();
        case HullAlgorithm::QuickHull: return std::make_unique<QuickHull>();
    }
    throw GeometryError("Unknown convex hull strategy.");
}

}  // namespace geoviz::algorithms
