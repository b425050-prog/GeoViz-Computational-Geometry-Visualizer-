// Included inside App. Tests call the same commands as the interactive controls.
void featureChecks() {
    std::vector<std::string> failures;
    int checks = 0;
    auto expect = [&](bool value, const std::string &name) {
        ++checks;
        if (!value)
            failures.push_back(name);
    };
    auto reset = [&] {
        securityView = 0;
        mode = labMode = Mode::Hull;
        scene = labScene = sampleScene(mode);
        security = loadSecuritySample((root / "data").u8string(), securityCatalogue.front());
        fenceDraft = security.fence();
        sessionPath.clear();
        labSessionPath.clear();
        securitySessionPath.clear();
        recentFiles.clear();
        evaluationDirty = true;
        sensitivityVisible = false;
        sensitivityCache.clear();
        settings.close();
        run.reset();
        previous.reset();
        comparison.reset();
        compareVisible = false;
        playback.reset();
        resetInteractions();
        view.fit(scene);
        edits = EditHistory{};
        edits.record(workspace());
        markSaved(true);
        markSaved(false);
    };
    auto test = [&](const std::string &name, const std::function<void()> &action) {
        reset();
        try {
            action();
        } catch (const std::exception &e) {
            failures.push_back(name + ": " + e.what());
        }
    };
    test("Lab polygon isolation", [&] {
        const auto before = securityKey();
        clearFence();
        expect(securityKey() == before, "Clearing a Lab polygon preserves security fence draft");
        setView(1);
        expect(scene.polygon().size() == security.fence().size(),
               "Security fence survives Lab clear");
    });
    test("Feature polygon isolation", [&] {
        setView(2);
        const auto before = securityKey();
        clearFence();
        expect(securityKey() == before, "Feature view cannot clear a geographic fence");
    });
    test("Draft through Settings", [&] {
        setView(1);
        scene.setPolygon({{1, 2}, {3, 4}});
        syncFence();
        openSettings();
        applySettings();
        expect(scene.polygon().size() == 2 && scene.polygon()[0].x() == 1,
               "Applying settings preserves the unfinished geographic draft");
    });
    test("Draft through feature analysis", [&] {
        setView(1);
        scene.setPolygon({{1, 2}});
        syncFence();
        setView(2);
        bool rejected = false;
        try {
            execute();
        } catch (const std::exception &) {
            rejected = true;
        }
        expect(rejected, "Feature analysis does not silently use an obsolete valid fence");
        setView(1);
        expect(scene.polygon().size() == 1, "Rejected analysis preserves draft for repair");
    });
    test("Imported workspace file destinations", [&] {
        const auto labPath = root / "output/audit-named.scene";
        sessionPath = labPath;
        save();
        const auto securityPath = root / "output/audit-import.security";
        security.save(securityPath.u8string());
        loadFile(securityPath.u8string());
        setView(0);
        expect(sessionPath == labPath, "Security import preserves Lab file destination");
        setView(1);
        loadFile(labPath.u8string());
        expect(mode == labMode, "Scene import from security restores the Lab algorithm");
    });
    test("Uppercase security extension", [&] {
        auto path = root / "output/audit-uppercase.SECURITY";
        security.save(path.u8string());
        loadFile(path.u8string());
        expect(securityView == 1, "Windows uppercase extension imports as a security snapshot");
    });
    test("Derived displays after sample replacement", [&] {
        setView(1);
        evaluationCache = security.evaluation();
        evaluationDirty = false;
        sensitivityVisible = true;
        sensitivityCache = security.sensitivity();
        chooseSample(1);
        expect(evaluationDirty, "Sample replacement invalidates cached evaluation");
        expect(!sensitivityVisible || sensitivityCache == security.sensitivity(),
               "Sample replacement does not display old sensitivity results");
    });
    test("Filter reset is transactional", [&] {
        std::vector<Event> many;
        for (int i = 0; i < 257; ++i) {
            Event event = security.events().front();
            event.setId("audit-" + std::to_string(i));
            event.setDevice(i == 0 ? "chosen" : "other");
            many.push_back(event);
        }
        auto parameters = security.settings();
        parameters.setDevice("chosen");
        security.replace(many, parameters, security.fence());
        setView(1);
        const auto before = workspaceKey(workspace());
        bool rejected = false;
        try {
            resetFilters();
        } catch (const std::exception &) {
            rejected = true;
        }
        expect(rejected && workspaceKey(workspace()) == before && scene.points().size() == 1,
               "Oversized filter reset preserves both model and displayed observations");
    });
    test("Supported coordinate boundary rendering", [&] {
        scene.setPolygon({});
        for (double offset : {-9999999.0, 9999999.0}) {
            scene.setPoints({{offset, offset}, {offset + .01, offset}, {offset, offset + .01}});
            view.fit(scene);
            execute();
            draw();
            expect(run->result().checked(), "Valid boundary coordinates render after Fit");
        }
        scene.setPoints({{-1e7, -1e7}, {1e7, 1e7}});
        view.fit(scene);
        for (const auto &point : scene.points())
            expect(CheckCollisionPointRec(view.screen(point), view.bounds()),
                   "Fit includes the full supported coordinate range");
        execute();
        draw();
    });
    test("Selection after undo in another workspace", [&] {
        labScene = Scene{};
        labSelected = 10;
        labEdge = 10;
        setView(1);
        labScene = Scene{};
        labSelected = labEdge = 10;
        setView(0);
        expect(selected == -1 && selectedEdge == -1,
               "Restored selections stay within restored topology");
    });
    test("Feature view only offers meaningful modes", [&] {
        setView(2);
        expect(!modeAvailable(Mode::Geofence) && modeAvailable(Mode::KDNearest),
               "Feature view disables geofence instead of offering an unusable Run");
    });
    test("All Lab samples and playback", [&] {
        for (const auto &descriptor : algorithmDescriptors()) {
            mode = descriptor.mode;
            for (int variant = 0; variant < SampleVariantCount; ++variant) {
                chooseSample(variant);
                execute();
                expect(run->result().checked(), "Sample checked: " + modeName(mode));
                const auto n = run->result().trace().frames().size();
                playback.seek(n, n);
                expect(frame() && playback.cursor() == n - 1, "Final frame reachable");
                playback.step(-1, n);
                playback.reset();
                playback.setPlaying(true);
                playback.update(0.25, n);
                expect(playback.cursor() > 0 || n == 1, "Recorded sample advances during playback");
            }
        }
    });
    test("Security scenarios and supported algorithms", [&] {
        for (int targetView : {1, 2}) {
            setView(targetView);
            for (int sample = 0; sample < static_cast<int>(securityCatalogue.size()); ++sample) {
                chooseSample(sample);
                for (const auto &descriptor : algorithmDescriptors()) {
                    if (!descriptor.security ||
                        (targetView == 2 && descriptor.mode == Mode::Geofence))
                        continue;
                    mode = descriptor.mode;
                    execute();
                    expect(run->result().checked(), "Security algorithm " + modeName(mode));
                }
            }
        }
    });
    reset();
    std::ofstream report(root / "output/FEATURE_CHECKS.txt");
    report << checks << " feature checks; " << failures.size() << " failures\n";
    for (const auto &failure : failures)
        report << "FAIL: " << failure << '\n';
    report.close();
    if (!failures.empty())
        throw std::runtime_error("Existing-feature audit: " + std::to_string(failures.size()) +
                                 " failures; see output/FEATURE_CHECKS.txt");
}

// Automated desktop scenarios and interaction checks.
// Exercise model/UI coordination through the same handlers used by interactive controls.
void smokeInteractions() {
    featureChecks();
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
void smokeSample(Mode selectedMode, int variant) {
    securityView = 0;
    mode = selectedMode;
    sampleVariant = variant;
    scene = sampleScene(mode, variant);
    resetInteractions();
    view.fit(scene);
    execute();
    finalFrame();
}
bool smokeStage(int stage) {
    const int securityEnd = 50 + static_cast<int>(securityCatalogue.size()) * 2;
    if (stage < 10)
        smokeSample(static_cast<Mode>(stage), sampleVariant);
    else if (stage == 10 || stage == 11) {
        setView(stage - 9);
        execute();
        if (stage == 10)
            finalFrame();
        selected = stage == 10 ? 11 : 0;
    } else if (stage == 12)
        openSettings();
    else if (stage == 13) {
        settings.close();
        help = true;
    } else if (stage < 18) {
        help = false;
        smokeSample(stage == 14   ? Mode::Hull
                    : stage == 15 ? Mode::DBSCAN
                                  : Mode::KDNearest,
                    sampleVariant);
        const auto &frames = run->result().trace().frames();
        std::size_t target =
            stage == 16 ? std::min<std::size_t>(3, frames.size() - 1) : frames.size() / 2;
        if (stage == 17)
            for (std::size_t i = 0; i < frames.size(); ++i)
                if (!frames[i].pruned().empty()) {
                    target = i;
                    break;
                }
        playback.seek(target, frames.size());
    } else if (stage < 28)
        smokeSample(static_cast<Mode>(stage - 18), stage % 2 + 2);
    else if (stage < 30) {
        setView(1);
        nextSample();
        execute();
        finalFrame();
        selected = static_cast<int>(scene.points().size()) - 1;
    } else if (stage < 50)
        smokeSample(static_cast<Mode>((stage - 30) % 10), 4 + (stage - 30) / 10);
    else if (stage < securityEnd) {
        securitySample = (stage - 50) / 2;
        security =
            loadSecuritySample((root / "data").u8string(), securityCatalogue[securitySample]);
        securityDataset = securityCatalogue[securitySample].name;
        securityView = (stage - 50) % 2 + 1;
        mode = Mode::IndexedDBSCAN;
        refreshSecurity();
        execute();
        selected = static_cast<int>(scene.points().size()) - 1;
        finalFrame();
    } else if (stage < securityEnd + 2) {
        security = loadSecuritySample((root / "data").u8string(), securityCatalogue.front());
        security.updateSettings([](SecuritySettings &draft) { draft.setDevice("no-such-device"); });
        securityDataset = "Empty filtered selection";
        securityView = stage - securityEnd + 1;
        mode = Mode::IndexedDBSCAN;
        refreshSecurity();
        if (feature() && !CheckCollisionPointRec(view.screen({0, 0}), view.bounds()))
            throw std::runtime_error("Empty feature view must retain visible coordinate axes.");
        execute();
    } else if (stage < securityEnd + 9) {
        settings.close();
        help = samplePicker = noteOpen = false;
        switch (stage - securityEnd - 2) {
        case 0:
            setView(0);
            mode = Mode::Hull;
            chooseSample(0);
            samplePicker = true;
            break;
        case 1:
            security = loadSecuritySample((root / "data").u8string(), securityCatalogue.front());
            fenceDraft = security.fence();
            securityDataset = securityCatalogue.front().name;
            setView(1);
            samplePicker = true;
            break;
        case 2:
            loadGrid();
            execute();
            compare();
            break;
        case 3:
            compareVisible = false;
            togglePresentation();
            break;
        case 4:
            togglePresentation();
            setView(1);
            openSettings();
            break;
        case 5:
            setView(2);
            execute();
            Ui::adjustText(6);
            break;
        case 6:
            Ui::adjustText(-6);
            selected = 0;
            noteVerdict = "suspicious";
            noteText = "Synthetic test note; review time is independent of truth labels.";
            noteOpen = true;
            break;
        }
    } else
        return false;
    return true;
}
void smokeReport() const {
    const int states = 59 + static_cast<int>(securityCatalogue.size()) * 2;
    if (smokeFrame < states * 5)
        throw std::runtime_error("Desktop smoke test ended before all states completed.");
    std::ofstream(root / "output/APP_SMOKE_REPORT.txt", std::ios::app)
        << "PASS: rendered and captured " << states << " states at " << Ui::pixelWidth() << 'x'
        << Ui::pixelHeight() << ".\n"
        << "Includes all algorithm modes, both security views, all security scenarios, "
           "intermediate frames, dialogs, comparison, presentation and enlarged text.\n";
}
