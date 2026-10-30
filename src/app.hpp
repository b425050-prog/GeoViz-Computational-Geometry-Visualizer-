#pragma once
#include "ui_support.hpp"
namespace {
// Coordinates the domain model, execution interface, playback and raylib presentation.
// Input handlers own edits; drawing reads the frozen Run and never advances algorithms.
class App {
    std::unique_ptr<AlgorithmRunner> engine = std::make_unique<Engine>();
    Scene scene = sampleScene(Mode::Hull, 1);
    Scene labScene = scene;
    std::vector<Point> fenceDraft;
    Mode labMode = Mode::Hull;
    View labView;
    int labSelected = -1, labEdge = -1;
    EditHistory edits;
    std::string savedLabKey, savedSecurityKey;
    std::filesystem::path sessionPath, labSessionPath, securitySessionPath;
    std::vector<std::string> recentFiles;
    bool samplePicker = false, compareVisible = false, presentation = false;
    bool noteOpen = false, sensitivityVisible = false;
    std::string sensitivityCache;
    std::string noteText, noteVerdict;
    std::unique_ptr<Run> comparison;
    int sampleVariant = 1, securitySample = 0;
    std::vector<SecuritySample> securityCatalogue;
    std::map<std::pair<int, int>, std::string> sampleExpectations;
    std::string securityDataset = "Compact reference";
    std::set<int> pressedKeys;
    Mode mode = Mode::Hull;
    std::unique_ptr<Run> run, previous;
    Playback playback;
    View view;
    SecurityConsole security;
    std::vector<Observation> observations;
    std::vector<MapLine> map;
    std::string evaluationCache;
    bool evaluationDirty = true;
    int securityView = 0; // 0 lab, 1 geographic, 2 behavioural.
    int selected = -1, selectedEdge = -1, edgeStart = -1, drag = -1, polygonDrag = -1;
    bool polygonTool = false, help = false;
    SettingsPanel settings;
    std::string status = "Click to add/select points; drag to move. Run records decisions.";
    std::filesystem::path root;
    bool smoke = false;
    bool closeRequested = false;
    int smokeFrame = 0;
    double sidebarScroll = 0;
    double helpScroll = 0;
    int windowWidth = 1440, windowHeight = 900;
    enum class RoutePick { None, Source, Destination };
    RoutePick routePick = RoutePick::None;

#include "../tests/desktop_checks.hpp"
#include "app_commands.hpp"
#include "app_input.hpp"
#include "app_inspector.hpp"
#include "app_render.hpp"
  public:
    explicit App(std::filesystem::path base, bool smokeMode, int width, int height)
        : root(std::move(base)), smoke(smokeMode), windowWidth(width), windowHeight(height) {
        std::filesystem::create_directories(root / "output");
        securityCatalogue = securitySamples((root / "data").u8string());
        security = loadSecuritySample((root / "data").u8string(), securityCatalogue.front());
        std::ifstream recent(root / "output/recent.txt");
        std::string f;
        while (std::getline(recent, f) && recentFiles.size() < 8)
            recentFiles.push_back(f);
        bool recovered = false;
        if (!smoke && std::filesystem::exists(root / "output/recovery.scene") &&
            std::filesystem::exists(root / "output/recovery.security")) {
            try {
                auto recoveredLab = SceneIO::load((root / "output/recovery.scene").u8string());
                auto recoveredSecurity =
                    SecurityConsole::load((root / "output/recovery.security").u8string());
                scene = std::move(recoveredLab);
                security = std::move(recoveredSecurity);
                recovered = true;
                status = "Recovered last workspace. Ctrl+S Save As; Ctrl+Z undo edits.";
            } catch (const std::exception &e) {
                status = std::string("Recovery unavailable: ") + e.what();
            }
        }
        labScene = scene;
        fenceDraft = security.fence();
        if (!smoke && std::filesystem::exists(root / "output/recovery.fence.scene"))
            try {
                fenceDraft =
                    SceneIO::load((root / "output/recovery.fence.scene").u8string()).polygon();
            } catch (const std::exception &) {
            }
        edits.record(workspace());
        savedLabKey = recovered ? "" : labKey();
        savedSecurityKey = recovered ? "" : securityKey();
    }
    // Keep the window and fonts alive around the loop; exceptions release graphics through RAII.
    void loop() {
        // Keep automated evidence paths inside the selected test directory.
        if (smoke)
            std::filesystem::current_path(root);
        Window window(smoke, smoke ? 960 : windowWidth, smoke ? 640 : windowHeight);
        Image icon = LoadImage((root / "assets/geoviz.png").u8string().c_str());
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
        while (!WindowShouldClose() && !closeRequested) {
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
                    auto path =
                        root / "output" / ("screen_" + std::to_string(smokeFrame / 5) + ".png");
                    std::filesystem::remove(path);
                    Ui::capture(path);
                    if (!std::filesystem::exists(path))
                        throw std::runtime_error("Smoke screenshot was not written.");
                }
                ++smokeFrame;
            } catch (const std::exception &e) {
                status = std::string("ERROR: ") + e.what();
                if (IsWindowReady()) {
                    DrawingSession drawing;
                    ClearBackground(BG);
                    wrap(status, 30, 50, static_cast<float>(Ui::width() - 60), 20, DANGER);
                    text("Correct the input or close the window. F1 for help.", 30, 150, 18);
                }
                if (smoke) {
                    throw;
                }
            }
        }
        if (!smoke)
            recover();
        if (smoke)
            smokeReport();
    }
};

} // namespace
