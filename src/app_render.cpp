#include "app.hpp"

namespace gv::ui {
void App::sampleDialog() {
    if (!samplePicker)
        return;
    DrawRectangle(0, 0, Ui::width(), Ui::height(), VEIL);
    float x = (Ui::width() - 1080) / 2, y = 35;
    text("SELECT SAMPLE | number keys select; Esc closes", x, y, 23, ACCENT);
    y += 46;
    int count = securityView ? static_cast<int>(securityCatalogue.size()) : SampleVariantCount;
    for (int i = 0; i < count; ++i) {
        std::string label = std::to_string(i + 1) + ". " +
                            (securityView ? securityCatalogue[i].name : sampleName(mode, i));
        bool chosen = button({x, y, 420, 42}, label);
        if (pressedKeys.count(KEY_ONE + i))
            chosen = true;
        if (!securityView) {
            auto key = std::make_pair(static_cast<int>(mode), i);
            auto &expected = sampleExpectations[key];
            if (expected.empty())
                expected = engine->execute(sampleScene(mode, i), mode, false).summary();
            wrap(algorithmDescriptors()[static_cast<int>(mode)].purpose +
                     std::string(" | Expected: ") + expected,
                 x + 438, y, 630, 15, MUTED);
        }
        if (securityView)
            wrap(securityCatalogue[i].purpose, x + 438, y, 630, 15, MUTED);
        if (chosen) {
            chooseSample(i);
            return;
        }
        y += 52;
    }
    if (!recentFiles.empty()) {
        text("RECENT SESSIONS", x, y, 18, ACCENT);
        y += 30;
        for (std::size_t i = 0; i < std::min<std::size_t>(3, recentFiles.size()); ++i) {
            if (button({x, y, 1040, 34}, recentFiles[i])) {
                loadFile(recentFiles[i]);
                samplePicker = false;
                return;
            }
            y += 38;
        }
    }
    if (button({x, y + 8, 160, 36}, "Close") || pressedKeys.count(KEY_ESCAPE))
        samplePicker = false;
}
void App::reviewDialog() {
    if (!noteOpen)
        return;
    if (pressed(KEY_TAB))
        noteFocus =
            (noteFocus + (modifier(pressedKeys, KEY_LEFT_SHIFT, KEY_RIGHT_SHIFT) ? 2 : 1)) % 3;
    DrawRectangle(0, 0, Ui::width(), Ui::height(), VEIL);
    float x = (Ui::width() - 940) / 2, y = (Ui::height() - 420) / 2;
    text("ANALYST REVIEW: " + noteVerdict, x, y, 23, ACCENT);
    wrap("Enter a review note (optional). Editing shortcuts match Settings. Review time is "
         "recorded in "
         "UTC, separately from synthetic truth.",
         x, y + 50, 900, 17);
    Rectangle field{x, y + 132, 900, 140};
    DrawRectangleRec(field, PANEL);
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(Ui::mouse(), field))
        noteFocus = 0;
    DrawRectangleLinesEx(field, 2, noteFocus == 0 ? FOCUS : BORDER);
    drawEditor(noteText, noteCursor,
               {field.x + 8, field.y + 8, field.width - 16, field.height - 16}, 17, true,
               noteFocus == 0);
    const bool closingNote = status.rfind("Finish this review", 0) == 0;
    text(closingNote ? status
                     : std::to_string(noteText.size()) +
                           " / 1024 bytes | Ctrl+A selects all; Tab changes focus",
         x, y + 278, 14, closingNote ? DANGER : MUTED);
    if (noteFocus == 0)
        editText(noteText, noteCursor, pressedKeys, 1024);
    if (button({x, y + 300, 210, 42}, "Save review") || (pressed(KEY_ENTER) && noteFocus != 2)) {
        review(noteVerdict, noteText);
        noteOpen = false;
    }
    if (button({x + 225, y + 300, 170, 42}, "Cancel") || (pressed(KEY_ENTER) && noteFocus == 2)) {
        noteOpen = false;
        status = "Review cancelled; existing verdicts are unchanged.";
    }
    if (noteFocus == 1 || noteFocus == 2)
        DrawRectangleLinesEx(
            {noteFocus == 1 ? x : x + 225, y + 300, noteFocus == 1 ? 210.0f : 170.0f, 42}, 3,
            FOCUS);
}
void App::settingsDialog() {
    if (settings.draw(pressedKeys)) {
        try {
            applySettings();
        } catch (const std::exception &e) {
            settings.showError(e.what());
        }
    }
}
// Scrollable control reference; keyboard and pointer input are blocked behind the overlay.
void App::helpDialog() {
    if (!help)
        return;
    DrawRectangle(0, 0, Ui::width(), Ui::height(), VEIL);
    float x = (Ui::width() - 1040) / 2, top = 42, bottom = Ui::height() - 42;
    DrawRectangleRounded({x - 20, top, 1080, bottom - top}, 0.025f, 6, PANEL);
    text("GEOVIZ | CONTROLS AND WORKFLOW", x, top + 20, 27, ACCENT);
    text("Scroll for all controls. Esc / F1 closes help.", x, top + 59, 16, MUTED);
    float clipTop = top + 96, clipBottom = bottom - 66;
    Ui::clip(x, clipTop, 1040, clipBottom - clipTop);
    float y = clipTop - static_cast<float>(helpScroll);
    y = wrap("CURRENT ALGORITHM: " + modeName(mode), x, y, 1020, 20, ACCENT) + 12;
    for (const auto &instruction : AlgorithmHelp::steps(mode, securityView))
        y = wrap(instruction, x, y, 1020, 17) + 12;
    y += 12;
    const char *lines[] = {
        "1. Select Lab, Security map or Security features. Choose an "
        "algorithm in the two top rows.",
        "2. Samples (F2) selects a named input; N cycles six laboratory inputs or nine security "
        "datasets. Grid loads the "
        "obstacle-navigation graph.",
        "3. Left-click empty canvas: add point. Click a point: select. "
        "Drag: move. Delete: remove selected point.",
        "4. P toggles polygon editing. Click to append vertices, drag "
        "existing vertices. Clear polygon rebuilds it.",
        "5. Right-click sets query. F fits data. Mouse wheel zooms around "
        "pointer; middle-drag pans.",
        "6. Shift-click two vertices to add an undirected edge. Click edge "
        "to select. B toggles blocked.",
        "7. Pick source / Pick destination, then click a point; or select a "
        "point and press R / T. Settings "
        "edits edge weight and all parameters.",
        "8. Enter runs. Space plays/pauses. Left/Right steps. Home rewinds. "
        "End jumps to final result.",
        "9. Ctrl+S: named Save As; Ctrl+O: Open. S saves current; L loads default. Ctrl+Z/Y: "
        "undo/redo. "
        "Drag/drop also opens a file.",
        "10. Drop a schema-compatible CSV to import authorized local logs; "
        "drop .security to restore evidence.",
        "11. E exports evidence with snapshots. C compares algorithms; V "
        "compares previous/current runs. [ ] changes speed.",
        "12. Security observations are read-only. Filters select records. "
        "Geographic polygons remain editable.",
        "13. Select a security observation and click Review "
        "benign/suspicious. Export retains those reviews.",
        "14. Scroll the right panel for scores, source records, neighbours "
        "and evaluation. F1 closes help.",
        "15. Collision mode interprets point pairs (0,1), (2,3), ... as "
        "independent closed segments.",
        "16. A* uses Euclidean or grid Manhattan lower bounds; incompatible "
        "edited weights fall back to h=0.",
        "17. Trace cap is 4096 frames. The prefix and final frame are "
        "retained if the cap is reached.",
        "19. F3 workspace; F4 algorithm; F5 presentation; F6 edge selection; F7 resets filters; F8 "
        "sensitivity. Ctrl+plus/minus text size.",
        "20. Tab focuses buttons; Enter activates. Shift+arrows moves query; Ctrl+I inserts at "
        "query; Ctrl+arrows moves a selected Lab point.",
        "18. Noise/outside labels suggest review. They are not attack "
        "verdicts. Included data/map are synthetic."};
    for (auto s : lines)
        y = wrap(s, x, y, 1010, 17) + 11;
    helpScroll =
        std::min(helpScroll, std::max(0.0, static_cast<double>(y + helpScroll - clipBottom + 16)));
    EndScissorMode();
    if (button({x, bottom - 52, 180, 38}, "Close help"))
        help = false;
}
// Render one logical-resolution UI frame through the RAII drawing session.
void App::draw() {
    DrawingSession drawing;
    ButtonFocus::begin(pressedKeys, help || settings.isOpen() || samplePicker || noteOpen);
    ClearBackground(BG);
    float w = Ui::width(), h = Ui::height();
    if (presentation) {
        text("GEOVIZ | " + modeName(mode), 16, 12, 25, ACCENT);
        text("Presentation | F5 returns to editing", w - 440, 18, 16, MUTED);
        canvas();
        sidebar();
        float x = 16, y = h - 78;
        auto action = [&](const std::string &label, const std::function<void()> &fn,
                          bool enabled = true) {
            float width = std::max(90.0f, Ui::measure(label.c_str(), 17) + 22);
            if (button({x, y, width, 40}, label, false, enabled))
                fn();
            x += width + 8;
        };
        action("Run", [&] { execute(); });
        action(
            playback.playing() ? "Pause" : "Play",
            [&] { playback.setPlaying(!playback.playing()); }, bool(run));
        action(
            "Back", [&] { playback.step(-1, run->result().trace().frames().size()); }, bool(run));
        action("Step", [&] { playback.step(1, run->result().trace().frames().size()); }, bool(run));
        action("Compare", [&] { compare(); }, bool(run));
        action("Exit F5", [&] { togglePresentation(); });
        wrap(status, 16, h - 28, w - 32, 14, MUTED);
        ButtonFocus::overlay(pressedKeys, help || samplePicker);
        if (help)
            helpDialog();
        else if (settings.isOpen())
            settingsDialog();
        else if (samplePicker)
            sampleDialog();
        else if (noteOpen)
            reviewDialog();
        ButtonFocus::finish();
        return;
    }
    text("GEOVIZ", 16, 13, 25, ACCENT);
    text("Geometry + security analysis", 145, 18, 16, MUTED);
    if (button({static_cast<float>(w - 515), 8, 85, 30}, "Lab", !securityView) && !help &&
        !settings.isOpen())
        setView(0);
    if (button({static_cast<float>(w - 424), 8, 147, 30}, "Security map", securityView == 1) &&
        !help && !settings.isOpen())
        setView(1);
    if (button({static_cast<float>(w - 271), 8, 175, 30}, "Security features", securityView == 2) &&
        !help && !settings.isOpen())
        setView(2);
    if (button({static_cast<float>(w - 90), 8, 76, 30}, "Help F1"))
        help = !help;
    float bw = (w - 32) / 5.0f;
    for (int i = 0; i < static_cast<int>(algorithmDescriptors().size()); ++i) {
        if (button({16 + (i % 5) * bw, 48.0f + (i / 5) * 38, bw - 7, 32},
                   std::to_string(i + 1) + " " + modeName(static_cast<Mode>(i)),
                   mode == static_cast<Mode>(i), modeAvailable(static_cast<Mode>(i))) &&
            !help && !settings.isOpen()) {
            setMode(static_cast<Mode>(i));
        }
    }
    canvas();
    sidebar();
    float y = h - 160;
    float bx = 16;
    auto action = [&](const std::string &label, float width, const std::function<void()> &fn) {
        width = std::max(width, Ui::measure(label.c_str(), 17) + 20);
        bool needsRun = label == "Play" || label == "Pause" || label == "< Step" ||
                        label == "Step >" || label == "Rewind" || label == "Final" ||
                        label == "Compare" || label == "Prior" || label == "Export";
        bool enabled = (!needsRun || run || (label == "Export" && securityView)) &&
                       (label != "Prior" || previous) &&
                       ((label != "Polygon P" && label != "Clear polygon") || !feature()) &&
                       ((label != "Benign" && label != "Suspect") || selected >= 0);
        bool clicked = button({bx, y, width, 40}, label, false, enabled);
        if (clicked && !help && !settings.isOpen())
            fn();
        bx += width + 6;
    };
    action("Run", 66, [&] { execute(); });
    action(playback.playing() ? "Pause" : "Play", 70, [&] {
        if (run)
            playback.setPlaying(!playback.playing());
    });
    action("< Step", 82, [&] {
        if (run)
            playback.step(-1, run->result().trace().frames().size());
    });
    action("Step >", 82, [&] {
        if (run)
            playback.step(1, run->result().trace().frames().size());
    });
    action("Rewind", 86, [&] { playback.reset(); });
    action("Final", 65, [&] {
        if (run)
            finalFrame();
    });
    action("Settings", 98, [&] { openSettings(); });
    action("Compare", 98, [&] { compare(); });
    action("Prior", 62, [&] { comparePrevious(); });
    action("Fit", 54, [&] { view.fit(scene, feature()); });
    y += 48;
    bx = 16;
    action("Samples", 86, [&] { openSamples(); });
    action("Grid", 60, [&] { loadGrid(); });
    action("Clear", 66, [&] { clearScene(); });
    action("Polygon P", 106, [&] {
        routePick = RoutePick::None;
        if (!feature())
            polygonTool = !polygonTool;
    });
    action("Clear polygon", 140, [&] { clearFence(); });
    action("Save As", 64, [&] { saveNamed(); });
    action("Open", 64, [&] { openNamed(); });
    action("Export", 76, [&] { exportEvidence(); });
    if (securityView) {
        action("Benign", 75, [&] { openReview("benign"); });
        action("Suspect", 85, [&] { openReview("suspicious"); });
    }
    DrawRectangle(0, h - 64, w, 64, SOFT_BG);
    Ui::clip(0, h - 64, w, 64);
    wrap((!dirty() ? "" : "UNSAVED | ") + status, 16, h - 57, w - 30, 15,
         status.rfind("Action failed:", 0) == 0 ? DANGER : INK);
    EndScissorMode();
    ButtonFocus::overlay(pressedKeys, help || samplePicker);
    if (help)
        helpDialog();
    else if (settings.isOpen())
        settingsDialog();
    else if (samplePicker)
        sampleDialog();
    else if (noteOpen)
        reviewDialog();
    ButtonFocus::finish();
}

} // namespace gv::ui
