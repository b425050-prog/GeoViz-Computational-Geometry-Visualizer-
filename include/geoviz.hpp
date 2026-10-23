#pragma once
// The engine deliberately has no raylib dependency. Coordinates here are doubles.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace gv {
constexpr double INF = std::numeric_limits<double>::infinity();
constexpr double TOL = 1e-9;
constexpr std::size_t MAX_POINTS = 256, MAX_EDGES = 4096, MAX_TRACE = 4096;
/**
 * Validated two-dimensional world coordinate.
 * Finite doubles are bounded at construction and mutation. The rendering layer
 * applies its own screen transform; geometry never operates in screen pixels.
 */
class Point {
    double x_ = 0;
    double y_ = 0;
    static double checked(double value) {
        if (!std::isfinite(value) || std::abs(value) > 1e7)
            throw std::invalid_argument("Coordinate must be finite and within +/-10000000.");
        return value;
    }

  public:
    Point() = default;
    Point(double x, double y) : x_(checked(x)), y_(checked(y)) {}
    double x() const noexcept {
        return x_;
    }
    void setX(double value) {
        x_ = checked(value);
    }
    double y() const noexcept {
        return y_;
    }
    void setY(double value) {
        y_ = checked(value);
    }
};
/**
 * Axis-aligned spatial bounds used for tree partitions and trace regions.
 * A zero-width or zero-height box is permitted: it represents a split line.
 */
class Box {
    double x0_ = 0;
    double y0_ = 0;
    double x1_ = 0;
    double y1_ = 0;

  public:
    Box() = default;
    Box(double x0, double y0, double x1, double y1) : x0_(x0), y0_(y0), x1_(x1), y1_(y1) {}
    double x0() const noexcept {
        return x0_;
    }
    void setX0(double value) {
        x0_ = value;
    }
    double y0() const noexcept {
        return y0_;
    }
    void setY0(double value) {
        y0_ = value;
    }
    double x1() const noexcept {
        return x1_;
    }
    void setX1(double value) {
        x1_ = value;
    }
    double y1() const noexcept {
        return y1_;
    }
    void setY1(double value) {
        y1_ = value;
    }
};
/**
 * An undirected weighted graph connection between input point indices.
 * Scene alone may mutate endpoints, cost and blocked status so topology changes
 * pass through its validation and revision-tracking operations.
 */
class Edge {
    friend class Scene;
    int a_ = 0;
    int b_ = 0;
    double weight_ = 1;
    bool blocked_ = false;
    void setA(int value) {
        a_ = value;
    }
    void setB(int value) {
        b_ = value;
    }
    void setWeight(double value) {
        weight_ = value;
    }
    void setBlocked(bool value) {
        blocked_ = value;
    }

  public:
    Edge() = default;
    Edge(int a, int b, double weight = 1, bool blocked = false)
        : a_(a), b_(b), weight_(weight), blocked_(blocked) {}
    int a() const noexcept {
        return a_;
    }
    int b() const noexcept {
        return b_;
    }
    double weight() const noexcept {
        return weight_;
    }
    bool blocked() const noexcept {
        return blocked_;
    }
};
/**
 * Aggregate input model and the sole owner of editable graph topology.
 * Const collection views prevent callers from bypassing edit operations. Each
 * successful change advances revision; replacing points clears dependent edges.
 */
class Scene {
    std::vector<Point> points_ = {};
    std::vector<Point> polygon_ = {};
    std::vector<Edge> edges_ = {};
    Point query_ = {300, 250};
    double radius_ = 75;
    int minPts_ = 4;
    int source_ = 0;
    int goal_ = 0;
    bool grid_ = false;
    std::uint64_t revision_ = 0;

  public:
    Scene() = default;
    const std::vector<Point> &points() const noexcept {
        return points_;
    }
    void setPoints(std::vector<Point> value);
    const std::vector<Point> &polygon() const noexcept {
        return polygon_;
    }
    void setPolygon(std::vector<Point> value);
    const std::vector<Edge> &edges() const noexcept {
        return edges_;
    }
    void setEdges(std::vector<Edge> value);
    const Point &query() const noexcept {
        return query_;
    }
    void setQuery(Point value) {
        query_ = value;
        touch();
    }
    double radius() const noexcept {
        return radius_;
    }
    void setRadius(double value) {
        if (!std::isfinite(value) || value <= 0 || value > 1e7)
            throw std::invalid_argument("Radius must be positive and at most 10000000.");
        radius_ = value;
        touch();
    }
    int minPts() const noexcept {
        return minPts_;
    }
    void setMinPts(int value) {
        if (value < 1 || value > static_cast<int>(MAX_POINTS))
            throw std::invalid_argument("MinPts must be between 1 and 256.");
        minPts_ = value;
        touch();
    }
    int source() const noexcept {
        return source_;
    }
    void setSource(int value) {
        if (value < 0 || (value != 0 && value >= static_cast<int>(points_.size())))
            throw std::out_of_range("Unknown source vertex.");
        source_ = value;
        touch();
    }
    int goal() const noexcept {
        return goal_;
    }
    void setGoal(int value) {
        if (value < 0 || (value != 0 && value >= static_cast<int>(points_.size())))
            throw std::out_of_range("Unknown goal vertex.");
        goal_ = value;
        touch();
    }
    bool grid() const noexcept {
        return grid_;
    }
    void setGrid(bool value) {
        grid_ = value;
        touch();
    }
    std::uint64_t revision() const noexcept {
        return revision_;
    }
    void touch() noexcept {
        ++revision_;
    }
    void addPoint(Point point);
    void movePoint(std::size_t index, Point point);
    void removePoint(std::size_t index);
    void addPolygonVertex(Point point);
    void movePolygonVertex(std::size_t index, Point point);
    void addEdge(Edge edge);
    void setEdgeWeight(std::size_t index, double weight);
    void setEdgeBlocked(std::size_t index, bool blocked);
};
enum class Mode {
    Hull,
    Geofence,
    Dijkstra,
    AStar,
    DBSCAN,
    ClosestPair,
    KDNearest,
    KDRadius,
    IndexedDBSCAN,
    Collision
};
std::string modeName(Mode m);
/**
 * Algorithm work counters independent of animation speed and rendering.
 * An operation has a documented meaning per algorithm; counts across unrelated
 * algorithms are not interchangeable CPU-time measurements.
 */
class Stats {
    std::uint64_t operations_ = 0;
    std::uint64_t queries_ = 0;
    std::uint64_t expanded_ = 0;

  public:
    Stats() = default;
    std::uint64_t operations() const noexcept {
        return operations_;
    }
    std::uint64_t queries() const noexcept {
        return queries_;
    }
    std::uint64_t expanded() const noexcept {
        return expanded_;
    }

    void countOperation() noexcept {
        ++operations_;
    }
    void countQuery() noexcept {
        ++queries_;
    }
    void countExpansion() noexcept {
        ++expanded_;
    }
};
/**
 * A value snapshot of one recorded decision.
 * Indices refer to the frozen input in Run. Optional vectors hold only the state
 * needed by that algorithm; no raylib types or graphics resources are stored.
 */
class Frame {
    std::string explanation_ = {};
    std::vector<int> chain_ = {};
    std::vector<int> active_ = {};
    std::vector<int> labels_ = {};
    std::vector<int> kind_ = {};
    std::vector<int> queue_ = {};
    std::vector<double> scores_ = {};
    std::vector<double> costs_ = {};
    std::vector<int> parents_ = {};
    std::vector<std::pair<int, int>> lines_ = {};
    std::vector<Box> regions_ = {};
    std::vector<Box> pruned_ = {};
    int focus_ = -1;
    double radius_ = 0;

  public:
    Frame() = default;
    const std::string &explanation() const noexcept {
        return explanation_;
    }
    void setExplanation(std::string value) {
        explanation_ = std::move(value);
    }
    const std::vector<int> &chain() const noexcept {
        return chain_;
    }
    void setChain(std::vector<int> value) {
        chain_ = std::move(value);
    }
    const std::vector<int> &active() const noexcept {
        return active_;
    }
    void setActive(std::vector<int> value) {
        active_ = std::move(value);
    }
    const std::vector<int> &labels() const noexcept {
        return labels_;
    }
    void setLabels(std::vector<int> value) {
        labels_ = std::move(value);
    }
    const std::vector<int> &kind() const noexcept {
        return kind_;
    }
    void setKind(std::vector<int> value) {
        kind_ = std::move(value);
    }
    const std::vector<int> &queue() const noexcept {
        return queue_;
    }
    const std::vector<double> &scores() const noexcept {
        return scores_;
    }
    const std::vector<double> &costs() const noexcept {
        return costs_;
    }
    void setCosts(std::vector<double> value) {
        costs_ = std::move(value);
    }
    const std::vector<int> &parents() const noexcept {
        return parents_;
    }
    void setParents(std::vector<int> value) {
        parents_ = std::move(value);
    }
    const std::vector<std::pair<int, int>> &lines() const noexcept {
        return lines_;
    }
    const std::vector<Box> &regions() const noexcept {
        return regions_;
    }
    void setRegions(std::vector<Box> value) {
        regions_ = std::move(value);
    }
    const std::vector<Box> &pruned() const noexcept {
        return pruned_;
    }
    void setPruned(std::vector<Box> value) {
        pruned_ = std::move(value);
    }
    int focus() const noexcept {
        return focus_;
    }
    void setFocus(int value) {
        focus_ = value;
    }
    double radius() const noexcept {
        return radius_;
    }
    void setRadius(double value) {
        radius_ = value;
    }

    void appendChain(int value) {
        chain_.push_back(value);
    }
    void appendActive(int value) {
        active_.push_back(value);
    }
    void clearActive() noexcept {
        active_.clear();
    }
    void appendQueue(int value) {
        queue_.push_back(value);
    }
    void clearQueue() noexcept {
        queue_.clear();
    }
    void resizeScores(std::size_t count) {
        scores_.resize(count);
    }
    void appendLines(std::pair<int, int> value) {
        lines_.push_back(std::move(value));
    }
    void appendRegions(Box value) {
        regions_.push_back(std::move(value));
    }
    void clearRegions() noexcept {
        regions_.clear();
    }
    void setScore(std::size_t index, double score) {
        scores_.at(index) = score;
    }
};
/**
 * Bounded sequence of decision snapshots.
 * The prefix is retained at the cap; finish replaces its last frame with the
 * final result and exposes truncation instead of silently implying a full trace.
 */
class Trace {
    std::vector<Frame> frames_ = {};
    bool truncated_ = false;
    bool enabled_ = true;

  public:
    Trace() = default;
    const std::vector<Frame> &frames() const noexcept {
        return frames_;
    }
    bool truncated() const noexcept {
        return truncated_;
    }
    bool enabled() const noexcept {
        return enabled_;
    }
    void setEnabled(bool value) {
        enabled_ = value;
    }
    void add(const Frame &frame) {
        if (!enabled_)
            return;
        if (frames_.size() < MAX_TRACE)
            frames_.push_back(frame);
        else
            truncated_ = true;
    }
    void finish(const Frame &frame) {
        if (!enabled_)
            return;
        if (frames_.size() >= MAX_TRACE) {
            frames_.back() = frame;
            truncated_ = true;
        } else
            frames_.push_back(frame);
    }
};
/**
 * Computed answer, work counts, verification status and optional playback.
 * Calculation and verification are separate, making a failed check visible to
 * the caller without changing the answer to match the reference.
 */
class Result {
    Mode mode_ = Mode::Hull;
    std::vector<int> answer_ = {};
    std::vector<int> labels_ = {};
    std::vector<int> kind_ = {};
    double value_ = INF;
    std::string summary_ = {};
    Stats stats_ = {};
    Trace trace_ = {};
    bool checked_ = false;
    std::string check_ = {};

  public:
    Result() = default;
    Mode mode() const noexcept {
        return mode_;
    }
    void setMode(Mode value) {
        mode_ = value;
    }
    const std::vector<int> &answer() const noexcept {
        return answer_;
    }
    void setAnswer(std::vector<int> value) {
        answer_ = std::move(value);
    }
    const std::vector<int> &labels() const noexcept {
        return labels_;
    }
    void setLabels(std::vector<int> value) {
        labels_ = std::move(value);
    }
    const std::vector<int> &kind() const noexcept {
        return kind_;
    }
    void setKind(std::vector<int> value) {
        kind_ = std::move(value);
    }
    double value() const noexcept {
        return value_;
    }
    void setValue(double value) {
        value_ = value;
    }
    const std::string &summary() const noexcept {
        return summary_;
    }
    void setSummary(std::string value) {
        summary_ = std::move(value);
    }
    const Stats &stats() const noexcept {
        return stats_;
    }
    Stats &stats() noexcept {
        return stats_;
    }
    const Trace &trace() const noexcept {
        return trace_;
    }
    Trace &trace() noexcept {
        return trace_;
    }
    bool checked() const noexcept {
        return checked_;
    }
    void setChecked(bool value) {
        checked_ = value;
    }
    const std::string &check() const noexcept {
        return check_;
    }
    void setCheck(std::string value) {
        check_ = std::move(value);
    }

    void appendAnswer(int value) {
        answer_.push_back(value);
    }
};
/**
 * A cursor and clock over immutable trace frames.
 * Changing playback never reruns an algorithm or changes its operation counts.
 * Manual stepping pauses playback; reset always returns to the first frame.
 */
class Playback {
    std::size_t cursor_ = 0;
    double clock_ = 0;

    bool playing_ = false;
    double seconds_ = 0.25;

  public:
    bool playing() const noexcept {
        return playing_;
    }
    void setPlaying(bool value) noexcept {
        playing_ = value;
    }
    double seconds() const noexcept {
        return seconds_;
    }
    void setSeconds(double value) {
        if (!std::isfinite(value) || value <= 0)
            throw std::invalid_argument("Playback interval must be positive.");
        seconds_ = value;
    }
    std::size_t cursor() const {
        return cursor_;
    }
    void reset() {
        cursor_ = 0;
        clock_ = 0;
        playing_ = false;
    }
    void step(int direction, std::size_t count);
    void update(double dt, std::size_t count);
};
/**
 * An immutable input/result pair retained for reproducibility.
 * Value ownership protects historical evidence from later scene edits; const
 * access allows the renderer and exporter to share the same snapshot safely.
 */
class Run {
    Scene input_ = {};
    Result result_ = {};

  public:
    Run() = default;
    Run(Scene input, Result result) : input_(std::move(input)), result_(std::move(result)) {}
    const Scene &input() const noexcept {
        return input_;
    }
    const Result &result() const noexcept {
        return result_;
    }
};

/**
 * Shared predicates for planar geometry.
 * Orientation uses a scale-aware tolerance; sorting instead uses strict exact
 * coordinate ordering to satisfy the standard library comparator contract.
 */
class GeometryKernel {
  public:
    static double cross(Point a, Point b, Point c);
    static double distance2(Point a, Point b);
    static double distance(Point a, Point b);
    static int orientation(Point a, Point b, Point c);
    static bool onSegment(Point p, Point a, Point b);
    static bool intersects(Point a, Point b, Point c, Point d);
    // -1 outside, 0 boundary, 1 inside.
    static int membership(Point q, const std::vector<Point> &polygon);
    static void validatePolygon(const std::vector<Point> &polygon);
};

// A spatial query can use a scan or a tree without changing its client.
/**
 * Abstract Strategy for interchangeable proximity implementations.
 * The virtual destructor permits safe deletion through unique_ptr<SpatialIndex>.
 * Both implementations include points exactly on a radius boundary.
 */
class SpatialIndex {
  public:
    virtual ~SpatialIndex() = default;
    virtual int nearest(Point query, Stats &stats, Trace *trace = nullptr) const = 0;
    virtual std::vector<int> radius(Point query, double distance, Stats &stats,
                                    Trace *trace = nullptr) const = 0;
};
/**
 * Exhaustive proximity Strategy and direct DBSCAN neighbourhood provider.
 * Ascending input order gives stable answers and resolves equal nearest ties.
 */
class LinearIndex final : public SpatialIndex {
    std::vector<Point> points_;

  public:
    explicit LinearIndex(std::vector<Point> points) : points_(std::move(points)) {}
    int nearest(Point query, Stats &stats, Trace *trace = nullptr) const override;
    std::vector<int> radius(Point query, double distance, Stats &stats,
                            Trace *trace = nullptr) const override;
};
/**
 * Balanced two-dimensional median-split proximity Strategy.
 * The tree owns a copy of its input. Node indices stay valid while the vector
 * grows; no recursive branch keeps a reference across a vector reallocation.
 */
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
    static double boxDistance2(Point q, Box box);

  public:
    explicit KDTree(const std::vector<Point> &points, Trace *trace = nullptr);
    int nearest(Point q, Stats &stats, Trace *trace = nullptr) const override;
    std::vector<int> radius(Point q, double r, Stats &stats, Trace *trace = nullptr) const override;
};

// Template Method: every algorithm follows validate -> calculate -> verify.
/**
 * Template-method base for validated, verified algorithm execution.
 * run performs the shared lifecycle; calculate is the polymorphic extension
 * point, so individual algorithms cannot skip validation through Engine.
 */
class Algorithm {
  protected:
    virtual Result calculate(const Scene &scene, bool record) const = 0;

  public:
    virtual ~Algorithm() = default;
    virtual Mode mode() const noexcept = 0;
    Result run(const Scene &scene, bool record = true) const;
};
/**
 * Monotone-chain hull Strategy; collinear side interiors are discarded.
 */
class HullAlgorithm final : public Algorithm {
    Result calculate(const Scene &scene, bool record) const override;

  public:
    Mode mode() const noexcept override {
        return Mode::Hull;
    }
};
/**
 * Boundary-first ray-parity polygon membership Strategy.
 */
class GeofenceAlgorithm final : public Algorithm {
    Result calculate(const Scene &scene, bool record) const override;

  public:
    Mode mode() const noexcept override {
        return Mode::Geofence;
    }
};
// The shared route search delegates only its heuristic to the subclasses.
/**
 * Shared non-negative priority-queue graph search.
 * Subclasses vary the heuristic while relaxation, path reconstruction, trace
 * recording and verification remain in one implementation.
 */
class RouteAlgorithm : public Algorithm {
    Result calculate(const Scene &scene, bool record) const final;

  protected:
    virtual double estimate(const Scene &scene, int from, int goal) const = 0;
};
/**
 * Route specialization whose heuristic is identically zero.
 */
class DijkstraAlgorithm final : public RouteAlgorithm {
    double estimate(const Scene &, int, int) const override {
        return 0.0;
    }

  public:
    Mode mode() const noexcept override {
        return Mode::Dijkstra;
    }
};
/**
 * Route specialization using Euclidean or grid-Manhattan estimates.
 * The shared search checks every open edge for compatibility before enabling
 * the heuristic; incompatible weights select the safe zero estimate.
 */
class AStarAlgorithm final : public RouteAlgorithm {
    double estimate(const Scene &scene, int from, int goal) const override;

  public:
    Mode mode() const noexcept override {
        return Mode::AStar;
    }
};
/**
 * Deterministic density expansion with a replaceable neighbourhood index.
 * The indexed flag chooses a Strategy, not a second cluster-expansion loop.
 */
class DBSCANAlgorithm final : public Algorithm {
    bool indexed_;
    Result calculate(const Scene &scene, bool record) const override;

  public:
    explicit DBSCANAlgorithm(bool indexed) : indexed_(indexed) {}
    Mode mode() const noexcept override {
        return indexed_ ? Mode::IndexedDBSCAN : Mode::DBSCAN;
    }
};
/**
 * Divide-and-conquer minimum pair distance with merged vertical order.
 */
class ClosestPairAlgorithm final : public Algorithm {
    Result calculate(const Scene &scene, bool record) const override;

  public:
    Mode mode() const noexcept override {
        return Mode::ClosestPair;
    }
};
/**
 * Closed-segment pair intersections, including touches and overlaps.
 */
class CollisionAlgorithm final : public Algorithm {
    Result calculate(const Scene &scene, bool record) const override;

  public:
    Mode mode() const noexcept override {
        return Mode::Collision;
    }
};
/**
 * Adapter between tree queries and the common Result/Trace interface.
 */
class SpatialQueryAlgorithm final : public Algorithm {
    Mode mode_;
    Result calculate(const Scene &scene, bool record) const override;

  public:
    explicit SpatialQueryAlgorithm(Mode mode) : mode_(mode) {
        if (mode != Mode::KDNearest && mode != Mode::KDRadius)
            throw std::invalid_argument("Unsupported spatial query mode.");
    }
    Mode mode() const noexcept override {
        return mode_;
    }
};
/**
 * Application-facing execution abstraction.
 * App depends on this interface; Engine supplies its concrete implementation.
 */
class AlgorithmRunner {
  public:
    virtual ~AlgorithmRunner() = default;
    virtual Result execute(const Scene &scene, Mode mode, bool record = true) const = 0;
};
/**
 * Owning registry of algorithm Strategies, keyed by scoped Mode values.
 * unique_ptr expresses exclusive ownership and releases every Strategy through
 * its virtual base destructor. Algorithms remain usable without a GUI.
 */
class Engine final : public AlgorithmRunner {
    std::map<Mode, std::unique_ptr<Algorithm>> algorithms_;

  public:
    Engine();
    Result execute(const Scene &scene, Mode mode, bool record = true) const override;
    static void validate(const Scene &scene);
    static bool verify(const Scene &scene, Result &result);
};

/**
 * Versioned scene persistence and checked-run evidence output.
 * Load constructs a temporary validated model before returning it. Full-precision
 * coordinates preserve inputs; summaries retain an adjacent scene snapshot.
 */
class SceneIO {
  public:
    static void save(const Scene &scene, const std::string &path);
    static Scene load(const std::string &path);
    static void summary(const Run &run, const std::string &path);
};
// Six deterministic inputs per mode, shared by the engine, interface and documentation.
// Keeping samples in the engine lets tests, documentation and the GUI use identical data.
constexpr int SampleVariantCount = 6;
Scene sampleScene(Mode mode, int variant = 0);
std::string sampleName(Mode mode, int variant);
Scene gridScene();

// Input timestamps are integer UTC seconds; no host-timezone interpretation.
/**
 * One pseudonymous source record, before filtering or aggregation.
 * Truth labels are retained for evaluation and are never clustering features.
 */
class Event {
    std::string id_ = {};
    std::string user_ = {};
    std::string device_ = {};
    std::string label_ = {};
    std::string summary_ = {};
    std::int64_t timestamp_ = 0;
    double lat_ = 0;
    double lon_ = 0;
    int requests_ = 0;
    int failures_ = 0;

  public:
    Event() = default;
    const std::string &id() const noexcept {
        return id_;
    }
    void setId(std::string value) {
        id_ = std::move(value);
    }
    const std::string &user() const noexcept {
        return user_;
    }
    void setUser(std::string value) {
        user_ = std::move(value);
    }
    const std::string &device() const noexcept {
        return device_;
    }
    void setDevice(std::string value) {
        device_ = std::move(value);
    }
    const std::string &label() const noexcept {
        return label_;
    }
    void setLabel(std::string value) {
        label_ = std::move(value);
    }
    const std::string &summary() const noexcept {
        return summary_;
    }
    void setSummary(std::string value) {
        summary_ = std::move(value);
    }
    std::int64_t timestamp() const noexcept {
        return timestamp_;
    }
    void setTimestamp(std::int64_t value) {
        timestamp_ = value;
    }
    double lat() const noexcept {
        return lat_;
    }
    void setLat(double value) {
        lat_ = value;
    }
    double lon() const noexcept {
        return lon_;
    }
    void setLon(double value) {
        lon_ = value;
    }
    int requests() const noexcept {
        return requests_;
    }
    void setRequests(int value) {
        requests_ = value;
    }
    int failures() const noexcept {
        return failures_;
    }
    void setFailures(int value) {
        failures_ = value;
    }
};
/**
 * One common time window, identity filters and measurement parameters.
 * SecurityConsole validates related settings together before any query or save.
 * Geographic and behavioural queries/radii remain separate because units differ.
 */
class SecuritySettings {
    Point geoQuery_ = {0, 0};
    Point featureQuery_ = {0, 0};
    std::int64_t start_ = 1800000000;
    std::int64_t end_ = 1800000600;
    std::string user_ = "*";
    std::string device_ = "*";
    double requestScale_ = 10;
    double failureScale_ = 1;
    double featureRadius_ = 0.8;
    double geoRadius_ = 150;
    int minPts_ = 3;
    double originLat_ = 20.2961;
    double originLon_ = 85.8245;

  public:
    SecuritySettings() = default;
    const Point &geoQuery() const noexcept {
        return geoQuery_;
    }
    void setGeoQuery(Point value) {
        geoQuery_ = value;
    }
    const Point &featureQuery() const noexcept {
        return featureQuery_;
    }
    void setFeatureQuery(Point value) {
        featureQuery_ = value;
    }
    std::int64_t start() const noexcept {
        return start_;
    }
    void setStart(std::int64_t value) {
        start_ = value;
    }
    std::int64_t end() const noexcept {
        return end_;
    }
    void setEnd(std::int64_t value) {
        end_ = value;
    }
    const std::string &user() const noexcept {
        return user_;
    }
    void setUser(std::string value) {
        user_ = std::move(value);
    }
    const std::string &device() const noexcept {
        return device_;
    }
    void setDevice(std::string value) {
        device_ = std::move(value);
    }
    double requestScale() const noexcept {
        return requestScale_;
    }
    void setRequestScale(double value) {
        requestScale_ = value;
    }
    double failureScale() const noexcept {
        return failureScale_;
    }
    void setFailureScale(double value) {
        failureScale_ = value;
    }
    double featureRadius() const noexcept {
        return featureRadius_;
    }
    void setFeatureRadius(double value) {
        featureRadius_ = value;
    }
    double geoRadius() const noexcept {
        return geoRadius_;
    }
    void setGeoRadius(double value) {
        geoRadius_ = value;
    }
    int minPts() const noexcept {
        return minPts_;
    }
    void setMinPts(int value) {
        minPts_ = value;
    }
    double originLat() const noexcept {
        return originLat_;
    }
    void setOriginLat(double value) {
        originLat_ = value;
    }
    double originLon() const noexcept {
        return originLon_;
    }
    void setOriginLon(double value) {
        originLon_ = value;
    }
};
/**
 * A displayed point linked to one or more original event indices.
 * Geographic observations represent individual records; behavioural observations
 * aggregate selected records per device over the complete selected window.
 */
class Observation {
    Point point_ = {};
    std::string name_ = {};
    std::vector<int> events_ = {};
    double requestRate_ = 0;
    double failureRate_ = 0;

  public:
    Observation() = default;
    Observation(Point point, std::string name, std::vector<int> events, double requestRate,
                double failureRate)
        : point_(std::move(point)), name_(std::move(name)), events_(std::move(events)),
          requestRate_(requestRate), failureRate_(failureRate) {}
    const Point &point() const noexcept {
        return point_;
    }
    void setPoint(Point value) {
        point_ = value;
    }
    const std::string &name() const noexcept {
        return name_;
    }
    void setName(std::string value) {
        name_ = std::move(value);
    }
    const std::vector<int> &events() const noexcept {
        return events_;
    }
    void setEvents(std::vector<int> value) {
        events_ = std::move(value);
    }
    double requestRate() const noexcept {
        return requestRate_;
    }
    void setRequestRate(double value) {
        requestRate_ = value;
    }
    double failureRate() const noexcept {
        return failureRate_;
    }
    void setFailureRate(double value) {
        failureRate_ = value;
    }

    void appendEvents(int value) {
        events_.push_back(value);
    }
};
/**
 * Abstract input boundary for approved local event exports.
 */
class LogAdapter {
  public:
    virtual ~LogAdapter() = default;
    virtual std::vector<Event> load(const std::string &path) const = 0;
};
/**
 * Strict single-line CSV implementation of the log-loading interface.
 */
class CsvLogAdapter final : public LogAdapter {
  public:
    std::vector<Event> load(const std::string &path) const override;
};
/**
 * Named geographic polyline projected into local world metres.
 */
class MapLine {
    std::string name_ = {};
    std::vector<Point> points_ = {};

  public:
    MapLine() = default;
    MapLine(std::string name, std::vector<Point> points)
        : name_(std::move(name)), points_(std::move(points)) {}
    const std::string &name() const noexcept {
        return name_;
    }
    void setName(std::string value) {
        name_ = std::move(value);
    }
    const std::vector<Point> &points() const noexcept {
        return points_;
    }
    void setPoints(std::vector<Point> value) {
        points_ = std::move(value);
    }

    void appendPoints(Point value) {
        points_.push_back(std::move(value));
    }
};
/**
 * Domain model connecting source events to reusable geometry algorithms.
 * Owns data, settings, fence and independent analyst verdicts. scene converts a
 * view into ordinary engine input, keeping ingestion and rendering separate.
 */
class SecurityConsole {
    std::vector<Event> events_;
    SecuritySettings settings_;
    std::vector<Point> fence_;
    // Event ID -> analyst verdict: benign, suspicious, or unreviewed.
    std::map<std::string, std::string> reviews_;

  public:
    const std::vector<Event> &events() const noexcept {
        return events_;
    }
    void setEvents(std::vector<Event> value) {
        events_ = std::move(value);
    }
    const SecuritySettings &settings() const noexcept {
        return settings_;
    }
    SecuritySettings &settings() noexcept {
        return settings_;
    }
    void setSettings(SecuritySettings value) {
        settings_ = std::move(value);
    }
    const std::vector<Point> &fence() const noexcept {
        return fence_;
    }
    void setFence(std::vector<Point> value) {
        fence_ = std::move(value);
    }
    const std::map<std::string, std::string> &reviews() const noexcept {
        return reviews_;
    }
    static Point project(double lat, double lon, const SecuritySettings &settings);
    static Point unproject(Point p, const SecuritySettings &settings);
    void validate() const;
    std::vector<Observation> observations(bool feature) const;
    Scene scene(bool feature) const;
    void save(const std::string &path) const;
    static SecurityConsole load(const std::string &path);
    std::string evaluation() const;
    void evidence(const std::string &path) const;
    std::vector<MapLine> loadMap(const std::string &path) const;
    void clearReviews() noexcept {
        reviews_.clear();
    }
    void setReview(const std::string &eventId, const std::string &verdict);
};
std::vector<std::string> parseCsv(const std::string &line);
struct SecuritySample {
    std::string file, name, purpose;
    double geographicRadius = 150, featureRadius = 0.8;
    int minPts = 3;
    bool eventFence = false;
};
std::vector<SecuritySample> securitySamples(const std::string &dataDirectory);
SecurityConsole loadSecuritySample(const std::string &dataDirectory, const SecuritySample &sample);
std::string csvQuote(const std::string &s);
std::string number(double x, int precision = 3);
} // namespace gv
