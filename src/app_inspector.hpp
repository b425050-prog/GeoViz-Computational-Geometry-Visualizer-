// Included inside App: app inspector.
// Return a read-only snapshot at the playback cursor, or no frame before a run.
const Frame *frame() const {
    return run && !run->result().trace().frames().empty()
               ? &run->result().trace().frames()[playback.cursor()]
               : nullptr;
}
// Supply a read-only presentation context; the renderer cannot mutate app state.
void canvas() const {
    if (compareVisible && comparison && run) {
        View left = view, right = view;
        left.area.width = (view.area.width - 12) / 2;
        right.area = left.area;
        right.area.x += left.area.width + 12;
        left.fit(run->input(), feature());
        right.fit(comparison->input(), feature());
        const auto &frames = comparison->result().trace().frames();
        auto count = run->result().trace().frames().size();
        auto index = frames.empty() ? 0
                                    : playback.cursor() * (frames.size() - 1) /
                                          std::max<std::size_t>(1, count - 1);
        SceneCanvas({run->input(), left, frame(), run->result().mode(), sampleVariant, securityView,
                     -1, -1, false, map, security.settings(), "Current"})
            .draw();
        SceneCanvas({comparison->input(), right, frames.empty() ? nullptr : &frames[index],
                     comparison->result().mode(), sampleVariant, securityView, -1, -1, false, map,
                     security.settings(), "Comparison"})
            .draw();
        for (const auto &pair : std::vector<std::pair<View, const Run *>>{
                 {left, run.get()}, {right, comparison.get()}}) {
            const auto &box = pair.first.bounds();
            DrawRectangleRec({box.x, box.y, box.width, 70}, CANVAS);
            wrap(modeName(pair.second->result().mode()) + " | " + pair.second->result().summary(),
                 box.x + 12, box.y + 12, box.width - 24, 17, ACCENT);
        }
    } else
        SceneCanvas({scene, view, frame(), mode, sampleVariant, securityView, selected,
                     selectedEdge, polygonTool, map, security.settings(), securityDataset})
            .draw();
    if (scene.points().empty())
        wrap(securityView ? "No observations match the current filters. F7 resets user/device "
                            "filters; Settings changes the UTC window."
                          : "Empty scene. Choose Samples or click to add a point.",
             view.area.x + 24, view.area.y + 90, view.area.width - 48, 18, FOCUS);
}
// Show results first; the scrollable inspector retains complete source and help details.
void sidebar() {
    auto panel = sidebarBounds();
    float x = panel.x + 16, top = panel.y, bottom = panel.y + panel.height - 32;
    float contentWidth = panel.width - 32;
    DrawRectangleRounded(panel, 0.025f, 6, PANEL);
    Ui::clip(panel.x + 8, top, panel.width - 16, panel.height - 32);
    float y = top + 14 - static_cast<float>(sidebarScroll);
    if (!presentation && !securityView && (mode == Mode::Dijkstra || mode == Mode::AStar)) {
        bool interactive =
            !help && !settings.isOpen() && CheckCollisionPointRec(Ui::mouse(), panel);
        float half = (contentWidth - 8) / 2;
        bool sourceClicked =
            button({x, y, half, 38}, "Pick source", routePick == RoutePick::Source);
        bool goalClicked = button({x + half + 8, y, half, 38}, "Pick destination",
                                  routePick == RoutePick::Destination);
        if (interactive && (sourceClicked || goalClicked)) {
            routePick = sourceClicked ? RoutePick::Source : RoutePick::Destination;
            polygonTool = false;
            drag = polygonDrag = edgeStart = -1;
            status = sourceClicked ? "Click a point on the canvas to set SOURCE. Esc cancels."
                                   : "Click a point on the canvas to set DESTINATION. Esc "
                                     "cancels.";
        }
        y += 46;
        y = wrap("Source #" + std::to_string(scene.source()) + "  |  Destination #" +
                     std::to_string(scene.goal()),
                 x, y, contentWidth, 16, ACCENT);
    }
    y += 16;
    text("INSPECT / EXPLAIN", x, y, 20, ACCENT);
    y += 34;
    y = wrap("Mode: " + modeName(mode), x, y, contentWidth);
    y = wrap("Points " + std::to_string(scene.points().size()) + " | radius " +
                 number(scene.radius()) + " | MinPts " + std::to_string(scene.minPts()),
             x, y + 8, contentWidth, 15, MUTED);
    if (polygonTool)
        y = wrap("Polygon tool ON. Press P to return to points.", x, y + 8, contentWidth, 15,
                 FOCUS);
    if (run) {
        auto &r = run->result();
        y = wrap(r.summary(), x, y + 14, contentWidth, 18);
        y = wrap(r.check(), x, y + 8, contentWidth, 15, r.checked() ? ACCENT : DANGER);
        y = wrap("Operations " + std::to_string(r.stats().operations()) + " | queries " +
                     std::to_string(r.stats().queries()) + " | expanded " +
                     std::to_string(r.stats().expanded()),
                 x, y + 8, contentWidth, 15, MUTED);
        y = wrap("Step " + std::to_string(playback.cursor() + 1) + " / " +
                     std::to_string(r.trace().frames().size()) +
                     (r.trace().truncated() ? " (trace capped; final frame retained)" : ""),
                 x, y + 8, contentWidth, 16);
        Rectangle timeline{x, y + 10, contentWidth, 18};
        DrawRectangleRec(timeline, SOFT_BG);
        float portion = static_cast<float>(playback.cursor()) /
                        std::max<std::size_t>(1, r.trace().frames().size() - 1);
        DrawRectangle(static_cast<int>(x), static_cast<int>(timeline.y),
                      static_cast<int>(portion * contentWidth), 18, ACCENT);
        if (!help && !settings.isOpen() && !samplePicker &&
            CheckCollisionPointRec(Ui::mouse(), timeline) && IsMouseButtonDown(MOUSE_BUTTON_LEFT))
            playback.seek(static_cast<std::size_t>(std::round((Ui::mouse().x - x) / contentWidth *
                                                              (r.trace().frames().size() - 1))),
                          r.trace().frames().size());
        y += 40;
        if (auto f = frame()) {
            y = wrap("Pseudocode line " + std::to_string(f->pseudocodeLine()) + ": " +
                         pseudoStep(mode, f->explanation()),
                     x, y + 6, contentWidth, 15, ACCENT);
            y = wrap(f->explanation(), x, y + 12, contentWidth, 17, FOCUS);
            if (!f->queue().empty()) {
                std::string q = "Queue:";
                for (std::size_t i = 0; i < std::min<std::size_t>(f->queue().size(), 18); ++i)
                    q += " " + std::to_string(f->queue()[i]);
                y = wrap(q, x, y + 8, contentWidth, 15);
            }
            if (!f->costs().empty()) {
                y = wrap("ID | g | f | predecessor (scroll for all)", x, y + 8, contentWidth, 14,
                         MUTED);
                for (std::size_t i = 0; i < f->costs().size(); ++i)
                    y = wrap(std::to_string(i) + " | " + number(f->costs()[i]) + " | " +
                                 number(f->scores()[i]) + " | " + std::to_string(f->parents()[i]),
                             x, y, contentWidth, 14);
            }
            if (selected >= 0 && selected < static_cast<int>(f->costs().size())) {
                y = wrap("Selected g=" + number(f->costs()[selected]) +
                             " f=" + number(f->scores()[selected]) +
                             " predecessor=" + std::to_string(f->parents()[selected]),
                         x, y + 8, contentWidth, 15);
            }
        }
    } else
        y = wrap("No current trace. Press Enter or Run.", x, y + 16, contentWidth, 17, FOCUS);
    if (selectedEdge >= 0 && selectedEdge < static_cast<int>(scene.edges().size())) {
        auto e = scene.edges()[selectedEdge];
        y = wrap("Selected edge " + std::to_string(e.a()) + " - " + std::to_string(e.b()) +
                     " cost=" + number(e.weight()) + (e.blocked() ? " BLOCKED" : "") +
                     ". B toggles; Settings edits cost.",
                 x, y + 12, contentWidth, 15);
    }
    if (selected >= 0 && selected < static_cast<int>(scene.points().size())) {
        auto p = scene.points()[selected];
        y = wrap("Selected #" + std::to_string(selected) + " (" + number(p.x()) + ", " +
                     number(p.y()) + ")",
                 x, y + 16, contentWidth, 16, ACCENT);
        if (auto current = frame(); current &&
                                    (mode == Mode::DBSCAN || mode == Mode::IndexedDBSCAN) &&
                                    selected < static_cast<int>(current->labels().size())) {
            int label = current->labels()[selected], kind = current->kind()[selected];
            y = wrap("Current label: " + (label == -2   ? std::string("unassigned")
                                          : label == -1 ? std::string("NOISE")
                                                        : "cluster " + std::to_string(label) +
                                                              (kind == 2 ? " CORE" : " BORDER")),
                     x, y + 6, contentWidth, 16);
        }
        if (securityView && selected < static_cast<int>(observations.size())) {
            auto &o = observations[selected];
            y = wrap("Identity: " + o.name(), x, y + 6, contentWidth, 16);
            if (feature())
                y = wrap("Raw request/min=" + number(o.requestRate()) +
                             " failed/min=" + number(o.failureRate()),
                         x, y + 5, contentWidth, 15);
            Stats stats;
            KDTree tree(scene.points());
            auto ns = tree.radius(p, scene.radius(), stats);
            std::string neighbours = "Neighbours:";
            for (int id : ns)
                neighbours += " " + std::to_string(id);
            y = wrap(neighbours, x, y + 8, contentWidth, 15);
            if (securityView == 1 && !scene.polygon().empty()) {
                int m = GeometryKernel::membership(p, scene.polygon());
                y = wrap(m == 1   ? "Geofence: INSIDE"
                         : m == 0 ? "Geofence: BOUNDARY"
                                  : "Geofence: OUTSIDE",
                         x, y + 5, contentWidth, 15);
            }
            y = wrap("Source records: " + std::to_string(o.events().size()) +
                         ". Scroll panel to inspect all.",
                     x, y + 8, contentWidth, 15, MUTED);
            for (int id : o.events()) {
                auto &e = security.events()[id];
                y = wrap(e.id() + " | " + e.user() + " | " + e.device() + " | UTC " +
                             std::to_string(e.timestamp()),
                         x, y + 10, contentWidth, 14);
                y = wrap("requests=" + std::to_string(e.requests()) +
                             " failures=" + std::to_string(e.failures()) + " truth=" + e.label(),
                         x, y + 3, contentWidth, 14);
                y = wrap(e.summary(), x, y + 3, contentWidth, 15);
                y = wrap("Review: " + (security.reviews().count(e.id())
                                           ? security.reviews().at(e.id())
                                           : "unreviewed"),
                         x, y + 3, contentWidth, 14, ACCENT);
            }
        }
    }
    if (!scene.points().empty()) {
        y = wrap("TEXT TABLE | ID / x / y", x, y + 16, contentWidth, 16, ACCENT);
        for (std::size_t i = 0; i < scene.points().size(); ++i)
            y = wrap(std::to_string(i) + " / " + exactNumber(scene.points()[i].x()) + " / " +
                         exactNumber(scene.points()[i].y()),
                     x, y, contentWidth, 14);
    }
    if (previous)
        y = wrap("Previous retained: " + modeName(previous->result().mode()) + " value " +
                     number(previous->result().value()),
                 x, y + 16, contentWidth, 15, MUTED);
    if (securityView) {
        if (sensitivityVisible)
            y = wrap(sensitivityCache, x, y + 16, contentWidth, 15, FOCUS);
        try {
            if (evaluationDirty) {
                evaluationCache = security.evaluation();
                evaluationDirty = false;
            }
            y = wrap(evaluationCache, x, y + 16, contentWidth, 14, MUTED);
        } catch (const std::exception &e) {
            y = wrap(std::string("Fix the polygon/settings before evaluation: ") + e.what(), x,
                     y + 16, contentWidth, 14, FOCUS);
        }
    }
    y += 24;
    text("HOW TO USE", x, y, 20, ACCENT);
    y += 34;
    int step = 1;
    for (const auto &instruction : AlgorithmHelp::steps(mode, securityView))
        y = wrap(std::to_string(step++) + ". " + instruction, x, y, contentWidth, 16) + 8;
    y = wrap("Scroll this panel. F1 opens all controls.", x, y + 16, contentWidth, 14, MUTED);
    sidebarScroll = std::min(sidebarScroll,
                             std::max(0.0, static_cast<double>(y + sidebarScroll - bottom + 24)));
    EndScissorMode();
    text("Scroll for results and details", x, bottom + 6, 14, MUTED);
}
