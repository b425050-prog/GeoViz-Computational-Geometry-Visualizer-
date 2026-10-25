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

#include "scene.hpp"

namespace gv {
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
std::string number(double x, int precision = 3);
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
    int pseudocodeLine_ = 0;
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
    int pseudocodeLine() const {
        return pseudocodeLine_;
    }
    void setPseudocodeLine(int value) {
        pseudocodeLine_ = value;
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
    void seek(std::size_t frame, std::size_t count) {
        cursor_ = count ? std::min(frame, count - 1) : 0;
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

} // namespace gv
