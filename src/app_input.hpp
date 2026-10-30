// Included inside App: app input.
// Translate native input into logical coordinates once, then delegate domain edits to Scene.
bool pressed(int key) const {
    return pressedKeys.count(key) != 0;
}
void input() {
    // Drain raylib's press-event queue once per frame. A quick down/up tap
    // can finish between frames; polling only the current key state would
    // miss it. Settings and the canvas share this same immutable frame set.
    pressedKeys.clear();
    for (int key = GetKeyPressed(); key != 0; key = GetKeyPressed())
        pressedKeys.insert(key);
    if (pressed(KEY_F1))
        help = !help;
    if (pressed(KEY_ESCAPE)) {
        help = false;
        samplePicker = false;
        noteOpen = false;
        settings.close();
        routePick = RoutePick::None;
    }
    if (help) {
        if (pressed(KEY_PAGE_DOWN))
            helpScroll += 240;
        if (pressed(KEY_PAGE_UP))
            helpScroll = std::max(0.0, helpScroll - 240);
        helpScroll = std::max(0.0, helpScroll - GetMouseWheelMove() * 48.0);
        return;
    }
    if (settings.isOpen() || samplePicker || noteOpen)
        return;
    if (IsFileDropped()) {
        auto list = LoadDroppedFiles();
        try {
            if (list.count)
                loadFile(list.paths[0]);
        } catch (...) {
            UnloadDroppedFiles(list);
            throw;
        }
        UnloadDroppedFiles(list);
    }
    if (pressed(KEY_ENTER) && !ButtonFocus::hasFocus())
        execute();
    if (pressed(KEY_SPACE) && run)
        playback.setPlaying(!playback.playing());
    if (pressed(KEY_RIGHT) && run)
        playback.step(1, run->result().trace().frames().size());
    if (pressed(KEY_LEFT) && run)
        playback.step(-1, run->result().trace().frames().size());
    if (pressed(KEY_HOME))
        playback.reset();
    if (pressed(KEY_END) && run) {
        finalFrame();
    }
    if (pressed(KEY_N))
        nextSample();
    if (pressed(KEY_F))
        view.fit(scene, feature());
    if (pressed(KEY_P) && !feature())
        polygonTool = !polygonTool;
    if (pressed(KEY_DELETE))
        removeSelected();
    bool control = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL) ||
                   IsKeyReleased(KEY_LEFT_CONTROL) || IsKeyReleased(KEY_RIGHT_CONTROL) ||
                   pressed(KEY_LEFT_CONTROL) || pressed(KEY_RIGHT_CONTROL);
    if (control && pressed(KEY_Q)) {
        closeRequested = true;
        return;
    }
    if (control && pressed(KEY_Z))
        restoreEdit(false);
    if (control && pressed(KEY_Y))
        restoreEdit(true);
    if (control && pressed(KEY_S))
        saveNamed();
    else if (pressed(KEY_S))
        save();
    if (control && pressed(KEY_O))
        openNamed();
    if (pressed(KEY_L))
        loadFile(
            (root / (securityView ? "output/session.security" : "output/session.scene")).string());
    if (pressed(KEY_C))
        compare();
    if (pressed(KEY_V))
        comparePrevious();
    if (pressed(KEY_PERIOD) && !scene.points().empty())
        selected = (selected + 1) % static_cast<int>(scene.points().size());
    if (pressed(KEY_COMMA) && !scene.points().empty())
        selected = (selected <= 0 ? static_cast<int>(scene.points().size()) : selected) - 1;
    if (pressed(KEY_LEFT_BRACKET))
        playback.setSeconds(std::min(2.0, playback.seconds() * 1.5));
    if (pressed(KEY_RIGHT_BRACKET))
        playback.setSeconds(std::max(0.02, playback.seconds() / 1.5));
    if (pressed(KEY_E))
        exportEvidence();
    if (pressed(KEY_O) && !control)
        openSettings();
    if (pressed(KEY_F2))
        openSamples();
    if (pressed(KEY_F3))
        setView((securityView + 1) % 3);
    if (pressed(KEY_F4)) {
        do {
            mode = static_cast<Mode>((static_cast<int>(mode) + 1) %
                                     static_cast<int>(algorithmDescriptors().size()));
        } while (!modeAvailable(mode));
        invalidate();
    }
    if (pressed(KEY_F5))
        togglePresentation();
    if (control && pressed(KEY_EQUAL))
        Ui::adjustText(1);
    if (control && pressed(KEY_MINUS))
        Ui::adjustText(-1);
    if (pressed(KEY_F6)) {
        selectedEdge = scene.edges().empty()
                           ? -1
                           : (selectedEdge + 1) % static_cast<int>(scene.edges().size());
        selected = -1;
    }
    if (pressed(KEY_F8) && securityView) {
        const bool show = !sensitivityVisible;
        syncFence(true);
        sensitivityCache = security.sensitivity();
        sensitivityVisible = show;
        status =
            show ? "Sensitivity comparison shown in inspector." : "Sensitivity comparison hidden.";
    }
    if (pressed(KEY_PAGE_DOWN))
        sidebarScroll += 240;
    if (pressed(KEY_PAGE_UP))
        sidebarScroll = std::max(0.0, sidebarScroll - 240);
    if (pressed(KEY_F7) && securityView) {
        resetFilters();
    }
    if (pressed(KEY_B) && selectedEdge >= 0 && !securityView) {
        scene.setEdgeBlocked(selectedEdge, !scene.edges()[selectedEdge].blocked());
        invalidate();
    }
    if (pressed(KEY_R) && selected >= 0 && !securityView) {
        scene.setSource(selected);
        invalidate();
    }
    if (pressed(KEY_T) && selected >= 0 && !securityView) {
        scene.setGoal(selected);
        invalidate();
    }
    if (control && pressed(KEY_I) && (!securityView || (polygonTool && !feature()))) {
        if (polygonTool)
            scene.addPolygonVertex(scene.query());
        else
            scene.addPoint(scene.query());
        invalidate();
        syncFence();
    }
    if (control && selected >= 0 && !securityView) {
        auto p = scene.points().at(selected);
        double dx = (pressed(KEY_RIGHT) ? 1 : 0) - (pressed(KEY_LEFT) ? 1 : 0),
               dy = (pressed(KEY_UP) ? 1 : 0) - (pressed(KEY_DOWN) ? 1 : 0);
        if (dx || dy) {
            scene.movePoint(selected, {p.x() + dx, p.y() + dy});
            invalidate();
        }
    }
    bool shiftKey = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT) ||
                    pressed(KEY_LEFT_SHIFT) || pressed(KEY_RIGHT_SHIFT);
    if (shiftKey) {
        auto p = scene.query();
        double dx = (pressed(KEY_RIGHT) ? 10 : 0) - (pressed(KEY_LEFT) ? 10 : 0),
               dy = (pressed(KEY_UP) ? 10 : 0) - (pressed(KEY_DOWN) ? 10 : 0);
        if (dx || dy) {
            Point q{p.x() + dx, p.y() + dy};
            scene.setQuery(q);
            if (securityView)
                security.updateSettings([&](SecuritySettings &draft) {
                    if (feature())
                        draft.setFeatureQuery(q);
                    else
                        draft.setGeoQuery(q);
                });
            invalidate();
        }
    }
    auto mouse = Ui::mouse();
    if (!compareVisible && CheckCollisionPointRec(mouse, view.area)) {
        float wheel = GetMouseWheelMove();
        if (wheel) {
            auto before = view.world(mouse);
            view.zoom = std::clamp(view.zoom * std::pow(1.15, wheel), 1e-8, 200.0);
            auto after = view.world(mouse);
            view.centre.setX(View::bounded(view.centre.x() + (before.x() - after.x())));
            view.centre.setY(View::bounded(view.centre.y() + (before.y() - after.y())));
        }
        if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
            auto delta = Ui::mouseDelta();
            view.centre.setX(View::bounded(view.centre.x() - (delta.x / view.zoom)));
            view.centre.setY(View::bounded(view.centre.y() + (delta.y / view.zoom)));
        }
        Point world = view.world(mouse);
        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            scene.setQuery(world);
            if (securityView == 1)
                security.updateSettings([&](SecuritySettings &draft) { draft.setGeoQuery(world); });
            if (securityView == 2)
                security.updateSettings(
                    [&](SecuritySettings &draft) { draft.setFeatureQuery(world); });
            invalidate();
        }
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            int near = -1;
            double best = 12;
            auto &points = polygonTool ? scene.polygon() : scene.points();
            for (std::size_t i = 0; i < points.size(); ++i) {
                auto p = view.screen(points[i]);
                double dist = std::hypot(p.x - mouse.x, p.y - mouse.y);
                if (dist < best) {
                    near = static_cast<int>(i);
                    best = dist;
                }
            }
            bool shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
            if (routePick != RoutePick::None) {
                if (!pickEndpoint(near))
                    status = "Click an existing point to choose the route endpoint. "
                             "Esc cancels.";
            } else if (polygonTool) {
                if (near >= 0)
                    polygonDrag = near;
                else {
                    scene.addPolygonVertex(world);
                    invalidate();
                    syncFence();
                }
            } else if (near >= 0) {
                selected = near;
                selectedEdge = -1;
                if (shift && !securityView) {
                    if (edgeStart < 0) {
                        edgeStart = near;
                        status = "Edge start selected. Shift-click its other endpoint.";
                    } else if (edgeStart != near) {
                        const auto from = std::exchange(edgeStart, -1);
                        scene.addEdge(
                            {from, near,
                             GeometryKernel::distance(scene.points()[from], scene.points()[near]),
                             false});
                        invalidate();
                    }
                } else if (!securityView)
                    drag = near;
            } else {
                selectedEdge = -1;
                for (std::size_t i = 0; i < scene.edges().size(); ++i) {
                    auto e = scene.edges()[i];
                    if (segmentPixelDistance(mouse, view.screen(scene.points()[e.a()]),
                                             view.screen(scene.points()[e.b()])) < 7) {
                        selectedEdge = static_cast<int>(i);
                        break;
                    }
                }
                if (selectedEdge >= 0) {
                    selected = -1;
                    status = "Edge selected. B blocks/unblocks; Settings edits its weight.";
                } else if (!securityView) {
                    scene.addPoint(world);
                    scene.setGoal(static_cast<int>(scene.points().size() - 1));
                    invalidate();
                } else
                    selected = -1;
            }
        }
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && drag >= 0) {
            auto old = scene.points()[drag];
            if (GeometryKernel::distance2(old, world) > TOL) {
                scene.movePoint(drag, world);
                invalidate();
            }
        }
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && polygonDrag >= 0) {
            auto old = scene.polygon()[polygonDrag];
            if (GeometryKernel::distance2(old, world) > TOL) {
                scene.movePolygonVertex(polygonDrag, world);
                invalidate();
                syncFence();
            }
        }
    }
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        drag = -1;
        polygonDrag = -1;
    }
    if (CheckCollisionPointRec(mouse, sidebarBounds()))
        sidebarScroll = std::max(0.0, sidebarScroll - GetMouseWheelMove() * 35.0);
    playback.update(GetFrameTime(), run ? run->result().trace().frames().size() : 0);
}
