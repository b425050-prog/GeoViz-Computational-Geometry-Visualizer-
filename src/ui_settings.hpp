#pragma once

// A modal edit buffer owns field input, presentation and candidate validation.
// It never commits changes to the live scene; App applies a complete candidate.
class SettingsPanel final {
    bool open_ = false;
    std::vector<std::string> fields;
    int field = -1;
    int context = 0;
    bool edgeAvailable = false;
    Mode activeMode = Mode::Hull;
    TextCursor cursor;
    std::string error;
    inline static const std::vector<std::string> names = {"Radius",
                                                          "MinPts",
                                                          "Source vertex ID",
                                                          "Goal vertex ID",
                                                          "Selected edge weight",
                                                          "UTC window start",
                                                          "UTC window end (exclusive)",
                                                          "User filter (* = all)",
                                                          "Device filter (* = all)",
                                                          "Requests/min scale",
                                                          "Failures/min scale",
                                                          "Feature radius",
                                                          "Geographic radius (metres)"};
    std::vector<int> visible() const {
        if (context == 1)
            return {1, 5, 6, 7, 8, 12};
        if (context == 2)
            return {1, 5, 6, 7, 8, 9, 10, 11};
        if (activeMode == Mode::Dijkstra || activeMode == Mode::AStar)
            return edgeAvailable ? std::vector<int>{2, 3, 4} : std::vector<int>{2, 3};
        if (activeMode == Mode::DBSCAN || activeMode == Mode::IndexedDBSCAN)
            return {0, 1};
        if (activeMode == Mode::KDRadius)
            return {0};
        return {};
    }

  public:
    struct Changes {
        Scene scene;
        SecurityConsole security;
    };
    bool isOpen() const noexcept { return open_; }
    void close() noexcept { open_ = false; }
    void setField(std::size_t index, std::string value) {
        fields.at(index) = std::move(value);
        if (field == static_cast<int>(index))
            cursor.reset(fields[index].size());
    }
    // Format a detached edit buffer; the model changes only when Apply succeeds.
    void open(const Scene &scene, const SecurityConsole &security, int selectedEdge,
              int securityView = 0, Mode mode = Mode::Hull) {
        fields = {exactNumber(scene.radius()),
                  std::to_string(scene.minPts()),
                  std::to_string(scene.source()),
                  std::to_string(scene.goal()),
                  selectedEdge >= 0 ? exactNumber(scene.edges()[selectedEdge].weight()) : "1",
                  utcText(security.settings().start()),
                  utcText(security.settings().end()),
                  security.settings().user(),
                  security.settings().device(),
                  exactNumber(security.settings().requestScale()),
                  exactNumber(security.settings().failureScale()),
                  exactNumber(security.settings().featureRadius()),
                  exactNumber(security.settings().geoRadius())};
        edgeAvailable = selectedEdge >= 0;
        context = securityView;
        activeMode = mode;
        error.clear();
        auto shown = visible();
        field = shown.empty() ? -1 : shown.front();
        cursor.reset(field < 0 ? 0 : fields[field].size());
        open_ = true;
    }
    Changes candidate(const Scene &scene, const SecurityConsole &security, int selectedEdge,
                      int securityView) const {
        auto d = [&](int i) {
            try {
                std::size_t pos = 0;
                double x = std::stod(fields[i], &pos);
                if (pos != fields[i].size() || !std::isfinite(x))
                    throw std::runtime_error("nonfinite or trailing data");
                return x;
            } catch (const std::exception &) {
                throw std::runtime_error(names[i] + ": expected a finite number.");
            }
        };
        auto integer = [&](int i) {
            std::size_t pos = 0;
            if (i == 5 || i == 6)
                return utcEpoch(fields[i]);
            auto x = static_cast<std::int64_t>(std::stoll(fields[i], &pos));
            if (pos != fields[i].size())
                throw std::runtime_error(names[i] + ": expected an integer.");
            return x;
        };
        auto bounded = [&](int i, int lo, int hi) {
            auto x = integer(i);
            if (x < lo || x > hi)
                throw std::runtime_error(names[i] + ": allowed range " + std::to_string(lo) + ".." +
                                         std::to_string(hi));
            return static_cast<int>(x);
        };
        Scene next = scene;
        if (!securityView)
            next.setRadius(d(0));
        next.setMinPts(bounded(1, 1, 256));
        next.setSource(bounded(2, 0, std::max(0, static_cast<int>(scene.points().size()) - 1)));
        next.setGoal(bounded(3, 0, std::max(0, static_cast<int>(scene.points().size()) - 1)));
        if (selectedEdge >= 0)
            next.setEdgeWeight(selectedEdge, d(4));
        Engine::validate(next);
        auto sec = security;
        if (securityView) {
            auto draft = sec.settings();
            draft.setStart(integer(5));
            draft.setEnd(integer(6));
            draft.setUser(fields[7]);
            draft.setDevice(fields[8]);
            draft.setRequestScale(d(9));
            draft.setFailureScale(d(10));
            draft.setFeatureRadius(d(11));
            draft.setGeoRadius(d(12));
            draft.setMinPts(next.minPts());
            sec.setSettings(std::move(draft));
            sec.validate();
            sec.observations(false);
            sec.observations(true);
        }
        return {std::move(next), std::move(sec)};
    }
    // Modal focus and caret editing keep shortcuts from changing the canvas.
    bool draw(const std::set<int> &pressedKeys) {
        if (!open_)
            return false;
        auto pressed = [&](int key) { return pressedKeys.count(key) != 0; };
        const auto shown = visible();
        DrawRectangle(0, 0, Ui::width(), Ui::height(), VEIL);
        float x = (Ui::width() - 940) / 2, y = (Ui::height() - 690) / 2;
        DrawRectangle(static_cast<int>(x), static_cast<int>(y), 940, 690, PANEL);
        text("SETTINGS | Tab fields; Enter applies; Esc cancels", x + 24, y + 20, 22, ACCENT);
        if (shown.empty())
            wrap("This algorithm has no editable numeric parameters. Use the canvas and sample "
                 "picker.",
                 x + 24, y + 90, 880);
        if (pressed(KEY_TAB) && !shown.empty()) {
            auto i = std::find(shown.begin(), shown.end(), field);
            int index = i == shown.end() ? 0 : static_cast<int>(i - shown.begin());
            index = (index + (modifier(pressedKeys, KEY_LEFT_SHIFT, KEY_RIGHT_SHIFT)
                                  ? static_cast<int>(shown.size()) - 1
                                  : 1)) %
                    static_cast<int>(shown.size());
            field = shown[index];
            cursor.reset(fields[field].size());
        }
        for (std::size_t j = 0; j < shown.size(); ++j) {
            int i = shown[j];
            float bx = x + 24 + (j / 4) * 455, by = y + 72 + (j % 4) * 94;
            text(names[i], bx, by, 16, MUTED);
            Rectangle box{bx, by + 26, 430, 42};
            DrawRectangleRec(box, field == i ? SELECTED_BG : BG);
            DrawRectangleLinesEx(box, field == i ? 2.0f : 1.0f, field == i ? FOCUS : BORDER);
            if (field == i)
                drawEditor(fields[i], cursor, {box.x + 7, box.y + 8, 416, 30});
            else
                text(Ui::elide(fields[i], 416, 17), box.x + 7, box.y + 8, 17);
            if (CheckCollisionPointRec(Ui::mouse(), box) &&
                IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                field = i;
                cursor.reset(fields[i].size());
            }
            if (i == 5 || i == 6) {
                try {
                    text("Epoch: " + std::to_string(utcEpoch(fields[i])), bx, by + 72, 14, MUTED);
                } catch (const std::exception &) {
                }
            }
        }
        if (field >= 0) {
            const auto before = fields[field];
            editText(fields[field], cursor, pressedKeys, 128);
            if (fields[field] != before)
                error.clear();
        }
        bool apply = button({x + 24, y + 482, 170, 42}, "Apply") || pressed(KEY_ENTER);
        if (button({x + 210, y + 482, 170, 42}, "Cancel")) {
            close();
            return false;
        }
        wrap(error.empty() ? "Only parameters used by this workspace and algorithm are shown. UTC "
                             "accepts calendar text or integer epoch seconds. Ctrl+A selects all; "
                             "Ctrl+V pastes; "
                             "arrows/Home/End move the caret."
                           : error,
             x + 24, y + 550, 880, 16, error.empty() ? MUTED : DANGER);
        return apply;
    }
    void showError(const std::string &value) { error = value; }
};
