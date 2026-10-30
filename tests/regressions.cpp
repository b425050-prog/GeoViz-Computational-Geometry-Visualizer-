#include "atomic_files.hpp"
#include "evidence_json.hpp"
#include "geoviz.hpp"
#include "numeric_text.hpp"
#include "workspace.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
using namespace gv;
namespace {
int checks = 0;
void expect(bool value, const std::string &message) {
    ++checks;
    if (!value)
        throw std::runtime_error(message);
}
template <class F> void rejects(F action, const std::string &message) {
    bool caught = false;
    try {
        action();
    } catch (const std::exception &) {
        caught = true;
    }
    expect(caught, message);
}
std::string bytes(const std::filesystem::path &p) {
    std::ifstream in(p, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), {}};
}
void put(const std::filesystem::path &p, const std::string &value) {
    std::ofstream out(p, std::ios::binary);
    out << value;
}
void numerical() {
    for (double offset : {0.0, 9999999.0, -9999999.0}) {
        std::vector<Point> p = {{offset, offset},
                                {offset + 0.01, offset},
                                {offset + 0.01, offset + 0.01},
                                {offset, offset + 0.01}};
        GeometryKernel::validatePolygon(p);
        expect(GeometryKernel::membership({offset + 0.005, offset + 0.005}, p) == 1,
               "translated polygon center");
        expect(GeometryKernel::membership(p.front(), p) == 0, "translated boundary");
        std::reverse(p.begin(), p.end());
        GeometryKernel::validatePolygon(p);
        expect(GeometryKernel::membership({offset + 0.02, offset + 0.02}, p) == -1,
               "reverse/translation membership");
    }
    for (double value : {1e-10, 1.234567891234567, 1e-200, 9999999.999999})
        expect(std::stod(exactNumber(value)) == value, "round-trip edit formatting");
    for (std::int64_t value : {0LL, 1800000000LL, 4102444800LL, 9000000000000LL})
        expect(utcEpoch(utcText(value)) == value, "UTC round trip");
    expect(number(1e-10).find("e-") != std::string::npos,
           "tiny nonzero values remain visible in inspector");
    rejects([] { utcEpoch("2026-02-30 12:00:00"); }, "invalid calendar date");
    rejects([] { utcEpoch("2026-01-01 24:00:00"); }, "invalid clock time");
    for (double offset : {0.0, 100000.0}) {
        Scene scene;
        scene.setPoints({{offset, offset},
                         {offset + 2, offset},
                         {offset - 2, offset},
                         {offset + 1, offset + 1e-10}});
        scene.setQuery({offset, offset});
        auto nearest = Engine{}.execute(scene, Mode::KDNearest, false);
        expect(nearest.answer() == std::vector<int>{0}, "nearest identity under translation");
        expect(Engine{}.execute(scene, Mode::ClosestPair, false).checked(),
               "near-collinear closest-pair reference");
    }
}
void routing() {
    for (double scale : {1e-6, 1.0, 1e6, 1e12}) {
        Scene scene;
        scene.setPoints({{0, 0}, {-4e-10 * scale, 0}, {4e-10 * scale, 0}});
        scene.setGoal(2);
        scene.setEdges({{0, 2, 5e-10 * scale, false}, {0, 1, 0, false}, {1, 2, 0, false}});
        auto a = Engine{}.execute(scene, Mode::AStar),
             d = Engine{}.execute(scene, Mode::Dijkstra, false);
        expect(a.value() == 0 && d.value() == 0, "zero-cost optimum across scales");
        expect(a.answer() == std::vector<int>({0, 1, 2}), "zero-cost path identities");
        expect(a.checked(), "small-scale reference");
        expect(a.trace().frames().front().pseudocodeLine() == 1 &&
                   a.trace().frames().back().pseudocodeLine() == 4,
               "trace retains pseudocode line metadata");
        Result wrong = a;
        wrong.setValue(5e-10 * scale);
        wrong.setAnswer({0, 2});
        expect(!Engine::verify(scene, wrong), "oracle rejects positive cost versus zero");
        auto noTrace = Engine{}.execute(scene, Mode::AStar, false);
        expect(noTrace.answer() == a.answer() &&
                   noTrace.stats().operations() == a.stats().operations() &&
                   noTrace.trace().frames().empty(),
               "recording-independent route contract");
    }
    Scene capped;
    for (int i = 0; i < 256; ++i)
        capped.addPoint({double(i), 0});
    std::vector<Edge> edges;
    for (int i = 0; i < 16; ++i)
        edges.emplace_back(i, i + 1, 1, false);
    for (int i = 0; i < 17; ++i)
        for (int j = 17; j < 256; ++j)
            edges.emplace_back(i, j, 10000 - i * 100, false);
    capped.setEdges(edges);
    capped.setGoal(255);
    auto stress = Engine{}.execute(capped, Mode::Dijkstra);
    expect(stress.checked() && stress.trace().truncated(), "dense route caps prefix");
    expect(stress.trace().frames().back().costs().at(255) == stress.value() &&
               stress.trace().frames().back().parents().at(255) == 16,
           "capped route final cost/parent table remains current");
    auto grid = gridScene();
    auto d = Engine{}.execute(grid, Mode::Dijkstra, false),
         a = Engine{}.execute(grid, Mode::AStar, false);
    expect(a.value() == d.value(), "grid heuristic eligibility");
}
void persistence(const std::filesystem::path &dir, const std::filesystem::path &root) {
    auto scene = sampleScene(Mode::Hull);
    auto run = Run(scene, Engine{}.execute(scene, Mode::Hull));
    auto path = dir / "report.txt";
    put(path, "PREVIOUS GOOD EVIDENCE");
    std::filesystem::create_directory(path.string() + ".scene");
    rejects([&] { SceneIO::summary(run, path.string()); }, "sidecar directory fails");
    expect(bytes(path) == "PREVIOUS GOOD EVIDENCE", "report preserved on sidecar failure");
    std::filesystem::remove(path.string() + ".scene");
    SceneIO::summary(run, path.string());
    auto oldReport = bytes(path), oldScene = bytes(path.string() + ".scene");
    {
        AtomicFiles tx;
        auto staged = tx.stage(path.string());
        std::ofstream failed(staged);
        failed << "partial";
        failed.setstate(std::ios::badbit);
        rejects([&] { AtomicFiles::close(failed); }, "simulated short write/flush failure");
    }
    expect(bytes(path) == oldReport && bytes(path.string() + ".scene") == oldScene,
           "failed stream preserves generation");
    {
        AtomicFiles tx;
        auto first = tx.stage(path.string());
        auto second = tx.stage(path.string() + ".scene");
        put(first, "replacement one");
        put(second, "replacement two");
        rejects(
            [&] {
                tx.commit([](std::size_t index) {
                    if (index == 1)
                        throw std::runtime_error("injected publish failure");
                });
            },
            "mid-publication failure");
        expect(bytes(path) == oldReport && bytes(path.string() + ".scene") == oldScene,
               "rollback restores both prior files");
    }
    rejects([&] { SceneIO::save(scene, (dir / "missing" / "file.scene").string()); },
            "unavailable parent");
    auto c = loadSecuritySample((root / "data").string(),
                                securitySamples((root / "data").string()).front());
    auto evidence = dir / "evidence.txt";
    c.evidence(evidence.string());
    const std::vector<std::string> suffixes = {"",          ".geo.scene", ".feature.scene",
                                               ".security", ".json",      ".sensitivity.txt"};
    std::vector<std::string> prior;
    for (auto &suffix : suffixes)
        prior.push_back(bytes(evidence.string() + suffix));
    std::filesystem::remove(evidence.string() + ".feature.scene");
    std::filesystem::create_directory(evidence.string() + ".feature.scene");
    rejects([&] { c.evidence(evidence.string()); }, "security sidecar failure");
    for (std::size_t i = 0; i < suffixes.size(); ++i)
        if (i != 2)
            expect(bytes(evidence.string() + suffixes[i]) == prior[i],
                   "security generation preserved");
    std::filesystem::remove(evidence.string() + ".feature.scene");
    put(evidence.string() + ".feature.scene", prior[2]);
    c.setReview(c.events()[0].id(), "benign", "Verified, with \"source\"\ncontext", 1800000123);
    c.save((dir / "notes.security").string());
    auto loaded = SecurityConsole::load((dir / "notes.security").string());
    expect(loaded.reviewNotes() == c.reviewNotes() && loaded.reviewTimes() == c.reviewTimes(),
           "review notes/time persisted independently");
    auto unicodePath = dir / std::filesystem::u8path(u8"scene-π.scene");
    SceneIO::save(scene, unicodePath.u8string());
    expect(SceneIO::load(unicodePath.u8string()).points().size() == scene.points().size(),
           "native Unicode session path");
    auto original = bytes(path.string() + ".scene");
    auto truncated = dir / "truncated.scene";
    for (std::size_t pos = 0; pos < original.size(); ++pos)
        if (original[pos] == '\n' && pos + 1 < original.size()) {
            put(truncated, original.substr(0, pos));
            rejects([&] { SceneIO::load(truncated.string()); }, "scene structural truncation");
        }
    auto securityBytes = bytes(dir / "notes.security");
    auto secPath = dir / "truncated.security";
    for (std::size_t pos = 0; pos < securityBytes.size(); ++pos)
        if (securityBytes[pos] == '\n' && pos + 1 < securityBytes.size()) {
            put(secPath, securityBytes.substr(0, pos));
            rejects([&] { SecurityConsole::load(secPath.string()); },
                    "security structural truncation");
        }
    put(truncated, "\xef\xbb\xbf" + original);
    rejects([&] { SceneIO::load(truncated.string()); }, "BOM rejected explicitly by schema");
    std::mt19937 gen(425050);
    for (int attempt = 0; attempt < 240; ++attempt) {
        std::string input(gen() % 512, ' ');
        for (char &ch : input)
            ch = static_cast<char>(gen() % 128);
        put(truncated, input);
        put(secPath, input);
        try {
            SceneIO::load(truncated.string());
        } catch (const std::exception &) {
        }
        try {
            SecurityConsole::load(secPath.string());
        } catch (const std::exception &) {
        }
        try {
            parseCsv(input);
        } catch (const std::exception &) {
        }
        ++checks;
    }
    for (auto &file : std::filesystem::directory_iterator(dir))
        expect(file.path().filename().string().find(".stage-") == std::string::npos,
               "staged export cleanup");
}
void transitions(const std::filesystem::path &root) {
    auto c = loadSecuritySample((root / "data").string(),
                                securitySamples((root / "data").string()).front());
    WorkspaceState state{sampleScene(Mode::AStar), c};
    EditHistory history;
    history.record(state);
    auto original = workspaceKey(state);
    state.lab.movePoint(0, {10, 20});
    history.record(state);
    state.lab.removePoint(1);
    history.record(state);
    state.lab.setRadius(1e-10);
    history.record(state);
    expect(history.canUndo(), "edit history available");
    history.undo();
    expect(history.undo().lab.points().size() == 12, "undo deletion restores topology");
    expect(workspaceKey(history.undo()) == original, "undo restores full input/settings");
    expect(history.canRedo(), "redo available");
    history.redo();
    state = history.current();
    state.lab.setPolygon({{0, 0}, {1, 0}, {1, 1}});
    history.record(state);
    expect(!history.canRedo(), "branching edit discards redo");
    Playback playback;
    playback.seek(100, 5);
    expect(playback.cursor() == 4 && !playback.playing(), "bounded direct seek");
    playback.seek(9, 0);
    expect(playback.cursor() == 0, "empty seek");
    playback.setPlaying(true);
    playback.update(0.2, 10);
    playback.step(1, 10);
    playback.setPlaying(true);
    playback.update(0.1, 10);
    expect(playback.cursor() == 1, "manual step resets the playback interval");
    playback.update(0.16, 10);
    expect(playback.cursor() == 2, "playback resumes after a complete interval");
}
void securityDomain(const std::filesystem::path &root) {
    auto c = loadSecuritySample((root / "data").string(),
                                securitySamples((root / "data").string()).front());
    WorkspaceState state{sampleScene(Mode::Hull), c};
    auto before = workspaceKey({state.lab, c});
    auto invalid = c.settings();
    invalid.setFeatureRadius(0);
    rejects([&] { c.setSettings(invalid); }, "validated settings update");
    expect(workspaceKey({state.lab, c}) == before, "failed settings preserves state");
    auto revision = c.revision();
    auto first = c.observations(true);
    c.updateSettings([](SecuritySettings &s) { s.setDevice("absent"); });
    expect(c.revision() > revision && c.observations(true).empty(),
           "revision invalidates observation cache");
    c.updateSettings([](SecuritySettings &s) { s.setDevice("*"); });
    expect(c.observations(true).size() == first.size(), "cache restores all devices");
    SecuritySettings params;
    params.setOriginLat(0);
    params.setOriginLon(0);
    auto events = c.events();
    for (auto &e : events) {
        e.setLat(0);
        e.setLon(0);
    }
    c.replace(events, params, {});
    auto prepared = PreparedSecurity::prepare(c, false, (root / "data/campus_map.csv").string());
    expect(prepared.map.empty() && prepared.observations.size() == events.size(),
           "custom origin without mandatory campus map");
}
void clustering() {
    for (int n : {0, 1, 2, 32, 256}) {
        Scene s;
        for (int i = 0; i < n; ++i)
            s.addPoint({double(i % 8), double(i / 8)});
        s.setRadius(1.2);
        auto a = Engine{}.execute(s, Mode::DBSCAN, false),
             b = Engine{}.execute(s, Mode::IndexedDBSCAN, false);
        expect(a.labels() == b.labels() && a.kind() == b.kind(), "density answer/kind identity");
        auto traced = Engine{}.execute(s, Mode::IndexedDBSCAN, true);
        expect(traced.labels() == b.labels() &&
                   traced.stats().operations() == b.stats().operations(),
               "recording-independent density contract");
    }
}
} // namespace
int main(int argc, char **argv) {
    try {
        if (argc != 4)
            throw std::runtime_error("regressions GROUP ROOT OUTPUT");
        std::string group = argv[1];
        std::filesystem::path root = argv[2],
                              dir =
                                  std::filesystem::path(argv[3]) /
                                  std::to_string(
                                      std::chrono::steady_clock::now().time_since_epoch().count());
        std::filesystem::create_directories(dir);
        if (group == "geometry")
            numerical();
        else if (group == "routing")
            routing();
        else if (group == "persistence")
            persistence(dir, root);
        else if (group == "security")
            securityDomain(root);
        else if (group == "transitions")
            transitions(root);
        else if (group == "clustering")
            clustering();
        else
            throw std::runtime_error("Unknown group");
        std::cout << "PASS " << group << ": " << checks << " checks; seed 425050\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "FAIL after " << checks << ": " << e.what() << '\n';
        return 1;
    }
}
