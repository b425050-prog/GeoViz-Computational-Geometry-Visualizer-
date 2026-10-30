#include "app.hpp"

namespace gv::ui {
App::App(std::filesystem::path base, bool smokeMode, int width, int height)
    : root(std::move(base)), files(root), smoke(smokeMode), windowWidth(width),
      windowHeight(height) {
    SessionFiles::verifyAssets(root);
    try {
        files.ensureOutput();
    } catch (const std::exception &error) {
        if (smoke)
            throw;
        status = error.what();
    }
    securityCatalogue = securitySamples((root / "data").u8string());
    security = loadSecuritySample((root / "data").u8string(), securityCatalogue.front());
    recentFiles = files.readRecent();
    bool recovered = false;
    if (!smoke) {
        try {
            if (auto snapshot = files.readRecovery()) {
                scene = std::move(snapshot->lab);
                security = std::move(snapshot->security);
                fenceDraft = std::move(snapshot->fenceDraft);
                recovered = true;
                status = "Recovered last workspace. Ctrl+S Save As; Ctrl+Z undo edits.";
            }
        } catch (const std::exception &error) {
            status = "Recovery not loaded: " + std::string(error.what()) +
                     " Open a backup from output/recovery-history if needed.";
        }
    }
    labScene = scene;
    if (!recovered)
        fenceDraft = security.fence();
    edits.record(workspace());
    savedLabKey = recovered ? "" : labKey();
    savedSecurityKey = recovered ? "" : securityKey();
    refreshDirty();
}

void App::loop() {
    // Keep automated evidence paths inside the selected test directory.
    if (smoke)
        std::filesystem::current_path(root);
    Window window(smoke, smoke ? 960 : windowWidth, smoke ? 640 : windowHeight);
    const auto iconData = readAsset(root / "assets/geoviz.png");
    Image icon = LoadImageFromMemory(".png", iconData.data(), static_cast<int>(iconData.size()));
    if (icon.data) {
        SetWindowIcon(icon);
        UnloadImage(icon);
    }
    Ui ui(root, smoke ? windowWidth : 0, smoke ? windowHeight : 0);
    updateLayout();
    view.fit(scene);
    if (smoke) {
        smokeInteractions();
    }
    while (true) {
        if (WindowShouldClose() || closeRequested) {
            closeRequested = false;
            if (smoke || requestClose())
                break;
        }
        try {
            ui.refresh();
            updateLayout();
            if (smoke) {
                if (smokeFrame % 5 == 0 && !smokeStage(smokeFrame / 5))
                    break;
            } else
                input();
            draw();
            if (!smoke)
                recordEdit();
            if (smoke && smokeFrame % 5 == 3) {
                auto path = root / "output" / ("screen_" + std::to_string(smokeFrame / 5) + ".png");
                std::filesystem::remove(path);
                Ui::capture(path);
                if (!std::filesystem::exists(path))
                    throw std::runtime_error("Smoke screenshot was not written.");
            }
            ++smokeFrame;
        } catch (const std::exception &e) {
            status = std::string("Action failed: ") + e.what();
            if (smoke) {
                throw;
            }
        }
    }
    if (smoke)
        smokeReport();
}
bool App::requestClose() {
    help = false;
    if (settings.isOpen()) {
        settings.showError("Apply or Cancel Settings before closing; your edits are still here.");
        return false;
    }
    if (noteOpen) {
        noteFocus = 1;
        status = "Finish this review with Save review or Cancel before closing.";
        return false;
    }
    try {
        recover();
        return true;
    } catch (const std::exception &error) {
        if (!dirty())
            return true;
        status = std::string("Action failed: ") + error.what() +
                 " Save each unsaved workspace with Ctrl+S, then close again.";
        return false;
    }
}
} // namespace gv::ui
