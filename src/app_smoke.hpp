// Included inside App: app smoke.
// Exercise model/UI coordination through the same handlers used by interactive controls.
void smokeInteractions() {
    auto expect = [](bool yes, const std::string &message) {
        if (!yes)
            throw std::runtime_error("App smoke regression: " + message);
    };
    auto probe = view.screen({123.5, -42.75});
    auto restored = view.world(probe);
    expect(GeometryKernel::distance(restored, {123.5, -42.75}) < 0.001,
           "logical canvas/world round trip");
    expect(std::abs(Ui::width() * Ui::scale() - Ui::pixelWidth()) < 0.01f,
           "logical/native width round trip");
    mode = Mode::Dijkstra;
    scene = sampleScene(mode);
    routePick = RoutePick::Source;
    expect(pickEndpoint(1) && scene.source() == 1, "source picker");
    routePick = RoutePick::Destination;
    expect(pickEndpoint(2) && scene.goal() == 2, "destination picker");
    expect(!pickEndpoint(-1), "invalid picker target");
    const int startingVariant = sampleVariant;
    for (int i = 0; i < SampleVariantCount; ++i) {
        nextSample();
        execute();
        expect(run && run->result().checked(), "sample cycle verified input");
    }
    expect(sampleVariant == startingVariant, "sample cycle wraps after all six inputs");
    const double radiusBefore = scene.radius();
    openSettings();
    settings.setField(0, "-1");
    bool rejectedSettings = false;
    try {
        applySettings();
    } catch (const std::exception &) {
        rejectedSettings = true;
    }
    expect(rejectedSettings && scene.radius() == radiusBefore && settings.isOpen(),
           "failed settings transaction preserves live scene and edit buffer");
    settings.close();
    mode = Mode::Hull;
    scene = sampleScene(mode);
    execute();
    double original = run->input().points()[0].x();
    scene.movePoint(0, {scene.points()[0].x() + 10, scene.points()[0].y()});
    invalidate();
    expect(!run && previous && previous->input().points()[0].x() == original,
           "edit invalidation/frozen previous input");
    execute();
    comparePrevious();
    expect(run && previous && run->input().points()[0].x() != previous->input().points()[0].x(),
           "prior/current snapshots");
    setView(2);
    openSettings();
    settings.setField(8, "device-00");
    applySettings();
    expect(scene.points().size() == 1 && observations[0].name() == "device-00",
           "settings/filter integration");
    execute();
    expect(run && run->result().checked(), "single-device security execution");
    openSettings();
    settings.setField(8, "*");
    applySettings();
    selected = 10;
    review("benign");
    security.updateSettings([&](SecuritySettings &draft) { draft.setFeatureQuery({1.25, 0.5}); });
    refreshSecurity();
    save();
    exportEvidence();
    auto saved = SecurityConsole::load((root / "output/session.security").u8string());
    expect(saved.reviews().at("event-10") == "benign" &&
               saved.settings().featureQuery().x() == 1.25,
           "reviews/query persistence");
    loadFile((root / "data/security_sample.csv").u8string());
    expect(security.reviews().empty(), "new CSV clears source-linked reviews");
    for (int targetView : {1, 2}) {
        securityView = targetView;
        const int startingSample = securitySample;
        for (std::size_t i = 0; i < securityCatalogue.size(); ++i) {
            nextSample();
            execute();
            expect(run && run->result().checked() && security.reviews().empty() &&
                       security.settings().user() == "*" && securityView == targetView,
                   "security catalogue cycle resets context and verifies both views");
        }
        expect(securitySample == startingSample, "security catalogue cycle wraps");
    }
    security.setSettings(SecuritySettings{});
    security.setFence({{-300, -250}, {350, -250}, {350, 300}, {-300, 300}});
    securityView = 0;
    mode = Mode::Hull;
    scene = sampleScene(mode);
    run.reset();
    previous.reset();
    playback.reset();
    selected = -1;
    view.fit(scene);
    markSaved(true);
    markSaved(false);
    scene.movePoint(0, {111, 222});
    invalidate();
    setView(1);
    markSaved(false);
    expect(dirty(), "saving security must not mark unsaved Lab edits saved");
    setView(0);
    // Review regressions exercise the production coordinator after every transitional tool state.
    for (double radius : {1e-10, 1.234567891234567}) {
        scene.setRadius(radius);
        openSettings();
        applySettings();
        expect(scene.radius() == radius, "no-edit Settings preserves exact radius");
    }
    mode = Mode::AStar;
    scene = sampleScene(mode);
    scene.movePoint(0, {111, 222});
    selected = 3;
    selectedEdge = 1;
    const auto labBefore = workspaceKey(workspace());
    setView(1);
    setView(2);
    setView(0);
    expect(workspaceKey(workspace()) == labBefore && selected == 3 && selectedEdge == 1 &&
               mode == Mode::AStar,
           "Lab geometry, mode and selection survive security roundtrip");
    edits.record(workspace());
    auto radius = scene.radius();
    scene.setRadius(radius + 1);
    invalidate();
    recordEdit();
    restoreEdit(false);
    expect(scene.radius() == radius && !run, "undo settings invalidates run");
    restoreEdit(true);
    expect(scene.radius() == radius + 1, "redo settings");
    for (int variant = 0; variant < SampleVariantCount; ++variant) {
        edgeStart = 2;
        drag = 1;
        polygonDrag = 0;
        selectedEdge = 1;
        routePick = RoutePick::Source;
        chooseSample(variant);
        expect(edgeStart == -1 && drag == -1 && polygonDrag == -1 && selectedEdge == -1 &&
                   routePick == RoutePick::None,
               "sample resets every editing tool");
    }
    loadGrid();
    expect(scene.grid() && mode == Mode::AStar && selected == -1, "Grid resets interaction");
    execute();
    compare();
    expect(compareVisible && comparison &&
               comparison->input().points().size() == scene.points().size(),
           "frozen comparison on same input");
    clearScene();
    expect(scene.points().empty() && !run && !compareVisible,
           "Clear invalidates both comparison traces");
    restoreEdit(false);
    loadGrid();
    auto beforeFailure = workspaceKey(workspace());
    int viewBefore = securityView;
    mode = Mode::Dijkstra;
    selected = 4;
    edgeStart = 1;
    std::ofstream(root / "output/invalid-import.security") << "GEOVIZ_SECURITY 2\n";
    bool rejected = false;
    try {
        loadFile((root / "output/invalid-import.security").u8string());
    } catch (const std::exception &) {
        rejected = true;
    }
    expect(rejected && workspaceKey(workspace()) == beforeFailure && securityView == viewBefore &&
               mode == Mode::Dijkstra && selected == 4 && edgeStart == 1,
           "failed import preserves complete workspace and tools");
    SecuritySettings origin;
    origin.setOriginLat(0);
    origin.setOriginLon(0);
    auto otherEvents = security.events();
    for (auto &e : otherEvents) {
        e.setLat(0);
        e.setLon(0);
    }
    SecurityConsole elsewhere;
    elsewhere.replace(otherEvents, origin, {});
    elsewhere.save((root / "output/other-origin.security").u8string());
    loadFile((root / "output/other-origin.security").u8string());
    expect(securityView == 1 && map.empty() && !observations.empty() && selected == -1 &&
               edgeStart == -1,
           "custom origin commits complete prepared view without campus background");
    setView(0);
    expect(scene.grid(), "Lab graph survives imported security session");
    security = loadSecuritySample((root / "data").u8string(), securityCatalogue.front());
    fenceDraft = security.fence();
    mode = Mode::Hull;
    scene = sampleScene(mode);
    run.reset();
    previous.reset();
    comparison.reset();
    compareVisible = false;
    resetInteractions();
    view.fit(scene);
    std::ofstream(root / "output/APP_SMOKE_REPORT.txt")
        << "PASS: application run/edit invalidation, frozen history, prior "
           "exports, "
           "six-sample cycle, rejected settings transaction, settings/device filter, source "
           "review, query/snapshot/evidence "
           "persistence, CSV "
           "review reset; exact no-edit settings; preserved Lab mode/geometry/selection; "
           "undo/redo; tool reset; Grid/Clear; frozen comparisons; failed import rollback; "
           "other-origin import. This covers programmatic application state, not "
           "exhaustive physical "
           "mouse/keyboard testing.\n";
}
