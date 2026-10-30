#include "app.hpp"

namespace gv::ui {
Rectangle App::sidebarBounds() const {
    float width = presentation ? 334.0f : std::clamp(Ui::width() * 0.27f, 360.0f, 440.0f);
    return {Ui::width() - width - 16, presentation ? 56.0f : 142.0f, width,
            Ui::height() - (presentation ? 150.0f : 314.0f)};
}
// Recompute logical bounds after resize; screen scaling is applied once by Ui.
void App::updateLayout() {
    auto panel = sidebarBounds();
    if (presentation) {
        panel.x = Ui::width() - 350;
        panel.width = 334;
    }
    view.area = {16, panel.y, panel.x - 32, panel.height};
}
// Source/destination picking is an explicit tool state, separate from point dragging.
bool App::pickEndpoint(int id) {
    if (routePick == RoutePick::None || securityView || id < 0 ||
        id >= static_cast<int>(scene.points().size()))
        return false;
    bool source = routePick == RoutePick::Source;
    if (source)
        scene.setSource(id);
    else
        scene.setGoal(id);
    selected = id;
    routePick = RoutePick::None;
    invalidate();
    status = std::string(source ? "Source" : "Destination") + " set to point #" +
             std::to_string(id) + ". Run to calculate the route.";
    return true;
}

bool App::feature() const {
    return securityView == 2;
}
bool App::modeAvailable(Mode candidate) const {
    return !securityView || (algorithmDescriptors()[static_cast<int>(candidate)].security &&
                             !(feature() && candidate == Mode::Geofence));
}
void App::setMode(Mode candidate) {
    if (candidate == mode || !modeAvailable(candidate))
        return;
    mode = candidate;
    routePick = RoutePick::None;
    invalidate();
    sidebarScroll = 0;
    status = modeName(mode) + " selected. Input retained; Run to calculate.";
}
void App::invalidateSecurityDisplays() {
    evaluationDirty = true;
    sensitivityVisible = false;
    sensitivityCache.clear();
}
// Move the old result to history before resetting playback; stale evidence is never current.
void App::invalidate() {
    if (securityView)
        invalidateSecurityDisplays();
    comparison.reset();
    compareVisible = false;
    if (run)
        previous = std::move(run);
    playback.reset();
    status = "Input changed. Old trace invalidated; Run again.";
}
// The geographic editor is the authority for its fence; the feature view has no polygon.
void App::syncFence(bool required) {
    if (!securityView)
        return;
    if (securityView == 1)
        fenceDraft = scene.polygon();
    invalidateSecurityDisplays();
    try {
        security.setFence(fenceDraft);
        evaluationDirty = true;
    } catch (const std::exception &) {
        if (required)
            throw;
        status = "Geofence draft is unfinished/invalid. Complete it before running, saving or "
                 "exporting security.";
    }
}
// Publish a fully prepared security view only after validation succeeds.
void App::publishSecurity(PreparedSecurity prepared) {
    scene = std::move(prepared.scene);
    observations = std::move(prepared.observations);
    map = std::move(prepared.map);
    invalidate();
    resetInteractions();
    view.fit(scene, feature());
}
void App::refreshSecurity() {
    auto prepared =
        PreparedSecurity::prepare(security, feature(), (root / "data/campus_map.csv").u8string());
    if (!feature())
        prepared.scene.setPolygon(fenceDraft);
    publishSecurity(std::move(prepared));
}
void App::resetFilters() {
    auto candidate = security;
    candidate.updateSettings([](SecuritySettings &draft) {
        draft.setUser("*");
        draft.setDevice("*");
    });
    auto prepared =
        PreparedSecurity::prepare(candidate, feature(), (root / "data/campus_map.csv").u8string());
    if (!feature())
        prepared.scene.setPolygon(scene.polygon());
    security = std::move(candidate);
    publishSecurity(std::move(prepared));
}
void App::resetInteractions() {
    selected = selectedEdge = edgeStart = drag = polygonDrag = -1;
    routePick = RoutePick::None;
    polygonTool = false;
}
WorkspaceView App::workspaceView() const {
    return {securityView ? labScene : scene, security,
            securityView == 1 ? scene.polygon() : fenceDraft};
}
WorkspaceState App::workspace() const {
    return workspaceView().snapshot();
}
void App::refreshDirty() {
    labDirty = labKey() != savedLabKey;
    securityDirty = securityKey() != savedSecurityKey;
}
std::string App::labKey() const {
    return workspaceKey({securityView ? labScene : scene, SecurityConsole{}});
}
std::string App::securityKey() const {
    return workspaceKey({Scene{}, security, securityView == 1 ? scene.polygon() : fenceDraft});
}
bool App::dirty() const {
    return labDirty || securityDirty || edits.pending(workspaceView());
}
void App::markSaved(bool laboratory) {
    if (laboratory)
        savedLabKey = labKey();
    else
        savedSecurityKey = securityKey();
    edits.observe(workspaceView());
    refreshDirty();
}
void App::recover() {
    files.writeRecovery(workspaceView());
}
void App::protectWork() {
    if (!smoke && dirty())
        files.preserve(workspaceView());
}
void App::finalFrame() {
    if (run) {
        const auto count = run->result().trace().frames().size();
        playback.seek(count ? count - 1 : 0, count);
    }
}
void App::recordEdit() {
    if (drag >= 0 || polygonDrag >= 0)
        return;
    if (edits.observe(workspaceView())) {
        refreshDirty();
        if (!smoke)
            recover();
    }
}
void App::restoreEdit(bool redo) {
    if (redo ? !edits.canRedo() : !edits.canUndo()) {
        status = "No further edit history.";
        return;
    }
    auto state = redo ? edits.redo() : edits.undo();
    labScene = state.lab;
    fenceDraft = state.fenceDraft;
    security = state.security;
    if (securityView) {
        refreshSecurity();
        if (securityView == 1)
            scene.setPolygon(fenceDraft);
    } else {
        scene = labScene;
        invalidate();
        view.fit(scene);
    }
    resetInteractions();
    refreshDirty();
    if (!smoke)
        recover();
    status = redo ? "Edit redone." : "Edit undone.";
}
void App::chooseSample(int index) {
    const int count =
        securityView ? static_cast<int>(securityCatalogue.size()) : SampleVariantCount;
    if (index < 0 || index >= count)
        throw std::out_of_range("That sample is unavailable. Choose one from Samples.");
    if (securityView) {
        const auto &sample = securityCatalogue.at(index);
        auto next = loadSecuritySample((root / "data").u8string(), sample);
        auto prepared =
            PreparedSecurity::prepare(next, feature(), (root / "data/campus_map.csv").u8string());
        protectWork();
        security = std::move(next);
        fenceDraft = security.fence();
        securitySample = index;
        securityDataset = sample.name;
        publishSecurity(std::move(prepared));
        status = sample.name + ": " + sample.purpose;
    } else {
        auto next = sampleScene(mode, index);
        protectWork();
        resetInteractions();
        sampleVariant = index;
        scene = std::move(next);
        labInputName = sampleName(mode, index) + " | " + std::to_string(index + 1) + "/6";
        labInputRevision = scene.revision();
        invalidate();
        view.fit(scene);
        status = sampleName(mode, index);
    }
    sidebarScroll = 0;
    samplePicker = false;
}
// Sample replaces the complete input context, including stale selections.
void App::nextSample() {
    chooseSample(securityView ? (securitySample + 1) % static_cast<int>(securityCatalogue.size())
                              : (sampleVariant + 1) % SampleVariantCount);
}
// Clear view-specific interaction and history before changing the coordinate units.
void App::setView(int v) {
    if (v == securityView)
        return;
    syncFence();
    PreparedSecurity prepared;
    if (v)
        prepared =
            PreparedSecurity::prepare(security, v == 2, (root / "data/campus_map.csv").u8string());
    if (!securityView) {
        labScene = scene;
        labMode = mode;
        labView = view;
        labSelected = selected;
        labEdge = selectedEdge;
    }
    if (!securityView)
        labSessionPath = sessionPath;
    else
        securitySessionPath = sessionPath;
    securityView = v;
    invalidateSecurityDisplays();
    sessionPath = v ? securitySessionPath : labSessionPath;
    resetInteractions();
    sidebarScroll = 0;
    run.reset();
    previous.reset();
    comparison.reset();
    compareVisible = false;
    playback.reset();
    if (v) {
        mode = Mode::IndexedDBSCAN;
        scene = std::move(prepared.scene);
        if (v == 1)
            scene.setPolygon(fenceDraft);
        observations = std::move(prepared.observations);
        map = std::move(prepared.map);
        view.fit(scene, feature());
    } else {
        scene = labScene;
        mode = labMode;
        view = labView;
        updateLayout();
        selected = labSelected >= 0 && labSelected < static_cast<int>(scene.points().size())
                       ? labSelected
                       : -1;
        selectedEdge =
            labEdge >= 0 && labEdge < static_cast<int>(scene.edges().size()) ? labEdge : -1;
    }
    status = v ? "Security workspace. Settings controls filters, UTC window and scales."
               : "Laboratory edits restored.";
}
// Freeze input and result together; playback reads this Run while later edits invalidate it.
void App::execute() {
    syncFence(true);
    Result r = engine->execute(scene, mode, true);
    if (run)
        previous = std::move(run);
    run = std::make_unique<Run>(scene, std::move(r));
    playback.reset();
    status = run->result().summary() + " " + run->result().check();
}
// Dispatch by input format and validate temporary security state before replacing current data.
void App::loadFile(const std::string &path) {
    protectWork();
    auto ext = std::filesystem::u8path(path).extension().u8string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (ext == ".csv" || ext == ".security") {
        auto next = ext == ".security" ? SecurityConsole::load(path) : security;
        if (ext == ".csv")
            next.replace(CsvLogAdapter{}.load(path), next.settings(), next.fence());
        auto prepared =
            PreparedSecurity::prepare(next, false, (root / "data/campus_map.csv").u8string());
        if (!securityView) {
            labScene = scene;
            labMode = mode;
            labView = view;
            labSelected = selected;
            labEdge = selectedEdge;
            labSessionPath = sessionPath;
        }
        security = std::move(next);
        fenceDraft = security.fence();
        securityView = 1;
        mode = Mode::IndexedDBSCAN;
        scene = std::move(prepared.scene);
        observations = std::move(prepared.observations);
        map = std::move(prepared.map);
        securityDataset = std::filesystem::u8path(path).filename().u8string();
        evaluationDirty = true;
    } else {
        auto next = SceneIO::load(path);
        if (securityView) {
            syncFence();
            securitySessionPath = sessionPath;
            mode = labMode;
        }
        securityView = 0;
        scene = std::move(next);
        labScene = scene;
        labInputName = std::filesystem::u8path(path).filename().u8string();
        labInputRevision = scene.revision();
    }
    resetInteractions();
    invalidate();
    view.fit(scene, feature());
    sessionPath = std::filesystem::u8path(path);
    if (securityView)
        securitySessionPath = sessionPath;
    else
        labSessionPath = sessionPath;
    recentFiles.erase(std::remove(recentFiles.begin(), recentFiles.end(), path), recentFiles.end());
    recentFiles.insert(recentFiles.begin(), path);
    if (recentFiles.size() > 8)
        recentFiles.resize(8);
    markSaved(!securityView);
    status = "Loaded " + path;
    try {
        files.writeRecent(recentFiles);
    } catch (const std::exception &error) {
        status += ". Recent list was not saved: " + std::string(error.what());
    }
}
void App::openNamed() {
    auto path = chooseSessionFile(false, securityView != 0, sessionPath.u8string());
    if (!path.empty())
        loadFile(path);
}
void App::saveNamed() {
    auto initial = sessionPath.empty()
                       ? root / "output" / (securityView ? "session.security" : "session.scene")
                       : sessionPath;
    initial.replace_extension(securityView ? ".security" : ".scene");
    auto path = chooseSessionFile(true, securityView != 0, initial.u8string());
    if (!path.empty()) {
        const auto previousPath = sessionPath;
        sessionPath = std::filesystem::u8path(path);
        try {
            save();
        } catch (...) {
            sessionPath = previousPath;
            throw;
        }
    }
}
// Save only input state; exported evidence separately includes checked results.
void App::save() {
    syncFence(true);
    auto path = sessionPath.empty()
                    ? root / (securityView ? "output/session.security" : "output/session.scene")
                    : sessionPath;
    if (securityView && path.extension() != ".security")
        path.replace_extension(".security");
    if (!securityView && path.extension() != ".scene")
        path.replace_extension(".scene");
    if (securityView)
        security.save(path.u8string());
    else
        SceneIO::save(scene, path.u8string());
    sessionPath = path;
    if (securityView)
        securitySessionPath = path;
    else
        labSessionPath = path;
    markSaved(!securityView);
    status = "Saved " + path.u8string();
}
// Keep evidence beside its exact reproducible snapshots in the generated output directory.
void App::exportEvidence() {
    syncFence(true);
    if (securityView) {
        AtomicFiles files;
        security.evidence((root / "output/evidence.txt").u8string(), &files);
        if (run)
            SceneIO::summary(*run, (root / "output/security_selected_run.txt").u8string(), &files);
        files.commit();
        status = "Exported evidence, full security snapshot and both view scenes "
                 "in output/.";
    } else {
        if (!run)
            throw std::runtime_error("Run an algorithm before exporting a checked summary.");
        SceneIO::summary(*run, (root / "output/run.txt").u8string());
        status = "Exported output/run.txt and its exact scene snapshot.";
    }
}
// Compare equivalent algorithms on the same scene, never on a previous edited input.
void App::compare() {
    if (compareVisible) {
        compareVisible = false;
        return;
    }
    if (!run)
        throw std::runtime_error("Run first.");
    if (mode == Mode::Dijkstra || mode == Mode::AStar || mode == Mode::DBSCAN ||
        mode == Mode::IndexedDBSCAN) {
        Mode other = mode == Mode::Dijkstra ? Mode::AStar
                     : mode == Mode::AStar  ? Mode::Dijkstra
                     : mode == Mode::DBSCAN ? Mode::IndexedDBSCAN
                                            : Mode::DBSCAN;
        auto result = engine->execute(scene, other, true);
        comparison = std::make_unique<Run>(scene, result);
        compareVisible = true;
        bool same =
            (mode == Mode::DBSCAN || mode == Mode::IndexedDBSCAN)
                ? result.labels() == run->result().labels()
                : ((!std::isfinite(result.value()) && !std::isfinite(run->result().value())) ||
                   equivalentCost(result.value(), run->result().value()));
        status = "Same input: " + modeName(other) + " value=" + number(result.value()) +
                 " ops=" + std::to_string(result.stats().operations()) +
                 " expanded=" + std::to_string(result.stats().expanded()) +
                 " agree=" + (same ? "YES" : "NO");
    } else if (previous)
        status = "Previous " + modeName(previous->result().mode()) +
                 " value=" + number(previous->result().value()) +
                 " ops=" + std::to_string(previous->result().stats().operations()) +
                 "; current value=" + number(run->result().value()) +
                 " ops=" + std::to_string(run->result().stats().operations());
    else
        status = "No previous run yet. Edit or rerun to retain one.";
}
// Historical comparison preserves both input snapshots and their separate result summaries.
void App::comparePrevious() {
    if (!run || !previous)
        throw std::runtime_error("Keep two completed runs before comparing history.");
    status = "Previous " + modeName(previous->result().mode()) + ": " +
             previous->result().summary() +
             " ops=" + std::to_string(previous->result().stats().operations()) + "; current " +
             modeName(run->result().mode()) + ": " + run->result().summary() +
             " ops=" + std::to_string(run->result().stats().operations());
    AtomicFiles files;
    SceneIO::summary(*previous, (root / "output/previous.txt").u8string(), &files);
    SceneIO::summary(*run, (root / "output/current.txt").u8string(), &files);
    files.commit();
}
void App::openSettings() {
    beginModal();
    settings.open(scene, security, selectedEdge, securityView, mode);
}
void App::beginModal() {
    // The shortcut/Enter that opens a dialog must not also edit or submit it.
    while (GetCharPressed()) {
    }
    pressedKeys.erase(KEY_ENTER);
    ButtonFocus::reset();
}
void App::openSamples() {
    beginModal();
    samplePicker = true;
}
void App::openReview(const std::string &verdict) {
    if (!securityView || selected < 0 || selected >= static_cast<int>(observations.size())) {
        status = "Select a source observation before reviewing it.";
        return;
    }
    beginModal();
    noteVerdict = verdict;
    noteText.clear();
    noteCursor.reset();
    noteFocus = 0;
    noteOpen = true;
}
// Commit only after the component has validated every candidate field.
void App::applySettings() {
    auto candidate = settings.candidate(scene, security, selectedEdge, securityView);
    if (securityView) {
        auto prepared = PreparedSecurity::prepare(candidate.security, feature(),
                                                  (root / "data/campus_map.csv").u8string());
        if (!feature())
            prepared.scene.setPolygon(scene.polygon());
        security = std::move(candidate.security);
        publishSecurity(std::move(prepared));
    } else {
        scene = std::move(candidate.scene);
        invalidate();
    }
    settings.close();
    status = "Settings applied. Run again.";
}
// The Scene remaps topology; the UI only clears interaction indices after deletion.
void App::removeSelected() {
    if (securityView)
        throw std::runtime_error("Source observations are inspected through filters; edit the "
                                 "CSV to change records.");
    if (selected < 0)
        return;
    scene.removePoint(selected);
    selected = -1;
    selectedEdge = -1;
    edgeStart = -1;
    invalidate();
}
void App::review(const std::string &verdict, const std::string &note) {
    invalidateSecurityDisplays();
    if (!securityView || selected < 0 || selected >= static_cast<int>(observations.size()))
        return;
    for (int id : observations[selected].events())
        security.setReview(security.events()[id].id(), verdict, note);
    status = "Analyst review set to " + verdict +
             " for this observation's source records. Export to retain evidence.";
}

void App::clearScene() {
    if (securityView)
        throw std::runtime_error("Use filters or import a new security dataset.");
    protectWork();
    scene = Scene{};
    labInputName = "Empty Lab input";
    labInputRevision = scene.revision();
    resetInteractions();
    invalidate();
}
void App::loadGrid() {
    protectWork();
    setView(0);
    mode = Mode::AStar;
    scene = gridScene();
    labInputName = "Obstacle navigation grid";
    labInputRevision = scene.revision();
    resetInteractions();
    invalidate();
    view.fit(scene);
}
void App::clearFence() {
    if (feature())
        return;
    scene.setPolygon({});
    if (securityView == 1)
        fenceDraft.clear();
    resetInteractions();
    invalidate();
    syncFence();
}

void App::togglePresentation() {
    presentation = !presentation;
    updateLayout();
    view.fit(scene, feature());
}

} // namespace gv::ui
