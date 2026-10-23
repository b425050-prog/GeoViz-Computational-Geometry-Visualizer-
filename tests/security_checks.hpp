#pragma once

// Contract and boundary checks for ingestion, filtering and security evidence.
void securityChecks(const std::filesystem::path &root, const std::filesystem::path &outputRoot) {
    Engine engine;
    const auto directory = (root / "data").string();
    const auto catalogue = securitySamples(directory);
    require(catalogue.size() == 9, "nine security scenarios are registered");
    const std::vector<int> records{12, 50, 42, 16, 36, 48, 9, 27, 128};
    const std::vector<int> selectedCounts{12, 48, 40, 16, 36, 48, 9, 24, 128};
    const std::vector<int> deviceCounts{12, 16, 20, 16, 12, 8, 9, 12, 64};
    for (std::size_t index = 0; index < catalogue.size(); ++index) {
        auto console = loadSecuritySample(directory, catalogue[index]);
        require(console.events().size() == static_cast<std::size_t>(records[index]),
                "scenario record count");
        require(console.observations(false).size() ==
                    static_cast<std::size_t>(selectedCounts[index]),
                "scenario selected count");
        const auto devices = console.observations(true);
        require(devices.size() == static_cast<std::size_t>(deviceCounts[index]),
                "scenario device count");
        for (const auto &observation : devices) {
            double requests = 0, failures = 0;
            for (int id : observation.events()) {
                const auto &event = console.events().at(id);
                require(event.device() == observation.name(), "aggregated source identity");
                requests += event.requests();
                failures += event.failures();
            }
            const double minutes = (console.settings().end() - console.settings().start()) / 60.0;
            require(std::abs(observation.requestRate() - requests / minutes) < 1e-9 &&
                        std::abs(observation.failureRate() - failures / minutes) < 1e-9,
                    "scenario rates use complete selected window");
        }
        for (bool feature : {false, true}) {
            const auto scene = console.scene(feature);
            for (Mode mode : {Mode::Hull, Mode::ClosestPair, Mode::KDNearest, Mode::KDRadius,
                              Mode::DBSCAN, Mode::IndexedDBSCAN})
                require(engine.execute(scene, mode, false).checked(),
                        "scenario algorithm reference");
            const auto direct = engine.execute(scene, Mode::DBSCAN, false);
            const auto indexed = engine.execute(scene, Mode::IndexedDBSCAN, false);
            require(direct.labels() == indexed.labels() && direct.kind() == indexed.kind(),
                    "scenario direct/indexed density agreement");
            if (!feature)
                require(engine.execute(scene, Mode::Geofence, false).checked(),
                        "scenario geofence reference");
        }
        console.setReview(console.events()[0].id(), "benign");
        console.settings().setGeoQuery({12.5, -5.25});
        console.settings().setFeatureQuery({0.25, 0.75});
        const auto snapshot =
            (outputRoot / "output" / ("scenario_" + std::to_string(index) + ".security")).string();
        console.save(snapshot);
        const auto restored = SecurityConsole::load(snapshot);
        require(restored.evaluation() == console.evaluation() &&
                    restored.reviews() == console.reviews(),
                "scenario evidence round trip");
        require(restored.scene(false).query().x() == 12.5 &&
                    restored.scene(true).query().y() == 0.75,
                "scenario view queries round trip");
        auto empty = console;
        empty.settings().setDevice("no-such-device");
        require(empty.observations(false).empty() && empty.observations(true).empty(),
                "empty identity selection");
        require(
            empty.evaluation().find("Candidate precision=N/A recall=N/A false-positive rate=N/A") !=
                std::string::npos,
            "empty evaluation has no zero-denominator result");
        empty.evidence((outputRoot / "output/empty_evidence.txt").string());
    }
    auto boundary = loadSecuritySample(directory, catalogue[3]);
    for (std::size_t i = 0; i < 16; ++i) {
        const auto &event = boundary.events()[i];
        const int location = GeometryKernel::membership(
            SecurityConsole::project(event.lat(), event.lon(), boundary.settings()),
            boundary.fence());
        require(location == (i < 8    ? 0
                             : i < 12 ? 1
                                      : -1),
                "perimeter corner/edge/interior/exterior");
    }
    auto quiet = loadSecuritySample(directory, catalogue[6]);
    require(quiet.evaluation().find("unknown=9") != std::string::npos &&
                quiet.evaluation().find("Candidate precision=N/A") != std::string::npos,
            "unknown truth does not enter confusion denominators");
    auto window = loadSecuritySample(directory, catalogue[7]);
    window.settings().setUser("lab, \"night\"");
    window.settings().setDevice("device,\"edge\"");
    require(window.observations(false).size() == 2 && window.observations(true).size() == 1,
            "quoted exact identity filters");
    require(window.observations(true)[0].requestRate() == 12, "two records over ten-minute window");
    window.settings().setStart(1800000300);
    require(window.observations(false).size() == 1 &&
                window.observations(true)[0].requestRate() == 16,
            "partial window recalculates denominator");
    window.settings().setStart(1800000000);
    window.settings().setEnd(1800000001);
    require(window.observations(false).size() == 1 &&
                window.observations(true)[0].requestRate() == 2400,
            "one-second window is supported");
    auto base = loadSecuritySample(directory, catalogue[0]);
    const auto seed = base.events()[0];
    for (int count : {256, 257, 4096}) {
        auto bounded = base;
        std::vector<Event> events;
        for (int i = 0; i < count; ++i) {
            auto event = seed;
            event.setId("bound-" + std::to_string(i));
            event.setRequests(0);
            event.setFailures(0);
            events.push_back(event);
        }
        bounded.setEvents(std::move(events));
        bounded.validate();
        require(bounded.observations(true).size() == 1,
                "source limit aggregates into bounded view");
        if (count == 256)
            require(bounded.observations(false).size() == 256,
                    "256 geographic observations accepted");
        else
            rejects([&] { bounded.observations(false); },
                    "geographic observation overflow rejected");
        if (count == 4096) {
            auto excessive = bounded.events();
            auto extra = seed;
            extra.setId("bound-extra");
            excessive.push_back(extra);
            bounded.setEvents(std::move(excessive));
            rejects([&] { bounded.validate(); }, "4097 source records rejected");
        }
    }
    for (int count : {1, 2}) {
        auto sparse = base;
        auto events = sparse.events();
        events.resize(count);
        sparse.setEvents(events);
        const auto result = engine.execute(sparse.scene(true), Mode::IndexedDBSCAN, false);
        require(std::all_of(result.labels().begin(), result.labels().end(),
                            [](int label) { return label == -1; }),
                "fewer devices than MinPts produce noise");
    }
    for (const double scale : {0.0, -1.0, INF, std::numeric_limits<double>::quiet_NaN(), 1e9 + 1}) {
        auto invalid = base;
        invalid.settings().setRequestScale(scale);
        rejects([&] { invalid.validate(); }, "invalid request scale");
        invalid = base;
        invalid.settings().setFailureScale(scale);
        rejects([&] { invalid.validate(); }, "invalid failure scale");
    }
    for (const int minimum : {0, 257}) {
        auto invalid = base;
        invalid.settings().setMinPts(minimum);
        rejects([&] { invalid.validate(); }, "invalid security MinPts");
    }
    auto invalid = base;
    invalid.settings().setEnd(invalid.settings().start() + 31LL * 86400);
    invalid.validate();
    require(true, "31-day window accepted");
    invalid.settings().setEnd(invalid.settings().end() + 1);
    rejects([&] { invalid.validate(); }, "overlong window rejected");
    invalid = base;
    invalid.settings().setStart(-1);
    rejects([&] { invalid.validate(); }, "negative window rejected");
    rejects([&] { base.setReview("missing-event", "benign"); }, "unknown review source rejected");
    rejects([&] { base.setReview(seed.id(), "invalid"); }, "invalid review verdict rejected");
    base.setReview(seed.id(), "suspicious");
    require(base.events()[0].label() == seed.label(), "analyst review preserves truth label");
    const std::string header =
        "event_id,timestamp,user_id,device_id,latitude,longitude,requests,failures,label,summary\n";
    const std::string valid = "id,1800000100,user,device,20.2961,85.8245,10,0,benign,summary\n";
    const auto csvPath = (outputRoot / "output/invalid_security.csv").string();
    auto csv = [&](const std::string &content) {
        std::ofstream out(csvPath, std::ios::binary);
        out << content;
    };
    csv(header);
    require(CsvLogAdapter{}.load(csvPath).empty(), "header-only CSV is an empty dataset");
    for (const auto &row : std::vector<std::string>{
             "id,1800000100,user,device,20.2961,85.8245,-1,0,benign,x\n",
             "id,1800000100,user,device,20.2961,85.8245,10,11,benign,x\n",
             "id,1800000100,user,device,20.2961,85.8245,1000000001,0,benign,x\n",
             "id,1800000100,user,device,nan,85.8245,1,0,benign,x\n",
             "id,1800000100,user,device,20.2961,inf,1,0,benign,x\n",
             "id,-1,user,device,20.2961,85.8245,1,0,benign,x\n",
             "id,1800000100,user,device,20.2961,85.8245,1,0,invalid,x\n",
             ",1800000100,user,device,20.2961,85.8245,1,0,benign,x\n",
             "id,1800000100,,device,20.2961,85.8245,1,0,benign,x\n",
             "id,1800000100,user,,20.2961,85.8245,1,0,benign,x\n",
             "id,1800000100,user,device,20.2961,85.8245,1,0,benign,\"open\n",
             "id,1800000100,user,device,20.2961,85.8245,1,0,benign,x,extra\n"}) {
        csv(header + row);
        rejects([&] { CsvLogAdapter{}.load(csvPath); }, "invalid CSV typed/schema value rejected");
    }
    csv(header + valid + valid);
    rejects([&] { CsvLogAdapter{}.load(csvPath); }, "duplicate CSV identity rejected");
    csv("wrong-header\n" + valid);
    rejects([&] { CsvLogAdapter{}.load(csvPath); }, "invalid CSV header rejected");
    csv(header + std::string(8 * 1024 * 1024, 'x'));
    rejects([&] { CsvLogAdapter{}.load(csvPath); }, "oversized CSV rejected before parsing");
    auto badSample = catalogue[0];
    badSample.file = "../security_sample.csv";
    rejects([&] { loadSecuritySample(directory, badSample); },
            "sample filename cannot traverse directories");
    for (bool feature : {false, true}) {
        for (const double radius : {0.0, -1.0, INF, std::numeric_limits<double>::quiet_NaN(),
                                    feature ? 1e7 + 1 : 100001.0}) {
            auto rejected = base;
            if (feature)
                rejected.settings().setFeatureRadius(radius);
            else
                rejected.settings().setGeoRadius(radius);
            rejects([&] { rejected.validate(); }, "security radius limits");
        }
    }
    auto emptyOrigin = base;
    emptyOrigin.setEvents({});
    rejects([&] { emptyOrigin.validate(); }, "stale source-linked review rejected");
    emptyOrigin.clearReviews();
    for (double latitude : {-80.0, 80.0}) {
        emptyOrigin.settings().setOriginLat(latitude);
        emptyOrigin.validate();
        require(emptyOrigin.observations(false).empty(), "projection latitude boundary accepted");
    }
    emptyOrigin.settings().setOriginLat(80.001);
    rejects([&] { emptyOrigin.validate(); }, "projection latitude limit");
    auto invalidFence = base;
    invalidFence.setFence({{0, 0}, {2, 2}, {0, 2}, {2, 0}});
    rejects([&] { invalidFence.validate(); }, "self-crossing security fence rejected");
    auto changedTruth = base;
    auto truthEvents = changedTruth.events();
    for (auto &event : truthEvents)
        event.setLabel("unknown");
    changedTruth.setEvents(std::move(truthEvents));
    require(engine.execute(changedTruth.scene(true), Mode::IndexedDBSCAN, false).labels() ==
                engine.execute(base.scene(true), Mode::IndexedDBSCAN, false).labels(),
            "truth labels cannot change clustering");
    // Rejected evidence must not truncate a previously exported file.
    auto tooMany = base;
    std::vector<Event> many;
    for (int i = 0; i < 257; ++i) {
        auto event = seed;
        event.setId("large-" + std::to_string(i));
        many.push_back(event);
    }
    tooMany.setEvents(std::move(many));
    tooMany.clearReviews();
    tooMany.validate();
    const auto evidencePath = (outputRoot / "output/protected_evidence.txt").string();
    {
        std::ofstream out(evidencePath);
        out << "previous evidence";
    }
    rejects([&] { tooMany.evidence(evidencePath); }, "oversized evidence rejected before writing");
    {
        std::ifstream in(evidencePath);
        std::string text;
        std::getline(in, text);
        require(text == "previous evidence", "rejected evidence preserves existing output");
    }
    const auto catalogueRoot = outputRoot / "catalogue-input";
    std::filesystem::create_directories(catalogueRoot);
    for (const auto &row :
         std::vector<std::string>{"", "../outside.csv,name,purpose,150,0.8,3,campus\n",
                                  "security_sample.csv,name,purpose,0,0.8,3,campus\n",
                                  "security_sample.csv,name,purpose,150,0.8,3.5,campus\n",
                                  "security_sample.csv,name,purpose,150,0.8,257,campus\n",
                                  "security_sample.csv,name,purpose,150,nan,3,campus\n",
                                  "security_sample.csv,name,purpose,150,0.8,3,invalid\n",
                                  "security_sample.csv,name,purpose,150,0.8,3,campus,extra\n",
                                  "security_sample.csv,name,purpose,150,0.8,3,campus\nsecurity_"
                                  "sample.csv,duplicate,purpose,150,0.8,3,campus\n"}) {
        {
            std::ofstream out(catalogueRoot / "security_samples.csv");
            out << "file,name,purpose,geo_radius,feature_radius,min_pts,fence\n" << row;
        }
        rejects([&] { securitySamples(catalogueRoot.string()); },
                "invalid sample catalogue rejected");
    }
}
