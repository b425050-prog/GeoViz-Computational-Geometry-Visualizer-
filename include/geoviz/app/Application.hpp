#pragma once

/**
 * @file Application.hpp
 * @brief Main desktop application coordinator for GeoViz.
 */

#include "geoviz/app/AlgorithmDemo.hpp"
#include "geoviz/app/Commands.hpp"
#include "geoviz/app/Renderer.hpp"

#include <raylib.h>

#include <cstddef>
#include <optional>
#include <random>
#include <string>

namespace geoviz::app {

/**
 * @brief Composes model, algorithms, command history, renderer, and UI state.
 *
 * Application owns its collaborators through values or unique ownership.  This
 * RAII design guarantees deterministic cleanup even if an algorithm throws.
 */
class Application final {
public:
    Application();
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&) = delete;
    Application& operator=(Application&&) = delete;

    [[nodiscard]] int run();

private:
    void update();
    void draw();
    void updateLayout() noexcept;
    void handleKeyboard();
    void handleCanvasInput();
    void handleDroppedFiles();
    void rebuildTrace();
    void selectDemo(std::size_t index);

    void drawHeader();
    void drawSidebar();
    void drawCanvasToolbar();
    void drawInspector();
    void drawFooter();
    void drawHelpOverlay();
    void drawToast();

    [[nodiscard]] SceneSnapshot smartPreset();
    [[nodiscard]] SceneSnapshot randomPreset(std::size_t count = 20U);
    [[nodiscard]] SceneSnapshot gridPreset() const;
    [[nodiscard]] SceneSnapshot ringPreset() const;
    void applyPreset(SceneSnapshot preset, std::string message);
    void saveScene();
    void loadScene();
    void showToast(std::string message, bool error = false);

    Theme theme_;
    CanvasTransform canvasTransform_;
    GeometryRenderer renderer_;
    SceneModel scene_;
    CommandManager commands_;
    DemoRegistry demos_;
    AlgorithmTrace trace_;
    TracePlayer player_;

    SceneModel::ObserverToken observerToken_{0U};
    std::size_t activeDemoIndex_{1U};
    bool traceDirty_{true};
    bool showHelp_{false};
    float sidebarScroll_{0.0F};

    std::optional<std::size_t> draggedPoint_;
    Point dragStart_;

    Rectangle headerRect_{};
    Rectangle sidebarRect_{};
    Rectangle inspectorRect_{};
    Rectangle footerRect_{};
    Rectangle toolbarRect_{};

    std::mt19937 randomEngine_{0xB425050U};
    std::string toastMessage_;
    float toastTimeRemaining_{0.0F};
    bool toastIsError_{false};
};

}  // namespace geoviz::app

