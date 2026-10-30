#pragma once
#include <functional>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <stdexcept>

#include "geometry.hpp"
#include "trace.hpp"

namespace gv {
inline bool equivalentCost(double a, double b) {
    if (a == b)
        return true;
    if (!std::isfinite(a) || !std::isfinite(b) || a == 0 || b == 0)
        return false;
    return std::abs(a - b) <=
           32 * std::numeric_limits<double>::epsilon() * std::max(std::abs(a), std::abs(b));
}

// Abstract Strategy for interchangeable proximity implementations.
class SpatialIndex {
  public:
    virtual ~SpatialIndex() = default;
    virtual int nearest(Point query, Stats &stats, Trace *trace = nullptr) const = 0;
    virtual std::vector<int> radius(Point query, double distance, Stats &stats,
                                    Trace *trace = nullptr) const = 0;
};
// Exhaustive proximity Strategy and direct DBSCAN neighbourhood provider.
class LinearIndex final : public SpatialIndex {
    std::vector<Point> points_;

  public:
    explicit LinearIndex(std::vector<Point> points) : points_(std::move(points)) {}
    int nearest(Point query, Stats &stats, Trace *trace = nullptr) const override;
    std::vector<int> radius(Point query, double distance, Stats &stats,
                            Trace *trace = nullptr) const override;
};
// Balanced two-dimensional median-split proximity Strategy.
class KDTree final : public SpatialIndex {
    class Node {
        friend class KDTree;
        int point, axis, left = -1, right = -1;
        Box box;

      public:
        Node(int id, int splitAxis, int l, int r, Box bounds)
            : point(id), axis(splitAxis), left(l), right(r), box(std::move(bounds)) {}
    };
    std::vector<Point> points_;
    std::vector<Node> nodes_;
    int root_ = -1;
    int build(std::vector<int> &ids, int l, int r, int depth, Box box, Trace *trace);
    static double boxDistance2(Point q, const Box &box);
    std::vector<int> search(Point q, double limit, bool nearestOnly, Stats &stats,
                            Trace *trace) const;

  public:
    explicit KDTree(const std::vector<Point> &points, Trace *trace = nullptr);
    int nearest(Point q, Stats &stats, Trace *trace = nullptr) const override;
    std::vector<int> radius(Point q, double r, Stats &stats, Trace *trace = nullptr) const override;
};

// Template-method base for validated, verified algorithm execution.
class Algorithm {
  protected:
    virtual Result calculate(const Scene &scene, bool record) const = 0;

  public:
    virtual ~Algorithm() = default;
    virtual Mode mode() const noexcept = 0;
    Result run(const Scene &scene, bool record = true) const;
};
// Monotone-chain hull Strategy; collinear side interiors are discarded.
class HullAlgorithm final : public Algorithm {
    Result calculate(const Scene &scene, bool record) const override;

  public:
    Mode mode() const noexcept override { return Mode::Hull; }
};
// Boundary-first ray-parity polygon membership Strategy.
class GeofenceAlgorithm final : public Algorithm {
    Result calculate(const Scene &scene, bool record) const override;

  public:
    Mode mode() const noexcept override { return Mode::Geofence; }
};
// Shared non-negative priority-queue graph search.
class RouteAlgorithm : public Algorithm {
    Result calculate(const Scene &scene, bool record) const final;

  protected:
    virtual double estimate(const Scene &scene, int from, int goal) const = 0;
};
// Route specialization whose heuristic is identically zero.
class DijkstraAlgorithm final : public RouteAlgorithm {
    double estimate(const Scene &, int, int) const override { return 0.0; }

  public:
    Mode mode() const noexcept override { return Mode::Dijkstra; }
};
// Route specialization using Euclidean or grid-Manhattan estimates.
class AStarAlgorithm final : public RouteAlgorithm {
    double estimate(const Scene &scene, int from, int goal) const override;

  public:
    Mode mode() const noexcept override { return Mode::AStar; }
};
// Deterministic density expansion with a replaceable neighbourhood index.
class DBSCANAlgorithm final : public Algorithm {
    bool indexed_;
    Result calculate(const Scene &scene, bool record) const override;

  public:
    explicit DBSCANAlgorithm(bool indexed) : indexed_(indexed) {}
    Mode mode() const noexcept override { return indexed_ ? Mode::IndexedDBSCAN : Mode::DBSCAN; }
};
// Divide-and-conquer minimum pair distance with merged vertical order.
class ClosestPairAlgorithm final : public Algorithm {
    Result calculate(const Scene &scene, bool record) const override;

  public:
    Mode mode() const noexcept override { return Mode::ClosestPair; }
};
// Closed-segment pair intersections, including touches and overlaps.
class CollisionAlgorithm final : public Algorithm {
    Result calculate(const Scene &scene, bool record) const override;

  public:
    Mode mode() const noexcept override { return Mode::Collision; }
};
// Adapter between tree queries and the common Result/Trace interface.
class SpatialQueryAlgorithm final : public Algorithm {
    Mode mode_;
    Result calculate(const Scene &scene, bool record) const override;

  public:
    explicit SpatialQueryAlgorithm(Mode mode) : mode_(mode) {
        if (mode != Mode::KDNearest && mode != Mode::KDRadius)
            throw std::invalid_argument("Unsupported spatial query mode.");
    }
    Mode mode() const noexcept override { return mode_; }
};
// Application-facing execution abstraction.
class AlgorithmRunner {
  public:
    virtual ~AlgorithmRunner() = default;
    virtual Result execute(const Scene &scene, Mode mode, bool record = true) const = 0;
};
// Owning registry of algorithm Strategies, keyed by scoped Mode values.
class Engine final : public AlgorithmRunner {
    std::map<Mode, std::unique_ptr<Algorithm>> algorithms_;

  public:
    Engine();
    Result execute(const Scene &scene, Mode mode, bool record = true) const override;
    static void validate(const Scene &scene);
    static bool verify(const Scene &scene, Result &result);
};

} // namespace gv
