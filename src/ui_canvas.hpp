#pragma once

// Immutable input for one canvas frame. Rendering can inspect model snapshots,
// but cannot edit a scene, change selection or advance algorithm playback.
struct CanvasContext {
    const Scene &scene;
    const View &view;
    const Frame *frame;
    Mode mode;
    int sampleVariant;
    int securityView;
    int selected;
    int selectedEdge;
    bool polygonTool;
    const std::vector<MapLine> &map;
    const SecuritySettings &securitySettings;
    const std::string &securityDataset;

    bool feature() const noexcept {
        return securityView == 2;
    }
};

// Layered presentation of Lab, geographic and feature scenes. Each layer has a
// single visual responsibility and shares only this frame's read-only context.
class SceneCanvas final {
    const CanvasContext context_;
    struct Extent {
        Point lo, hi;
    };

    Extent surfaceAndGrid() const {
        DrawRectangleRec(context_.view.bounds(),
                         context_.feature() ? Color{246, 250, 252, 255} : CANVAS);
        Ui::clip(static_cast<int>(context_.view.bounds().x),
                 static_cast<int>(context_.view.bounds().y),
                 static_cast<int>(context_.view.bounds().width),
                 static_cast<int>(context_.view.bounds().height));
        art::surface(context_.view, context_.mode, context_.sampleVariant,
                     context_.securityView == 1, context_.feature());
        const double target = 80 / context_.view.magnification();
        const double base = std::pow(10.0, std::floor(std::log10(target)));
        const double step = base * (target / base >= 5 ? 5 : target / base >= 2 ? 2 : 1);
        auto lo = context_.view.world({context_.view.bounds().x,
                                       context_.view.bounds().y + context_.view.bounds().height}),
             hi = context_.view.world({context_.view.bounds().x + context_.view.bounds().width,
                                       context_.view.bounds().y});
        for (double x = std::ceil(lo.x() / step) * step; x < hi.x(); x += step)
            line(context_.view, {x, lo.y()}, {x, hi.y()}, GRID, 1);
        for (double y = std::ceil(lo.y() / step) * step; y < hi.y(); y += step)
            line(context_.view, {lo.x(), y}, {hi.x(), y}, GRID, 1);
        line(context_.view, {0, lo.y()}, {0, hi.y()}, AXIS, 1);
        line(context_.view, {lo.x(), 0}, {hi.x(), 0}, AXIS, 1);
        if (context_.securityView == 1)
            art::campus(context_.view, context_.map);
        return {lo, hi};
    }

    void regions(const Extent &extent) const {
        const auto *f = context_.frame;
        const auto &lo = extent.lo;
        const auto &hi = extent.hi;
        auto region = [&](Box b, Color c, bool dashed) {
            b.setX0(std::max(b.x0(), lo.x()));
            b.setX1(std::min(b.x1(), hi.x()));
            b.setY0(std::max(b.y0(), lo.y()));
            b.setY1(std::min(b.y1(), hi.y()));
            if (b.x0() > b.x1() || b.y0() > b.y1())
                return;
            line(context_.view, {b.x0(), b.y0()}, {b.x1(), b.y0()}, c, 2, dashed);
            line(context_.view, {b.x1(), b.y0()}, {b.x1(), b.y1()}, c, 2, dashed);
            line(context_.view, {b.x1(), b.y1()}, {b.x0(), b.y1()}, c, 2, dashed);
            line(context_.view, {b.x0(), b.y1()}, {b.x0(), b.y0()}, c, 2, dashed);
        };
        if (f) {
            for (auto b : f->regions())
                region(b, FOCUS, false);
            for (auto b : f->pruned())
                region(b, DANGER, true);
        }
    }

    void topology() const {
        const auto *f = context_.frame;
        bool showPolygon =
            context_.mode == Mode::Geofence || context_.polygonTool || context_.securityView == 1;
        if (showPolygon) {
            for (std::size_t i = 0; i < context_.scene.polygon().size(); ++i) {
                Point a = context_.scene.polygon()[i],
                      b = context_.scene.polygon()[(i + 1) % context_.scene.polygon().size()];
                Color c = ACCENT;
                if (f && context_.mode == Mode::Geofence && f->focus() == static_cast<int>(i))
                    c = FOCUS;
                line(context_.view, a, b, c, 3);
                auto p = context_.view.screen(a);
                DrawRectangle(static_cast<int>(p.x - 4), static_cast<int>(p.y - 4), 8, 8, c);
                text("v" + std::to_string(i), p.x + 5, p.y + 5, 13, c);
            }
        }
        for (std::size_t i = 0; i < context_.scene.edges().size(); ++i) {
            auto e = context_.scene.edges()[i];
            auto a = context_.scene.points()[e.a()], b = context_.scene.points()[e.b()];
            line(context_.view, a, b,
                 e.blocked() ? DANGER
                             : (static_cast<int>(i) == context_.selectedEdge ? FOCUS : EDGE_INK),
                 2, e.blocked());
            auto mid = context_.view.screen({(a.x() + b.x()) / 2, (a.y() + b.y()) / 2});
            if (!context_.scene.grid()) {
                std::string label = e.blocked() ? "X" : number(e.weight(), 0);
                DrawRectangleRec({mid.x + 1, mid.y, Ui::measure(label.c_str(), 12) + 5, 19},
                                 Fade(CANVAS, 0.94f));
                text(label, mid.x + 3, mid.y, 12, e.blocked() ? DANGER : MUTED);
            }
        }
        if (context_.mode == Mode::Collision)
            for (std::size_t i = 0; i + 1 < context_.scene.points().size(); i += 2)
                line(context_.view, context_.scene.points()[i], context_.scene.points()[i + 1],
                     MUTED, 3);
    }

    void traceHighlights() const {
        const auto *f = context_.frame;
        if (f) {
            for (std::size_t i = 1; i < f->chain().size(); ++i) {
                int a = f->chain()[i - 1], b = f->chain()[i];
                if (a >= 0 && b >= 0 && a < static_cast<int>(context_.scene.points().size()) &&
                    b < static_cast<int>(context_.scene.points().size()))
                    line(context_.view, context_.scene.points()[a], context_.scene.points()[b],
                         ACCENT, 4);
            }
            if (context_.mode == Mode::Collision)
                for (auto [a, b] : f->lines())
                    line(context_.view, context_.scene.points()[a], context_.scene.points()[b],
                         DANGER, 4);
            if (f->radius() > 0 && f->focus() >= 0 &&
                f->focus() < static_cast<int>(context_.scene.points().size())) {
                auto p = context_.view.screen(context_.scene.points()[f->focus()]);
                DrawCircleLinesV(p, static_cast<float>(f->radius() * context_.view.magnification()),
                                 FOCUS);
            }
        }
    }

    void pointsAndLabels() const {
        const auto *f = context_.frame;
        std::vector<Rectangle> labelBoxes;
        for (std::size_t i = 0; i < context_.scene.points().size(); ++i) {
            auto p = context_.view.screen(context_.scene.points()[i]);
            Color c = POINT_INK;
            std::string suffix;
            int pointKind = -1;
            if (f && i < f->labels().size()) {
                if (context_.mode == Mode::DBSCAN || context_.mode == Mode::IndexedDBSCAN) {
                    int label = f->labels()[i];
                    c = label >= 0 ? CLUSTERS[label % 6] : (label == -1 ? DANGER : MUTED);
                    if (label == -1) {
                        suffix = " N";
                        pointKind = 0;
                    } else if (label == -2)
                        suffix = " ?";
                    else if (i < f->kind().size()) {
                        pointKind = f->kind()[i];
                        suffix = pointKind == 2 ? " K" : " B";
                    }
                } else if (context_.mode == Mode::Dijkstra || context_.mode == Mode::AStar)
                    c = f->labels()[i] ? ACCENT : MUTED;
            }
            if (f && std::find(f->active().begin(), f->active().end(), static_cast<int>(i)) !=
                         f->active().end())
                DrawCircleLinesV(p, 11, FOCUS);
            if (pointKind == 0) {
                DrawLineEx({p.x - 5, p.y - 5}, {p.x + 5, p.y + 5}, 2, c);
                DrawLineEx({p.x - 5, p.y + 5}, {p.x + 5, p.y - 5}, 2, c);
            } else if (pointKind == 1)
                DrawRectangleLinesEx({p.x - 5, p.y - 5, 10, 10}, 2, c);
            else
                DrawCircleV(p, 6, c);
            if (static_cast<int>(i) == context_.selected)
                DrawCircleLinesV(p, 15, INK);
            if (f && f->focus() == static_cast<int>(i))
                DrawRectangleLines(static_cast<int>(p.x - 10), static_cast<int>(p.y - 10), 20, 20,
                                   FOCUS);
            // Dense inputs retain inspectable IDs without printing overlapping
            // labels at every point. Zoom or select to reveal an individual ID.
            if (context_.scene.points().size() <= 40 || static_cast<int>(i) == context_.selected ||
                (f && f->focus() == static_cast<int>(i))) {
                const std::string label = std::to_string(i) + suffix;
                const float width = Ui::measure(label.c_str(), 13) + 4;
                const Vector2 offsets[] = {{10, -22},        {10, 4},           {-width - 10, -22},
                                           {-width - 10, 4}, {-width / 2, -36}, {-width / 2, 16}};
                for (auto offset : offsets) {
                    Rectangle box{p.x + offset.x, p.y + offset.y, width, 20};
                    bool clear =
                        box.x >= context_.view.bounds().x &&
                        box.x + box.width <=
                            context_.view.bounds().x + context_.view.bounds().width &&
                        box.y > context_.view.bounds().y + (context_.securityView == 1 ? 90 : 68) &&
                        box.y + box.height <
                            context_.view.bounds().y + context_.view.bounds().height - 30;
                    for (auto other : labelBoxes)
                        clear = clear && !CheckCollisionRecs(box, other);
                    for (std::size_t j = 0; clear && j < context_.scene.points().size(); ++j)
                        if (j != i)
                            clear = !CheckCollisionCircleRec(
                                context_.view.screen(context_.scene.points()[j]), 8, box);
                    if (clear) {
                        labelBoxes.push_back(box);
                        DrawRectangleRec(box, Fade(CANVAS, 0.94f));
                        text(label, box.x + 2, box.y + 1, 13, c);
                        break;
                    }
                }
            }
            // A diamond marks observations strictly outside the permitted zone.
            // It is a geographic review cue; event truth labels are not inputs.
            if (context_.securityView == 1 && !context_.scene.polygon().empty() &&
                GeometryKernel::membership(context_.scene.points()[i], context_.scene.polygon()) <
                    0) {
                DrawLineEx({p.x, p.y - 12}, {p.x + 12, p.y}, 2, DANGER);
                DrawLineEx({p.x + 12, p.y}, {p.x, p.y + 12}, 2, DANGER);
                DrawLineEx({p.x, p.y + 12}, {p.x - 12, p.y}, 2, DANGER);
                DrawLineEx({p.x - 12, p.y}, {p.x, p.y - 12}, 2, DANGER);
            }
            if (!context_.scene.edges().empty() &&
                (static_cast<int>(i) == context_.scene.source() ||
                 static_cast<int>(i) == context_.scene.goal())) {
                bool source = static_cast<int>(i) == context_.scene.source();
                DrawCircleLinesV(p, 13, source ? ACCENT : DESTINATION);
                const char *endpoint = source ? "SOURCE" : "DESTINATION";
                const auto &area = context_.view.bounds();
                float labelX = std::clamp(p.x - 24, area.x + 4,
                                          area.x + area.width - Ui::measure(endpoint, 12) - 4);
                text(endpoint, labelX, p.y + 15, 12, source ? ACCENT : DESTINATION);
            }
        }
    }

    void query(const Extent &extent) const {
        const auto &hi = extent.hi;
        if (context_.mode == Mode::Geofence || context_.mode == Mode::KDNearest ||
            context_.mode == Mode::KDRadius) {
            auto q = context_.view.screen(context_.scene.query());
            DrawLine(static_cast<int>(q.x - 9), static_cast<int>(q.y), static_cast<int>(q.x + 9),
                     static_cast<int>(q.y), INK);
            DrawLine(static_cast<int>(q.x), static_cast<int>(q.y - 9), static_cast<int>(q.x),
                     static_cast<int>(q.y + 9), INK);
            text("QUERY", q.x + 11, q.y, 13, INK);
            if (context_.mode == Mode::Geofence)
                line(context_.view, context_.scene.query(), {hi.x(), context_.scene.query().y()},
                     FOCUS, 1, true);
            if (context_.mode == Mode::KDRadius)
                DrawCircleLinesV(
                    q, static_cast<float>(context_.scene.radius() * context_.view.magnification()),
                    ACCENT);
        }
    }

    void headerAndCoordinates() const {
        // A solid header keeps labels readable above every surface.
        const bool density = context_.mode == Mode::DBSCAN || context_.mode == Mode::IndexedDBSCAN;
        const bool tallHeader = context_.securityView != 0 || density;
        DrawRectangleRec({context_.view.bounds().x, context_.view.bounds().y,
                          context_.view.bounds().width, tallHeader ? 86.0f : 65.0f},
                         Fade(CANVAS, 0.97f));
        text(context_.feature() ? "x: requests/min / request scale     y: failures/min / "
                                  "failure scale"
             : context_.securityView == 1
                 ? "x: east metres     y: north metres | offline geographic map"
                 : "World coordinates: x right, y up",
             context_.view.bounds().x + 12, context_.view.bounds().y + 12, 15, MUTED);
        if (context_.securityView)
            text(context_.securityDataset + (context_.securityView == 1
                                                 ? " | diamonds: outside"
                                                 : " | Sample / N: next dataset"),
                 context_.view.bounds().x + 12, context_.view.bounds().y + 34, 14, MUTED);
        if (density)
            text("K: core circle | B: border square | N: noise cross | select for "
                 "cluster ID",
                 context_.view.bounds().x + 12, context_.view.bounds().y + (tallHeader ? 56 : 34),
                 14, MUTED);
        if (!context_.securityView)
            text(sampleName(context_.mode, context_.sampleVariant) + " | " +
                     std::to_string(context_.sampleVariant + 1) + "/" +
                     std::to_string(SampleVariantCount) + " | Sample / N: next input",
                 context_.view.bounds().x + 12, context_.view.bounds().y + 35, 14, MUTED);
        auto mouseWorld = context_.view.world(Ui::mouse());
        std::string coord = "x=" + number(mouseWorld.x()) + " y=" + number(mouseWorld.y());
        if (context_.securityView == 1) {
            auto geo = SecurityConsole::unproject(mouseWorld, context_.securitySettings);
            coord += " lat=" + number(geo.x(), 5) + " lon=" + number(geo.y(), 5);
        }
        text(coord, context_.view.bounds().x + 12,
             context_.view.bounds().y + context_.view.bounds().height - 24, 14, MUTED);
    }

  public:
    explicit SceneCanvas(CanvasContext context) : context_(context) {}

    void draw() const {
        const auto extent = surfaceAndGrid();
        regions(extent);
        topology();
        traceHighlights();
        pointsAndLabels();
        query(extent);
        headerAndCoordinates();
        EndScissorMode();
    }
};
