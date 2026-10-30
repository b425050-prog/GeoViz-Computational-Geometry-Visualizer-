#include "geoviz.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <type_traits>
using namespace gv;
static_assert(std::is_abstract_v<Algorithm> && std::is_abstract_v<RouteAlgorithm>);
static_assert(std::has_virtual_destructor_v<Algorithm> &&
              std::has_virtual_destructor_v<SpatialIndex> &&
              std::has_virtual_destructor_v<LogAdapter>);
static_assert(
    std::is_same_v<decltype(std::declval<Scene &>().points()), const std::vector<Point> &>);
static_assert(std::is_same_v<decltype(std::declval<Run &>().input()), const Scene &>);
namespace {
int checks = 0;
void require(bool condition, const std::string &message) {
    ++checks;
    if (!condition)
        throw std::runtime_error("FAIL: " + message);
}
template <class F> void rejects(F f, const std::string &message) {
    bool failed = false;
    try {
        f();
    } catch (const std::exception &) {
        failed = true;
    }
    require(failed, message);
}
void expected(const Scene &s, Mode m, double value) {
    auto r = Engine{}.execute(s, m);
    require(r.checked(), modeName(m) + " reference check");
    require(std::abs(r.value() - value) < 1e-7, modeName(m) + " expected value");
    require(!r.trace().frames().empty(), "recorded frames");
}
#include "security_checks.hpp"
} // namespace
int main(int argc, char **argv) {
    try {
        const auto root = std::filesystem::weakly_canonical(argc > 1 ? argv[1] : ".");
        const auto outputRoot = std::filesystem::weakly_canonical(
            argc > 2 ? std::filesystem::path(argv[2])
                     : std::filesystem::absolute(argv[0]).parent_path() / "test-output");
        if (std::filesystem::exists(outputRoot) && std::filesystem::equivalent(outputRoot, root))
            throw std::invalid_argument("Test output must be separate from the source root.");
        for (auto ancestor = outputRoot; !ancestor.empty();) {
            if (std::filesystem::exists(ancestor))
                for (const char *part :
                     {"data", "src", "include", "tests", "docs", "tools", "assets", ".github"})
                    if (std::filesystem::exists(root / part) &&
                        std::filesystem::equivalent(ancestor, root / part))
                        throw std::invalid_argument(
                            "Test output cannot be placed in source or fixture directories.");
            const auto parent = ancestor.parent_path();
            if (parent == ancestor)
                break;
            ancestor = parent;
        }
        std::filesystem::create_directories(outputRoot / "output");
        std::filesystem::create_directories(outputRoot / "data");
        Engine engine;
        // Exercise the new ownership boundaries and virtual dispatch directly.
        std::unique_ptr<AlgorithmRunner> runner = std::make_unique<Engine>();
        for (int i = 0; i < 10; ++i) {
            const auto mode = static_cast<Mode>(i);
            const auto result = runner->execute(sampleScene(mode), mode, false);
            require(result.checked() && result.mode() == mode && result.trace().frames().empty(),
                    "polymorphic runner without trace " + modeName(mode));
        }
        rejects([&] { runner->execute(Scene{}, static_cast<Mode>(99)); }, "unknown strategy");
        std::unique_ptr<Algorithm> algorithm = std::make_unique<HullAlgorithm>();
        require(algorithm->run(sampleScene(Mode::Hull)).checked(), "direct algorithm interface");
        Scene editable;
        editable.setPoints({{0, 0}, {1, 0}, {2, 0}, {3, 0}});
        editable.addEdge({0, 1, 1});
        editable.addEdge({1, 2, 1});
        editable.addEdge({2, 3, 1});
        const Run snapshot{editable, engine.execute(editable, Mode::Hull)};
        const auto revision = editable.revision();
        editable.removePoint(1);
        require(editable.points().size() == 3 && editable.edges().size() == 1 &&
                    editable.edges()[0].a() == 1 && editable.edges()[0].b() == 2 &&
                    editable.goal() == 2 && editable.revision() > revision,
                "point removal owns topology and revision updates");
        require(snapshot.input().points().size() == 4 && snapshot.input().edges().size() == 3,
                "saved run owns an independent scene snapshot");
        const auto edgesBefore = editable.edges().size();
        rejects([&] { editable.setEdges({{0, 99, 1}}); }, "invalid topology transaction");
        require(editable.edges().size() == edgesBefore, "failed edit preserves original edges");
        rejects([&] { editable.movePoint(99, {1, 2}); }, "out of range point edit");
        rejects([] { Point invalid{INF, 0}; }, "point constructor rejects infinity");
        Point point{1, 2};
        rejects([&] { point.setX(INF); }, "coordinate setter rejects infinity");
        require(point.x() == 1, "failed coordinate edit preserves point");
        for (bool indexed : {false, true}) {
            std::vector<Point> original{{-1, 0}, {1, 0}, {9, 0}};
            std::unique_ptr<SpatialIndex> index;
            if (indexed)
                index = std::make_unique<KDTree>(original);
            else
                index = std::make_unique<LinearIndex>(original);
            original.clear();
            Stats stats;
            require(index->nearest({0, 0}, stats) == 0 &&
                        index->radius({0, 0}, 1, stats) == std::vector<int>({0, 1}),
                    "spatial strategy owns its data and preserves boundary/tie policy");
        }
        Playback clock;
        rejects([&] { clock.setSeconds(0); }, "playback rejects invalid interval");
        std::unique_ptr<LogAdapter> adapter = std::make_unique<CsvLogAdapter>();
        require(adapter->load((root / "data/security_sample.csv").string()).size() == 12,
                "polymorphic log adapter");
        using G = GeometryKernel;
        require(G::orientation({0, 0}, {1, 0}, {0, 1}) == 1, "counterclockwise");
        require(G::intersects({0, 0}, {2, 2}, {0, 2}, {2, 0}), "crossing segments");
        require(G::intersects({0, 0}, {2, 0}, {1, 0}, {3, 0}), "overlapping segments");
        require(G::intersects({0, 0}, {1, 0}, {1, 0}, {2, 1}), "endpoint touch");
        require(!G::intersects({0, 0}, {1, 0}, {2, 0}, {3, 0}), "disjoint collinear");
        require(G::intersects({1, 1}, {1, 1}, {0, 0}, {2, 2}), "zero-length segment on segment");
        expected(sampleScene(Mode::Collision), Mode::Collision, 2);
        Scene s;
        s.setPoints({{0, 0}, {2, 0}, {2, 2}, {0, 2}, {1, 1}, {0, 0}});
        expected(s, Mode::Hull, 4);
        s.setPoints({{0, 0}, {1, 1}, {2, 2}, {1, 1}});
        expected(s, Mode::Hull, 2);
        expected(s, Mode::ClosestPair, 0);
        s.setPoints({});
        expected(s, Mode::Hull, 0);
        require(!std::isfinite(engine.execute(s, Mode::ClosestPair).value()), "empty closest pair");
        s.setPoints({{5, 7}});
        expected(s, Mode::Hull, 1);
        s.setQuery({5, 7});
        expected(s, Mode::KDNearest, 0);
        s.setPolygon({{0, 0}, {4, 0}, {4, 4}, {0, 4}});
        for (auto q : std::vector<Point>{{0, 0}, {2, 0}, {4, 2}, {0, 4}}) {
            s.setQuery(q);
            expected(s, Mode::Geofence, 0);
        }
        s.setQuery({2, 2});
        expected(s, Mode::Geofence, 1);
        s.setQuery({5, 2});
        expected(s, Mode::Geofence, -1);
        rejects([] { G::validatePolygon({{0, 0}, {2, 2}, {0, 2}, {2, 0}}); },
                "reject bow-tie polygon");
        rejects([] { G::validatePolygon({{0, 0}, {2, 0}, {1, 0}, {1, 2}, {0, 2}}); },
                "reject adjacent overlap");
        s = Scene{};
        s.setPoints({{0, 0}, {1, 0}, {2, 0}, {1, 1}});
        s.setEdges({{0, 1, 1, false}, {1, 2, 1, false}, {0, 3, 5, false}, {3, 2, 5, false}});
        s.setGoal(2);
        expected(s, Mode::Dijkstra, 2);
        expected(s, Mode::AStar, 2);
        s.setEdgeBlocked(1, true);
        expected(s, Mode::AStar, 10);
        s.setEdgeBlocked(3, true);
        require(!std::isfinite(engine.execute(s, Mode::Dijkstra).value()), "unreachable");
        s.setGoal(0);
        expected(s, Mode::Dijkstra, 0);
        auto parallel = s;
        rejects([&] { parallel.addEdge(parallel.edges()[0]); },
                "duplicate undirected edge rejected");
        rejects([&] { s.setEdgeWeight(0, -1); }, "negative edge");
        s = Scene{};
        s.setPoints({{0, 0}, {100, 0}, {200, 0}});
        s.setEdges({{0, 1, 1, false}, {1, 2, 1, false}, {0, 2, 3, false}});
        s.setGoal(2);
        auto fallback = engine.execute(s, Mode::AStar);
        require(fallback.value() == 2 && fallback.summary().find("h=0") != std::string::npos,
                "safe heuristic fallback");
        s = gridScene();
        auto d = engine.execute(s, Mode::Dijkstra), a = engine.execute(s, Mode::AStar);
        require(a.checked() && d.checked() && a.value() == d.value() && a.value() == 15,
                "obstacle grid agreement");
        s = Scene{};
        s.setRadius(1.01);
        s.setMinPts(4);
        s.setPoints({{0, 0}, {1, 0}, {1.1, 0}, {1, 0.2}, {20, 20}});
        auto b = engine.execute(s, Mode::DBSCAN);
        require(b.labels() == std::vector<int>({0, 0, 0, 0, -1}),
                "border later reclaimed from noise");
        require(b.kind() == std::vector<int>({1, 2, 1, 1, 0}), "hand-labelled core border noise");
        require(b.checked(), "DBSCAN independent check");
        s.setMinPts(1);
        s.setRadius(0.01);
        expected(s, Mode::DBSCAN, 5);
        s = Scene{};
        s.setPoints({{0, 0}, {1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 0}});
        s.setQuery({0, 0});
        s.setRadius(1);
        auto kr = engine.execute(s, Mode::KDRadius);
        require(kr.answer() == std::vector<int>({0, 1, 2, 3, 4, 5}), "inclusive radius duplicates");
        s.setQuery({0.5, 0});
        require(engine.execute(s, Mode::KDNearest).answer() == std::vector<int>({0}),
                "nearest tie chooses lowest ID");
        std::mt19937 rng(425050);
        std::uniform_real_distribution<double> coord(-100, 100);
        for (int test = 0; test < 80; ++test) {
            Scene t;
            int n = test % 65;
            for (int i = 0; i < n; ++i)
                t.addPoint({coord(rng), coord(rng)});
            t.setQuery({coord(rng), coord(rng)});
            t.setRadius(5 + test % 40);
            t.setMinPts(1 + test % 6);
            for (Mode m : {Mode::Hull, Mode::ClosestPair, Mode::KDNearest, Mode::KDRadius,
                           Mode::DBSCAN, Mode::IndexedDBSCAN})
                require(engine.execute(t, m, false).checked(), "seeded random " + modeName(m));
            auto scan = engine.execute(t, Mode::DBSCAN, false),
                 indexed = engine.execute(t, Mode::IndexedDBSCAN, false);
            require(scan.labels() == indexed.labels() && scan.kind() == indexed.kind(),
                    "indexed baseline equivalence");
            if (n) {
                t.setGoal(n - 1);
                for (int i = 0; i < n; ++i)
                    for (int j = i + 1; j < n; ++j)
                        if (rng() % 9 == 0)
                            t.addEdge({i, j, G::distance(t.points()[i], t.points()[j]) + rng() % 20,
                                       rng() % 7 == 0});
                auto one = engine.execute(t, Mode::Dijkstra, false),
                     two = engine.execute(t, Mode::AStar, false);
                require(one.checked() && two.checked(), "random routes against Bellman-Ford");
            }
        }
        s = sampleScene(Mode::Hull);
        auto run = Run{s, engine.execute(s, Mode::Hull)};
        auto file = (outputRoot / "output/test_roundtrip.scene").string();
        SceneIO::save(s, file);
        auto loaded = SceneIO::load(file);
        auto after = engine.execute(loaded, Mode::Hull);
        require(run.result().answer() == after.answer() &&
                    run.result().stats().operations() == after.stats().operations(),
                "scene replay exact");
        SceneIO::summary(run, (outputRoot / "output/test_checked.txt").string());
        // Saving a partially edited boundary preserves the draft; geofence Run
        // remains responsible for rejecting geometry that cannot define a region.
        int draftNumber = 0;
        for (const auto &polygon : std::vector<std::vector<Point>>{
                 {{3.25, 7.5}}, {{0, 0}, {2, 0}}, {{0, 0}, {2, 2}, {0, 2}, {2, 0}}}) {
            Scene draft = s;
            draft.setPolygon(polygon);
            const auto draftPath =
                (outputRoot / "output" / ("draft_" + std::to_string(draftNumber++) + ".scene"))
                    .string();
            SceneIO::save(draft, draftPath);
            const auto restoredDraft = SceneIO::load(draftPath);
            bool samePolygon = restoredDraft.polygon().size() == polygon.size();
            for (std::size_t i = 0; samePolygon && i < polygon.size(); ++i)
                samePolygon = restoredDraft.polygon()[i].x() == polygon[i].x() &&
                              restoredDraft.polygon()[i].y() == polygon[i].y();
            require(samePolygon, "draft polygon snapshot preserves every vertex");
            rejects([&] { engine.execute(restoredDraft, Mode::Geofence); },
                    "draft polygon is rejected at geofence execution");
            require(engine.execute(restoredDraft, Mode::Hull, false).checked(),
                    "unused draft polygon does not prevent a point algorithm");
        }
        // Empty and populated scenes use the same endpoint policy before and
        // after persistence; invalid file endpoints must not bypass model checks.
        for (const int pointCount : {0, 3}) {
            Scene endpoints;
            for (int i = 0; i < pointCount; ++i)
                endpoints.addPoint({double(i), 0});
            endpoints.setSource(pointCount ? 2 : 0);
            endpoints.setGoal(pointCount ? 1 : 0);
            rejects([&] { endpoints.setSource(-1); }, "negative source rejected before save");
            rejects([&] { endpoints.setGoal(pointCount + 1); },
                    "unknown goal rejected before save");
            const auto endpointPath =
                (outputRoot / "output" / ("endpoints_" + std::to_string(pointCount) + ".scene"))
                    .string();
            SceneIO::save(endpoints, endpointPath);
            const auto restoredEndpoints = SceneIO::load(endpointPath);
            require(restoredEndpoints.source() == endpoints.source() &&
                        restoredEndpoints.goal() == endpoints.goal(),
                    "valid source and goal survive failed edits and scene replay");
            for (const bool badSource : {false, true}) {
                std::ofstream malformed(endpointPath);
                malformed << "GEOVIZ 1\n" << pointCount << '\n';
                for (int i = 0; i < pointCount; ++i)
                    malformed << i << " 0\n";
                malformed << "0\n0\n0 0 75 4 " << (badSource ? pointCount + 1 : 0) << ' '
                          << (badSource ? 0 : pointCount + 1) << " 0\n";
                malformed.close();
                rejects([&] { SceneIO::load(endpointPath); },
                        "unknown source or goal rejected while loading");
            }
        }
        Playback playback;
        auto n = run.result().trace().frames().size();
        playback.step(1, n);
        require(playback.cursor() == 1, "step forward");
        playback.step(-1, n);
        require(playback.cursor() == 0, "step back");
        playback.setPlaying(true);
        playback.update(0.25, n);
        require(playback.cursor() == 1, "timed playback");
        playback.reset();
        require(!playback.playing() && playback.cursor() == 0, "rewind");
        Trace trace;
        Frame f;
        for (std::size_t i = 0; i < MAX_TRACE + 20; ++i) {
            f.setExplanation(std::to_string(i));
            trace.add(f);
        }
        f.setExplanation("FINAL");
        trace.finish(f);
        require(trace.truncated() && trace.frames().size() == MAX_TRACE &&
                    trace.frames().back().explanation() == "FINAL",
                "bounded trace final preserved");
        rejects(
            [&] {
                Scene bad;
                bad.setRadius(0);
                engine.execute(bad, Mode::Hull);
            },
            "invalid radius");
        {
            std::ofstream out(outputRoot / "output/bad.scene");
            out << "GEOVIZ 1\n99999999\n";
        }
        rejects([&] { SceneIO::load((outputRoot / "output/bad.scene").string()); },
                "oversized scene load");
        require(parseCsv("a,\"b,c\",\"d\"\"e\"") == std::vector<std::string>({"a", "b,c", "d\"e"}),
                "CSV quotes");
        rejects([] { parseCsv("a,\"unfinished"); }, "malformed CSV");
        SecurityConsole sec;
        sec.setEvents(CsvLogAdapter{}.load((root / "data/security_sample.csv").string()));
        sec.setFence({{-300, -250}, {350, -250}, {350, 300}, {-300, 300}});
        sec.validate();
        auto obs = sec.observations(true);
        require(obs.size() == 12, "twelve device feature points");
        require(std::abs(obs[0].requestRate() - 10) < 1e-8, "fixed-window rate");
        Point projected = SecurityConsole::project(20.297, 85.825, sec.settings()),
              geo = SecurityConsole::unproject(projected, sec.settings());
        require(std::abs(geo.x() - 20.297) < 1e-10 && std::abs(geo.y() - 85.825) < 1e-10,
                "projection round trip");
        require(!sec.loadMap((root / "data/campus_map.csv").string()).empty(),
                "geographic map integration");
        sec.updateSettings([&](SecuritySettings &draft) { draft.setGeoQuery({75, -25}); });
        sec.updateSettings([&](SecuritySettings &draft) { draft.setFeatureQuery({1.5, 0.3}); });
        sec.setReview(sec.events()[10].id(), "benign");
        sec.save((outputRoot / "output/test.security").string());
        auto restored = SecurityConsole::load((outputRoot / "output/test.security").string());
        require(restored.events().size() == sec.events().size() &&
                    restored.reviews() == sec.reviews(),
                "security snapshot round trip");
        require(restored.scene(false).query().x() == 75 && restored.scene(true).query().y() == 0.3,
                "view-specific query round trip");
        require(sec.evaluation() == restored.evaluation(), "security evaluation reproducibility");
        sec.evidence((outputRoot / "output/test_evidence.txt").string());
        require(sec.evaluation().find("FP=") != std::string::npos, "false positive report");
        require(sec.evaluation().find("TP=1 FP=1 TN=10 FN=0") != std::string::npos,
                "known sample candidate confusion counts");
        require(sec.evaluation().find("analyst-reviewed=1 reviewed benign=1") != std::string::npos,
                "actual benign analyst review retained");
        auto filtered = sec;
        filtered.updateSettings([&](SecuritySettings &draft) { draft.setDevice("device-00"); });
        require(filtered.observations(true).size() == 1, "device filter");
        filtered.updateSettings([&](SecuritySettings &draft) { draft.setUser("absent"); });
        require(filtered.observations(false).empty(), "combined user filter");
        filtered = sec;
        rejects(
            [&] {
                filtered.updateSettings(
                    [&](SecuritySettings &draft) { draft.setEnd(filtered.settings().start()); });
            },
            "empty time window");
        filtered = sec;
        rejects(
            [&] {
                filtered.updateSettings([&](SecuritySettings &draft) { draft.setRequestScale(0); });
            },
            "zero feature scale");
        filtered = sec;
        auto duplicateEvents = filtered.events();
        duplicateEvents.push_back(duplicateEvents[0]);
        rejects([&] { filtered.setEvents(std::move(duplicateEvents)); }, "duplicate source IDs");
        filtered = sec;
        auto distantEvents = filtered.events();
        distantEvents[0].setLat(0);
        rejects([&] { filtered.setEvents(std::move(distantEvents)); }, "projection extent limit");
        // Generated scenes and summaries are test artifacts, never source fixtures.
        for (int i = 0; i < 10; ++i) {
            auto m = static_cast<Mode>(i);
            auto sample = sampleScene(m);
            Run item{sample, engine.execute(sample, m)};
            require(item.result().checked(), "sample checked " + modeName(m));
            SceneIO::summary(
                item, (outputRoot / "output" / ("sample_" + std::to_string(i) + ".txt")).string());
            SceneIO::save(
                sample,
                (outputRoot / "data" / ("sample_" + std::to_string(i) + ".scene")).string());
        }
        // Validate and round-trip all gallery inputs, not only the compact defaults.
        std::filesystem::create_directories(outputRoot / "data/gallery");
        std::filesystem::create_directories(outputRoot / "output/gallery");
        rejects([] { modeName(static_cast<Mode>(-1)); }, "invalid display mode");
        rejects([] { sampleScene(Mode::Hull, SampleVariantCount); }, "invalid sample variant");
        for (int variant = 1; variant < SampleVariantCount; ++variant)
            for (int i = 0; i < 10; ++i) {
                const Mode m = static_cast<Mode>(i);
                Scene sample = sampleScene(m, variant);
                const std::string name =
                    "mode_" + std::to_string(i) + "_" + std::to_string(variant);
                const std::string scenePath =
                    (outputRoot / "data/gallery" / (name + ".scene")).string();
                Run item{sample, engine.execute(sample, m)};
                require(item.result().checked(), "gallery " + name);
                SceneIO::save(sample, scenePath);
                const auto replay = engine.execute(SceneIO::load(scenePath), m, false);
                require(replay.answer() == item.result().answer() &&
                            replay.labels() == item.result().labels() &&
                            replay.kind() == item.result().kind() &&
                            replay.stats().operations() == item.result().stats().operations(),
                        "gallery round trip " + name);
                SceneIO::summary(item, (outputRoot / "output/gallery" / (name + ".txt")).string());
                if (m == Mode::Dijkstra) {
                    auto astar = engine.execute(sample, Mode::AStar, false);
                    require(astar.checked() &&
                                std::abs(astar.value() - item.result().value()) < 1e-7,
                            "gallery route agreement");
                }
                if (m == Mode::DBSCAN) {
                    auto indexed = engine.execute(sample, Mode::IndexedDBSCAN, false);
                    require(indexed.labels() == item.result().labels() &&
                                indexed.kind() == item.result().kind(),
                            "gallery density agreement");
                }
            }
        for (int variant : {4, 5}) {
            expected(sampleScene(Mode::ClosestPair, variant), Mode::ClosestPair, 5);
            const auto nearest =
                engine.execute(sampleScene(Mode::KDNearest, variant), Mode::KDNearest, false);
            require(nearest.checked() && nearest.answer() == std::vector<int>({0}) &&
                        std::abs(nearest.value() - (variant == 4 ? 72 : 60)) < 1e-9,
                    "landscape nearest tie selects lowest input ID");
            const auto radius =
                engine.execute(sampleScene(Mode::KDRadius, variant), Mode::KDRadius, false);
            require(radius.checked() && radius.answer() == std::vector<int>({0, 1, 2, 3}),
                    "landscape radius includes boundary points and excludes nearby exterior");
            expected(sampleScene(Mode::Geofence, variant), Mode::Geofence, variant == 4 ? 0 : 1);
            expected(sampleScene(Mode::Collision, variant), Mode::Collision, variant == 4 ? 6 : 14);
            const auto density =
                engine.execute(sampleScene(Mode::DBSCAN, variant), Mode::DBSCAN, false);
            require(density.checked() && density.value() == (variant == 4 ? 2 : 4) &&
                        std::count(density.labels().begin(), density.labels().end(), -1) ==
                            (variant == 4 ? 3 : 4),
                    "landscape clusters and isolated noise match designed groups");
        }
        Scene bridges = sampleScene(Mode::Dijkstra, 4);
        const auto openBridges = engine.execute(bridges, Mode::Dijkstra, false);
        bool bridgeBlocked = false;
        for (std::size_t i = 0; i < bridges.edges().size(); ++i) {
            const auto &edge = bridges.edges()[i];
            if ((edge.a() == 13 && edge.b() == 14) || (edge.a() == 14 && edge.b() == 13)) {
                bridges.setEdgeBlocked(i, true);
                bridgeBlocked = true;
                break;
            }
        }
        const auto detour = engine.execute(bridges, Mode::Dijkstra, false);
        const auto guidedDetour = engine.execute(bridges, Mode::AStar, false);
        require(bridgeBlocked && openBridges.checked() && detour.checked() &&
                    guidedDetour.checked() && std::isfinite(detour.value()) &&
                    detour.value() > openBridges.value() &&
                    std::abs(detour.value() - guidedDetour.value()) < 1e-7,
                "closed island bridge forces a longer route with A-star agreement");
        for (const std::string name : {"campus", "research"}) {
            SecurityConsole extended;
            extended.setEvents(
                CsvLogAdapter{}.load((root / "data" / ("security_" + name + ".csv")).string()));
            extended.setFence(sec.fence());
            auto geographic = extended.observations(false), features = extended.observations(true);
            require(geographic.size() == (name == "campus" ? 48 : 40), "extended time window");
            require(features.size() == (name == "campus" ? 16 : 20), "extended aggregation");
            require(features[0].events().size() == (name == "campus" ? 3 : 2), "source linkage");
            for (bool feature : {false, true}) {
                auto input = extended.scene(feature);
                auto scan = engine.execute(input, Mode::DBSCAN, false);
                auto indexed = engine.execute(input, Mode::IndexedDBSCAN, false);
                require(scan.checked() && indexed.checked() && scan.labels() == indexed.labels(),
                        "extended density checks");
            }
            extended.evidence(
                (outputRoot / "output" / ("security_" + name + "_evidence.txt")).string());
        }
        securityChecks(root, outputRoot);
        // The upper bound is exercised with both dense and widely spread inputs.
        Scene maximum;
        for (int i = 0; i < 256; ++i)
            maximum.addPoint({double((i * 37) % 257), double((i * 73) % 263)});
        maximum.setRadius(28);
        for (Mode m : {Mode::Hull, Mode::ClosestPair, Mode::KDNearest, Mode::KDRadius, Mode::DBSCAN,
                       Mode::IndexedDBSCAN})
            require(engine.execute(maximum, m, false).checked(), "256-point bound");
        rejects([&] { maximum.addPoint({1, 1}); }, "257th point rejected");
        Scene allCrossing;
        for (int i = 0; i < 128; ++i) {
            allCrossing.addPoint({-100, i - 63.5});
            allCrossing.addPoint({100, 63.5 - i});
        }
        const auto collisionStress = engine.execute(allCrossing, Mode::Collision);
        require(collisionStress.value() == 8128 && collisionStress.trace().truncated() &&
                    collisionStress.trace().frames().size() == MAX_TRACE &&
                    collisionStress.trace().frames().back().lines().size() == 128,
                "dense collision: exact pair count, bounded unique highlights and final state");
        std::ostringstream report;
        report << "PASS: " << checks
               << " checks. Seed 425050. Algorithms, independent references, edge cases, "
                  "persistence, playback, CSV, security filters/projection/evidence.\n";
        std::ofstream(outputRoot / "output/TEST_REPORT.txt") << report.str();
        std::cout << report.str();
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << " (after " << checks << " checks)\n";
        return 1;
    }
}
