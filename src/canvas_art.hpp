#pragma once
// Presentation-only artwork. These routines never modify Scene or algorithm
// coordinates; all decoration is clipped by the caller to the canvas viewport.
namespace art {
using namespace gv;

// Screen-space pattern density is bounded, even after extreme zoom/pan. The
// fixed arithmetic gives reproducible surfaces without image-file dependencies.
inline void surface(const View &v, Mode mode, int variant, bool geographic, bool feature) {
    const Rectangle a = v.bounds();
    if (feature)
        return; // Rate plots keep an untextured plotting plane for precise reading.
    const bool technical = mode == Mode::KDNearest || mode == Mode::KDRadius;
    const bool paving = mode == Mode::Dijkstra || mode == Mode::AStar || mode == Mode::Collision;
    if (!geographic && variant == 0)
        return;
    if (!geographic && variant == 4) {
        // Tidal paper: broad translucent washes, fine ripples and ivory glints.
        // Continuous open curves remain distinct from scene points and obstacles.
        DrawRectangleGradientV(static_cast<int>(a.x), static_cast<int>(a.y),
                               static_cast<int>(a.width), static_cast<int>(a.height),
                               Color{185, 225, 225, 32}, Color{244, 233, 209, 24});
        for (int band = -3; band < a.height / 34 + 4; ++band) {
            Vector2 last{}, lastHighlight{};
            for (int k = 0; k <= 80; ++k) {
                const float x = k * a.width / 80;
                const float drift =
                    17 * std::sin(x / 155 + band * 0.19f) + 7 * std::sin(x / 67 - band * 0.27f);
                const float y = a.y + band * 34.0f + drift;
                const Vector2 next{a.x + x, y};
                const Vector2 highlight{a.x + x, y + 5 + 2 * std::sin(x / 91)};
                if (k) {
                    DrawLineEx(last, next, 7, Color{116, 184, 185, 17});
                    DrawLineEx(last, next, 1, Color{108, 165, 171, 43});
                    DrawLineEx(lastHighlight, highlight, 1, Color{255, 253, 238, 150});
                }
                last = next;
                lastHighlight = highlight;
            }
        }
        // Short, staggered fibres add close-up texture without dot-like markers.
        for (int row = 0; row < a.height / 23 + 1; ++row)
            for (int col = 0; col < a.width / 43 + 1; ++col) {
                const float x = a.x + col * 43.0f + (row % 2) * 19;
                const float y = a.y + row * 23.0f + (col % 3) * 3;
                DrawLineEx({x, y}, {x + 8 + (col % 3) * 2.0f, y - 1}, 1, Color{121, 171, 174, 24});
            }
        return;
    }
    if (!geographic && variant == 5) {
        // Limestone paper: soft terraced courses, terracotta seams and a fine
        // alternating hatch. These are surface marks, never filled map regions.
        DrawRectangleGradientV(static_cast<int>(a.x), static_cast<int>(a.y),
                               static_cast<int>(a.width), static_cast<int>(a.height),
                               Color{237, 219, 190, 35}, Color{229, 235, 214, 25});
        for (int course = -3; course < a.height / 45 + 4; ++course) {
            Vector2 last{}, lastInset{};
            for (int k = 0; k <= 80; ++k) {
                const float x = k * a.width / 80;
                const float bend =
                    13 * std::sin(x / 183 + course * 0.29f) + 5 * std::cos(x / 79 + course * 0.41f);
                const float y = a.y + course * 45.0f + bend;
                const Vector2 next{a.x + x, y};
                const Vector2 inset{a.x + x, y + 9 + 2 * std::sin(x / 113)};
                if (k) {
                    DrawLineEx(last, next, 12, Color{194, 156, 118, 15});
                    DrawLineEx(last, next, 1, Color{181, 137, 106, 40});
                    DrawLineEx(lastInset, inset, 1, Color{157, 177, 137, 37});
                }
                last = next;
                lastInset = inset;
            }
        }
        for (int row = 0; row < a.height / 29 + 1; ++row)
            for (int col = 0; col < a.width / 35 + 1; ++col) {
                const float x = a.x + col * 35.0f + (row % 2) * 15;
                const float y = a.y + row * 29.0f + (col % 3) * 2;
                const float lean = (row + col) % 2 ? 5.0f : -5.0f;
                DrawLineEx({x, y}, {x + lean, y + 7}, 1, Color{182, 151, 118, 28});
                DrawLineEx({x + 4, y}, {x + 4 + lean, y + 7}, 1, Color{255, 251, 237, 120});
            }
        return;
    }
    if (paving && !geographic && variant == 2) {
        // Warehouse scenes use wide panel courses with restrained diagonal seams.
        for (int row = 0; row < a.height / 48 + 1; ++row) {
            const float y = a.y + row * 48.0f;
            DrawRectangleRec({a.x, y, a.width, 45}, Color{224, 216, 196, 72});
            for (float x = a.x - 60; x < a.x + a.width; x += 110)
                DrawLineEx({x + (row % 2) * 50, y}, {x + 30 + (row % 2) * 50, y + 45}, 1,
                           Color{164, 150, 120, 45});
        }
    } else if (paving && !geographic) {
        // Offset stone courses provide a different surface from the survey plots.
        for (int row = 0; row < a.height / 28 + 1; ++row)
            for (int col = 0; col < a.width / 58 + 2; ++col) {
                Rectangle tile{a.x + col * 58 - (row % 2) * 29.0f, a.y + row * 28.0f, 55, 25};
                DrawRectangleRounded(tile, 0.13f, 3, Color{231, 228, 215, 95});
            }
    } else if (technical) {
        // Fine survey dots leave partition and pruning outlines easy to see.
        for (float y = a.y + 8; y < a.y + a.height; y += 18)
            for (float x = a.x + 8; x < a.x + a.width; x += 18)
                DrawCircleV({x, y}, 0.7f, Color{155, 182, 190, 75});
    } else if (!geographic && variant == 2) {
        // Orbital surveys use nested ellipses; these are artwork, not tree regions.
        for (int ring = 1; ring < 16; ++ring)
            DrawEllipseLines(static_cast<int>(a.x + a.width * 0.47f),
                             static_cast<int>(a.y + a.height * 0.54f), ring * 41.0f, ring * 25.0f,
                             Color{151, 178, 194, 43});
    } else if (!geographic && variant == 3) {
        // Sparse orchard stippling differs from both contour and orbital scenes.
        for (int row = 0; row < a.height / 38 + 1; ++row)
            for (int col = 0; col < a.width / 45 + 1; ++col) {
                const float x = a.x + col * 45.0f + (row % 2) * 18;
                const float y = a.y + row * 38.0f;
                DrawCircleV({x, y}, 3, Color{167, 186, 137, 35});
                DrawLineEx({x, y + 5}, {x + 3, y - 3}, 1, Color{146, 170, 125, 45});
            }
    } else {
        // Contour bands are generated as smooth curves, not repeated bitmaps.
        for (int band = -4; band < a.height / 23 + 5; ++band) {
            Vector2 last{};
            for (int k = 0; k <= 80; ++k) {
                float x = k * a.width / 80;
                float y = band * 25.0f + 20 * std::sin(x / 105 + variant) +
                          11 * std::cos(x / 63 + band * 0.31f);
                Vector2 next{a.x + x, a.y + y};
                if (k)
                    DrawLineEx(last, next, 1, Color{168, 188, 151, 55});
                last = next;
            }
        }
    }
}

// A closed map path is an axis-aligned footprint in the supplied campus layer.
// Open paths are roads. The CSV still holds the geographic source coordinates.
inline void campus(const View &v, const std::vector<MapLine> &paths) {
    for (const auto &path : paths) {
        const auto &p = path.points();
        if (p.size() < 2)
            continue;
        const bool closed = p.size() > 3 && GeometryKernel::distance2(p.front(), p.back()) < 1e-6;
        const bool boundary = path.name().find("boundary") != std::string::npos;
        if (closed && !boundary) {
            auto a = v.screen(p[0]), b = a;
            for (auto point : p) {
                auto q = v.screen(point);
                a.x = std::min(a.x, q.x);
                a.y = std::min(a.y, q.y);
                b.x = std::max(b.x, q.x);
                b.y = std::max(b.y, q.y);
            }
            Rectangle box{a.x, a.y, b.x - a.x, b.y - a.y};
            const bool green = path.name().find("Garden") != std::string::npos ||
                               path.name().find("Sports") != std::string::npos;
            const bool water = path.name().find("Pond") != std::string::npos;
            Color fill = water   ? Color{201, 226, 230, 255}
                         : green ? Color{220, 232, 200, 255}
                                 : Color{228, 219, 203, 255};
            DrawRectangleRounded({box.x + 3, box.y + 4, box.width, box.height}, 0.08f, 4,
                                 Color{72, 87, 71, 24});
            DrawRectangleRounded(box, 0.08f, 4, fill);
            // Roof seams, planting rows and water ripples distinguish land uses.
            for (float y = box.y + 9; y < box.y + box.height - 3; y += water ? 8 : 13)
                DrawLineEx({box.x + 4, y}, {box.x + box.width - 4, y}, 1,
                           water ? Color{145, 190, 201, 120} : Color{154, 166, 139, 80});
            DrawRectangleRoundedLinesEx(box, 0.08f, 4, 1, Color{158, 174, 149, 180});
            // Small footprints are inspected by zooming; labels never shrink.
            if (box.width > Ui::measure(path.name().c_str(), 13) + 10 && box.height > 27)
                text(path.name(), box.x + 5, box.y + 4, 13, MUTED);
        } else {
            for (std::size_t i = 1; i < p.size(); ++i) {
                line(v, p[i - 1], p[i], boundary ? MAP_INK : Color{204, 200, 180, 255},
                     boundary ? 1.5f : 14.0f, boundary);
                if (!boundary)
                    line(v, p[i - 1], p[i], Color{249, 247, 235, 255}, 10);
            }
        }
    }
    // The scale bar represents a distance in world metres and follows zoom.
    const double metres = std::pow(10.0, std::floor(std::log10(100 / v.magnification())));
    const float pixels = static_cast<float>(metres * v.magnification());
    const float x = v.bounds().x + 18, y = v.bounds().y + v.bounds().height - 45;
    DrawLineEx({x, y}, {x + pixels, y}, 3, INK);
    DrawLineEx({x, y - 4}, {x, y + 4}, 2, INK);
    DrawLineEx({x + pixels, y - 4}, {x + pixels, y + 4}, 2, INK);
    text(number(metres, 0) + " m", x, y - 24, 13, MUTED);
    const float nx = v.bounds().x + v.bounds().width - 30, ny = v.bounds().y + 95;
    DrawTriangle({nx, ny}, {nx - 6, ny + 18}, {nx + 6, ny + 18}, ACCENT);
    text("N", nx - 6, ny - 25, 14, INK);
}
} // namespace art
