#pragma once

// A modal edit buffer owns field input, presentation and candidate validation.
// It never commits changes to the live scene; App applies a complete candidate.
class SettingsPanel final {
    bool open_ = false;
    std::vector<std::string> fields;
    int field = -1;

  public:
    struct Changes {
        Scene scene;
        SecurityConsole security;
    };
    bool isOpen() const noexcept {
        return open_;
    }
    void close() noexcept {
        open_ = false;
    }
    void setField(std::size_t index, std::string value) {
        fields.at(index) = std::move(value);
    }
    // Format a detached edit buffer; the model changes only when Apply succeeds.
    void open(const Scene &scene, const SecurityConsole &security, int selectedEdge) {
        fields = {number(scene.radius(), 8),
                  std::to_string(scene.minPts()),
                  std::to_string(scene.source()),
                  std::to_string(scene.goal()),
                  selectedEdge >= 0 ? number(scene.edges()[selectedEdge].weight(), 8) : "1",
                  std::to_string(security.settings().start()),
                  std::to_string(security.settings().end()),
                  security.settings().user(),
                  security.settings().device(),
                  number(security.settings().requestScale(), 8),
                  number(security.settings().failureScale(), 8),
                  number(security.settings().featureRadius(), 8),
                  number(security.settings().geoRadius(), 8)};
        field = -1;
        open_ = true;
    }
    Changes candidate(const Scene &scene, const SecurityConsole &security, int selectedEdge,
                      int securityView) const {
        auto d = [&](int i) {
            std::size_t pos = 0;
            double x = std::stod(fields[i], &pos);
            if (pos != fields[i].size() || !std::isfinite(x))
                throw std::runtime_error("Invalid numeric field.");
            return x;
        };
        auto integer = [&](int i) {
            std::size_t pos = 0;
            auto x = std::stoll(fields[i], &pos);
            if (pos != fields[i].size())
                throw std::runtime_error("Invalid integer field.");
            return x;
        };
        auto bounded = [&](int i, int lo, int hi) {
            auto x = integer(i);
            if (x < lo || x > hi)
                throw std::runtime_error("Integer field out of range.");
            return static_cast<int>(x);
        };
        Scene next = scene;
        next.setRadius(d(0));
        next.setMinPts(bounded(1, 1, 256));
        next.setSource(bounded(2, 0, std::max(0, static_cast<int>(scene.points().size()) - 1)));
        next.setGoal(bounded(3, 0, std::max(0, static_cast<int>(scene.points().size()) - 1)));
        if (selectedEdge >= 0)
            next.setEdgeWeight(selectedEdge, d(4));
        Engine::validate(next);
        auto sec = security;
        if (securityView) {
            sec.settings().setStart(integer(5));
            sec.settings().setEnd(integer(6));
            sec.settings().setUser(fields[7]);
            sec.settings().setDevice(fields[8]);
            sec.settings().setRequestScale(d(9));
            sec.settings().setFailureScale(d(10));
            sec.settings().setFeatureRadius(d(11));
            sec.settings().setGeoRadius(d(12));
            sec.settings().setMinPts(next.minPts());
            sec.validate();
            sec.observations(false);
            sec.observations(true);
        }
        return {std::move(next), std::move(sec)};
    }
    // Edit a buffer inside a modal so clicks cannot also modify the canvas underneath.
    bool draw(const std::set<int> &pressedKeys) {
        if (!open_)
            return false;
        auto pressed = [&](int key) { return pressedKeys.count(key) != 0; };
        DrawRectangle(0, 0, Ui::width(), Ui::height(), VEIL);
        float x = (Ui::width() - 940) / 2.0f, y = (Ui::height() - 730) / 2.0f;
        DrawRectangle(static_cast<int>(x), static_cast<int>(y), 940, 730, PANEL);
        text("SETTINGS | click a field; Ctrl+A clears; Enter applies", x + 22, y + 20, 23, ACCENT);
        const char *names[] = {"Lab/query radius",           "MinPts (also security)",
                               "Source vertex ID",           "Goal vertex ID",
                               "Selected edge weight",       "UTC window start (epoch)",
                               "UTC window end (exclusive)", "User filter (* = all)",
                               "Device filter (* = all)",    "Requests/min scale",
                               "Failed logins/min scale",    "Feature DBSCAN radius",
                               "Geographic radius (metres)"};
        for (int i = 0; i < 13; ++i) {
            int col = i >= 5 ? 1 : 0, row = i >= 5 ? i - 5 : i;
            float bx = x + 24 + col * 455, by = y + 64 + row * 60;
            text(names[i], bx, by, 15, MUTED);
            Rectangle r{bx, by + 22, 430, 36};
            DrawRectangleRec(r, field == i ? SELECTED_BG : BG);
            Ui::clip(static_cast<int>(r.x + 6), static_cast<int>(r.y), 418, 36);
            text(fields[i] + (field == i ? "_" : ""), r.x + 7, r.y + 7, 17);
            EndScissorMode();
            if (CheckCollisionPointRec(Ui::mouse(), r) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                field = i;
        }
        if (field >= 0) {
            if (IsKeyDown(KEY_LEFT_CONTROL) && pressed(KEY_A))
                fields[field].clear();
            if (pressed(KEY_BACKSPACE) && !fields[field].empty())
                fields[field].pop_back();
            int ch = GetCharPressed();
            while (ch) {
                if (ch >= 32 && ch <= 126 && fields[field].size() < 128)
                    fields[field] += static_cast<char>(ch);
                ch = GetCharPressed();
            }
        }
        const bool apply = button({x + 24, y + 570, 170, 38}, "Apply") || pressed(KEY_ENTER);
        if (button({x + 210, y + 570, 170, 38}, "Cancel"))
            close();
        wrap("Only security fields apply to security views. Radius for these views "
             "comes from the "
             "feature/geographic fields. Source/goal and edge weight apply in the "
             "laboratory. The "
             "window uses UTC seconds, not local clock time.",
             x + 24, y + 620, 890, 15, MUTED);
        return apply;
    }
};
