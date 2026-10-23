#include "geoviz.hpp"
#include "raylib.h"
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
    static const Font &font(int size) {
        return size >= 20 ? active_->bold_ : active_->regular_;
    }
    static Font load(const std::filesystem::path &path) {
        Font result = LoadFontEx(path.string().c_str(), 96, nullptr, 0);
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
    static float fontSize(int requested) {
        return static_cast<float>(std::max(17, requested + 3));
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
bool button(Rectangle r, const std::string &label, bool selected = false) {
    bool hover = CheckCollisionPointRec(Ui::mouse(), r);
    DrawRectangleRounded(r, 0.18f, 6, selected ? SELECTED_BG : (hover ? SOFT_BG : PANEL));
    DrawRectangleRoundedLinesEx(r, 0.18f, 6, 1, selected ? ACCENT : BORDER);
    int size = 17;
    while (size > 14 && Ui::measure(label.c_str(), size) > r.width - 16)
        --size;
    Ui::draw(label, r.x + 8, r.y + (r.height - Ui::fontSize(size)) / 2, size,
             selected ? ACCENT : INK);
    return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
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

// Coordinates the domain model, execution interface, playback and raylib presentation.
// Input handlers own edits; drawing reads the frozen Run and never advances algorithms.
class App {
    std::unique_ptr<AlgorithmRunner> engine = std::make_unique<Engine>();
    Scene scene = sampleScene(Mode::Hull, 1);
    int sampleVariant = 1, securitySample = 0;
    std::vector<SecuritySample> securityCatalogue;
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
    int smokeFrame = 0;
    double sidebarScroll = 0;
    double helpScroll = 0;
    int windowWidth = 1440, windowHeight = 900;
    enum class RoutePick { None, Source, Destination };
    RoutePick routePick = RoutePick::None;

    Rectangle sidebarBounds() const {
        float width = std::clamp(Ui::width() * 0.27f, 360.0f, 440.0f);
        return {Ui::width() - width - 16, 142, width, Ui::height() - 314};
    }
    // Recompute logical bounds after resize; screen scaling is applied once by Ui.
    void updateLayout() {
        auto panel = sidebarBounds();
        view.area = {16, 142, panel.x - 32, panel.height};
    }
    // Source/destination picking is an explicit tool state, separate from point dragging.
    bool pickEndpoint(int id) {
        if (routePick == RoutePick::None || securityView || id < 0 ||
            id >= static_cast<int>(scene.points().size()))
            return false;
        bool source = routePick == RoutePick::Source;
        if (source)
            scene.setSource(id);
        else
            scene.setGoal(id);
        selected = id;
        routePick = RoutePick::None;
        invalidate();
        status = std::string(source ? "Source" : "Destination") + " set to point #" +
                 std::to_string(id) + ". Run to calculate the route.";
        return true;
    }

    bool feature() const {
        return securityView == 2;
    }
    // Move the old result to history before resetting playback; stale evidence is never current.
    void invalidate() {
        scene.touch();
        if (run)
            previous = std::move(run);
        playback.reset();
        status = "Input changed. Old trace invalidated; Run again.";
    }
    // The geographic editor is the authority for its fence; the feature view has no polygon.
    void syncFence() {
        if (securityView == 1) {
            security.setFence(scene.polygon());
            evaluationDirty = true;
        }
    }
    // Rebuild derived observations and map positions after a source/filter/scaling change.
    void refreshSecurity() {
        evaluationDirty = true;
        scene = security.scene(feature());
        observations = security.observations(feature());
        map = security.loadMap((root / "data/campus_map.csv").string());
        invalidate();
        selected = -1;
        selectedEdge = -1;
        view.fit(scene, feature());
    }
    // Sample replaces the complete input context, including stale selections.
    void nextSample() {
        routePick = RoutePick::None;
        selected = selectedEdge = edgeStart = drag = polygonDrag = -1;
        polygonTool = false;
        sidebarScroll = 0;
        if (securityView) {
            const int targetView = securityView;
            const int nextIndex = (securitySample + 1) % static_cast<int>(securityCatalogue.size());
            const auto &sample = securityCatalogue[nextIndex];
            auto next = loadSecuritySample((root / "data").string(), sample);
            security = std::move(next);
            securitySample = nextIndex;
            securityDataset = sample.name;
            securityView = targetView;
            refreshSecurity();
            status = sample.name + ": " + sample.purpose + ". Sample cycles " +
                     std::to_string(securityCatalogue.size()) + " datasets.";
        } else {
            sampleVariant = (sampleVariant + 1) % SampleVariantCount;
            scene = sampleScene(mode, sampleVariant);
            invalidate();
            view.fit(scene);
            status =
                sampleName(mode, sampleVariant) + ". Sample cycles six inputs for this algorithm.";
        }
    }
    // Clear view-specific interaction and history before changing the coordinate units.
    void setView(int v) {
        routePick = RoutePick::None;
        sidebarScroll = 0;
        syncFence();
        if (v == securityView)
            return;
        securityView = v;
        polygonTool = false;
        run.reset();
        previous.reset();
        playback.reset();
        selected = -1;
        if (v) {
            mode = Mode::IndexedDBSCAN;
            refreshSecurity();
            status = "Security data loaded. Settings controls filters, window and "
                     "scaling.";
        } else {
            scene = sampleScene(mode, sampleVariant);
            view.fit(scene);
            status = "Algorithm laboratory.";
        }
    }
    // Freeze input and result together; playback reads this Run while later edits invalidate it.
    void execute() {
        syncFence();
        Result r = engine->execute(scene, mode, true);
        if (run)
            previous = std::move(run);
        run = std::make_unique<Run>(Run{scene, std::move(r)});
        playback.reset();
        status = run->result().summary() + " " + run->result().check();
    }
    // Dispatch by input format and validate temporary security state before replacing current data.
    void loadFile(const std::string &path) {
        auto ext = std::filesystem::path(path).extension().string();
        if (ext == ".csv") {
            auto next = security;
            std::unique_ptr<LogAdapter> adapter = std::make_unique<CsvLogAdapter>();
            next.setEvents(adapter->load(path));
            next.clearReviews();
            next.validate();
            next.observations(false);
            next.observations(true);
            security = std::move(next);
            securityView = 1;
            securityDataset = std::filesystem::path(path).filename().string();
            mode = Mode::IndexedDBSCAN;
            refreshSecurity();
        } else if (ext == ".security") {
            auto next = SecurityConsole::load(path);
            next.observations(false);
            next.observations(true);
            security = std::move(next);
            securityView = 1;
            securityDataset = std::filesystem::path(path).filename().string();
            mode = Mode::IndexedDBSCAN;
            refreshSecurity();
        } else {
            auto next = SceneIO::load(path);
            securityView = 0;
            scene = std::move(next);
            invalidate();
            view.fit(scene);
            selected = -1;
            selectedEdge = -1;
        }
        status = "Loaded " + path;
    }
    // Save only input state; exported evidence separately includes checked results.
    void save() {
        syncFence();
        if (securityView)
            security.save((root / "output/session.security").string());
        else
            SceneIO::save(scene, (root / "output/session.scene").string());
        status = "Saved current session in output/.";
    }
    // Keep evidence beside its exact reproducible snapshots in the generated output directory.
    void exportEvidence() {
        syncFence();
        if (securityView) {
            security.evidence((root / "output/evidence.txt").string());
            if (run)
                SceneIO::summary(*run, (root / "output/security_selected_run.txt").string());
            status = "Exported evidence, full security snapshot and both view scenes "
                     "in output/.";
        } else {
            if (!run)
                throw std::runtime_error("Run an algorithm before exporting a checked summary.");
            SceneIO::summary(*run, (root / "output/run.txt").string());
            status = "Exported output/run.txt and its exact scene snapshot.";
        }
    }
    // Compare equivalent algorithms on the same scene, never on a previous edited input.
    void compare() {
        if (!run)
            throw std::runtime_error("Run first.");
        if (mode == Mode::Dijkstra || mode == Mode::AStar || mode == Mode::DBSCAN ||
            mode == Mode::IndexedDBSCAN) {
            Mode other = mode == Mode::Dijkstra ? Mode::AStar
                         : mode == Mode::AStar  ? Mode::Dijkstra
                         : mode == Mode::DBSCAN ? Mode::IndexedDBSCAN
                                                : Mode::DBSCAN;
            auto result = engine->execute(scene, other, false);
            bool same =
                (mode == Mode::DBSCAN || mode == Mode::IndexedDBSCAN)
                    ? result.labels() == run->result().labels()
                    : ((!std::isfinite(result.value()) && !std::isfinite(run->result().value())) ||
                       std::abs(result.value() - run->result().value()) < 1e-7);
            status = "Same input: " + modeName(other) + " value=" + number(result.value()) +
                     " ops=" + std::to_string(result.stats().operations()) +
                     " expanded=" + std::to_string(result.stats().expanded()) +
                     " agree=" + (same ? "YES" : "NO");
        } else if (previous)
            status = "Previous " + modeName(previous->result().mode()) +
                     " value=" + number(previous->result().value()) +
                     " ops=" + std::to_string(previous->result().stats().operations()) +
                     "; current value=" + number(run->result().value()) +
                     " ops=" + std::to_string(run->result().stats().operations());
        else
            status = "No previous run yet. Edit or rerun to retain one.";
    }
    // Historical comparison preserves both input snapshots and their separate result summaries.
    void comparePrevious() {
        if (!run || !previous)
            throw std::runtime_error("Keep two completed runs before comparing history.");
        status = "Previous " + modeName(previous->result().mode()) + ": " +
                 previous->result().summary() +
                 " ops=" + std::to_string(previous->result().stats().operations()) + "; current " +
                 modeName(run->result().mode()) + ": " + run->result().summary() +
                 " ops=" + std::to_string(run->result().stats().operations());
        SceneIO::summary(*previous, (root / "output/previous.txt").string());
        SceneIO::summary(*run, (root / "output/current.txt").string());
    }
    void openSettings() {
        settings.open(scene, security, selectedEdge);
    }
    // Commit only after the component has validated every candidate field.
    void applySettings() {
        auto candidate = settings.candidate(scene, security, selectedEdge, securityView);
        if (securityView) {
            security = std::move(candidate.security);
            refreshSecurity();
        } else {
            scene = std::move(candidate.scene);
            invalidate();
        }
        settings.close();
        status = "Settings applied. Run again.";
    }
    // The Scene remaps topology; the UI only clears interaction indices after deletion.
    void removeSelected() {
        if (securityView)
            throw std::runtime_error("Source observations are inspected through filters; edit the "
                                     "CSV to change records.");
        if (selected < 0)
            return;
        scene.removePoint(selected);
        selected = -1;
        selectedEdge = -1;
        edgeStart = -1;
        invalidate();
    }
    void review(const std::string &verdict) {
        evaluationDirty = true;
        if (!securityView || selected < 0 || selected >= static_cast<int>(observations.size()))
            return;
        for (int id : observations[selected].events())
            security.setReview(security.events()[id].id(), verdict);
        status = "Analyst review set to " + verdict +
                 " for this observation's source records. Export to retain evidence.";
    }
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
            settings.close();
            routePick = RoutePick::None;
        }
        if (help) {
            helpScroll = std::max(0.0, helpScroll - GetMouseWheelMove() * 48.0);
            return;
        }
        if (settings.isOpen())
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
        if (pressed(KEY_ENTER))
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
            while (playback.cursor() + 1 < run->result().trace().frames().size())
                playback.step(1, run->result().trace().frames().size());
        }
        if (pressed(KEY_N))
            nextSample();
        if (pressed(KEY_F))
            view.fit(scene, feature());
        if (pressed(KEY_P) && !feature())
            polygonTool = !polygonTool;
        if (pressed(KEY_DELETE))
            removeSelected();
        if (pressed(KEY_S))
            save();
        if (pressed(KEY_L))
            loadFile((root / (securityView ? "output/session.security" : "output/session.scene"))
                         .string());
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
        if (pressed(KEY_O))
            openSettings();
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
        auto mouse = Ui::mouse();
        if (CheckCollisionPointRec(mouse, view.area)) {
            float wheel = GetMouseWheelMove();
            if (wheel) {
                auto before = view.world(mouse);
                view.zoom = std::clamp(view.zoom * std::pow(1.15, wheel), 0.001, 200.0);
                auto after = view.world(mouse);
                view.centre.setX(view.centre.x() + (before.x() - after.x()));
                view.centre.setY(view.centre.y() + (before.y() - after.y()));
            }
            if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
                auto delta = Ui::mouseDelta();
                view.centre.setX(view.centre.x() - (delta.x / view.zoom));
                view.centre.setY(view.centre.y() + (delta.y / view.zoom));
            }
            Point world = view.world(mouse);
            if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                scene.setQuery(world);
                if (securityView == 1)
                    security.settings().setGeoQuery(world);
                if (securityView == 2)
                    security.settings().setFeatureQuery(world);
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
                        if (scene.polygon().size() >= MAX_POINTS)
                            throw std::runtime_error("Polygon vertex limit reached.");
                        scene.addPolygonVertex(world);
                        invalidate();
                        syncFence();
                    }
                } else if (near >= 0) {
                    selected = near;
                    if (shift && !securityView) {
                        if (edgeStart < 0) {
                            edgeStart = near;
                            status = "Edge start selected. Shift-click its other endpoint.";
                        } else if (edgeStart != near) {
                            if (scene.edges().size() >= MAX_EDGES)
                                throw std::runtime_error("Edge limit reached.");
                            for (auto edge : scene.edges())
                                if ((edge.a() == edgeStart && edge.b() == near) ||
                                    (edge.b() == edgeStart && edge.a() == near)) {
                                    edgeStart = -1;
                                    throw std::runtime_error("That edge already exists. Select "
                                                             "it to edit its weight.");
                                }
                            scene.addEdge({edgeStart, near,
                                           GeometryKernel::distance(scene.points()[edgeStart],
                                                                    scene.points()[near]),
                                           false});
                            edgeStart = -1;
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
                        status = "Edge selected. B blocks/unblocks; Settings edits its weight.";
                    } else if (!securityView) {
                        if (scene.points().size() >= MAX_POINTS)
                            throw std::runtime_error("Point limit reached.");
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
                    scene.setGrid(false);
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
    // Return a read-only snapshot at the playback cursor, or no frame before a run.
    const Frame *frame() const {
        return run && !run->result().trace().frames().empty()
                   ? &run->result().trace().frames()[playback.cursor()]
                   : nullptr;
    }
    // Supply a read-only presentation context; the renderer cannot mutate app state.
    void canvas() const {
        SceneCanvas({scene, view, frame(), mode, sampleVariant, securityView, selected,
                     selectedEdge, polygonTool, map, security.settings(), securityDataset})
            .draw();
    }
    // Show results first; the scrollable inspector retains complete source and help details.
    void sidebar() {
        auto panel = sidebarBounds();
        float x = panel.x + 16, top = panel.y, bottom = panel.y + panel.height - 32;
        float contentWidth = panel.width - 32;
        DrawRectangleRounded(panel, 0.025f, 6, PANEL);
        Ui::clip(panel.x + 8, top, panel.width - 16, panel.height - 32);
        float y = top + 14 - static_cast<float>(sidebarScroll);
        if (!securityView && (mode == Mode::Dijkstra || mode == Mode::AStar)) {
            bool interactive =
                !help && !settings.isOpen() && CheckCollisionPointRec(Ui::mouse(), panel);
            float half = (contentWidth - 8) / 2;
            bool sourceClicked =
                button({x, y, half, 38}, "Pick source", routePick == RoutePick::Source);
            bool goalClicked = button({x + half + 8, y, half, 38}, "Pick destination",
                                      routePick == RoutePick::Destination);
            if (interactive && (sourceClicked || goalClicked)) {
                routePick = sourceClicked ? RoutePick::Source : RoutePick::Destination;
                polygonTool = false;
                drag = polygonDrag = edgeStart = -1;
                status = sourceClicked ? "Click a point on the canvas to set SOURCE. Esc cancels."
                                       : "Click a point on the canvas to set DESTINATION. Esc "
                                         "cancels.";
            }
            y += 46;
            y = wrap("Source #" + std::to_string(scene.source()) + "  |  Destination #" +
                         std::to_string(scene.goal()),
                     x, y, contentWidth, 16, ACCENT);
        }
        y += 16;
        text("INSPECT / EXPLAIN", x, y, 20, ACCENT);
        y += 34;
        y = wrap("Mode: " + modeName(mode), x, y, contentWidth);
        y = wrap("Points " + std::to_string(scene.points().size()) + " | radius " +
                     number(scene.radius()) + " | MinPts " + std::to_string(scene.minPts()),
                 x, y + 8, contentWidth, 15, MUTED);
        if (polygonTool)
            y = wrap("Polygon tool ON. Press P to return to points.", x, y + 8, contentWidth, 15,
                     FOCUS);
        if (run) {
            auto &r = run->result();
            y = wrap(r.summary(), x, y + 14, contentWidth, 18);
            y = wrap(r.check(), x, y + 8, contentWidth, 15, r.checked() ? ACCENT : DANGER);
            y = wrap("Operations " + std::to_string(r.stats().operations()) + " | queries " +
                         std::to_string(r.stats().queries()) + " | expanded " +
                         std::to_string(r.stats().expanded()),
                     x, y + 8, contentWidth, 15, MUTED);
            y = wrap("Step " + std::to_string(playback.cursor() + 1) + " / " +
                         std::to_string(r.trace().frames().size()) +
                         (r.trace().truncated() ? " (trace capped; final frame retained)" : ""),
                     x, y + 8, contentWidth, 16);
            if (auto f = frame()) {
                y = wrap(f->explanation(), x, y + 12, contentWidth, 17, FOCUS);
                if (!f->queue().empty()) {
                    std::string q = "Queue:";
                    for (std::size_t i = 0; i < std::min<std::size_t>(f->queue().size(), 18); ++i)
                        q += " " + std::to_string(f->queue()[i]);
                    y = wrap(q, x, y + 8, contentWidth, 15);
                }
                if (selected >= 0 && selected < static_cast<int>(f->costs().size())) {
                    y = wrap("Selected g=" + number(f->costs()[selected]) +
                                 " f=" + number(f->scores()[selected]) +
                                 " predecessor=" + std::to_string(f->parents()[selected]),
                             x, y + 8, contentWidth, 15);
                }
            }
        } else
            y = wrap("No current trace. Press Enter or Run.", x, y + 16, contentWidth, 17, FOCUS);
        if (selectedEdge >= 0 && selectedEdge < static_cast<int>(scene.edges().size())) {
            auto e = scene.edges()[selectedEdge];
            y = wrap("Selected edge " + std::to_string(e.a()) + " - " + std::to_string(e.b()) +
                         " cost=" + number(e.weight()) + (e.blocked() ? " BLOCKED" : "") +
                         ". B toggles; Settings edits cost.",
                     x, y + 12, contentWidth, 15);
        }
        if (selected >= 0 && selected < static_cast<int>(scene.points().size())) {
            auto p = scene.points()[selected];
            y = wrap("Selected #" + std::to_string(selected) + " (" + number(p.x()) + ", " +
                         number(p.y()) + ")",
                     x, y + 16, contentWidth, 16, ACCENT);
            if (auto current = frame(); current &&
                                        (mode == Mode::DBSCAN || mode == Mode::IndexedDBSCAN) &&
                                        selected < static_cast<int>(current->labels().size())) {
                int label = current->labels()[selected], kind = current->kind()[selected];
                y = wrap("Current label: " + (label == -2 ? std::string("unassigned")
                                              : label == -1
                                                  ? std::string("NOISE")
                                                  : "cluster " + std::to_string(label) +
                                                        (kind == 2 ? " CORE" : " BORDER")),
                         x, y + 6, contentWidth, 16);
            }
            if (securityView && selected < static_cast<int>(observations.size())) {
                auto &o = observations[selected];
                y = wrap("Identity: " + o.name(), x, y + 6, contentWidth, 16);
                if (feature())
                    y = wrap("Raw request/min=" + number(o.requestRate()) +
                                 " failed/min=" + number(o.failureRate()),
                             x, y + 5, contentWidth, 15);
                Stats stats;
                KDTree tree(scene.points());
                auto ns = tree.radius(p, scene.radius(), stats);
                std::string neighbours = "Neighbours:";
                for (int id : ns)
                    neighbours += " " + std::to_string(id);
                y = wrap(neighbours, x, y + 8, contentWidth, 15);
                if (securityView == 1 && !scene.polygon().empty()) {
                    int m = GeometryKernel::membership(p, scene.polygon());
                    y = wrap(m == 1   ? "Geofence: INSIDE"
                             : m == 0 ? "Geofence: BOUNDARY"
                                      : "Geofence: OUTSIDE",
                             x, y + 5, contentWidth, 15);
                }
                y = wrap("Source records: " + std::to_string(o.events().size()) +
                             ". Scroll panel to inspect all.",
                         x, y + 8, contentWidth, 15, MUTED);
                for (int id : o.events()) {
                    auto &e = security.events()[id];
                    y = wrap(e.id() + " | " + e.user() + " | " + e.device() + " | UTC " +
                                 std::to_string(e.timestamp()),
                             x, y + 10, contentWidth, 14);
                    y = wrap("requests=" + std::to_string(e.requests()) + " failures=" +
                                 std::to_string(e.failures()) + " truth=" + e.label(),
                             x, y + 3, contentWidth, 14);
                    y = wrap(e.summary(), x, y + 3, contentWidth, 15);
                    y = wrap("Review: " + (security.reviews().count(e.id())
                                               ? security.reviews().at(e.id())
                                               : "unreviewed"),
                             x, y + 3, contentWidth, 14, ACCENT);
                }
            }
        }
        if (previous)
            y = wrap("Previous retained: " + modeName(previous->result().mode()) + " value " +
                         number(previous->result().value()),
                     x, y + 16, contentWidth, 15, MUTED);
        if (securityView) {
            try {
                if (evaluationDirty) {
                    evaluationCache = security.evaluation();
                    evaluationDirty = false;
                }
                y = wrap(evaluationCache, x, y + 16, contentWidth, 14, MUTED);
            } catch (const std::exception &e) {
                y = wrap(std::string("Fix the polygon/settings before evaluation: ") + e.what(), x,
                         y + 16, contentWidth, 14, FOCUS);
            }
        }
        y += 24;
        text("HOW TO USE", x, y, 20, ACCENT);
        y += 34;
        int step = 1;
        for (const auto &instruction : AlgorithmHelp::steps(mode, securityView))
            y = wrap(std::to_string(step++) + ". " + instruction, x, y, contentWidth, 16) + 8;
        y = wrap("Scroll this panel. F1 opens all controls.", x, y + 16, contentWidth, 14, MUTED);
        sidebarScroll = std::min(
            sidebarScroll, std::max(0.0, static_cast<double>(y + sidebarScroll - bottom + 24)));
        EndScissorMode();
        text("Scroll for results and details", x, bottom + 6, 14, MUTED);
    }
    void settingsDialog() {
        if (settings.draw(pressedKeys))
            applySettings();
    }
    // Scrollable control reference; keyboard and pointer input are blocked behind the overlay.
    void helpDialog() {
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
        const char *lines[] = {"1. Select Lab, Security map or Security features. Choose an "
                               "algorithm in the two top rows.",
                               "2. Sample (N) cycles six laboratory inputs or nine security "
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
                               "9. S saves output/session.scene or session.security. L reloads it. "
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
                               "18. Noise/outside labels suggest review. They are not attack "
                               "verdicts. Included data/map are synthetic."};
        for (auto s : lines)
            y = wrap(s, x, y, 1010, 17) + 11;
        helpScroll = std::min(helpScroll,
                              std::max(0.0, static_cast<double>(y + helpScroll - clipBottom + 16)));
        EndScissorMode();
        if (button({x, bottom - 52, 180, 38}, "Close help"))
            help = false;
    }
    // Render one logical-resolution UI frame through the RAII drawing session.
    void draw() {
        DrawingSession drawing;
        ClearBackground(BG);
        float w = Ui::width(), h = Ui::height();
        text("GEOVIZ", 16, 13, 25, ACCENT);
        text("Geometry + security analysis", 145, 18, 16, MUTED);
        if (button({static_cast<float>(w - 515), 8, 85, 30}, "Lab", !securityView) && !help &&
            !settings.isOpen())
            setView(0);
        if (button({static_cast<float>(w - 424), 8, 147, 30}, "Security map", securityView == 1) &&
            !help && !settings.isOpen())
            setView(1);
        if (button({static_cast<float>(w - 271), 8, 175, 30}, "Security features",
                   securityView == 2) &&
            !help && !settings.isOpen())
            setView(2);
        if (button({static_cast<float>(w - 90), 8, 76, 30}, "Help F1"))
            help = !help;
        float bw = (w - 32) / 5.0f;
        for (int i = 0; i < 10; ++i) {
            if (button({16 + (i % 5) * bw, 48.0f + (i / 5) * 38, bw - 7, 32},
                       std::to_string(i + 1) + " " + modeName(static_cast<Mode>(i)),
                       mode == static_cast<Mode>(i)) &&
                !help && !settings.isOpen()) {
                routePick = RoutePick::None;
                mode = static_cast<Mode>(i);
                invalidate();
                sidebarScroll = 0;
            }
        }
        canvas();
        sidebar();
        float y = h - 160;
        float bx = 16;
        auto action = [&](const std::string &label, float width, const std::function<void()> &fn) {
            width = std::max(width, Ui::measure(label.c_str(), 17) + 20);
            bool clicked = button({bx, y, width, 40}, label);
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
                while (playback.cursor() + 1 < run->result().trace().frames().size())
                    playback.step(1, run->result().trace().frames().size());
        });
        action("Settings", 98, [&] { openSettings(); });
        action("Compare", 98, [&] { compare(); });
        action("Prior", 62, [&] { comparePrevious(); });
        action("Fit", 54, [&] { view.fit(scene, feature()); });
        y += 48;
        bx = 16;
        action("Sample", 86, [&] { nextSample(); });
        action("Grid", 60, [&] {
            routePick = RoutePick::None;
            securityView = 0;
            mode = Mode::AStar;
            scene = gridScene();
            selected = -1;
            selectedEdge = -1;
            invalidate();
            view.fit(scene);
        });
        action("Clear", 66, [&] {
            routePick = RoutePick::None;
            if (securityView)
                throw std::runtime_error(
                    "Use filters or import another dataset in security views.");
            scene = Scene{};
            selected = -1;
            selectedEdge = -1;
            edgeStart = -1;
            invalidate();
        });
        action("Polygon P", 106, [&] {
            routePick = RoutePick::None;
            if (!feature())
                polygonTool = !polygonTool;
        });
        action("Clear polygon", 140, [&] {
            scene.setPolygon({});
            invalidate();
            syncFence();
        });
        action("Save", 64, [&] { save(); });
        action("Load", 64, [&] {
            loadFile((root / (securityView ? "output/session.security" : "output/session.scene"))
                         .string());
        });
        action("Export", 76, [&] { exportEvidence(); });
        if (securityView) {
            action("Benign", 75, [&] { review("benign"); });
            action("Suspect", 85, [&] { review("suspicious"); });
        }
        DrawRectangle(0, h - 64, w, 64, SOFT_BG);
        Ui::clip(0, h - 64, w, 64);
        wrap(status, 16, h - 57, w - 30, 15);
        EndScissorMode();
        settingsDialog();
        helpDialog();
    }

    // Exercise model/UI coordination through the same handlers used by interactive controls.
    void smokeInteractions() {
        auto expect = [](bool yes, const std::string &message) {
            if (!yes)
                throw std::runtime_error("App smoke regression: " + message);
        };
        auto probe = view.screen({123.5, -42.75});
        auto restored = view.world(probe);
        expect(GeometryKernel::distance(restored, {123.5, -42.75}) < 0.001,
               "logical canvas/world round trip");
        expect(std::abs(Ui::width() * Ui::scale() - Ui::pixelWidth()) < 0.01f,
               "logical/native width round trip");
        mode = Mode::Dijkstra;
        scene = sampleScene(mode);
        routePick = RoutePick::Source;
        expect(pickEndpoint(1) && scene.source() == 1, "source picker");
        routePick = RoutePick::Destination;
        expect(pickEndpoint(2) && scene.goal() == 2, "destination picker");
        expect(!pickEndpoint(-1), "invalid picker target");
        const int startingVariant = sampleVariant;
        for (int i = 0; i < SampleVariantCount; ++i) {
            nextSample();
            execute();
            expect(run && run->result().checked(), "sample cycle verified input");
        }
        expect(sampleVariant == startingVariant, "sample cycle wraps after all six inputs");
        const double radiusBefore = scene.radius();
        openSettings();
        settings.setField(0, "-1");
        bool rejectedSettings = false;
        try {
            applySettings();
        } catch (const std::exception &) {
            rejectedSettings = true;
        }
        expect(rejectedSettings && scene.radius() == radiusBefore && settings.isOpen(),
               "failed settings transaction preserves live scene and edit buffer");
        settings.close();
        mode = Mode::Hull;
        scene = sampleScene(mode);
        execute();
        double original = run->input().points()[0].x();
        scene.movePoint(0, {scene.points()[0].x() + 10, scene.points()[0].y()});
        invalidate();
        expect(!run && previous && previous->input().points()[0].x() == original,
               "edit invalidation/frozen previous input");
        execute();
        comparePrevious();
        expect(run && previous && run->input().points()[0].x() != previous->input().points()[0].x(),
               "prior/current snapshots");
        setView(2);
        openSettings();
        settings.setField(8, "device-00");
        applySettings();
        expect(scene.points().size() == 1 && observations[0].name() == "device-00",
               "settings/filter integration");
        execute();
        expect(run && run->result().checked(), "single-device security execution");
        openSettings();
        settings.setField(8, "*");
        applySettings();
        selected = 10;
        review("benign");
        security.settings().setFeatureQuery({1.25, 0.5});
        refreshSecurity();
        save();
        exportEvidence();
        auto saved = SecurityConsole::load((root / "output/session.security").string());
        expect(saved.reviews().at("event-10") == "benign" &&
                   saved.settings().featureQuery().x() == 1.25,
               "reviews/query persistence");
        loadFile((root / "data/security_sample.csv").string());
        expect(security.reviews().empty(), "new CSV clears source-linked reviews");
        for (int targetView : {1, 2}) {
            securityView = targetView;
            const int startingSample = securitySample;
            for (std::size_t i = 0; i < securityCatalogue.size(); ++i) {
                nextSample();
                execute();
                expect(run && run->result().checked() && security.reviews().empty() &&
                           security.settings().user() == "*" && securityView == targetView,
                       "security catalogue cycle resets context and verifies both views");
            }
            expect(securitySample == startingSample, "security catalogue cycle wraps");
        }
        security.setSettings(SecuritySettings{});
        security.setFence({{-300, -250}, {350, -250}, {350, 300}, {-300, 300}});
        securityView = 0;
        mode = Mode::Hull;
        scene = sampleScene(mode);
        run.reset();
        previous.reset();
        playback.reset();
        selected = -1;
        view.fit(scene);
        std::ofstream(root / "output/APP_SMOKE_REPORT.txt")
            << "PASS: application run/edit invalidation, frozen history, prior "
               "exports, "
               "six-sample cycle, rejected settings transaction, settings/device filter, source "
               "review, query/snapshot/evidence "
               "persistence, CSV "
               "review reset. This covers programmatic application state, not "
               "exhaustive physical "
               "mouse/keyboard testing.\n";
    }

  public:
    explicit App(std::filesystem::path base, bool smokeMode, int width, int height)
        : root(std::move(base)), smoke(smokeMode), windowWidth(width), windowHeight(height) {
        std::filesystem::create_directories(root / "output");
        securityCatalogue = securitySamples((root / "data").string());
        security = loadSecuritySample((root / "data").string(), securityCatalogue.front());
    }
    // Keep the window and fonts alive around the loop; exceptions release graphics through RAII.
    void loop() {
        // Keep automated evidence paths inside the selected test directory.
        if (smoke)
            std::filesystem::current_path(root);
        Window window(smoke, smoke ? 960 : windowWidth, smoke ? 640 : windowHeight);
        Image icon = LoadImage((root / "assets/geoviz.png").string().c_str());
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
        while (!WindowShouldClose()) {
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
                            security = loadSecuritySample((root / "data").string(),
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
                            security = loadSecuritySample((root / "data").string(),
                                                          securityCatalogue.front());
                            security.settings().setDevice("no-such-device");
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
                        } else
                            break;
                    }
                } else
                    input();
                draw();
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
        if (smoke) {
            const int expectedStates = 52 + static_cast<int>(securityCatalogue.size()) * 2;
            if (smokeFrame < expectedStates * 5)
                throw std::runtime_error(
                    "Visual smoke run interrupted before all states completed.");
            std::ofstream(root / "output/APP_SMOKE_REPORT.txt", std::ios::app)
                << "PASS: rendered and captured " << expectedStates
                << " states: 10 algorithm modes, 2 "
                   "security views, "
                   "settings/help, intermediate hull/DBSCAN/tree-build and a pruned "
                   "region, ten varied gallery inputs, both extended security datasets, "
                   "all twenty tidal/terrace gallery inputs and both views of all "
                << securityCatalogue.size()
                << " security scenarios, plus both empty filtered views.\n"
                << "Offscreen render " << Ui::pixelWidth() << 'x' << Ui::pixelHeight()
                << "; logical " << Ui::width() << 'x' << Ui::height() << "; UI scale "
                << Ui::scale() << "; body font pixels " << Ui::fontSize(17) * Ui::scale() << "\n"
                << "PASS: source/destination pickers and logical/world/native "
                   "transforms.\n";
        }
    }
};
} // namespace
int main(int argc, char **argv) {
    try {
        std::filesystem::path root = std::filesystem::absolute(GetApplicationDirectory());
        if (!std::filesystem::exists(root / "data/security_sample.csv"))
            root = root.parent_path();
        bool smoke = false;
        int width = 1440, height = 900;
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--smoke")
                smoke = true;
            else if (arg == "--smoke-size" && i + 2 < argc) {
                width = std::stoi(argv[++i]);
                height = std::stoi(argv[++i]);
                if (width < 960 || height < 640 || width > 7680 || height > 4320)
                    throw std::runtime_error("Smoke dimensions must be 960x640 to 7680x4320.");
            } else if (arg == "--root" && i + 1 < argc)
                root = argv[++i];
            else
                throw std::runtime_error("Usage: geoviz [--root PROJECT_DIRECTORY] "
                                         "[--smoke] [--smoke-size W H]");
        }
        SetTraceLogLevel(LOG_WARNING);
        App app(std::filesystem::absolute(root), smoke, width, height);
        app.loop();
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "GeoViz: " << e.what() << '\n';
        return 1;
    }
}
