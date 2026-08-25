/**
 * @file Application.cpp
 * @brief Immediate-mode desktop UI, input routing, presets, and app lifecycle.
 */

#include "geoviz/app/Application.hpp"

#include "geoviz/algorithms/Predicates.hpp"
#include "geoviz/io/SceneSerializer.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <exception>
#include <filesystem>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace geoviz::app {
namespace {

inline constexpr float kHeaderHeight = 68.0F;
inline constexpr float kFooterHeight = 30.0F;
inline constexpr float kSidebarWidth = 252.0F;
inline constexpr float kInspectorWidth = 322.0F;
inline constexpr std::string_view kSceneFilename = "geoviz_scene.geoviz";

void drawText(std::string_view text, float x, float y, float size, Color color,
              float spacing = 0.45F) {
    const std::string owned(text);
    DrawTextEx(GetFontDefault(), owned.c_str(), {x, y}, size, spacing, color);
}

[[nodiscard]] float textWidth(std::string_view text, float size, float spacing = 0.45F) {
    const std::string owned(text);
    return MeasureTextEx(GetFontDefault(), owned.c_str(), size, spacing).x;
}

void drawPanel(Rectangle rectangle, const Theme& theme, Color fill, float roundness = 0.08F) {
    DrawRectangleRounded(rectangle, roundness, 10, fill);
    DrawRectangleRoundedLinesEx(rectangle, roundness, 10, 1.0F, Fade(theme.border, 0.72F));
}

[[nodiscard]] bool uiButton(Rectangle rectangle, std::string_view label, const Theme& theme,
                            Color accent, bool active = false, bool enabled = true) {
    const bool hovered = enabled && CheckCollisionPointRec(GetMousePosition(), rectangle);
    Color fill = active ? Fade(accent, 0.19F) : Fade(theme.backgroundRaised, 0.88F);
    if (hovered) fill = active ? Fade(accent, 0.28F) : theme.panelHover;
    DrawRectangleRounded(rectangle, 0.18F, 8, fill);
    DrawRectangleRoundedLinesEx(rectangle, 0.18F, 8, active ? 1.5F : 1.0F,
                                active ? Fade(accent, 0.8F) : Fade(theme.border, 0.7F));
    const float fontSize = rectangle.height >= 38.0F ? 14.0F : 12.0F;
    const float width = textWidth(label, fontSize);
    drawText(label, rectangle.x + (rectangle.width - width) * 0.5F,
             rectangle.y + (rectangle.height - fontSize) * 0.5F - 1.0F,
             fontSize, enabled ? (active ? accent : theme.text) : Fade(theme.textMuted, 0.45F));
    return hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

float drawWrappedText(std::string_view text, Rectangle bounds, float fontSize, Color color,
                      float lineMultiplier = 1.35F) {
    std::istringstream input{std::string(text)};
    std::string word;
    std::string line;
    float y = bounds.y;
    const float lineHeight = fontSize * lineMultiplier;
    while (input >> word) {
        const std::string candidate = line.empty() ? word : line + " " + word;
        if (!line.empty() && textWidth(candidate, fontSize) > bounds.width) {
            drawText(line, bounds.x, y, fontSize, color);
            y += lineHeight;
            line = word;
            if (y > bounds.y + bounds.height) break;
        } else {
            line = candidate;
        }
    }
    if (!line.empty() && y <= bounds.y + bounds.height) {
        drawText(line, bounds.x, y, fontSize, color);
        y += lineHeight;
    }
    return y;
}

void drawChip(Rectangle rectangle, std::string_view label, Color color, const Theme& theme) {
    DrawRectangleRounded(rectangle, 0.45F, 10, Fade(color, 0.12F));
    DrawRectangleRoundedLinesEx(rectangle, 0.45F, 10, 1.0F, Fade(color, 0.4F));
    const float size = 10.0F;
    drawText(label, rectangle.x + (rectangle.width - textWidth(label, size)) * 0.5F,
             rectangle.y + 5.0F, size, color);
    static_cast<void>(theme);
}

[[nodiscard]] std::string integerText(std::size_t value) {
    return std::to_string(value);
}

[[nodiscard]] std::string decimalText(double value, int precision = 1) {
    std::ostringstream output;
    output.setf(std::ios::fixed);
    output.precision(precision);
    output << value;
    return output.str();
}

}  // namespace

Application::Application()
    : renderer_(canvasTransform_, theme_), commands_(120U) {
    activeDemoIndex_ = demos_.indexOf(DemoId::HullMonotonic);
    observerToken_ = scene_.subscribe([this] { traceDirty_ = true; });
    scene_.replace(randomPreset(18U));
    commands_.clear();
}

Application::~Application() {
    scene_.unsubscribe(observerToken_);
}

int Application::run() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(1440, 900, "GeoViz | Computational Geometry Visualizer");
    SetWindowMinSize(1080, 720);
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);

    // Generate a crisp project icon entirely in C++; no runtime asset path is
    // required when the executable is moved or packaged.
    Image icon = GenImageColor(64, 64, theme_.backgroundRaised);
    ImageDrawCircle(&icon, 16, 47, 5, theme_.coral);
    ImageDrawCircle(&icon, 32, 13, 5, theme_.amber);
    ImageDrawCircle(&icon, 49, 44, 5, theme_.cyan);
    ImageDrawLine(&icon, 16, 47, 32, 13, theme_.violet);
    ImageDrawLine(&icon, 32, 13, 49, 44, theme_.cyan);
    ImageDrawLine(&icon, 49, 44, 16, 47, theme_.cyan);
    SetWindowIcon(icon);
    UnloadImage(icon);

    while (!WindowShouldClose()) {
        update();
        draw();
    }
    CloseWindow();
    return 0;
}

void Application::updateLayout() noexcept {
    const float width = static_cast<float>(GetScreenWidth());
    const float height = static_cast<float>(GetScreenHeight());
    headerRect_ = {0.0F, 0.0F, width, kHeaderHeight};
    footerRect_ = {0.0F, height - kFooterHeight, width, kFooterHeight};
    sidebarRect_ = {12.0F, kHeaderHeight + 12.0F, kSidebarWidth - 18.0F,
                    height - kHeaderHeight - kFooterHeight - 24.0F};
    inspectorRect_ = {width - kInspectorWidth - 12.0F, kHeaderHeight + 12.0F,
                      kInspectorWidth, height - kHeaderHeight - kFooterHeight - 24.0F};
    const Rectangle canvasRect{
        kSidebarWidth + 8.0F, kHeaderHeight + 12.0F,
        width - kSidebarWidth - kInspectorWidth - 28.0F,
        height - kHeaderHeight - kFooterHeight - 24.0F
    };
    canvasTransform_.setViewport(canvasRect);
    toolbarRect_ = {canvasRect.x + 14.0F, canvasRect.y + 14.0F,
                    std::min(548.0F, canvasRect.width - 28.0F), 42.0F};
}

void Application::update() {
    updateLayout();
    handleDroppedFiles();
    handleKeyboard();

    const Vector2 mouse = GetMousePosition();
    if (CheckCollisionPointRec(mouse, sidebarRect_)) {
        sidebarScroll_ = std::clamp(sidebarScroll_ - GetMouseWheelMove() * 48.0F, 0.0F, 290.0F);
    }
    handleCanvasInput();
    if (traceDirty_) rebuildTrace();
    player_.update(GetFrameTime(), trace_.size());
    toastTimeRemaining_ = std::max(0.0F, toastTimeRemaining_ - GetFrameTime());
}

void Application::handleKeyboard() {
    const bool control = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
    if (control && IsKeyPressed(KEY_Z)) {
        if (commands_.undo(scene_)) showToast("Undo applied");
    }
    if (control && IsKeyPressed(KEY_Y)) {
        if (commands_.redo(scene_)) showToast("Redo applied");
    }
    if (control && IsKeyPressed(KEY_S)) saveScene();
    if (control && IsKeyPressed(KEY_O)) loadScene();
    if (IsKeyPressed(KEY_SPACE)) player_.toggle();
    if (IsKeyPressed(KEY_LEFT)) {
        player_.setPlaying(false);
        player_.previous();
    }
    if (IsKeyPressed(KEY_RIGHT)) {
        player_.setPlaying(false);
        player_.next(trace_.size());
    }
    if (IsKeyPressed(KEY_R) && !control) applyPreset(smartPreset(), "Smart preset loaded");
    if (IsKeyPressed(KEY_HOME)) {
        canvasTransform_.resetView();
        traceDirty_ = true;
        showToast("View reset");
    }
    if (IsKeyPressed(KEY_DELETE)) {
        applyPreset({}, "Canvas cleared");
    }
    if (IsKeyPressed(KEY_H) || IsKeyPressed(KEY_F1)) showHelp_ = !showHelp_;
    if (IsKeyPressed(KEY_ESCAPE) && showHelp_) showHelp_ = false;
}

void Application::handleCanvasInput() {
    if (showHelp_) return;
    const Vector2 mouse = GetMousePosition();
    const Rectangle canvas = canvasTransform_.viewport();
    const bool overCanvas = CheckCollisionPointRec(mouse, canvas);
    const bool overToolbar = CheckCollisionPointRec(mouse, toolbarRect_);

    if (overCanvas && !overToolbar) {
        const float wheel = GetMouseWheelMove();
        if (std::abs(wheel) > 0.01F) {
            canvasTransform_.zoomAt(mouse, std::pow(1.12F, wheel));
            traceDirty_ = true;
        }
        if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
            canvasTransform_.panBy(GetMouseDelta());
            traceDirty_ = true;
        }
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            const Point world = canvasTransform_.screenToWorld(mouse);
            draggedPoint_ = scene_.nearestPoint(world, 13.0 / canvasTransform_.zoom());
            if (draggedPoint_.has_value()) {
                dragStart_ = scene_.points()[*draggedPoint_];
            } else {
                commands_.execute(std::make_unique<AddPointCommand>(world), scene_);
            }
        }
        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            const Point world = canvasTransform_.screenToWorld(mouse);
            if (const auto nearest = scene_.nearestPoint(world, 16.0 / canvasTransform_.zoom());
                nearest.has_value()) {
                commands_.execute(std::make_unique<RemovePointCommand>(*nearest), scene_);
            }
        }
    }

    if (draggedPoint_.has_value() && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        scene_.setPoint(*draggedPoint_, canvasTransform_.screenToWorld(mouse));
    }
    if (draggedPoint_.has_value() && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        const std::size_t index = *draggedPoint_;
        const Point destination = scene_.points()[index];
        if (!algorithms::samePoint(dragStart_, destination)) {
            scene_.setPoint(index, dragStart_);
            commands_.execute(
                std::make_unique<MovePointCommand>(index, dragStart_, destination), scene_);
        }
        draggedPoint_.reset();
    }
}

void Application::handleDroppedFiles() {
    if (!IsFileDropped()) return;
    FilePathList files = LoadDroppedFiles();
    if (files.count > 0U) {
        try {
            SceneSnapshot loaded = io::SceneSerializer::load(files.paths[0]);
            commands_.execute(std::make_unique<ReplaceSceneCommand>(std::move(loaded)), scene_);
            showToast("Dropped scene loaded");
        } catch (const std::exception& error) {
            showToast(error.what(), true);
        }
    }
    UnloadDroppedFiles(files);
}

void Application::rebuildTrace() {
    try {
        trace_ = demos_.at(activeDemoIndex_).buildTrace(scene_, canvasTransform_.visibleBounds());
    } catch (const std::exception& error) {
        const DemoMetadata& metadata = demos_.at(activeDemoIndex_).metadata();
        trace_ = AlgorithmTrace{metadata.title, metadata.complexity};
        VisualFrame frame;
        frame.headline = "Input needs attention";
        frame.explanation = error.what();
        frame.sourcePoints = scene_.points();
        trace_.addFrame(std::move(frame));
    }
    player_.reset(trace_.size());
    traceDirty_ = false;
}

void Application::selectDemo(std::size_t index) {
    if (index == activeDemoIndex_) return;
    activeDemoIndex_ = index;
    traceDirty_ = true;
    rebuildTrace();
}

void Application::draw() {
    BeginDrawing();
    ClearBackground(theme_.background);
    renderer_.drawGrid();
    if (!trace_.empty()) {
        const std::size_t index = std::min(player_.frameIndex(), trace_.size() - 1U);
        renderer_.drawFrame(trace_.frames()[index],
                            theme_.accent(demos_.at(activeDemoIndex_).metadata().accentIndex));
    }
    DrawRectangleRoundedLinesEx(canvasTransform_.viewport(), 0.018F, 8, 1.0F,
                                Fade(theme_.border, 0.85F));

    drawCanvasToolbar();
    drawHeader();
    drawSidebar();
    drawInspector();
    drawFooter();
    drawToast();
    if (showHelp_) drawHelpOverlay();
    EndDrawing();
}

void Application::drawHeader() {
    DrawRectangleGradientH(0, 0, static_cast<int>(headerRect_.width),
                           static_cast<int>(headerRect_.height),
                           theme_.backgroundRaised, Color{15, 27, 47, 255});
    DrawLine(0, static_cast<int>(headerRect_.height - 1.0F),
             static_cast<int>(headerRect_.width), static_cast<int>(headerRect_.height - 1.0F),
             Fade(theme_.border, 0.7F));

    const Vector2 logoCentre{34.0F, 34.0F};
    DrawCircleV(logoCentre, 20.0F, Fade(theme_.cyan, 0.13F));
    DrawCircleLines(static_cast<int>(logoCentre.x), static_cast<int>(logoCentre.y), 18.0F, theme_.cyan);
    DrawLineEx({22.0F, 42.0F}, {45.0F, 23.0F}, 2.0F, theme_.violet);
    DrawCircleV({22.0F, 42.0F}, 3.8F, theme_.coral);
    DrawCircleV({45.0F, 23.0F}, 3.8F, theme_.cyan);
    DrawCircleV({38.0F, 38.0F}, 3.3F, theme_.amber);

    drawText("GEOVIZ", 66.0F, 15.0F, 23.0F, theme_.text, 1.4F);
    drawText("COMPUTATIONAL GEOMETRY VISUALIZER", 67.0F, 42.0F, 10.0F,
             theme_.textMuted, 1.2F);

    const float authorWidth = 318.0F;
    const Rectangle author{headerRect_.width - authorWidth - 18.0F, 10.0F, authorWidth, 48.0F};
    DrawRectangleRounded(author, 0.18F, 8, Fade(theme_.panel, 0.75F));
    DrawRectangleRoundedLinesEx(author, 0.18F, 8, 1.0F, Fade(theme_.border, 0.6F));
    DrawCircleV({author.x + 25.0F, author.y + 24.0F}, 13.0F, Fade(theme_.violet, 0.24F));
    drawText("SD", author.x + 17.0F, author.y + 18.0F, 11.0F, theme_.violet);
    drawText("SATYAM DHAL  |  B425050", author.x + 49.0F, author.y + 9.0F, 13.0F, theme_.text);
    drawText("IIIT BHUBANESWAR  |  CSE-B  |  SEM 3", author.x + 49.0F,
             author.y + 29.0F, 9.5F, theme_.textMuted);
}

void Application::drawSidebar() {
    drawPanel(sidebarRect_, theme_, theme_.panel);
    drawText("ALGORITHM ATLAS", sidebarRect_.x + 14.0F, sidebarRect_.y + 14.0F,
             13.0F, theme_.text, 0.9F);
    drawText(integerText(demos_.size()) + " LABS", sidebarRect_.x + sidebarRect_.width - 58.0F,
             sidebarRect_.y + 16.0F, 9.0F, theme_.cyan);
    DrawLine(static_cast<int>(sidebarRect_.x + 12.0F), static_cast<int>(sidebarRect_.y + 43.0F),
             static_cast<int>(sidebarRect_.x + sidebarRect_.width - 12.0F),
             static_cast<int>(sidebarRect_.y + 43.0F), Fade(theme_.border, 0.6F));

    const Rectangle listBounds{sidebarRect_.x + 5.0F, sidebarRect_.y + 49.0F,
                               sidebarRect_.width - 10.0F, sidebarRect_.height - 58.0F};
    BeginScissorMode(static_cast<int>(listBounds.x), static_cast<int>(listBounds.y),
                     static_cast<int>(listBounds.width), static_cast<int>(listBounds.height));
    float y = listBounds.y + 2.0F - sidebarScroll_;
    std::string previousCategory;
    for (std::size_t index = 0U; index < demos_.size(); ++index) {
        const DemoMetadata& metadata = demos_.at(index).metadata();
        if (metadata.category != previousCategory) {
            y += 8.0F;
            drawText(metadata.category, listBounds.x + 10.0F, y, 8.5F,
                     Fade(theme_.textMuted, 0.82F), 0.9F);
            y += 19.0F;
            previousCategory = metadata.category;
        }
        const Rectangle item{listBounds.x + 5.0F, y, listBounds.width - 10.0F, 36.0F};
        const bool visible = item.y + item.height >= listBounds.y &&
                             item.y <= listBounds.y + listBounds.height;
        if (uiButton(item, metadata.sidebarLabel, theme_, theme_.accent(metadata.accentIndex),
                     index == activeDemoIndex_, visible)) {
            selectDemo(index);
        }
        y += 42.0F;
    }
    EndScissorMode();

    const float thumbHeight = std::max(34.0F, listBounds.height * 0.55F);
    const float scrollRatio = sidebarScroll_ / 290.0F;
    DrawRectangleRounded({sidebarRect_.x + sidebarRect_.width - 5.0F,
                          listBounds.y + scrollRatio * (listBounds.height - thumbHeight),
                          2.0F, thumbHeight}, 1.0F, 4, Fade(theme_.cyan, 0.45F));
}

void Application::drawCanvasToolbar() {
    drawPanel(toolbarRect_, theme_, Fade(theme_.panel, 0.94F), 0.16F);
    const float gap = 6.0F;
    const float buttonWidth = (toolbarRect_.width - 12.0F - 4.0F * gap) / 5.0F;
    float x = toolbarRect_.x + 6.0F;
    const float y = toolbarRect_.y + 6.0F;
    const float height = toolbarRect_.height - 12.0F;
    if (uiButton({x, y, buttonWidth, height}, "SMART", theme_, theme_.cyan)) {
        applyPreset(smartPreset(), "Smart preset loaded");
    }
    x += buttonWidth + gap;
    if (uiButton({x, y, buttonWidth, height}, "RANDOM", theme_, theme_.violet)) {
        applyPreset(randomPreset(), "Random cloud generated");
    }
    x += buttonWidth + gap;
    if (uiButton({x, y, buttonWidth, height}, "GRID", theme_, theme_.sky)) {
        applyPreset(gridPreset(), "Grid preset loaded");
    }
    x += buttonWidth + gap;
    if (uiButton({x, y, buttonWidth, height}, "RING", theme_, theme_.amber)) {
        applyPreset(ringPreset(), "Ring preset loaded");
    }
    x += buttonWidth + gap;
    if (uiButton({x, y, buttonWidth, height}, "CLEAR", theme_, theme_.danger)) {
        applyPreset({}, "Canvas cleared");
    }

    const Rectangle canvas = canvasTransform_.viewport();
    const std::string zoom = "ZOOM " + decimalText(canvasTransform_.zoom() * 100.0F, 0) + "%";
    drawChip({canvas.x + canvas.width - 113.0F, canvas.y + 16.0F, 96.0F, 24.0F},
             zoom, theme_.textMuted, theme_);
    const std::string count = integerText(scene_.points().size()) + " POINTS";
    drawChip({canvas.x + canvas.width - 113.0F, canvas.y + 45.0F, 96.0F, 24.0F},
             count, theme_.cyan, theme_);

    drawText("LMB add/drag  |  RMB remove  |  MMB pan  |  Wheel zoom",
             canvas.x + 18.0F, canvas.y + canvas.height - 25.0F, 9.5F,
             Fade(theme_.textMuted, 0.78F));
}

void Application::drawInspector() {
    drawPanel(inspectorRect_, theme_, theme_.panel);
    const DemoMetadata& metadata = demos_.at(activeDemoIndex_).metadata();
    const Color accent = theme_.accent(metadata.accentIndex);
    BeginScissorMode(static_cast<int>(inspectorRect_.x), static_cast<int>(inspectorRect_.y),
                     static_cast<int>(inspectorRect_.width), static_cast<int>(inspectorRect_.height));

    float y = inspectorRect_.y + 14.0F;
    drawChip({inspectorRect_.x + 14.0F, y, std::min(166.0F, textWidth(metadata.category, 10.0F) + 24.0F),
              22.0F}, metadata.category, accent, theme_);
    y += 32.0F;
    y = drawWrappedText(metadata.title,
                        {inspectorRect_.x + 14.0F, y, inspectorRect_.width - 28.0F, 56.0F},
                        20.0F, theme_.text, 1.15F) + 7.0F;
    y = drawWrappedText(metadata.description,
                        {inspectorRect_.x + 14.0F, y, inspectorRect_.width - 28.0F, 72.0F},
                        11.0F, theme_.textMuted, 1.42F) + 8.0F;

    const Rectangle complexityCard{inspectorRect_.x + 14.0F, y, inspectorRect_.width - 28.0F, 45.0F};
    DrawRectangleRounded(complexityCard, 0.15F, 8, Fade(accent, 0.08F));
    drawText("TIME COMPLEXITY", complexityCard.x + 10.0F, complexityCard.y + 7.0F,
             8.0F, Fade(accent, 0.82F), 0.8F);
    drawText(metadata.complexity, complexityCard.x + 10.0F, complexityCard.y + 23.0F,
             12.0F, theme_.text);
    y += 54.0F;

    drawText("INPUT MODEL", inspectorRect_.x + 14.0F, y, 8.5F, theme_.textMuted, 0.8F);
    y += 16.0F;
    const Rectangle hintCard{inspectorRect_.x + 14.0F, y, inspectorRect_.width - 28.0F, 58.0F};
    DrawRectangleRounded(hintCard, 0.12F, 8, Fade(theme_.backgroundRaised, 0.9F));
    DrawRectangleRoundedLinesEx(hintCard, 0.12F, 8, 1.0F, Fade(theme_.border, 0.6F));
    drawWrappedText(metadata.inputHint,
                    {hintCard.x + 10.0F, hintCard.y + 8.0F, hintCard.width - 20.0F, hintCard.height - 12.0F},
                    10.0F, theme_.textMuted, 1.35F);
    y += 68.0F;

    const float statWidth = (inspectorRect_.width - 36.0F) / 3.0F;
    const std::size_t frameIndex = trace_.empty() ? 0U : std::min(player_.frameIndex(), trace_.size() - 1U);
    const std::size_t comparisons = trace_.empty() ? 0U : trace_.frames()[frameIndex].comparisons;
    const std::array<std::pair<std::string, std::string>, 3> stats{{
        {integerText(scene_.points().size()), "POINTS"},
        {integerText(trace_.size()), "FRAMES"},
        {integerText(comparisons), "TESTS"}
    }};
    for (std::size_t index = 0U; index < stats.size(); ++index) {
        const Rectangle card{inspectorRect_.x + 14.0F + static_cast<float>(index) * (statWidth + 4.0F),
                             y, statWidth, 50.0F};
        DrawRectangleRounded(card, 0.16F, 8, Fade(theme_.backgroundRaised, 0.88F));
        drawText(stats[index].first, card.x + (card.width - textWidth(stats[index].first, 16.0F)) * 0.5F,
                 card.y + 7.0F, 16.0F, index == 1U ? accent : theme_.text);
        drawText(stats[index].second, card.x + (card.width - textWidth(stats[index].second, 7.5F)) * 0.5F,
                 card.y + 31.0F, 7.5F, theme_.textMuted, 0.7F);
    }
    y += 62.0F;

    drawText("PLAYBACK", inspectorRect_.x + 14.0F, y, 8.5F, theme_.textMuted, 0.8F);
    y += 15.0F;
    const float controlWidth = (inspectorRect_.width - 44.0F) / 5.0F;
    const std::array<std::string_view, 5> labels{"<<", "<", player_.playing() ? "II" : ">", ">", ">>"};
    for (std::size_t index = 0U; index < labels.size(); ++index) {
        const Rectangle control{inspectorRect_.x + 14.0F + static_cast<float>(index) * (controlWidth + 4.0F),
                                y, controlWidth, 32.0F};
        if (uiButton(control, labels[index], theme_, accent, index == 2U && player_.playing())) {
            if (index == 0U) player_.first();
            if (index == 1U) player_.previous();
            if (index == 2U) player_.toggle();
            if (index == 3U) player_.next(trace_.size());
            if (index == 4U) player_.last(trace_.size());
        }
    }
    y += 42.0F;
    const float progress = trace_.size() <= 1U ? 1.0F
        : static_cast<float>(frameIndex) / static_cast<float>(trace_.size() - 1U);
    DrawRectangleRounded({inspectorRect_.x + 14.0F, y, inspectorRect_.width - 110.0F, 6.0F},
                         1.0F, 6, theme_.backgroundRaised);
    DrawRectangleRounded({inspectorRect_.x + 14.0F, y,
                          (inspectorRect_.width - 110.0F) * progress, 6.0F},
                         1.0F, 6, accent);
    const Rectangle slower{inspectorRect_.x + inspectorRect_.width - 87.0F, y - 10.0F, 24.0F, 25.0F};
    const Rectangle faster{inspectorRect_.x + inspectorRect_.width - 38.0F, y - 10.0F, 24.0F, 25.0F};
    if (uiButton(slower, "-", theme_, accent)) player_.setSpeed(player_.speed() - 0.5F);
    if (uiButton(faster, "+", theme_, accent)) player_.setSpeed(player_.speed() + 0.5F);
    drawText(decimalText(player_.speed(), 1) + "x", slower.x + 27.0F, y - 3.0F, 9.0F, theme_.textMuted);
    y += 25.0F;

    if (!trace_.empty()) {
        const VisualFrame& frame = trace_.frames()[frameIndex];
        const Rectangle stepCard{inspectorRect_.x + 14.0F, y, inspectorRect_.width - 28.0F,
                                 std::max(80.0F, inspectorRect_.y + inspectorRect_.height - y - 62.0F)};
        DrawRectangleRounded(stepCard, 0.10F, 8, Fade(theme_.backgroundRaised, 0.86F));
        DrawRectangleRoundedLinesEx(stepCard, 0.10F, 8, 1.0F, Fade(accent, 0.25F));
        drawText("STEP " + integerText(frameIndex + 1U) + " / " + integerText(trace_.size()),
                 stepCard.x + 11.0F, stepCard.y + 9.0F, 8.5F, accent, 0.8F);
        float stepY = drawWrappedText(frame.headline,
            {stepCard.x + 11.0F, stepCard.y + 26.0F, stepCard.width - 22.0F, 42.0F},
            13.0F, theme_.text, 1.22F);
        drawWrappedText(frame.explanation,
            {stepCard.x + 11.0F, stepY + 4.0F, stepCard.width - 22.0F, stepCard.height - 57.0F},
            10.0F, theme_.textMuted, 1.35F);
    }

    const float legendY = inspectorRect_.y + inspectorRect_.height - 32.0F;
    const std::array<std::pair<Color, std::string_view>, 3> legend{{
        {accent, "accepted"}, {theme_.amber, "candidate"}, {theme_.danger, "rejected"}
    }};
    float legendX = inspectorRect_.x + 16.0F;
    for (const auto& [color, label] : legend) {
        DrawCircleV({legendX + 3.0F, legendY + 5.0F}, 3.0F, color);
        drawText(label, legendX + 10.0F, legendY, 8.0F, theme_.textMuted);
        legendX += textWidth(label, 8.0F) + 26.0F;
    }
    EndScissorMode();
}

void Application::drawFooter() {
    DrawRectangleRec(footerRect_, Color{8, 15, 27, 255});
    DrawLine(0, static_cast<int>(footerRect_.y), static_cast<int>(footerRect_.width),
             static_cast<int>(footerRect_.y), Fade(theme_.border, 0.55F));
    DrawCircleV({16.0F, footerRect_.y + 15.0F}, 3.5F, theme_.mint);
    drawText("READY  |  C++20 / RAYLIB  |  OOPS LAB", 27.0F, footerRect_.y + 9.0F,
             9.0F, theme_.textMuted, 0.8F);
    const std::string shortcuts = "H HELP   CTRL+S SAVE   CTRL+O LOAD   CTRL+Z UNDO   R PRESET";
    drawText(shortcuts, footerRect_.width - textWidth(shortcuts, 8.5F) - 18.0F,
             footerRect_.y + 9.0F, 8.5F, theme_.textMuted, 0.8F);
}

void Application::drawHelpOverlay() {
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.74F));
    const float width = 640.0F;
    const float height = 500.0F;
    const Rectangle card{(static_cast<float>(GetScreenWidth()) - width) * 0.5F,
                         (static_cast<float>(GetScreenHeight()) - height) * 0.5F,
                         width, height};
    drawPanel(card, theme_, Color{14, 26, 45, 255}, 0.06F);
    DrawCircleV({card.x + 35.0F, card.y + 35.0F}, 18.0F, Fade(theme_.cyan, 0.17F));
    drawText("?", card.x + 30.0F, card.y + 24.0F, 20.0F, theme_.cyan);
    drawText("GEOVIZ QUICK GUIDE", card.x + 65.0F, card.y + 20.0F, 20.0F, theme_.text);
    drawText("Interactive computational geometry, designed for learning.",
             card.x + 65.0F, card.y + 47.0F, 10.5F, theme_.textMuted);
    DrawLine(static_cast<int>(card.x + 22.0F), static_cast<int>(card.y + 78.0F),
             static_cast<int>(card.x + card.width - 22.0F), static_cast<int>(card.y + 78.0F),
             Fade(theme_.border, 0.65F));

    const std::array<std::pair<std::string_view, std::string_view>, 10> shortcuts{{
        {"Left click", "Add a point or drag an existing point"},
        {"Right click", "Remove the nearest point"},
        {"Middle drag", "Pan the mathematical canvas"},
        {"Mouse wheel", "Zoom around the cursor"},
        {"Space", "Play or pause algorithm frames"},
        {"Left / Right", "Step backward or forward"},
        {"R", "Load a demo-aware smart preset"},
        {"Ctrl + Z / Y", "Undo or redo scene edits"},
        {"Ctrl + S / O", "Save or load geoviz_scene.geoviz"},
        {"Home", "Reset pan and zoom"}
    }};
    float y = card.y + 100.0F;
    for (const auto& [key, action] : shortcuts) {
        drawChip({card.x + 28.0F, y, 112.0F, 24.0F}, key, theme_.cyan, theme_);
        drawText(action, card.x + 158.0F, y + 5.0F, 11.0F, theme_.textMuted);
        y += 34.0F;
    }
    drawText("Press H, F1, or Esc to close", card.x + 28.0F, card.y + card.height - 34.0F,
             10.0F, theme_.amber);
}

void Application::drawToast() {
    if (toastTimeRemaining_ <= 0.0F || toastMessage_.empty()) return;
    const Color accent = toastIsError_ ? theme_.danger : theme_.mint;
    const float width = std::clamp(textWidth(toastMessage_, 11.0F) + 48.0F, 190.0F, 430.0F);
    const Rectangle canvas = canvasTransform_.viewport();
    const Rectangle toast{canvas.x + (canvas.width - width) * 0.5F,
                          canvas.y + canvas.height - 66.0F, width, 38.0F};
    DrawRectangleRounded(toast, 0.22F, 10, Color{17, 31, 50, 248});
    DrawRectangleRoundedLinesEx(toast, 0.22F, 10, 1.0F, Fade(accent, 0.72F));
    DrawCircleV({toast.x + 18.0F, toast.y + 19.0F}, 4.0F, accent);
    drawText(toastMessage_, toast.x + 32.0F, toast.y + 12.0F, 11.0F, theme_.text);
}

SceneSnapshot Application::randomPreset(std::size_t count) {
    std::uniform_real_distribution<double> xDistribution(-410.0, 410.0);
    std::uniform_real_distribution<double> yDistribution(-270.0, 270.0);
    SceneSnapshot preset;
    preset.points.reserve(count);
    for (std::size_t index = 0U; index < count; ++index) {
        preset.points.emplace_back(xDistribution(randomEngine_), yDistribution(randomEngine_));
    }
    return preset;
}

SceneSnapshot Application::gridPreset() const {
    SceneSnapshot preset;
    for (int row = -2; row <= 2; ++row) {
        for (int column = -3; column <= 3; ++column) {
            preset.points.emplace_back(column * 115.0 + (row % 2) * 24.0,
                                       row * 105.0 + (column % 2) * 14.0);
        }
    }
    return preset;
}

SceneSnapshot Application::ringPreset() const {
    SceneSnapshot preset;
    constexpr std::size_t count = 20U;
    for (std::size_t index = 0U; index < count; ++index) {
        const double angle = 2.0 * kPi * static_cast<double>(index) / static_cast<double>(count);
        const double radius = 235.0 + 55.0 * std::sin(3.0 * angle);
        preset.points.emplace_back(radius * std::cos(angle), radius * std::sin(angle));
    }
    preset.points.emplace_back(0.0, 0.0);
    preset.points.emplace_back(72.0, -48.0);
    return preset;
}

SceneSnapshot Application::smartPreset() {
    const DemoId id = demos_.at(activeDemoIndex_).metadata().id;
    if (id == DemoId::HalfPlaneIntersection) {
        return {{{-360.0, -220.0}, {360.0, -220.0},
                 {360.0, -220.0}, {360.0, 220.0},
                 {360.0, 220.0}, {-360.0, 220.0},
                 {-360.0, 220.0}, {-360.0, -220.0},
                 {-430.0, -80.0}, {230.0, 300.0}}, {}};
    }
    if (id == DemoId::SegmentIntersections) {
        return {{{-380.0, -210.0}, {340.0, 220.0},
                 {-330.0, 230.0}, {370.0, -190.0},
                 {-420.0, 40.0}, {410.0, 40.0},
                 {-80.0, -280.0}, {110.0, 280.0}}, {}};
    }
    if (id == DemoId::EarClipping || id == DemoId::PointInPolygon) {
        std::vector<Point> polygon{{-350.0, -160.0}, {-120.0, -245.0}, {20.0, -120.0},
                                   {190.0, -235.0}, {365.0, -60.0}, {220.0, 205.0},
                                   {40.0, 125.0}, {-110.0, 260.0}, {-330.0, 145.0}};
        if (id == DemoId::PointInPolygon) polygon.emplace_back(55.0, 20.0);
        return {std::move(polygon), {}};
    }
    if (id == DemoId::PrimitiveLab) {
        return {{{-260.0, -80.0}, {90.0, 145.0}, {260.0, -135.0},
                 {-70.0, 245.0}, {335.0, 180.0}}, {}};
    }
    return randomPreset(id == DemoId::Delaunay || id == DemoId::Voronoi ? 15U : 20U);
}

void Application::applyPreset(SceneSnapshot preset, std::string message) {
    commands_.execute(std::make_unique<ReplaceSceneCommand>(std::move(preset)), scene_);
    showToast(std::move(message));
}

void Application::saveScene() {
    try {
        io::SceneSerializer::save(scene_.snapshot(), std::filesystem::path{kSceneFilename});
        showToast("Saved geoviz_scene.geoviz");
    } catch (const std::exception& error) {
        showToast(error.what(), true);
    }
}

void Application::loadScene() {
    try {
        SceneSnapshot loaded = io::SceneSerializer::load(std::filesystem::path{kSceneFilename});
        commands_.execute(std::make_unique<ReplaceSceneCommand>(std::move(loaded)), scene_);
        showToast("Loaded geoviz_scene.geoviz");
    } catch (const std::exception& error) {
        showToast(error.what(), true);
    }
}

void Application::showToast(std::string message, bool error) {
    toastMessage_ = std::move(message);
    toastTimeRemaining_ = 3.2F;
    toastIsError_ = error;
}

}  // namespace geoviz::app
