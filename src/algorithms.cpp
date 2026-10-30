#include "algorithms.hpp"
#include "persistence.hpp"
#include <numeric>
#include <sstream>
#include <tuple>
namespace gv {
namespace {
using G = GeometryKernel;
bool xyLess(const std::vector<Point> &p, int a, int b) {
    return std::make_tuple(p[a].x(), p[a].y(), a) < std::make_tuple(p[b].x(), p[b].y(), b);
}
} // anonymous namespace

// Sort exact coordinates, remove duplicates, then build two strictly counterclockwise chains.
Result HullAlgorithm::calculate(const Scene &s, bool record) const {
    Result r(mode(), record);
    std::vector<int> ids(s.points().size());
    std::iota(ids.begin(), ids.end(), 0);
    std::sort(ids.begin(), ids.end(), [&](int a, int b) {
        r.stats().countOperation();
        return xyLess(s.points(), a, b);
    });
    ids.erase(std::unique(ids.begin(), ids.end(),
                          [&](int a, int b) {
                              return s.points()[a].x() == s.points()[b].x() &&
                                     s.points()[a].y() == s.points()[b].y();
                          }),
              ids.end());
    Frame f;
    auto chain = [&](bool reverse) {
        std::vector<int> out;
        for (std::size_t k = 0; k < ids.size(); ++k) {
            int i = ids[reverse ? ids.size() - 1 - k : k];
            f.setFocus(i);
            f.setChain(out);
            f.setPseudocodeLine(2);
            f.setExplanation(reverse ? "Upper chain: consider candidate."
                                     : "Lower chain: consider candidate.");
            r.trace().add(f);
            while (out.size() >= 2) {
                r.stats().countOperation();
                if (G::orientation(s.points()[out[out.size() - 2]], s.points()[out.back()],
                                   s.points()[i]) > 0)
                    break;
                f.setActive({out.back()});
                out.pop_back();
                f.setChain(out);
                f.setPseudocodeLine(3);
                f.setExplanation("Remove a clockwise/collinear middle vertex.");
                r.trace().add(f);
            }
            out.push_back(i);
            f.setChain(out);
            f.clearActive();
            f.setPseudocodeLine(4);
            f.setExplanation("Accept candidate into this chain.");
            r.trace().add(f);
        }
        return out;
    };
    if (ids.size() <= 1)
        r.setAnswer(ids);
    else {
        auto low = chain(false), up = chain(true);
        low.pop_back();
        up.pop_back();
        low.insert(low.end(), up.begin(), up.end());
        r.setAnswer(std::move(low));
    }
    f.setChain(r.answer());
    f.setFocus(-1);
    f.clearActive();
    f.setPseudocodeLine(5);
    f.setExplanation("Hull complete: extreme endpoints retained on collinear sides.");
    if (r.answer().size() > 2)
        f.appendChain(r.answer().front());
    r.trace().finish(f);
    r.setValue(static_cast<double>(r.answer().size()));
    r.setSummary("Hull has " + std::to_string(r.answer().size()) +
                 (r.answer().size() == 1 ? " vertex." : " vertices."));
    return r;
}
// Record boundary tests before ray crossings so edge and vertex queries are explicit.
Result GeofenceAlgorithm::calculate(const Scene &s, bool record) const {
    G::validatePolygon(s.polygon());
    Result r(mode(), record);
    Frame f;
    int classification = -1;
    for (std::size_t i = 0; i < s.polygon().size(); ++i) {
        r.stats().countOperation();
        f.setFocus(static_cast<int>(i));
        f.setPseudocodeLine(1);
        f.setExplanation("Boundary test on polygon edge " + std::to_string(i) + ".");
        r.trace().add(f);
        if (G::onSegment(s.query(), s.polygon()[i], s.polygon()[(i + 1) % s.polygon().size()])) {
            classification = 0;
            break;
        }
    }
    if (classification != 0) {
        bool inside = false;
        for (std::size_t i = 0; i < s.polygon().size(); ++i) {
            r.stats().countOperation();
            Point a = s.polygon()[i], b = s.polygon()[(i + 1) % s.polygon().size()];
            bool crossing = (a.y() > s.query().y()) != (b.y() > s.query().y());
            if (crossing)
                crossing = s.query().x() <
                           a.x() + (s.query().y() - a.y()) * (b.x() - a.x()) / (b.y() - a.y());
            if (crossing) {
                inside = !inside;
                f.appendActive(static_cast<int>(i));
            }
            f.setFocus(static_cast<int>(i));
            f.setPseudocodeLine(2);
            f.setExplanation(crossing ? "Rightward ray crosses: toggle parity."
                                      : "No rightward crossing; parity unchanged.");
            r.trace().add(f);
        }
        classification = inside ? 1 : -1;
    }
    r.setValue(classification);
    r.setSummary(classification == 0 ? "BOUNDARY" : (classification == 1 ? "INSIDE" : "OUTSIDE"));
    f.setPseudocodeLine(3);
    f.setExplanation("Query classification: " + r.summary());
    r.trace().finish(f);
    return r;
}
// Grid spacing is 60 world units per unit-cost move; other graphs use Euclidean length.
double AStarAlgorithm::estimate(const Scene &s, int a, int goal) const {
    if (s.grid())
        return (std::abs(s.points()[a].x() - s.points()[goal].x()) +
                std::abs(s.points()[a].y() - s.points()[goal].y())) /
               60.0;
    return G::distance(s.points()[a], s.points()[goal]);
}
// Use lazy heap entries: improved tentative costs insert a new entry; stale entries are skipped.
Result RouteAlgorithm::calculate(const Scene &s, bool record) const {
    const Mode selectedMode = mode();
    int n = static_cast<int>(s.points().size());
    if (!n || s.source() < 0 || s.source() >= n || s.goal() < 0 || s.goal() >= n)
        throw std::runtime_error("Select valid source and goal vertices.");
    Result r(mode(), record);
    std::vector<std::vector<std::pair<int, double>>> adj(n);
    bool compatible = true;
    for (const auto &e : s.edges())
        if (!e.blocked()) {
            adj[e.a()].push_back({e.b(), e.weight()});
            adj[e.b()].push_back({e.a(), e.weight()});
            if (e.weight() < estimate(s, e.a(), e.b()) ||
                estimate(s, e.a(), s.goal()) > e.weight() + estimate(s, e.b(), s.goal()) ||
                estimate(s, e.b(), s.goal()) > e.weight() + estimate(s, e.a(), s.goal()))
                compatible = false;
        }
    for (auto &v : adj)
        std::sort(v.begin(), v.end());
    bool useH = selectedMode == Mode::AStar && compatible;
    std::vector<double> d(n, INF);
    std::vector<int> parent(n, -1), closed(n, 0);
    using Entry = std::tuple<double, double, int>; // f, g, vertex ID: deterministic ties.
    std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> heap;
    d[s.source()] = 0;
    heap.emplace((useH ? estimate(s, s.source(), s.goal()) : 0.0), 0, s.source());
    Frame f;
    auto snapshot = [&](int focus, const char *explanation, int codeLine, bool final = false) {
        if (!record || (!final && r.trace().truncated()))
            return;
        f.setPseudocodeLine(codeLine);
        f.setFocus(focus);
        f.setCosts(d);
        f.setParents(parent);
        f.setLabels(closed);
        f.resizeScores(n);
        for (int i = 0; i < n; ++i)
            f.setScore(i, d[i] + (useH ? estimate(s, i, s.goal()) : 0.0));
        f.clearActive();
        for (int i = 0; i < n; ++i)
            if (!closed[i] && std::isfinite(d[i]))
                f.appendActive(i);
        f.setExplanation(explanation);
        if (final)
            r.trace().finish(f);
        else
            r.trace().add(f);
    };
    snapshot(s.source(), "Initialize source cost to zero.", 1);
    while (!heap.empty()) {
        auto [fv, gv, u] = heap.top();
        heap.pop();
        (void)fv;
        r.stats().countOperation();
        if (closed[u] || gv != d[u])
            continue;
        closed[u] = 1;
        r.stats().countExpansion();
        snapshot(u, "Settle least-score frontier vertex.", 2);
        if (u == s.goal())
            break;
        for (auto [v, w] : adj[u]) {
            r.stats().countOperation();
            if (!closed[v] && d[u] + w < d[v]) {
                d[v] = d[u] + w;
                parent[v] = u;
                heap.emplace(d[v] + (useH ? estimate(s, v, s.goal()) : 0.0), d[v], v);
                snapshot(v, "Relax edge: improve cost and predecessor.", 3);
            }
        }
    }
    r.setValue(d[s.goal()]);
    if (std::isfinite(r.value())) {
        for (int v = s.goal(); v != -1; v = parent[v])
            r.appendAnswer(v);
        auto path = r.answer();
        std::reverse(path.begin(), path.end());
        r.setAnswer(std::move(path));
        r.setSummary("Optimal cost " + number(r.value()) + "; expanded " +
                     std::to_string(r.stats().expanded()) +
                     (r.stats().expanded() == 1 ? " vertex." : " vertices."));
    } else
        r.setSummary("NO PATH: goal is unreachable.");
    if (selectedMode == Mode::AStar && !compatible)
        r.setSummary(r.summary() + (" Incompatible weights: safe h=0 fallback."));
    // Refresh the final table even when intermediate recording reached the trace limit.
    f.setChain(r.answer());
    snapshot(-1, r.summary().c_str(), 4, true);
    return r;
}
// One deterministic breadth-first expansion serves both linear and indexed radius queries.
Result DBSCANAlgorithm::calculate(const Scene &s, bool record) const {
    const bool indexed = indexed_;
    Result r(mode(), record);
    int n = static_cast<int>(s.points().size());
    // -2 unassigned; -1 noise; >=0 cluster. kind: 0 noise, 1 border, 2 core.
    std::vector<int> labels(n, -2), kind(n, 0), visited(n, 0);
    std::unique_ptr<SpatialIndex> index;
    if (indexed)
        index = std::make_unique<KDTree>(s.points());
    else
        index = std::make_unique<LinearIndex>(s.points());
    Frame f;
    auto neighbours = [&](int i) {
        r.stats().countQuery();
        auto out = index->radius(s.points()[i], s.radius(), r.stats());
        if (record) {
            f.setFocus(i);
            f.setRadius(s.radius());
            f.setActive(out);
        }
        return out;
    };
    auto snap = [&](const char *text, const std::queue<int> &q, int codeLine) {
        if (!record || r.trace().truncated())
            return;
        f.setPseudocodeLine(codeLine);
        f.setLabels(labels);
        f.setKind(kind);
        f.clearQueue();
        auto copy = q;
        while (!copy.empty()) {
            f.appendQueue(copy.front());
            copy.pop();
        }
        f.setExplanation(text);
        r.trace().add(f);
    };
    int cluster = 0;
    for (int i = 0; i < n; ++i) {
        if (visited[i])
            continue;
        visited[i] = 1;
        auto ns = neighbours(i);
        std::queue<int> q;
        if (static_cast<int>(ns.size()) < s.minPts()) {
            labels[i] = -1;
            snap("Too few neighbours: provisional noise.", q, 2);
            continue;
        }
        labels[i] = cluster;
        kind[i] = 2;
        std::vector<int> queued(n, 0);
        for (int j : ns)
            if (j != i) {
                q.push(j);
                queued[j] = 1;
            }
        snap("Core point: start deterministic cluster expansion.", q, 3);
        while (!q.empty()) {
            int j = q.front();
            q.pop();
            if (!visited[j]) {
                visited[j] = 1;
                auto next = neighbours(j);
                if (static_cast<int>(next.size()) >= s.minPts()) {
                    kind[j] = 2;
                    for (int k : next)
                        if (!queued[k]) {
                            q.push(k);
                            queued[k] = 1;
                        }
                }
            }
            if (labels[j] < 0) {
                labels[j] = cluster;
                if (kind[j] != 2)
                    kind[j] = 1;
            }
            snap("Expand queue; earlier noise may become border.", q, 4);
        }
        ++cluster;
    }
    r.setLabels(labels);
    r.setKind(kind);
    r.setValue(cluster);
    f.setLabels(labels);
    f.setKind(kind);
    f.clearQueue();
    const auto noise = std::count(labels.begin(), labels.end(), -1);
    r.setSummary(std::to_string(cluster) + (cluster == 1 ? " cluster; " : " clusters; ") +
                 std::to_string(noise) + (noise == 1 ? " noise point." : " noise points."));
    r.finish(f, 5);
    return r;
}
// Sort by x once, solve recursively, and merge y order to preserve O(n log n) computation.
Result ClosestPairAlgorithm::calculate(const Scene &s, bool record) const {
    Result r(mode(), record);
    std::vector<int> ids(s.points().size()), temp(ids.size());
    std::iota(ids.begin(), ids.end(), 0);
    std::sort(ids.begin(), ids.end(), [&](int a, int b) {
        r.stats().countOperation();
        return xyLess(s.points(), a, b);
    });
    double best = INF;
    Frame f;
    auto consider = [&](int a, int b) {
        r.stats().countOperation();
        double d = G::distance2(s.points()[a], s.points()[b]);
        if (d < best || (d == best &&
                         std::pair<int, int>{std::min(a, b), std::max(a, b)} <
                             std::pair<int, int>{r.answer().empty() ? INT32_MAX : r.answer()[0],
                                                 r.answer().empty() ? INT32_MAX : r.answer()[1]})) {
            best = d;
            r.setAnswer({std::min(a, b), std::max(a, b)});
        }
        f.setActive({a, b});
        f.setChain(r.answer());
        f.setPseudocodeLine(2);
        f.setExplanation("Compare candidate pair; best distance " + number(std::sqrt(best)));
        r.trace().add(f);
    };
    auto byY = [&](int a, int b) {
        r.stats().countOperation();
        return std::make_tuple(s.points()[a].y(), s.points()[a].x(), a) <
               std::make_tuple(s.points()[b].y(), s.points()[b].x(), b);
    };
    std::function<void(int, int)> solve = [&](int l, int h) {
        if (h - l <= 3) {
            for (int i = l; i < h; ++i)
                for (int j = i + 1; j < h; ++j)
                    consider(ids[i], ids[j]);
            std::sort(ids.begin() + l, ids.begin() + h, byY);
            return;
        }
        int m = (l + h) / 2;
        double split = s.points()[ids[m]].x();
        double lo = s.points()[ids[l]].x(), hi = s.points()[ids[h - 1]].x();
        f.setRegions({{lo, -1000000, hi, 1000000}});
        f.setPseudocodeLine(1);
        f.setExplanation("Divide x-sorted region at x=" + number(split));
        r.trace().add(f);
        solve(l, m);
        solve(m, h);
        std::merge(ids.begin() + l, ids.begin() + m, ids.begin() + m, ids.begin() + h,
                   temp.begin() + l, byY);
        std::copy(temp.begin() + l, temp.begin() + h, ids.begin() + l);
        std::vector<int> strip;
        for (int k = l; k < h; ++k) {
            r.stats().countOperation();
            double dx = s.points()[ids[k]].x() - split;
            if (dx * dx <= best)
                strip.push_back(ids[k]);
        }
        double delta = std::sqrt(best);
        f.setRegions({{split - delta, -1000000, split + delta, 1000000}});
        f.setPseudocodeLine(3);
        f.setExplanation("Merge y-order and compare points in the dividing strip.");
        r.trace().add(f);
        // At most seven following points suffice in 2D once each half is solved.
        for (std::size_t i = 0; i < strip.size(); ++i)
            for (std::size_t j = i + 1; j < strip.size() && j <= i + 7; ++j) {
                double dy = s.points()[strip[j]].y() - s.points()[strip[i]].y();
                if (dy * dy > best)
                    break;
                consider(strip[i], strip[j]);
            }
    };
    // Handle duplicates explicitly, avoiding a zero-width strip degeneracy.
    for (std::size_t i = 1; i < ids.size(); ++i)
        if (G::distance2(s.points()[ids[i - 1]], s.points()[ids[i]]) == 0)
            consider(ids[i - 1], ids[i]);
    if (best != 0)
        solve(0, static_cast<int>(ids.size()));
    r.setValue(std::sqrt(best));
    r.setSummary(r.answer().empty() ? "Need at least two points."
                                    : "Closest distance " + number(r.value()));
    f.setChain(r.answer());
    f.clearActive();
    f.clearRegions();
    r.finish(f, 4);
    return r;
}
// Consecutive endpoint pairs define segments; test each unordered segment pair exactly once.
Result CollisionAlgorithm::calculate(const Scene &s, bool record) const {
    Result r(mode(), record);
    Frame f;
    int count = 0;
    std::vector<bool> highlighted(s.points().size() / 2, false);
    // Every consecutive pair (0,1), (2,3), ... is one independent segment.
    for (int a = 0; a + 1 < static_cast<int>(s.points().size()); a += 2)
        for (int b = a + 2; b + 1 < static_cast<int>(s.points().size()); b += 2) {
            r.stats().countOperation();
            f.setActive({a, a + 1, b, b + 1});
            bool hit =
                G::intersects(s.points()[a], s.points()[a + 1], s.points()[b], s.points()[b + 1]);
            if (hit) {
                ++count;
                // Count every intersecting pair, but retain each highlighted
                // segment once. Dense collisions must not multiply trace memory.
                for (int endpoint : {a, b})
                    if (!highlighted[endpoint / 2]) {
                        highlighted[endpoint / 2] = true;
                        f.appendLines({endpoint, endpoint + 1});
                    }
            }
            f.setPseudocodeLine(1);
            f.setExplanation(hit ? "Segments intersect (touching and overlap included)."
                                 : "Segments do not intersect.");
            r.trace().add(f);
        }
    r.setValue(count);
    r.setSummary(std::to_string(count) +
                 (count == 1 ? " intersecting segment pair." : " intersecting segment pairs."));
    f.clearActive();
    r.finish(f, 4);
    return r;
}

// Median selection balances node counts; depth parity alternates the split axis.
int KDTree::build(std::vector<int> &ids, int l, int r, int depth, Box box, Trace *trace) {
    if (l >= r)
        return -1;
    int axis = depth % 2, m = (l + r) / 2;
    auto less = [&](int a, int b) {
        double va = axis ? points_[a].y() : points_[a].x(),
               vb = axis ? points_[b].y() : points_[b].x();
        return va < vb || (va == vb && a < b);
    };
    std::nth_element(ids.begin() + l, ids.begin() + m, ids.begin() + r, less);
    int idx = static_cast<int>(nodes_.size());
    nodes_.push_back({ids[m], axis, -1, -1, box});
    double split = axis ? points_[ids[m]].y() : points_[ids[m]].x();
    if (trace) {
        Frame f;
        f.setFocus(ids[m]);
        f.setRegions({box});
        Box partition = box;
        if (axis) {
            partition.setY0(split);
            partition.setY1(split);
        } else {
            partition.setX0(split);
            partition.setX1(split);
        }
        f.appendRegions(partition);
        f.setPseudocodeLine(1);
        f.setExplanation(std::string("Median split on ") + (axis ? "y" : "x") + " at " +
                         number(split));
        trace->add(f);
    }
    Box left = box, right = box;
    if (axis) {
        left.setY1(split);
        right.setY0(split);
    } else {
        left.setX1(split);
        right.setX0(split);
    }
    int a = build(ids, l, m, depth + 1, left, trace),
        b = build(ids, m + 1, r, depth + 1, right, trace);
    nodes_[idx].left = a;
    nodes_[idx].right = b;
    return idx;
}
// Compute the root bounds and build using stable integer node indices rather than pointers.
KDTree::KDTree(const std::vector<Point> &p, Trace *trace) : points_(p) {
    if (p.empty())
        return;
    Box b{p[0].x(), p[0].y(), p[0].x(), p[0].y()};
    for (auto v : p) {
        b.setX0(std::min(b.x0(), v.x()));
        b.setX1(std::max(b.x1(), v.x()));
        b.setY0(std::min(b.y0(), v.y()));
        b.setY1(std::max(b.y1(), v.y()));
    }
    std::vector<int> ids(p.size());
    std::iota(ids.begin(), ids.end(), 0);
    nodes_.reserve(p.size());
    root_ = build(ids, 0, static_cast<int>(p.size()), 0, b, trace);
}
// Clamp each query coordinate to the box to obtain a lower bound on any contained distance.
double KDTree::boxDistance2(Point q, const Box &b) {
    double dx = std::max({b.x0() - q.x(), 0.0, q.x() - b.x1()}),
           dy = std::max({b.y0() - q.y(), 0.0, q.y() - b.y1()});
    return dx * dx + dy * dy;
}
// Nearest and radius queries share traversal, pruning, statistics and recording.
std::vector<int> KDTree::search(Point q, double limit, bool nearestOnly, Stats &stats,
                                Trace *trace) const {
    std::vector<int> out;
    std::function<void(int)> visit = [&](int u) {
        if (u < 0)
            return;
        const auto &node = nodes_[u];
        if (boxDistance2(q, node.box) > limit) {
            if (trace) {
                Frame frame;
                frame.setPruned({node.box});
                frame.setPseudocodeLine(3);
                frame.setExplanation(
                    "Prune box: its lower-bound distance exceeds the search limit.");
                trace->add(frame);
            }
            return;
        }
        stats.countOperation();
        stats.countExpansion();
        const double distance = GeometryKernel::distance2(q, points_[node.point]);
        if (nearestOnly) {
            if (distance < limit ||
                (distance == limit && (out.empty() || node.point < out.front()))) {
                limit = distance;
                out = {node.point};
            }
        } else if (distance <= limit)
            out.push_back(node.point);
        if (trace) {
            Frame frame;
            frame.setFocus(node.point);
            frame.setActive(out);
            frame.setRegions({node.box});
            frame.setPseudocodeLine(2);
            frame.setExplanation(nearestOnly ? "Visit node and update the nearest point."
                                             : "Visit node and test radius membership.");
            trace->add(frame);
        }
        const bool left =
            node.axis ? q.y() <= points_[node.point].y() : q.x() <= points_[node.point].x();
        visit(left ? node.left : node.right);
        visit(left ? node.right : node.left);
    };
    visit(root_);
    std::sort(out.begin(), out.end());
    return out;
}
int KDTree::nearest(Point q, Stats &stats, Trace *trace) const {
    auto ids = search(q, INF, true, stats, trace);
    return ids.empty() ? -1 : ids.front();
}
std::vector<int> KDTree::radius(Point q, double r, Stats &stats, Trace *trace) const {
    return search(q, r * r, false, stats, trace);
}

// Validate bounded finite model input before any algorithm indexes its collections.
void Engine::validate(const Scene &s) {
    if (s.points().size() > MAX_POINTS || s.polygon().size() > MAX_POINTS ||
        s.edges().size() > MAX_EDGES)
        throw std::runtime_error("Scene exceeds point/polygon/edge limit.");
    auto valid = [](Point p) {
        return std::isfinite(p.x()) && std::isfinite(p.y()) && std::abs(p.x()) <= 1e7 &&
               std::abs(p.y()) <= 1e7;
    };
    if (!valid(s.query()))
        throw std::runtime_error("Invalid query coordinates.");
    for (auto p : s.points())
        if (!valid(p))
            throw std::runtime_error("Invalid point coordinates.");
    for (auto p : s.polygon())
        if (!valid(p))
            throw std::runtime_error("Invalid polygon coordinates.");
    if (!std::isfinite(s.radius()) || s.radius() <= 0 || s.radius() > 1e7 || s.minPts() < 1 ||
        s.minPts() > static_cast<int>(MAX_POINTS))
        throw std::runtime_error("Radius/MinPts is outside its allowed range.");
    for (const auto &e : s.edges())
        if (e.a() < 0 || e.b() < 0 || e.a() >= static_cast<int>(s.points().size()) ||
            e.b() >= static_cast<int>(s.points().size()) || e.a() == e.b() ||
            !std::isfinite(e.weight()) || e.weight() < 0 || e.weight() > 1e12)
            throw std::runtime_error("Invalid edge endpoints or non-negative cost.");
    std::set<std::pair<int, int>> uniqueEdges;
    for (const auto &e : s.edges())
        if (!uniqueEdges.insert({std::min(e.a(), e.b()), std::max(e.a(), e.b())}).second)
            throw std::runtime_error("Only one edge per undirected vertex pair is supported.");
}
// Rebuild the index for this frozen input, recording construction and query decisions.
Result SpatialQueryAlgorithm::calculate(const Scene &s, bool record) const {
    Result r(mode(), record);
    std::unique_ptr<SpatialIndex> index =
        std::make_unique<KDTree>(s.points(), record ? &r.trace() : nullptr);
    if (mode_ == Mode::KDNearest) {
        int id = index->nearest(s.query(), r.stats(), record ? &r.trace() : nullptr);
        if (id >= 0) {
            r.setAnswer({id});
            r.setValue(G::distance(s.query(), s.points()[id]));
        }
    } else {
        r.setAnswer(index->radius(s.query(), s.radius(), r.stats(), record ? &r.trace() : nullptr));
        r.setValue(static_cast<double>(r.answer().size()));
    }
    r.setSummary(mode_ == Mode::KDNearest
                     ? "Nearest distance " + number(r.value())
                     : std::to_string(r.answer().size()) + (r.answer().size() == 1
                                                                ? " point inside radius."
                                                                : " points inside radius."));
    Frame f;
    f.setActive(r.answer());
    r.finish(f, 4);
    return r;
}
// The shared execution contract is validate, calculate, then independent result verification.
Result Algorithm::run(const Scene &scene, bool record) const {
    Engine::validate(scene);
    Result result = calculate(scene, record);
    Engine::verify(scene, result);
    return result;
}
// Register each concrete behavior once; callers select a mode without owning its Strategy.
Engine::Engine() {
    auto add = [this](std::unique_ptr<Algorithm> algorithm) {
        const Mode key = algorithm->mode();
        algorithms_.emplace(key, std::move(algorithm));
    };
    add(std::make_unique<HullAlgorithm>());
    add(std::make_unique<GeofenceAlgorithm>());
    add(std::make_unique<DijkstraAlgorithm>());
    add(std::make_unique<AStarAlgorithm>());
    add(std::make_unique<DBSCANAlgorithm>(false));
    add(std::make_unique<ClosestPairAlgorithm>());
    add(std::make_unique<SpatialQueryAlgorithm>(Mode::KDNearest));
    add(std::make_unique<SpatialQueryAlgorithm>(Mode::KDRadius));
    add(std::make_unique<DBSCANAlgorithm>(true));
    add(std::make_unique<CollisionAlgorithm>());
}
// Reject unknown modes before dispatch through the abstract algorithm interface.
Result Engine::execute(const Scene &scene, Mode mode, bool record) const {
    auto found = algorithms_.find(mode);
    if (found == algorithms_.end())
        throw std::invalid_argument("Unknown algorithm mode.");
    return found->second->run(scene, record);
}

// Strict improvement preserves the first (lowest-index) equal-distance answer.
int LinearIndex::nearest(Point query, Stats &stats, Trace *) const {
    int best = -1;
    double distance = INF;
    for (std::size_t i = 0; i < points_.size(); ++i) {
        stats.countOperation();
        const double candidate = GeometryKernel::distance2(query, points_[i]);
        if (candidate < distance) {
            distance = candidate;
            best = static_cast<int>(i);
        }
    }
    return best;
}
// Baseline radius scan: each visited point contributes one distance-test operation.
std::vector<int> LinearIndex::radius(Point query, double radius, Stats &stats, Trace *) const {
    std::vector<int> ids;
    for (std::size_t i = 0; i < points_.size(); ++i) {
        stats.countOperation();
        if (GeometryKernel::distance2(query, points_[i]) <= radius * radius)
            ids.push_back(static_cast<int>(i));
    }
    return ids;
}

// Reference work is deliberately excluded from Stats: it checks answers, not reported search cost.
bool Engine::verify(const Scene &s, Result &r) {
    bool ok = true;
    std::string reason;
    const auto sameCost = equivalentCost;
    if (r.mode() == Mode::Dijkstra || r.mode() == Mode::AStar) {
        std::vector<double> d(s.points().size(), INF);
        d[s.source()] = 0;
        for (std::size_t k = 1; k < s.points().size(); ++k)
            for (const auto &e : s.edges())
                if (!e.blocked()) {
                    double a = d[e.a()], b = d[e.b()];
                    d[e.b()] = std::min(d[e.b()], a + e.weight());
                    d[e.a()] = std::min(d[e.a()], b + e.weight());
                }
        ok = (!std::isfinite(d[s.goal()]) && !std::isfinite(r.value())) ||
             sameCost(d[s.goal()], r.value());
        if (std::isfinite(r.value())) {
            bool pathOk = !r.answer().empty() && r.answer().front() == s.source() &&
                          r.answer().back() == s.goal();
            double sum = 0;
            for (std::size_t i = 1; i < r.answer().size(); ++i) {
                int a = r.answer()[i - 1], b = r.answer()[i];
                double weight = INF;
                for (const auto &edge : s.edges())
                    if (!edge.blocked() &&
                        ((edge.a() == a && edge.b() == b) || (edge.a() == b && edge.b() == a)))
                        weight = std::min(weight, edge.weight());
                sum += weight;
            }
            ok = ok && pathOk && std::isfinite(sum) && sameCost(sum, r.value());
        } else
            ok = ok && r.answer().empty();
        reason = "Compared cost with Bellman-Ford and checked returned path edges/cost.";
    } else if (r.mode() == Mode::ClosestPair) {
        double best = INF;
        for (std::size_t i = 0; i < s.points().size(); ++i)
            for (std::size_t j = i + 1; j < s.points().size(); ++j)
                best = std::min(best, G::distance(s.points()[i], s.points()[j]));
        ok = (!std::isfinite(best) && !std::isfinite(r.value())) ||
             std::abs(best - r.value()) < 1e-7;
        reason = "Compared with exhaustive all-pairs distance.";
    } else if (r.mode() == Mode::KDNearest || r.mode() == Mode::KDRadius) {
        std::vector<int> ids;
        double best = INF;
        int id = -1;
        for (std::size_t i = 0; i < s.points().size(); ++i) {
            double d = G::distance2(s.query(), s.points()[i]);
            if (d < best) {
                best = d;
                id = static_cast<int>(i);
            }
            if (d <= s.radius() * s.radius())
                ids.push_back(static_cast<int>(i));
        }
        if (r.mode() == Mode::KDNearest) {
            ids.clear();
            if (id >= 0)
                ids.push_back(id);
        }
        ok = ids == r.answer();
        reason = "Compared query with exhaustive scan.";
    } else if (r.mode() == Mode::DBSCAN || r.mode() == Mode::IndexedDBSCAN) {
        // Independent core-graph flood fill; border tie is first cluster in ID order.
        int n = static_cast<int>(s.points().size());
        std::vector<std::vector<int>> ns(n);
        std::vector<int> lab(n, -1), kind(n, 0);
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j)
                if (G::distance2(s.points()[i], s.points()[j]) <= s.radius() * s.radius())
                    ns[i].push_back(j);
            if (static_cast<int>(ns[i].size()) >= s.minPts())
                kind[i] = 2;
        }
        int c = 0;
        for (int i = 0; i < n; ++i)
            if (kind[i] == 2 && lab[i] < 0) {
                std::queue<int> q;
                q.push(i);
                lab[i] = c;
                while (!q.empty()) {
                    int u = q.front();
                    q.pop();
                    for (int v : ns[u])
                        if (kind[v] == 2 && lab[v] < 0) {
                            lab[v] = c;
                            q.push(v);
                        }
                }
                ++c;
            }
        for (int i = 0; i < n; ++i)
            if (kind[i] != 2) {
                int first = INT32_MAX;
                for (int j : ns[i])
                    if (kind[j] == 2)
                        first = std::min(first, lab[j]);
                if (first != INT32_MAX) {
                    lab[i] = first;
                    kind[i] = 1;
                }
            }
        ok = lab == r.labels() && kind == r.kind();
        reason = "Compared with independent core-component/border reference.";
    } else if (r.mode() == Mode::Hull) {
        auto &h = r.answer();
        if (h.empty())
            ok = s.points().empty();
        else if (h.size() == 1) {
            for (auto p : s.points())
                ok = ok && G::distance2(p, s.points()[h[0]]) == 0;
        } else if (h.size() == 2) {
            for (auto p : s.points())
                ok = ok && G::onSegment(p, s.points()[h[0]], s.points()[h[1]]);
        } else
            for (std::size_t i = 0; i < h.size(); ++i) {
                Point a = s.points()[h[i]], b = s.points()[h[(i + 1) % h.size()]],
                      c = s.points()[h[(i + 2) % h.size()]];
                ok = ok && G::orientation(a, b, c) > 0;
                for (auto p : s.points())
                    ok = ok && G::orientation(a, b, p) >= 0;
            }
        reason = "Checked convexity and containment of every input point.";
    } else if (r.mode() == Mode::Geofence) {
        int winding = 0;
        bool boundary = false;
        for (std::size_t i = 0; i < s.polygon().size(); ++i) {
            auto a = s.polygon()[i], b = s.polygon()[(i + 1) % s.polygon().size()];
            if (G::onSegment(s.query(), a, b))
                boundary = true;
            if (a.y() <= s.query().y() && b.y() > s.query().y() &&
                G::orientation(a, b, s.query()) > 0)
                ++winding;
            if (a.y() > s.query().y() && b.y() <= s.query().y() &&
                G::orientation(a, b, s.query()) < 0)
                --winding;
        }
        int expected = boundary ? 0 : (winding ? 1 : -1);
        ok = r.value() == expected;
        reason = "Compared ray parity with winding-number classification.";
    } else
        reason = "Collision predicate covered by independent expected-case tests (no separate "
                 "runtime oracle).";
    r.setChecked(ok);
    const std::string strength = r.mode() == Mode::Collision ? "TEST-COVERED (no runtime oracle)"
                                 : r.mode() == Mode::Hull    ? "INVARIANT-CHECKED"
                                                             : "REFERENCE-VERIFIED";
    r.setCheck((ok ? "PASS [" + strength + "]: " : "FAIL: ") + reason);
    return ok;
}
} // namespace gv
