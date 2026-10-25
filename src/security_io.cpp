#include "evidence_json.hpp"
#include "io_detail.hpp"
#ifndef GEOVIZ_SOURCE_REVISION
#define GEOVIZ_SOURCE_REVISION "local-unversioned"
#endif
namespace gv {
using namespace io_detail;
// Snapshot source data, filters, metric scales, queries, geofence and analyst reviews together.
void SecurityConsole::save(const std::string &path, AtomicFiles *external) const {
    validate();
    AtomicFiles localTransaction;
    auto &transaction = external ? *external : localTransaction;
    auto out = openOutput(transaction.stage(path));
    const auto &s = settings_;
    out << "GEOVIZ_SECURITY 2\n"
        << s.start() << ' ' << s.end() << ' ' << std::quoted(s.user()) << ' '
        << std::quoted(s.device()) << '\n'
        << s.requestScale() << ' ' << s.failureScale() << ' ' << s.featureRadius() << ' '
        << s.geoRadius() << ' ' << s.minPts() << ' ' << s.originLat() << ' ' << s.originLon() << ' '
        << s.geoQuery().x() << ' ' << s.geoQuery().y() << ' ' << s.featureQuery().x() << ' '
        << s.featureQuery().y() << '\n';
    out << fence_.size() << '\n';
    for (auto p : fence_)
        out << p.x() << ' ' << p.y() << '\n';
    out << events_.size() << '\n';
    for (const auto &e : events_)
        out << std::quoted(e.id()) << ' ' << e.timestamp() << ' ' << std::quoted(e.user()) << ' '
            << std::quoted(e.device()) << ' ' << e.lat() << ' ' << e.lon() << ' ' << e.requests()
            << ' ' << e.failures() << ' ' << std::quoted(e.label()) << ' '
            << std::quoted(e.summary()) << '\n';
    out << reviews_.size() << '\n';
    for (auto &[id, verdict] : reviews_)
        out << std::quoted(id) << ' ' << std::quoted(verdict) << ' '
            << std::quoted(reviewNotes_.count(id) ? reviewNotes_.at(id) : "") << ' '
            << (reviewTimes_.count(id) ? reviewTimes_.at(id) : 0) << '\n';
    if (!out)
        throw std::runtime_error("Security snapshot write failed.");
    AtomicFiles::close(out);
    if (!external)
        transaction.commit();
}
// Restore into a temporary console; publish only after complete schema and domain validation.
SecurityConsole SecurityConsole::load(const std::string &path) {
    auto in = openInput(path);
    SecurityConsole c;
    auto &s = c.settings_;
    std::string magic;
    int version = 0;
    in >> magic >> version;
    if (magic != "GEOVIZ_SECURITY" || (version != 1 && version != 2))
        throw std::runtime_error("Unsupported security snapshot/version.");
    s.setStart(readValue<std::int64_t>(in));
    s.setEnd(readValue<std::int64_t>(in));
    s.setUser(readQuoted(in));
    s.setDevice(readQuoted(in));
    s.setRequestScale(readValue<double>(in));
    s.setFailureScale(readValue<double>(in));
    s.setFeatureRadius(readValue<double>(in));
    s.setGeoRadius(readValue<double>(in));
    s.setMinPts(readValue<int>(in));
    s.setOriginLat(readValue<double>(in));
    s.setOriginLon(readValue<double>(in));
    s.setGeoQuery(readPoint(in));
    s.setFeatureQuery(readPoint(in));
    c.fence_.resize(count(in, MAX_POINTS));
    for (auto &p : c.fence_)
        p = readPoint(in);
    c.events_.resize(count(in, 4096));
    for (auto &e : c.events_) {
        e.setId(readQuoted(in));
        e.setTimestamp(readValue<std::int64_t>(in));
        e.setUser(readQuoted(in));
        e.setDevice(readQuoted(in));
        e.setLat(readValue<double>(in));
        e.setLon(readValue<double>(in));
        e.setRequests(readValue<int>(in));
        e.setFailures(readValue<int>(in));
        e.setLabel(readQuoted(in));
        e.setSummary(readQuoted(in));
    }
    auto n = count(in, 4096);
    for (std::size_t i = 0; i < n; ++i) {
        std::string id, v;
        in >> std::quoted(id) >> std::quoted(v);
        if (c.reviews_.count(id))
            throw std::runtime_error("Duplicate review.");
        c.reviews_[id] = v;
        if (version >= 2) {
            c.reviewNotes_[id] = readQuoted(in);
            c.reviewTimes_[id] = readValue<std::int64_t>(in);
            if (c.reviewNotes_[id].size() > 1024 || c.reviewTimes_[id] < 0)
                throw std::runtime_error("Invalid review note/time.");
        }
    }
    endInput(in);
    c.validate();
    return c;
}
// Export both unit systems, all source links, reference checks and a reproducible full snapshot.
void SecurityConsole::evidence(const std::string &path, AtomicFiles *external) const {
    validate();
    const auto geographicObservations = observations(false),
               featureObservations = observations(true);
    const Scene geographicScene = scene(false), featureScene = scene(true);
    const auto evaluationText = evaluation();
    AtomicFiles localTransaction;
    auto &transaction = external ? *external : localTransaction;
    auto out = openOutput(transaction.stage(path));
    auto &s = settings_;
    out << "SPATIAL SECURITY EVIDENCE\nUTC epoch window [start,end): " << s.start() << ' '
        << s.end() << "\nUser=" << s.user() << " device=" << s.device()
        << "\nProjection: local equirectangular, R=6371000 metres; origin lat/lon=" << s.originLat()
        << ' ' << s.originLon()
        << "\nSupported radius from origin: 50 km; latitude +/-80; approximate local distances.\n"
        << "Feature x=(sum requests/window minutes)/" << s.requestScale()
        << "; y=(sum failures/window minutes)/" << s.failureScale()
        << "\nFeature radius=" << s.featureRadius() << " geographic radius(m)=" << s.geoRadius()
        << " MinPts(including self)=" << s.minPts()
        << "\nStable order: geographic input CSV order; feature device lexicographic order.\n"
        << evaluationText;
    Engine e;
    for (bool feature : {false, true}) {
        const auto &obs = feature ? featureObservations : geographicObservations;
        const Scene &input = feature ? featureScene : geographicScene;
        auto scan = e.execute(input, Mode::DBSCAN, false),
             indexed = e.execute(input, Mode::IndexedDBSCAN, false);
        auto nearest = e.execute(input, Mode::KDNearest, false),
             nearby = e.execute(input, Mode::KDRadius, false);
        out << "\nVIEW " << (feature ? "FEATURE (scaled rates)" : "GEOGRAPHIC (metres)") << "\n"
            << indexed.check() << "\nBaseline/indexed labels agree="
            << (scan.labels() == indexed.labels() && scan.kind() == indexed.kind())
            << "\nScan distance tests=" << scan.stats().operations()
            << " indexed distance tests=" << indexed.stats().operations()
            << " (index build and verification excluded).\n";
        out << "Query x=" << input.query().x() << " y=" << input.query().y()
            << " nearest distance=" << number(nearest.value(), 12) << " nearest IDs:";
        for (int id : nearest.answer())
            out << ' ' << id;
        out << "\nRadius matches:";
        for (int id : nearby.answer())
            out << ' ' << id;
        out << "\n" << nearest.check() << "\n" << nearby.check() << "\n";
        if (!feature && !input.polygon().empty())
            out << "Query permitted-region classification="
                << GeometryKernel::membership(input.query(), input.polygon()) << "\n";
        for (std::size_t i = 0; i < obs.size(); ++i) {
            auto &o = obs[i];
            out << "Point " << i << ' ' << o.name() << " x=" << o.point().x()
                << " y=" << o.point().y() << " cluster=" << indexed.labels()[i]
                << " kind=" << indexed.kind()[i];
            if (!feature && !fence_.empty())
                out << " geofence=" << GeometryKernel::membership(o.point(), fence_);
            out << "\n";
            for (int id : o.events()) {
                auto &event = events_[id];
                out << "  source=" << event.id() << " utc=" << event.timestamp()
                    << " user=" << event.user() << " device=" << event.device()
                    << " requests=" << event.requests() << " failures=" << event.failures()
                    << " label=" << event.label() << " review="
                    << (reviews_.count(event.id()) ? reviews_.at(event.id()) : "unreviewed")
                    << " summary=" << event.summary() << "\n";
            }
        }
        SceneIO::save(input, path + (feature ? ".feature.scene" : ".geo.scene"), &transaction);
    }
    if (!out)
        throw std::runtime_error("Evidence write failed.");
    save(path + ".security", &transaction);
    std::set<int> featureNoiseEvents;
    auto featureResult = e.execute(featureScene, Mode::IndexedDBSCAN, false);
    for (std::size_t i = 0; i < featureObservations.size(); ++i)
        if (featureResult.labels()[i] == -1)
            for (int id : featureObservations[i].events())
                featureNoiseEvents.insert(id);
    auto structured = openOutput(transaction.stage(path + ".json"));
    std::ostringstream canonical;
    canonical << std::setprecision(17);
    for (const auto &event : events_)
        canonical << std::quoted(event.id()) << event.timestamp() << std::quoted(event.user())
                  << std::quoted(event.device()) << event.lat() << ',' << event.lon() << ','
                  << event.requests() << ',' << event.failures() << std::quoted(event.label())
                  << std::quoted(event.summary());
    structured
        << "{\"schema\":\"geoviz.evidence.v1\",\"app_version\":\"2.0.0\",\"source_revision\":"
        << jsonText(GEOVIZ_SOURCE_REVISION)
        << ",\"input_hash\":" << jsonText(consistencyHash(canonical.str()))
        << ",\"hash_scope\":\"canonical source records, consistency only; "
           "unsigned\",\"parameters\":{\"start_utc\":"
        << s.start() << ",\"end_utc\":" << s.end() << ",\"user\":" << jsonText(s.user())
        << ",\"device\":" << jsonText(s.device()) << ",\"origin_lat\":" << s.originLat()
        << ",\"origin_lon\":" << s.originLon() << ",\"request_scale\":" << s.requestScale()
        << ",\"failure_scale\":" << s.failureScale() << ",\"feature_radius\":" << s.featureRadius()
        << ",\"geo_radius_m\":" << s.geoRadius() << ",\"min_pts\":" << s.minPts()
        << "},\"views\":[";
    for (int view = 0; view < 2; ++view) {
        bool feature = view == 1;
        auto input = scene(feature);
        auto obs = observations(feature);
        auto result = e.execute(input, Mode::IndexedDBSCAN, false);
        if (view)
            structured << ',';
        structured << "{\"units\":"
                   << jsonText(feature ? "scaled requests/failures per minute" : "metres")
                   << ",\"algorithm\":\"Indexed DBSCAN\",\"check\":" << jsonText(result.check())
                   << ",\"observations\":[";
        for (std::size_t i = 0; i < obs.size(); ++i) {
            if (i)
                structured << ',';
            bool outside = false;
            for (int id : obs[i].events())
                if (!fence_.empty() &&
                    GeometryKernel::membership(project(events_[id].lat(), events_[id].lon(), s),
                                               fence_) < 0)
                    outside = true;
            structured << "{\"id\":" << i << ",\"name\":" << jsonText(obs[i].name())
                       << ",\"x\":" << obs[i].point().x() << ",\"y\":" << obs[i].point().y()
                       << ",\"cluster\":" << result.labels()[i]
                       << ",\"noise\":" << (result.labels()[i] == -1 ? "true" : "false")
                       << ",\"outside\":" << (outside ? "true" : "false") << ",\"sources\":[";
            for (std::size_t j = 0; j < obs[i].events().size(); ++j) {
                if (j)
                    structured << ',';
                int sourceIndex = obs[i].events()[j];
                auto &event = events_[sourceIndex];
                bool featureNoise = featureNoiseEvents.count(sourceIndex) != 0;
                bool sourceOutside =
                    !fence_.empty() &&
                    GeometryKernel::membership(project(event.lat(), event.lon(), s), fence_) < 0;
                auto id = event.id();
                structured << "{\"event_id\":" << jsonText(id)
                           << ",\"truth\":" << jsonText(event.label()) << ",\"candidate\":"
                           << ((featureNoise || sourceOutside) ? "true" : "false")
                           << ",\"feature_noise\":" << (featureNoise ? "true" : "false")
                           << ",\"outside_region\":" << (sourceOutside ? "true" : "false")
                           << ",\"review\":"
                           << jsonText(reviews_.count(id) ? reviews_.at(id) : "unreviewed")
                           << ",\"review_note\":"
                           << jsonText(reviewNotes_.count(id) ? reviewNotes_.at(id) : "")
                           << ",\"reviewed_utc\":"
                           << (reviewTimes_.count(id) ? reviewTimes_.at(id) : 0) << '}';
            }
            structured << "]}";
        }
        structured << "]}";
    }
    structured << "],\"reproduce\":\"Load adjacent .security snapshot; feature noise OR outside is "
                  "the event candidate rule. Geographic noise is a separate view result.\"}\n";
    AtomicFiles::close(structured);
    auto sensitivityOut = openOutput(transaction.stage(path + ".sensitivity.txt"));
    sensitivityOut << sensitivity();
    AtomicFiles::close(sensitivityOut);
    AtomicFiles::close(out);
    if (!external)
        transaction.commit();
}

} // namespace gv
