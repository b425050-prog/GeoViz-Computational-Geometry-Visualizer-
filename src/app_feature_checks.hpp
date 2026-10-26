// Included inside App. Regression tests use production commands in an isolated smoke root.
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
