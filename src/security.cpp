#include "security.hpp"
#include "algorithms.hpp"
#include "persistence.hpp"
#include "security_selection.hpp"
#include <ctime>
#include <sstream>
namespace gv {
// Local equirectangular projection: east and north offsets in metres about an explicit origin.
Point SecurityConsole::project(double lat, double lon, const SecuritySettings &s) {
    constexpr double pi = 3.14159265358979323846, R = 6371000;
    return {R * (lon - s.originLon()) * pi / 180 * std::cos(s.originLat() * pi / 180),
            R * (lat - s.originLat()) * pi / 180};
}
// Invert the same local model so map inspection can report latitude and longitude.
Point SecurityConsole::unproject(Point p, const SecuritySettings &s) {
    constexpr double pi = 3.14159265358979323846, R = 6371000;
    return {s.originLat() + p.y() / R * 180 / pi,
            s.originLon() + p.x() / (R * std::cos(s.originLat() * pi / 180)) * 180 / pi};
}
// Keep analyst judgement separate from sample truth and require a known source identity.
void SecurityConsole::setReview(const std::string &eventId, const std::string &verdict,
                                const std::string &note, std::int64_t reviewedAt) {
    if (note.size() > 1024 || reviewedAt < 0)
        throw std::invalid_argument("Invalid review note/time.");
    if (verdict != "benign" && verdict != "suspicious" && verdict != "unreviewed")
        throw std::invalid_argument("Unknown analyst verdict.");
    auto found = std::find_if(events_.begin(), events_.end(),
                              [&](const Event &event) { return event.id() == eventId; });
    if (found == events_.end())
        throw std::invalid_argument("Unknown event ID.");
    reviews_[eventId] = verdict;
    reviewNotes_[eventId] = note;
    reviewTimes_[eventId] = reviewedAt ? reviewedAt : static_cast<std::int64_t>(std::time(nullptr));
    changed();
}

// Validate the common window, compatible measurement settings, record identities and study extent.
void SecurityConsole::validate() const {
    auto queryValid = [](Point p) {
        return std::isfinite(p.x()) && std::isfinite(p.y()) && std::abs(p.x()) <= 1e7 &&
               std::abs(p.y()) <= 1e7;
    };
    if (!queryValid(settings_.geoQuery()) || !queryValid(settings_.featureQuery()))
        throw std::runtime_error("Invalid security query coordinates.");
    if (settings_.start() < 0 || settings_.end() <= settings_.start() ||
        settings_.end() - settings_.start() > 31LL * 86400)
        throw std::runtime_error("Security window must be positive and at most 31 days.");
    if (!std::isfinite(settings_.requestScale()) || !std::isfinite(settings_.failureScale()) ||
        settings_.requestScale() <= 0 || settings_.failureScale() <= 0 ||
        settings_.requestScale() > 1e9 || settings_.failureScale() > 1e9)
        throw std::runtime_error("Feature scales must be positive finite numbers <= 1e9.");
    if (!std::isfinite(settings_.featureRadius()) || !std::isfinite(settings_.geoRadius()) ||
        settings_.featureRadius() <= 0 || settings_.geoRadius() <= 0 ||
        settings_.geoRadius() > 100000 || settings_.featureRadius() > 1e7 ||
        settings_.minPts() < 1 || settings_.minPts() > 256)
        throw std::runtime_error("Invalid security radius/MinPts.");
    if (!std::isfinite(settings_.originLat()) || !std::isfinite(settings_.originLon()) ||
        std::abs(settings_.originLat()) > 80 || std::abs(settings_.originLon()) > 180)
        throw std::runtime_error("Invalid projection origin (latitude must be within +/-80).");
    if (events_.size() > 4096 || fence_.size() > MAX_POINTS)
        throw std::runtime_error("Security dataset exceeds limits.");
    if (!fence_.empty())
        GeometryKernel::validatePolygon(fence_);
    validateRecords();
    for (const auto &e : events_) {
        Point p = project(e.lat(), e.lon(), settings_);
        if (std::hypot(p.x(), p.y()) > 50000)
            throw std::runtime_error("Event is outside the supported 50 km local projection area.");
    }
}
void SecurityConsole::validateRecords() const {
    if (events_.size() > 4096)
        throw std::runtime_error("Too many source records.");
    std::set<std::string> ids;
    for (const auto &e : events_) {
        if (e.id().empty() || e.user().empty() || e.device().empty() || e.id().size() > 128 ||
            e.user().size() > 128 || e.device().size() > 128 || e.summary().size() > 512 ||
            !ids.insert(e.id()).second)
            throw std::runtime_error("Missing/oversized identity or duplicate event ID.");
        if (e.timestamp() < 0 || e.timestamp() > 9000000000000LL || !std::isfinite(e.lat()) ||
            !std::isfinite(e.lon()) || std::abs(e.lat()) > 90 || std::abs(e.lon()) > 180 ||
            e.requests() < 0 || e.failures() < 0 || e.failures() > e.requests() ||
            e.requests() > 1000000000)
            throw std::runtime_error("Invalid event values.");
        if (e.label() != "benign" && e.label() != "suspicious" && e.label() != "unknown")
            throw std::runtime_error("Event label must be benign, suspicious or unknown.");
    }
    for (const auto &review : reviews_)
        if (!ids.count(review.first) ||
            (review.second != "benign" && review.second != "suspicious" &&
             review.second != "unreviewed"))
            throw std::runtime_error("Invalid analyst review.");
}
// Filter first; then either project each record or aggregate per-device fixed-window rates.
std::vector<Observation> SecurityConsole::observations(bool feature) const {
    validate();
    auto cached = observationCache_.find(feature);
    if (cached != observationCache_.end())
        return cached->second;
    std::vector<Observation> out;
    if (!feature) {
        for (std::size_t i = 0; i < events_.size(); ++i)
            if (selected(events_[i], settings_))
                out.push_back({project(events_[i].lat(), events_[i].lon(), settings_),
                               events_[i].id(),
                               {static_cast<int>(i)},
                               0,
                               0});
    } else {
        std::map<std::string, Observation> groups;
        for (std::size_t i = 0; i < events_.size(); ++i)
            if (selected(events_[i], settings_)) {
                auto &o = groups[events_[i].device()];
                o.setName(events_[i].device());
                o.appendEvents(static_cast<int>(i));
                o.setRequestRate(o.requestRate() + (events_[i].requests()));
                o.setFailureRate(o.failureRate() + (events_[i].failures()));
            }
        double minutes = (settings_.end() - settings_.start()) / 60.0;
        for (auto &[key, o] : groups) {
            (void)key;
            o.setRequestRate(o.requestRate() / (minutes));
            o.setFailureRate(o.failureRate() / (minutes));
            o.setPoint({o.requestRate() / settings_.requestScale(),
                        o.failureRate() / settings_.failureScale()});
            out.push_back(o);
        }
    }
    if (out.size() > MAX_POINTS)
        throw std::runtime_error(
            "Filtered view exceeds 256 observations: narrow the window/user/device filters.");
    observationCache_[feature] = out;
    return out;
}
// Adapt security observations into the same model consumed by every geometry Strategy.
Scene SecurityConsole::scene(bool feature) const {
    auto obs = observations(feature);
    Scene s;
    for (auto &o : obs)
        s.addPoint(o.point());
    s.setRadius(feature ? settings_.featureRadius() : settings_.geoRadius());
    s.setMinPts(settings_.minPts());
    if (!feature)
        s.setPolygon(fence_);
    s.setGoal(s.points().empty() ? 0 : static_cast<int>(s.points().size() - 1));
    s.setQuery(feature ? settings_.featureQuery() : settings_.geoQuery());
    return s;
}
// Compare review candidates with event truth; feature-device flags propagate to their source
// events.
std::string SecurityConsole::evaluation() const {
    if (!evaluationCache_.empty())
        return evaluationCache_;
    auto obs = observations(true);
    Scene features = scene(true);
    Engine engine;
    auto result = engine.execute(features, Mode::IndexedDBSCAN, false);
    std::set<int> flagged;
    for (std::size_t i = 0; i < obs.size(); ++i)
        if (result.labels()[i] == -1)
            for (int id : obs[i].events())
                flagged.insert(id);
    if (!fence_.empty())
        for (std::size_t i = 0; i < events_.size(); ++i)
            if (selected(events_[i], settings_) &&
                GeometryKernel::membership(project(events_[i].lat(), events_[i].lon(), settings_),
                                           fence_) == -1)
                flagged.insert(static_cast<int>(i));
    int tp = 0, fp = 0, tn = 0, fn = 0, unknown = 0, reviewed = 0, reviewBenign = 0;
    for (std::size_t i = 0; i < events_.size(); ++i)
        if (selected(events_[i], settings_)) {
            const auto &e = events_[i];
            bool flag = flagged.count(static_cast<int>(i)) != 0;
            if (e.label() == "unknown")
                ++unknown;
            else if (flag) {
                if (e.label() == "suspicious")
                    ++tp;
                else
                    ++fp;
            } else {
                if (e.label() == "suspicious")
                    ++fn;
                else
                    ++tn;
            }
            auto it = reviews_.find(e.id());
            if (flag && it != reviews_.end() && it->second != "unreviewed") {
                ++reviewed;
                if (it->second == "benign")
                    ++reviewBenign;
            }
        }
    auto ratio = [](int a, int b) {
        return b ? number(static_cast<double>(a) / b) : std::string("N/A");
    };
    std::ostringstream out;
    out << "Review-candidate rule: feature DBSCAN noise OR strictly outside permitted polygon. "
           "Boundary is permitted.\n"
        << "Synthetic/sample labels are evaluation truth, never model inputs. Unit: selected "
           "source event (device flags propagate to its events).\n"
        << "TP=" << tp << " FP=" << fp << " TN=" << tn << " FN=" << fn << " unknown=" << unknown
        << "\n"
        << "Candidate precision=" << ratio(tp, tp + fp) << " recall=" << ratio(tp, tp + fn)
        << " false-positive rate=" << ratio(fp, fp + tn) << "\n"
        << "Flagged events=" << flagged.size() << " analyst-reviewed=" << reviewed
        << " reviewed benign=" << reviewBenign << "\n"
        << "A candidate is an investigation prompt, not an attack verdict. Small synthetic samples "
           "do not establish production accuracy.\n";
    evaluationCache_ = out.str();
    return evaluationCache_;
}

std::string SecurityConsole::sensitivity() const {
    std::ostringstream out;
    out << "SENSITIVITY (teaching comparison, not accuracy estimates)\n";
    const auto base = Engine{}.execute(scene(true), Mode::IndexedDBSCAN, false);
    for (double radiusFactor : {0.75, 1.0, 1.25})
        for (int delta : {-1, 0, 1}) {
            auto next = *this;
            auto params = settings_;
            params.setFeatureRadius(std::min(1e7, settings_.featureRadius() * radiusFactor));
            params.setMinPts(std::clamp(settings_.minPts() + delta, 1, 256));
            next.setSettings(params);
            auto result = Engine{}.execute(next.scene(true), Mode::IndexedDBSCAN, false);
            int changedCount = 0;
            for (std::size_t i = 0; i < base.labels().size(); ++i)
                if ((base.labels()[i] == -1) != (result.labels()[i] == -1))
                    ++changedCount;
            out << "radius=" << params.featureRadius() << " MinPts=" << params.minPts()
                << " noise=" << std::count(result.labels().begin(), result.labels().end(), -1)
                << " changed_noise_status=" << changedCount << '\n';
        }
    for (double scaleFactor : {0.75, 1.25}) {
        auto next = *this;
        auto params = settings_;
        params.setRequestScale(std::min(1e9, settings_.requestScale() * scaleFactor));
        params.setFailureScale(std::min(1e9, settings_.failureScale() * scaleFactor));
        next.setSettings(params);
        try {
            auto result = Engine{}.execute(next.scene(true), Mode::IndexedDBSCAN, false);
            out << "both scale factor=" << scaleFactor
                << " noise=" << std::count(result.labels().begin(), result.labels().end(), -1)
                << '\n';
        } catch (const std::exception &e) {
            out << "both scale factor=" << scaleFactor << " unsupported: " << e.what() << '\n';
        }
    }
    return out.str();
}
} // namespace gv
