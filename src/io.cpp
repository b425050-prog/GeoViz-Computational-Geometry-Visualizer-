#include "geoviz.hpp"
#include <fstream>
#include <iomanip>
#include <sstream>
namespace gv {
namespace {
template <class T> T readValue(std::istream &in) {
    T value{};
    in >> value;
    if (!in)
        throw std::runtime_error("Malformed or incomplete file.");
    return value;
}
std::string readQuoted(std::istream &in) {
    std::string value;
    in >> std::quoted(value);
    if (!in)
        throw std::runtime_error("Malformed quoted value.");
    return value;
}
Point readPoint(std::istream &in) {
    const double x = readValue<double>(in);
    const double y = readValue<double>(in);
    return {x, y};
}
Edge readEdge(std::istream &in) {
    const int a = readValue<int>(in), b = readValue<int>(in);
    const double weight = readValue<double>(in);
    const bool blocked = readValue<bool>(in);
    return {a, b, weight, blocked};
}
std::ifstream openInput(const std::string &path) {
    std::ifstream in(path, std::ios::binary);
    if (!in)
        throw std::runtime_error("Cannot open: " + path);
    in.seekg(0, std::ios::end);
    if (in.tellg() > 8 * 1024 * 1024)
        throw std::runtime_error("Input exceeds 8 MiB limit.");
    in.seekg(0);
    return in;
}
std::ofstream openOutput(const std::string &path) {
    std::ofstream out(path);
    if (!out)
        throw std::runtime_error("Cannot write: " + path);
    out << std::setprecision(17);
    return out;
}
std::size_t count(std::istream &in, std::size_t maximum) {
    long long n = -1;
    in >> n;
    if (!in || n < 0 || static_cast<unsigned long long>(n) > maximum)
        throw std::runtime_error("Invalid or oversized record count.");
    return static_cast<std::size_t>(n);
}
void endInput(std::istream &in) {
    if (!in)
        throw std::runtime_error("Malformed or incomplete file.");
    in >> std::ws;
    if (!in.eof())
        throw std::runtime_error("Unexpected trailing file content.");
}
bool selected(const Event &e, const SecuritySettings &s) {
    return e.timestamp() >= s.start() && e.timestamp() < s.end() &&
           (s.user() == "*" || s.user() == e.user()) &&
           (s.device() == "*" || s.device() == e.device());
}
double asDouble(const std::string &text) {
    std::size_t n = 0;
    double v = std::stod(text, &n);
    if (n != text.size() || !std::isfinite(v))
        throw std::runtime_error("Invalid numeric CSV value: " + text);
    return v;
}
// Snapshots preserve editable drafts. Polygon validity belongs to algorithms
// that consume a completed polygon, while structural checks apply to both I/O paths.
void validateSnapshot(const Scene &s) {
    Engine::validate(s);
    if (!s.points().empty() &&
        (s.source() < 0 || s.goal() < 0 || s.source() >= static_cast<int>(s.points().size()) ||
         s.goal() >= static_cast<int>(s.points().size())))
        throw std::runtime_error("Saved source/goal index is invalid.");
}
std::int64_t asInteger(const std::string &text) {
    std::size_t n = 0;
    auto v = std::stoll(text, &n);
    if (n != text.size())
        throw std::runtime_error("Invalid integer CSV value: " + text);
    return v;
}
} // namespace
// Write a versioned input snapshot using full-precision doubles and checked counts.
void SceneIO::save(const Scene &s, const std::string &path) {
    validateSnapshot(s);
    auto out = openOutput(path);
    out << "GEOVIZ 1\n" << s.points().size() << '\n';
    for (auto p : s.points())
        out << p.x() << ' ' << p.y() << '\n';
    out << s.polygon().size() << '\n';
    for (auto p : s.polygon())
        out << p.x() << ' ' << p.y() << '\n';
    out << s.edges().size() << '\n';
    for (auto e : s.edges())
        out << e.a() << ' ' << e.b() << ' ' << e.weight() << ' ' << e.blocked() << '\n';
    out << s.query().x() << ' ' << s.query().y() << ' ' << s.radius() << ' ' << s.minPts() << ' '
        << s.source() << ' ' << s.goal() << ' ' << s.grid() << '\n';
    if (!out)
        throw std::runtime_error("Scene write failed.");
}
// Parse into a local Scene, reject trailing content, then validate snapshot structure.
Scene SceneIO::load(const std::string &path) {
    auto in = openInput(path);
    std::string magic;
    int version = 0;
    in >> magic >> version;
    if (magic != "GEOVIZ" || version != 1)
        throw std::runtime_error("Unsupported scene format/version.");
    Scene s;
    auto n = count(in, MAX_POINTS);
    for (std::size_t i = 0; i < n; ++i)
        s.addPoint(readPoint(in));
    n = count(in, MAX_POINTS);
    for (std::size_t i = 0; i < n; ++i)
        s.addPolygonVertex(readPoint(in));
    n = count(in, MAX_EDGES);
    for (std::size_t i = 0; i < n; ++i)
        s.addEdge(readEdge(in));
    s.setQuery(readPoint(in));
    s.setRadius(readValue<double>(in));
    s.setMinPts(readValue<int>(in));
    s.setSource(readValue<int>(in));
    s.setGoal(readValue<int>(in));
    s.setGrid(readValue<bool>(in));
    endInput(in);
    validateSnapshot(s);
    return s;
}
// Pair a readable checked result with the exact input required to reproduce it.
void SceneIO::summary(const Run &run, const std::string &path) {
    auto out = openOutput(path);
    auto &r = run.result();
    out << "GEOVIZ CHECKED RUN\nAlgorithm: " << modeName(r.mode()) << "\nResult: " << r.summary()
        << "\n"
        << r.check() << "\nOperations: " << r.stats().operations()
        << "\nRange queries: " << r.stats().queries()
        << "\nVisited/expanded: " << r.stats().expanded()
        << "\nScene revision: " << run.input().revision() << "\nRadius: " << run.input().radius()
        << "\nMinPts: " << run.input().minPts() << "\nTrace frames: " << r.trace().frames().size()
        << "\nTrace truncated: " << r.trace().truncated()
        << "\nTie policy: ascending input index; heap (f,g,index); strict coordinate "
           "sort.\nTolerance: 1e-9 scale-aware orientation; exact radius inclusion.\nValue: "
        << number(r.value(), 12) << "\nAnswer indices:";
    for (int id : r.answer())
        out << ' ' << id;
    out << "\nCluster labels:";
    for (int label : r.labels())
        out << ' ' << label;
    out << "\nKinds (0 noise,1 border,2 core):";
    for (int kind : r.kind())
        out << ' ' << kind;
    out << "\nReproduce: load the adjacent .scene snapshot, select this algorithm, and Run.\n";
    if (!out)
        throw std::runtime_error("Summary write failed.");
    SceneIO::save(run.input(), path + ".scene");
}
// A small state machine handles quoted commas and doubled quotes; multiline fields are rejected.
std::vector<std::string> parseCsv(const std::string &line) {
    std::vector<std::string> fields;
    std::string field;
    bool quoted = false, closed = false;
    for (std::size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (quoted) {
            if (c == '"') {
                if (i + 1 < line.size() && line[i + 1] == '"') {
                    field += '"';
                    ++i;
                } else {
                    quoted = false;
                    closed = true;
                }
            } else
                field += c;
        } else if (c == ',') {
            fields.push_back(field);
            field.clear();
            closed = false;
        } else if (c == '"') {
            if (!field.empty() || closed)
                throw std::runtime_error("Malformed CSV quote.");
            quoted = true;
        } else {
            if (closed)
                throw std::runtime_error("Characters after closing CSV quote.");
            field += c;
        }
    }
    if (quoted)
        throw std::runtime_error("Unterminated CSV quote (multiline fields are unsupported).");
    fields.push_back(field);
    return fields;
}
// Double embedded quotes before surrounding a field with CSV quote delimiters.
std::string csvQuote(const std::string &s) {
    std::string out = "\"";
    for (char c : s) {
        if (c == '"')
            out += '"';
        out += c;
    }
    return out + '"';
}
// Require the exact schema, check typed values, and attach row numbers to parsing errors.
std::vector<Event> CsvLogAdapter::load(const std::string &path) const {
    auto in = openInput(path);
    std::string line;
    std::getline(in, line);
    if (!line.empty() && line.back() == '\r')
        line.pop_back();
    if (line !=
        "event_id,timestamp,user_id,device_id,latitude,longitude,requests,failures,label,summary")
        throw std::runtime_error("CSV header does not match the documented event schema.");
    std::vector<Event> events;
    int row = 1;
    while (std::getline(in, line)) {
        ++row;
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (line.empty())
            continue;
        try {
            auto f = parseCsv(line);
            if (f.size() != 10)
                throw std::runtime_error("Expected 10 CSV columns.");
            auto req = asInteger(f[6]), fail = asInteger(f[7]);
            if (req < 0 || fail < 0 || fail > req || req > 1000000000)
                throw std::runtime_error("Invalid request/failure counts.");
            Event e;
            e.setId(f[0]);
            e.setTimestamp(asInteger(f[1]));
            e.setUser(f[2]);
            e.setDevice(f[3]);
            e.setLat(asDouble(f[4]));
            e.setLon(asDouble(f[5]));
            e.setRequests(static_cast<int>(req));
            e.setFailures(static_cast<int>(fail));
            e.setLabel(f[8]);
            e.setSummary(f[9]);
            events.push_back(e);
            if (events.size() > 4096)
                throw std::runtime_error("At most 4096 source records are supported.");
        } catch (const std::exception &e) {
            throw std::runtime_error("CSV row " + std::to_string(row) + ": " + e.what());
        }
    }
    SecurityConsole check;
    check.setEvents(events);
    check.validate();
    return events;
}
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
void SecurityConsole::setReview(const std::string &eventId, const std::string &verdict) {
    if (verdict != "benign" && verdict != "suspicious" && verdict != "unreviewed")
        throw std::invalid_argument("Unknown analyst verdict.");
    auto found = std::find_if(events_.begin(), events_.end(),
                              [&](const Event &event) { return event.id() == eventId; });
    if (found == events_.end())
        throw std::invalid_argument("Unknown event ID.");
    reviews_[eventId] = verdict;
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
        Point p = project(e.lat(), e.lon(), settings_);
        if (std::hypot(p.x(), p.y()) > 50000)
            throw std::runtime_error("Event is outside the supported 50 km local projection area.");
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
// Snapshot source data, filters, metric scales, queries, geofence and analyst reviews together.
void SecurityConsole::save(const std::string &path) const {
    validate();
    auto out = openOutput(path);
    const auto &s = settings_;
    out << "GEOVIZ_SECURITY 1\n"
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
        out << std::quoted(id) << ' ' << std::quoted(verdict) << '\n';
    if (!out)
        throw std::runtime_error("Security snapshot write failed.");
}
// Restore into a temporary console; publish only after complete schema and domain validation.
SecurityConsole SecurityConsole::load(const std::string &path) {
    auto in = openInput(path);
    SecurityConsole c;
    auto &s = c.settings();
    std::string magic;
    int version = 0;
    in >> magic >> version;
    if (magic != "GEOVIZ_SECURITY" || version != 1)
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
    }
    endInput(in);
    c.validate();
    return c;
}
// Compare review candidates with event truth; feature-device flags propagate to their source
// events.
std::string SecurityConsole::evaluation() const {
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
    return out.str();
}
// Export both unit systems, all source links, reference checks and a reproducible full snapshot.
void SecurityConsole::evidence(const std::string &path) const {
    validate();
    const auto geographicObservations = observations(false),
               featureObservations = observations(true);
    const Scene geographicScene = scene(false), featureScene = scene(true);
    const auto evaluationText = evaluation();
    auto out = openOutput(path);
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
        SceneIO::save(input, path + (feature ? ".feature.scene" : ".geo.scene"));
    }
    if (!out)
        throw std::runtime_error("Evidence write failed.");
    save(path + ".security");
}
// Project named geographic paths with explicit vertex and local-extent limits.
std::vector<MapLine> SecurityConsole::loadMap(const std::string &path) const {
    auto in = openInput(path);
    std::string line;
    std::getline(in, line);
    if (!line.empty() && line.back() == '\r')
        line.pop_back();
    if (line != "name,latitude,longitude")
        throw std::runtime_error("Invalid map CSV header.");
    std::vector<MapLine> lines;
    std::map<std::string, std::size_t> indices;
    int total = 0;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (line.empty())
            continue;
        auto f = parseCsv(line);
        if (f.size() != 3)
            throw std::runtime_error("Invalid map row.");
        double lat = asDouble(f[1]), lon = asDouble(f[2]);
        if (std::abs(lat) > 90 || std::abs(lon) > 180)
            throw std::runtime_error("Map latitude/longitude out of range.");
        Point p = project(lat, lon, settings_);
        if (std::hypot(p.x(), p.y()) > 50000)
            throw std::runtime_error("Map is outside local projection area.");
        if (!indices.count(f[0])) {
            indices[f[0]] = lines.size();
            lines.push_back({f[0], {}});
        }
        lines[indices[f[0]]].appendPoints(p);
        if (++total > 4096)
            throw std::runtime_error("Map exceeds 4096 vertices.");
    }
    return lines;
}
} // namespace gv
