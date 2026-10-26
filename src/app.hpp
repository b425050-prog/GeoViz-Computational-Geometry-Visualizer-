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
    double frameMilliseconds = 0, maximumFrameMilliseconds = 0;
    std::size_t measuredFrames = 0;
    double sidebarScroll = 0;
    double helpScroll = 0;
    int windowWidth = 1440, windowHeight = 900;
    enum class RoutePick { None, Source, Destination };
    RoutePick routePick = RoutePick::None;

#include "app_commands.hpp"
#include "app_input.hpp"
#include "app_inspector.hpp"
#include "app_render.hpp"
#include "app_feature_checks.hpp"
#include "app_smoke.hpp"
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
                    if (smokeFrame % 5 == 0) {
                        int stage = smokeFrame / 5;
                        if (stage < 10) {
                            securityView = 0;
                            mode = static_cast<Mode>(stage);
                            scene = sampleScene(mode, sampleVariant);
                            view.fit(scene);
                            execute();
                            while (playback.cursor() + 1 < run->result().trace().frames().size())
                                playback.step(1, run->result().trace().frames().size());
                        } else if (stage == 10) {
                            setView(1);
                            execute();
                            while (playback.cursor() + 1 < run->result().trace().frames().size())
                                playback.step(1, run->result().trace().frames().size());
                            selected = 11;
                        } else if (stage == 11) {
                            setView(2);
                            execute();
                            selected = 0;
                        } else if (stage == 12) {
                            openSettings();
                        } else if (stage == 13) {
                            settings.close();
                            help = true;
                        } else if (stage >= 14 && stage <= 17) {
                            help = false;
                            securityView = 0;
                            mode = stage == 14   ? Mode::Hull
                                   : stage == 15 ? Mode::DBSCAN
                                                 : Mode::KDNearest;
                            scene = sampleScene(mode, sampleVariant);
                            view.fit(scene);
                            execute();
                            auto &frames = run->result().trace().frames();
                            std::size_t target = frames.size() / 2;
                            if (stage == 16)
                                target = std::min<std::size_t>(3, frames.size() - 1);
                            if (stage == 17)
                                for (std::size_t i = 0; i < frames.size(); ++i)
                                    if (!frames[i].pruned().empty()) {
                                        target = i;
                                        break;
                                    }
                            while (playback.cursor() < target)
                                playback.step(1, frames.size());
                        } else if (stage >= 18 && stage < 28) {
                            securityView = 0;
                            mode = static_cast<Mode>(stage - 18);
                            sampleVariant = stage % 2 + 2;
                            scene = sampleScene(mode, sampleVariant);
                            selected = selectedEdge = -1;
                            view.fit(scene);
                            execute();
                            while (playback.cursor() + 1 < run->result().trace().frames().size())
                                playback.step(1, run->result().trace().frames().size());
                        } else if (stage == 28 || stage == 29) {
                            setView(1);
                            nextSample();
                            execute();
                            while (playback.cursor() + 1 < run->result().trace().frames().size())
                                playback.step(1, run->result().trace().frames().size());
                            selected = static_cast<int>(scene.points().size()) - 1;
                        } else if (stage >= 30 && stage < 50) {
                            securityView = 0;
                            mode = static_cast<Mode>((stage - 30) % 10);
                            sampleVariant = 4 + (stage - 30) / 10;
                            scene = sampleScene(mode, sampleVariant);
                            selected = selectedEdge = -1;
                            view.fit(scene);
                            execute();
                            while (playback.cursor() + 1 < run->result().trace().frames().size())
                                playback.step(1, run->result().trace().frames().size());
                        } else if (stage >= 50 &&
                                   stage < 50 + static_cast<int>(securityCatalogue.size()) * 2) {
                            securitySample = (stage - 50) / 2;
                            security = loadSecuritySample((root / "data").u8string(),
                                                          securityCatalogue[securitySample]);
                            securityDataset = securityCatalogue[securitySample].name;
                            securityView = (stage - 50) % 2 + 1;
                            mode = Mode::IndexedDBSCAN;
                            refreshSecurity();
                            execute();
                            selected = static_cast<int>(scene.points().size()) - 1;
                            while (playback.cursor() + 1 < run->result().trace().frames().size())
                                playback.step(1, run->result().trace().frames().size());
                        } else if (stage < 52 + static_cast<int>(securityCatalogue.size()) * 2) {
                            security = loadSecuritySample((root / "data").u8string(),
                                                          securityCatalogue.front());
                            security.updateSettings([&](SecuritySettings &draft) {
                                draft.setDevice("no-such-device");
                            });
                            securityDataset = "Empty filtered selection";
                            securityView =
                                stage - 50 - static_cast<int>(securityCatalogue.size()) * 2 + 1;
                            mode = Mode::IndexedDBSCAN;
                            refreshSecurity();
                            if (feature() &&
                                !CheckCollisionPointRec(view.screen({0, 0}), view.bounds()))
                                throw std::runtime_error(
                                    "Empty feature view must retain visible coordinate axes.");
                            execute();
                        } else if (stage < 59 + static_cast<int>(securityCatalogue.size()) * 2) {
                            int extra = stage - 52 - static_cast<int>(securityCatalogue.size()) * 2;
                            settings.close();
                            help = false;
                            samplePicker = false;
                            noteOpen = false;
                            if (extra == 0) {
                                setView(0);
                                mode = Mode::Hull;
                                chooseSample(0);
                                samplePicker = true;
                            }
                            if (extra == 1) {
                                security = loadSecuritySample((root / "data").u8string(),
                                                              securityCatalogue.front());
                                fenceDraft = security.fence();
                                securityDataset = securityCatalogue.front().name;
                                setView(1);
                                samplePicker = true;
                            }
                            if (extra == 2) {
                                loadGrid();
                                execute();
                                compare();
                            }
                            if (extra == 3) {
                                compareVisible = false;
                                togglePresentation();
                            }
                            if (extra == 4) {
                                togglePresentation();
                                setView(1);
                                openSettings();
                            }
                            if (extra == 5) {
                                setView(2);
                                execute();
                                Ui::adjustText(6);
                            }
                            if (extra == 6) {
                                Ui::adjustText(-6);
                                selected = 0;
                                noteVerdict = "suspicious";
                                noteText = "Synthetic test note; review time is stored "
                                           "independently of truth.";
                                noteOpen = true;
                            }
                        } else
                            break;
                    }
                } else
                    input();
                auto frameStart = std::chrono::steady_clock::now();
                draw();
                double elapsed = std::chrono::duration<double, std::milli>(
                                     std::chrono::steady_clock::now() - frameStart)
                                     .count();
                frameMilliseconds += elapsed;
                maximumFrameMilliseconds = std::max(maximumFrameMilliseconds, elapsed);
                ++measuredFrames;
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
        if (smoke) {
            const int expectedStates = 59 + static_cast<int>(securityCatalogue.size()) * 2;
            if (smokeFrame < expectedStates * 5)
                throw std::runtime_error(
                    "Visual smoke run interrupted before all states completed.");
            std::ofstream(root / "output/APP_SMOKE_REPORT.txt", std::ios::app)
                << "Measured render/frame latency incl. FPS wait: avg_ms="
                << (measuredFrames ? frameMilliseconds / measuredFrames : 0)
                << " max_ms=" << maximumFrameMilliseconds << " frames=" << measuredFrames << "\n"
                << "PASS: rendered and captured " << expectedStates
                << " states: 10 algorithm modes, 2 "
                   "security views, "
                   "settings/help, intermediate hull/DBSCAN/tree-build and a pruned "
                   "region, ten varied gallery inputs, both extended security datasets, "
                   "all twenty tidal/terrace gallery inputs and both views of all "
                << securityCatalogue.size()
                << " security scenarios, plus both empty filtered views; named sample pickers, "
                   "frozen comparison, presentation layout, geographic UTC Settings, enlarged text "
                   "and review-note editor.\n"
                << "Offscreen render " << Ui::pixelWidth() << 'x' << Ui::pixelHeight()
                << "; logical " << Ui::width() << 'x' << Ui::height() << "; UI scale "
                << Ui::scale() << "; body font pixels " << Ui::fontSize(17) * Ui::scale() << "\n"
                << "PASS: source/destination pickers and logical/world/native "
                   "transforms.\n";
        }
    }
};

} // namespace
