#pragma once
#include "algorithm_metadata.hpp"
#include "atomic_files.hpp"
#include "file_dialogs.hpp"
#include "geoviz.hpp"
#include "numeric_text.hpp"
#include "raylib.h"
#include "workspace.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace {
using namespace gv;
// Warm paper, sage and earth colours keep text and algorithm states legible.
const Color BG{246, 245, 240, 255}, PANEL{255, 255, 252, 255}, INK{39, 52, 48, 255},
    MUTED{82, 101, 92, 255}, ACCENT{36, 103, 77, 255}, FOCUS{143, 91, 20, 255},
    DANGER{171, 62, 48, 255}, DESTINATION{117, 71, 127, 255}, BORDER{204, 214, 204, 255},
    SELECTED_BG{220, 237, 222, 255}, SOFT_BG{234, 239, 229, 255}, CANVAS{252, 252, 247, 255},
    GRID{231, 235, 225, 255}, AXIS{184, 199, 184, 255}, MAP_INK{163, 179, 164, 255},
    EDGE_INK{129, 150, 138, 255}, POINT_INK{57, 93, 111, 255}, VEIL{246, 245, 240, 240};
const Color CLUSTERS[] = {{35, 106, 145, 255}, {155, 94, 21, 255}, {117, 71, 127, 255},
                          {45, 112, 66, 255},  {169, 65, 53, 255}, {111, 111, 36, 255}};
// RAII: stack objects pair raylib acquisition and release, including
// exceptions.
class Window final {
  public:
    explicit Window(bool automated = false, int width = 1440, int height = 900) {
        unsigned int flags = FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT;
        if (automated)
            flags |= FLAG_WINDOW_HIDDEN | FLAG_WINDOW_ALWAYS_RUN;
        SetConfigFlags(flags);
        InitWindow(width, height, "GeoViz | Geometry and Spatial Security");
        if (!IsWindowReady())
            throw std::runtime_error("Could not create the application window.");
        SetWindowMinSize(960, 640);
        if (!automated) {
            int monitor = GetCurrentMonitor();
            int w = std::max(960, std::min(1600, GetMonitorWidth(monitor) - 80));
            int h = std::max(640, std::min(1000, GetMonitorHeight(monitor) - 100));
            SetWindowSize(w, h);
            auto origin = GetMonitorPosition(monitor);
            SetWindowPosition(static_cast<int>(origin.x) + (GetMonitorWidth(monitor) - w) / 2,
                              static_cast<int>(origin.y) + (GetMonitorHeight(monitor) - h) / 2);
        }
        SetExitKey(KEY_NULL);
        SetTargetFPS(60);
    }
    ~Window() {
        CloseWindow();
    }
    Window(const Window &) = delete;
    Window &operator=(const Window &) = delete;
};
// Own fonts once; render directly at the display resolution in logical UI
// units.
class Ui final {
    inline static Ui *active_ = nullptr;
    Font regular_{};
    Font bold_{};
    RenderTexture2D testTarget_{};
    int testWidth_ = 0, testHeight_ = 0;
    float scale_ = 1;
    inline static int textAdjustment_ = 0;
    static const Font &font(int size) {
        return size >= 20 ? active_->bold_ : active_->regular_;
    }
    static Font load(const std::filesystem::path &path) {
        Font result = LoadFontEx(path.string().c_str(), 64, nullptr, 0);
        if (!result.texture.id || result.texture.id == GetFontDefault().texture.id)
            throw std::runtime_error("Cannot load font: " + path.string());
        SetTextureFilter(result.texture, TEXTURE_FILTER_BILINEAR);
        return result;
    }

  public:
    explicit Ui(const std::filesystem::path &root, int testWidth = 0, int testHeight = 0)
        : testWidth_(testWidth), testHeight_(testHeight) {
        regular_ = load(root / "assets/Lato-Regular.ttf");
        try {
            bold_ = load(root / "assets/Lato-Bold.ttf");
            if (testWidth_) {
                testTarget_ = LoadRenderTexture(testWidth_, testHeight_);
                if (!testTarget_.id)
                    throw std::runtime_error("Could not create the automated render target.");
            }
        } catch (...) {
            if (bold_.texture.id)
                UnloadFont(bold_);
            UnloadFont(regular_);
            throw;
        }
        active_ = this;
        refresh();
    }
    ~Ui() {
        if (testTarget_.id)
            UnloadRenderTexture(testTarget_);
        UnloadFont(bold_);
        UnloadFont(regular_);
        active_ = nullptr;
    }
    Ui(const Ui &) = delete;
    Ui &operator=(const Ui &) = delete;
    void refresh() {
        scale_ = std::max(0.8f, std::min(pixelWidth() / 1440.0f, pixelHeight() / 900.0f));
    }
    static int pixelWidth() {
        return active_->testWidth_ ? active_->testWidth_ : GetScreenWidth();
    }
    static int pixelHeight() {
        return active_->testHeight_ ? active_->testHeight_ : GetScreenHeight();
    }
    static float scale() {
        return active_->scale_;
    }
    static float width() {
        return pixelWidth() / scale();
    }
    static float height() {
        return pixelHeight() / scale();
    }
    static Vector2 mouse() {
        auto p = GetMousePosition();
        return {p.x / scale(), p.y / scale()};
    }
    static Vector2 mouseDelta() {
        auto p = GetMouseDelta();
        return {p.x / scale(), p.y / scale()};
    }
    static void adjustText(int delta) {
        textAdjustment_ = std::clamp(textAdjustment_ + delta, -2, 6);
    }
    static float fontSize(int requested) {
        return static_cast<float>(std::max(17, requested + 3 + textAdjustment_));
    }
    static float measure(const char *value, int size) {
        return MeasureTextEx(font(size), value, fontSize(size), 0.6f).x;
    }
    static void draw(const std::string &value, float x, float y, int size, Color color) {
        DrawTextEx(font(size), value.c_str(), {x, y}, fontSize(size), 0.6f, color);
    }
    static Camera2D camera() {
        return {{0, 0}, {0, 0}, 0, scale()};
    }
    static void beginTarget() {
        if (active_->testTarget_.id)
            BeginTextureMode(active_->testTarget_);
    }
    static void endTarget() {
        if (active_->testTarget_.id)
            EndTextureMode();
    }
    static void capture(const std::filesystem::path &path) {
        std::string filename = path.string();
        Image image = LoadImageFromTexture(active_->testTarget_.texture);
        ImageFlipVertical(&image);
        bool saved = ExportImage(image, filename.c_str());
        UnloadImage(image);
        if (!saved)
            throw std::runtime_error("Could not save the automated screenshot.");
    }
    static void clip(float x, float y, float width, float height) {
        int left = static_cast<int>(std::floor(x * scale()));
        int top = static_cast<int>(std::floor(y * scale()));
        BeginScissorMode(left, top, static_cast<int>(std::ceil((x + width) * scale())) - left,
                         static_cast<int>(std::ceil((y + height) * scale())) - top);
    }
};
// Each strategy has a concrete workflow, shown before its results in the
// sidebar.
class AlgorithmHelp final {
  public:
    static std::vector<std::string> steps(Mode mode, int securityView) {
        std::vector<std::string> result;
        if (securityView)
            result.push_back("Observations are read-only. Click to inspect; Settings changes "
                             "filters and radii. P edits the geographic fence.");
        switch (mode) {
        case Mode::Hull:
            result.push_back("Click to add points, drag to move them, or choose Sample.");
            result.push_back("Run, then Play or Step: watch turns remove interior "
                             "points and form the outer boundary.");
            break;
        case Mode::Geofence:
            result.push_back("Press P (or Polygon) to add fence vertices in boundary "
                             "order. P returns to the points tool.");
            result.push_back("Right-click sets the query point. Run, then Step to "
                             "follow ray crossings: inside, boundary or outside.");
            break;
        case Mode::Dijkstra:
        case Mode::AStar:
            if (securityView)
                return {"Routing uses an editable graph. Click Lab, choose Dijkstra or A*, "
                        "then Sample or Grid."};
            result.push_back("Start with Sample or Grid. To build a graph, add "
                             "points, then Shift-click two points to join them.");
            result.push_back("Click Pick source below, then click a point. Click "
                             "Pick destination, then click another point. Or select "
                             "a point and press R / T.");
            result.push_back("Run, then Play or Step to explore the route. Click an "
                             "edge: B blocks it; Settings edits its cost.");
            break;
        case Mode::DBSCAN:
        case Mode::IndexedDBSCAN:
            result.push_back(securityView
                                 ? "Settings sets the geographic/feature radius, MinPts and "
                                   "filters. The point itself counts toward MinPts."
                                 : "Add points or choose Sample. Settings sets radius and "
                                   "MinPts (the point itself counts).");
            result.push_back("Run, then Step through neighbours and cluster growth. Filled "
                             "circles = core, squares = border, crosses = noise.");
            result.push_back("Compare checks scan and indexed DBSCAN on the same input.");
            break;
        case Mode::ClosestPair:
            result.push_back("Add at least two points, drag them to experiment, or "
                             "choose Sample.");
            result.push_back("Run, then Step through comparisons. The highlighted "
                             "pair has the smallest Euclidean distance.");
            break;
        case Mode::KDNearest:
            result.push_back("Add points or choose Sample. Right-click anywhere to "
                             "place the QUERY cross.");
            result.push_back("Run, then Step through tree construction and search. Dashed regions "
                             "are pruned; the final point is nearest.");
            break;
        case Mode::KDRadius:
            result.push_back("Add points or choose Sample. Right-click to place "
                             "QUERY; set the search radius in Settings.");
            result.push_back("Run, then Step through the tree. Highlighted points "
                             "are within the query circle, including its boundary.");
            break;
        case Mode::Collision:
            result.push_back("Add points in pairs: (0,1), (2,3), ... Each pair is "
                             "one closed segment. Sample provides crossings.");
            result.push_back("Run, then Step through segment pairs. Crossings, touching endpoints "
                             "and collinear overlaps count as collisions.");
            break;
        }
        return result;
    }
};
class DrawingSession final {
  public:
    DrawingSession() {
        BeginDrawing();
        Ui::beginTarget();
        BeginMode2D(Ui::camera());
    }
    ~DrawingSession() {
        EndMode2D();
        Ui::endTarget();
        EndDrawing();
    }
    DrawingSession(const DrawingSession &) = delete;
    DrawingSession &operator=(const DrawingSession &) = delete;
};
// Bidirectional world/screen transform. Only presentation depends on viewport size.
class View {
    friend class App;
    Rectangle area{};
    Point centre{350, 240};
    double zoom = 1;

  public:
    const Rectangle &bounds() const noexcept {
        return area;
    }
    double magnification() const noexcept {
        return zoom;
    }
    Vector2 screen(Point p) const {
        return {area.x + area.width / 2 + static_cast<float>((p.x() - centre.x()) * zoom),
                area.y + area.height / 2 - static_cast<float>((p.y() - centre.y()) * zoom)};
    }
    Point world(Vector2 p) const {
        return {centre.x() + (p.x - area.x - area.width / 2) / zoom,
                centre.y() - (p.y - area.y - area.height / 2) / zoom};
    }
    void fit(const Scene &s, bool feature = false) {
        std::vector<Point> all = s.points();
        if (!feature)
            all.insert(all.end(), s.polygon().begin(), s.polygon().end());
        if (all.empty()) {
            centre = feature ? Point{0, 0} : Point{350, 240};
            zoom = feature ? 40 : 1;
            return;
        }
        double x0 = all[0].x(), x1 = x0, y0 = all[0].y(), y1 = y0;
        for (auto p : all) {
            x0 = std::min(x0, p.x());
            x1 = std::max(x1, p.x());
            y0 = std::min(y0, p.y());
            y1 = std::max(y1, p.y());
        }
        centre = {(x0 + x1) / 2, (y0 + y1) / 2};
        zoom = std::min((area.width - 40) / std::max(100.0, x1 - x0 + 80),
                        (area.height - 150) / std::max(100.0, y1 - y0 + 80));
        if (feature)
            zoom = std::min((area.width - 40) / std::max(2.0, (x1 - x0) * 1.3 + 1),
                            (area.height - 150) / std::max(2.0, (y1 - y0) * 1.3 + 1));
        zoom = std::clamp(zoom, 0.001, 200.0);
        centre.setY(centre.y() + 45 / zoom); // Reserve a header above all data.
    }
};
class ButtonFocus final {
    inline static int focus_ = -1, count_ = 0, previous_ = 0;
    inline static bool enter_ = false, blocked_ = false;

  public:
    static bool allowsInput() {
        return !blocked_;
    }
    static bool hasFocus() {
        return focus_ >= 0;
    }
    static void begin(const std::set<int> &keys, bool modal) {
        count_ = 0;
        blocked_ = modal;
        enter_ = keys.count(KEY_ENTER) != 0;
        if (keys.count(KEY_TAB) && !modal)
            focus_ = (focus_ + (IsKeyDown(KEY_LEFT_SHIFT) ? std::max(1, previous_) - 1 : 1) +
                      std::max(1, previous_)) %
                     std::max(1, previous_);
    }
    static void overlay() {
        blocked_ = false;
    }
    static bool current(bool enabled, Rectangle r) {
        if (blocked_ || !enabled)
            return false;
        int id = count_++;
        bool focus = id == focus_;
        if (focus)
            DrawRectangleLinesEx(r, 3, FOCUS);
        return focus && enter_;
    }
    static void finish() {
        previous_ = count_;
        if (focus_ >= count_)
            focus_ = -1;
    }
};
bool button(Rectangle r, const std::string &label, bool selected = false, bool enabled = true) {
    bool hover = CheckCollisionPointRec(Ui::mouse(), r);
    DrawRectangleRounded(r, 0.18f, 6, selected ? SELECTED_BG : (hover ? SOFT_BG : PANEL));
    DrawRectangleRoundedLinesEx(r, 0.18f, 6, 1, selected ? ACCENT : BORDER);
    int size = 17;
    while (size > 14 && Ui::measure(label.c_str(), size) > r.width - 16)
        --size;
    Ui::draw(label, r.x + 8, r.y + (r.height - Ui::fontSize(size)) / 2, size,
             selected ? ACCENT : INK);
    bool keyboard = ButtonFocus::current(enabled, r);
    if (!enabled)
        DrawRectangleRec(r, {246, 245, 240, 145});
    return enabled && ButtonFocus::allowsInput() &&
           (keyboard || (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)));
}
void text(const std::string &s, float x, float y, int size = 17, Color color = INK) {
    Ui::draw(s, x, y, size, color);
}
float wrap(const std::string &value, float x, float y, float width, int size = 17,
           Color color = INK) {
    std::istringstream in(value);
    std::string word, line;
    while (in >> word) {
        std::string next = line.empty() ? word : line + " " + word;
        if (Ui::measure(next.c_str(), size) > width && !line.empty()) {
            text(line, x, y, size, color);
            y += Ui::fontSize(size) + 7;
            line = word;
        } else
            line = next;
        // Split very long identifiers so they cannot overrun the panel.
        while (Ui::measure(line.c_str(), size) > width && line.size() > 1) {
            std::size_t cut = line.size();
            while (cut > 1 && Ui::measure(line.substr(0, cut).c_str(), size) > width)
                --cut;
            text(line.substr(0, cut), x, y, size, color);
            y += Ui::fontSize(size) + 7;
            line.erase(0, cut);
        }
    }
    if (!line.empty()) {
        text(line, x, y, size, color);
        y += Ui::fontSize(size) + 7;
    }
    return y;
}
void line(const View &view, Point a, Point b, Color color, float thick = 2, bool dashed = false) {
    auto u = view.screen(a), v = view.screen(b);
    if (!dashed)
        DrawLineEx(u, v, thick, color);
    else {
        float dx = v.x - u.x, dy = v.y - u.y, len = std::hypot(dx, dy);
        if (len < 0.1f)
            return;
        for (float t = 0; t < len; t += 14) {
            float end = std::min(t + 7, len);
            DrawLineEx({u.x + dx * t / len, u.y + dy * t / len},
                       {u.x + dx * end / len, u.y + dy * end / len}, thick, color);
        }
    }
}
double segmentPixelDistance(Vector2 p, Vector2 a, Vector2 b) {
    double dx = b.x - a.x, dy = b.y - a.y, d = dx * dx + dy * dy;
    double t = d ? std::clamp(((p.x - a.x) * dx + (p.y - a.y) * dy) / d, 0.0, 1.0) : 0;
    return std::hypot(p.x - a.x - t * dx, p.y - a.y - t * dy);
}

#include "canvas_art.hpp"
#include "ui_canvas.hpp"
#include "ui_settings.hpp"

} // namespace
