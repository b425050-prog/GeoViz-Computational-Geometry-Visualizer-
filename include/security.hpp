#pragma once
#include "atomic_files.hpp"
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

#include "geometry.hpp"
#include "scene.hpp"

namespace gv {
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
    std::map<std::string, std::string> reviewNotes_;
    std::map<std::string, std::int64_t> reviewTimes_;
    std::uint64_t revision_ = 0;
    mutable std::map<bool, std::vector<Observation>> observationCache_;
    mutable std::string evaluationCache_;
    void changed() {
        ++revision_;
        observationCache_.clear();
        evaluationCache_.clear();
    }

  public:
    const std::vector<Event> &events() const noexcept {
        return events_;
    }
    void setEvents(std::vector<Event> value) {
        auto candidate = *this;
        candidate.events_ = std::move(value);
        candidate.clearReviews();
        candidate.validate();
        *this = std::move(candidate);
        changed();
    }
    const SecuritySettings &settings() const noexcept {
        return settings_;
    }
    void setSettings(SecuritySettings value) {
        auto candidate = *this;
        candidate.settings_ = std::move(value);
        candidate.validate();
        *this = std::move(candidate);
        changed();
    }
    void updateSettings(const std::function<void(SecuritySettings &)> &edit) {
        auto candidate = settings_;
        edit(candidate);
        setSettings(std::move(candidate));
    }
    void replace(std::vector<Event> events, SecuritySettings settings, std::vector<Point> fence) {
        SecurityConsole candidate;
        candidate.events_ = std::move(events);
        candidate.settings_ = std::move(settings);
        candidate.fence_ = std::move(fence);
        candidate.validate();
        *this = std::move(candidate);
        changed();
    }
    std::uint64_t revision() const noexcept {
        return revision_;
    }
    const std::vector<Point> &fence() const noexcept {
        return fence_;
    }
    void setFence(std::vector<Point> value) {
        if (!value.empty())
            GeometryKernel::validatePolygon(value);
        fence_ = std::move(value);
        changed();
    }
    const std::map<std::string, std::string> &reviews() const noexcept {
        return reviews_;
    }
    static Point project(double lat, double lon, const SecuritySettings &settings);
    static Point unproject(Point p, const SecuritySettings &settings);
    void validate() const;
    void validateRecords() const;
    static void checkRecords(const std::vector<Event> &events) {
        SecurityConsole candidate;
        candidate.events_ = events;
        candidate.validateRecords();
    }
    std::vector<Observation> observations(bool feature) const;
    Scene scene(bool feature) const;
    void save(const std::string &path, AtomicFiles *external = nullptr) const;
    static SecurityConsole load(const std::string &path);
    std::string evaluation() const;
    void evidence(const std::string &path, AtomicFiles *external = nullptr) const;
    std::vector<MapLine> loadMap(const std::string &path) const;
    void clearReviews() {
        reviews_.clear();
        reviewNotes_.clear();
        reviewTimes_.clear();
        changed();
    }
    const std::map<std::string, std::string> &reviewNotes() const {
        return reviewNotes_;
    }
    const std::map<std::string, std::int64_t> &reviewTimes() const {
        return reviewTimes_;
    }
    void setReview(const std::string &eventId, const std::string &verdict,
                   const std::string &note = "", std::int64_t reviewedAt = 0);
    std::string sensitivity() const;
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

} // namespace gv
