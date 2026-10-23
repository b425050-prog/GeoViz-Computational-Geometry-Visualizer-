#include "geoviz.hpp"

namespace gv {
using G = GeometryKernel;
Scene gridScene() {
    Scene s;
    s.setGrid(true);
    for (int y = 0; y < 7; ++y)
        for (int x = 0; x < 10; ++x)
            s.addPoint({80.0 + 60 * x, 70.0 + 60 * y});
    for (int y = 0; y < 7; ++y)
        for (int x = 0; x < 10; ++x) {
            int a = y * 10 + x;
            if (x < 9)
                s.addEdge({a, a + 1, 1, x == 4 && y != 5});
            if (y < 6)
                s.addEdge({a, a + 10, 1, false});
        }
    s.setGoal(69);
    return s;
}
static Scene referenceScene(Mode mode) {
    Scene s;
    s.setPoints({{100, 100},
                 {180, 80},
                 {260, 140},
                 {330, 90},
                 {430, 150},
                 {540, 90},
                 {600, 230},
                 {470, 320},
                 {300, 340},
                 {120, 290},
                 {220, 220},
                 {350, 240}});
    s.setPolygon({{100, 90}, {570, 90}, {610, 320}, {330, 420}, {90, 330}});
    s.setQuery({300, 220});
    s.setGoal(11);
    if (mode == Mode::Dijkstra || mode == Mode::AStar) {
        std::vector<std::pair<int, int>> es = {{0, 1},  {1, 2},   {2, 3},  {3, 4},  {4, 5},
                                               {5, 6},  {6, 7},   {7, 8},  {8, 9},  {9, 0},
                                               {2, 10}, {10, 11}, {11, 7}, {3, 11}, {9, 10}};
        for (auto [a, b] : es)
            s.addEdge({a, b, G::distance(s.points()[a], s.points()[b]), false});
    }
    if (mode == Mode::DBSCAN || mode == Mode::IndexedDBSCAN) {
        s.setPoints({});
        for (int g = 0; g < 2; ++g)
            for (int i = 0; i < 12; ++i)
                s.addPoint({150.0 + g * 300 + (i % 4) * 25, 160.0 + (i / 4) * 25});
        s.addPoint({380, 400});
        s.addPoint({620, 380});
        s.setRadius(45);
        s.setMinPts(4);
    }
    if (mode == Mode::Collision)
        s.setPoints({{100, 100},
                     {450, 350},
                     {100, 350},
                     {450, 100},
                     {500, 100},
                     {600, 100},
                     {550, 100},
                     {650, 100}});
    return s;
}
// Two additional families keep their own construction path so the original
// four inputs and their saved traces remain reproducible.
static Scene landscapeScene(Mode mode, int variant) {
    const bool tidal = variant == 4;
    constexpr double pi = 3.141592653589793;
    Scene s;
    s.setQuery({350, 240});
    s.setRadius(72);
    s.setMinPts(4);
    if (tidal)
        s.setPolygon({{70, 70},
                      {640, 70},
                      {640, 160},
                      {410, 160},
                      {410, 320},
                      {640, 320},
                      {640, 430},
                      {70, 430}});
    else
        s.setPolygon({{80, 70},
                      {620, 70},
                      {620, 170},
                      {460, 170},
                      {460, 270},
                      {300, 270},
                      {300, 420},
                      {80, 420}});

    if (mode == Mode::Dijkstra || mode == Mode::AStar) {
        auto connect = [&](int a, int b, double multiplier, bool blocked = false) {
            s.addEdge({a, b, G::distance(s.points()[a], s.points()[b]) * multiplier, blocked});
        };
        if (tidal) {
            // Three islands have inexpensive local paths. The direct northern
            // bridge is closed, and middle bridges charge a large crossing cost.
            const double columns[] = {70, 150, 285, 365, 500, 580};
            for (int y = 0; y < 3; ++y)
                for (double x : columns)
                    s.addPoint({x, 100.0 + y * 140});
            for (int y = 0; y < 3; ++y)
                for (int x = 0; x < 6; ++x) {
                    int a = y * 6 + x;
                    if (y < 2)
                        connect(a, a + 6, 1);
                    if (x < 5) {
                        const bool bridge = x == 1 || x == 3;
                        connect(a, a + 1, bridge && y == 1 ? 4.0 : 1.0,
                                (x == 1 && y == 0) || (x == 3 && y == 2));
                    }
                }
            s.setGoal(5);
        } else {
            // Offset terraces and two diagonal ramps contrast with the regular
            // island bridges. Gates force a change of level along the route.
            for (int y = 0; y < 4; ++y)
                for (int x = 0; x < 5; ++x)
                    s.addPoint({75.0 + x * 125 + (y % 2) * 18, 70.0 + y * 110 + x * 5});
            for (int y = 0; y < 4; ++y)
                for (int x = 0; x < 5; ++x) {
                    int a = y * 5 + x;
                    if (y < 3)
                        connect(a, a + 5, 1.0 + (x % 2) * 0.12);
                    if (x < 4)
                        connect(a, a + 1, 1.0 + ((x + y) % 3) * 0.08,
                                (x == 1 && y != 2) || (x == 3 && y != 0));
                }
            connect(1, 7, 1.18);
            connect(12, 18, 1.18);
            s.setSource(15);
            s.setGoal(4);
        }
    } else if (mode == Mode::DBSCAN || mode == Mode::IndexedDBSCAN) {
        if (tidal) {
            // Hollow rings are density connected despite having empty centres.
            for (int ring = 0; ring < 2; ++ring) {
                const int count = ring == 0 ? 40 : 24;
                const double radius = ring == 0 ? 105 : 65;
                for (int i = 0; i < count; ++i) {
                    double angle = 2 * pi * i / count;
                    s.addPoint({(ring == 0 ? 220.0 : 530.0) + radius * std::cos(angle),
                                240 + radius * std::sin(angle)});
                }
            }
            for (Point p : std::vector<Point>{{350, 70}, {350, 420}, {650, 370}})
                s.addPoint(p);
            s.setRadius(35);
            s.setMinPts(4);
        } else {
            const Point centres[] = {{140, 130}, {360, 130}, {560, 300}, {250, 345}};
            for (Point centre : centres)
                for (int i = 0; i < 18; ++i) {
                    double angle = i * 2.3999632297;
                    double radius = 9 * std::sqrt(i + 1.0);
                    s.addPoint({centre.x() + radius * std::cos(angle),
                                centre.y() + radius * std::sin(angle)});
                }
            for (Point p : std::vector<Point>{{45, 425}, {650, 65}, {430, 400}, {50, 240}})
                s.addPoint(p);
            s.setRadius(29);
            s.setMinPts(4);
        }
    } else if (mode == Mode::Collision) {
        if (tidal)
            // Six intersecting pairs include proper crossings, shared ends and overlap.
            s.setPoints({{80, 160},
                         {300, 160},
                         {300, 160},
                         {390, 250},
                         {150, 160},
                         {240, 160},
                         {190, 80},
                         {190, 310},
                         {390, 250},
                         {610, 250},
                         {510, 80},
                         {510, 390},
                         {85, 360},
                         {320, 360},
                         {570, 340},
                         {640, 410}});
        else
            // A square tile with diagonals has thirteen intersecting pairs;
            // the separate point-on-segment contact contributes the fourteenth.
            s.setPoints({{80, 80},
                         {320, 80},
                         {320, 80},
                         {320, 320},
                         {320, 320},
                         {80, 320},
                         {80, 320},
                         {80, 80},
                         {80, 80},
                         {320, 320},
                         {80, 320},
                         {320, 80},
                         {430, 240},
                         {600, 240},
                         {515, 240},
                         {515, 240}});
    } else if (mode == Mode::KDNearest || mode == Mode::KDRadius) {
        // Four equidistant points lie exactly on the radius. Lowest input ID
        // resolves the nearest tie; the fifth point is just outside the boundary.
        const double radius = tidal ? 72 : 60;
        s.setRadius(radius);
        s.setPoints({{350 + radius, 240},
                     {350, 240 + radius},
                     {350 - radius, 240},
                     {350, 240 - radius},
                     {350 + radius + 0.25, 240}});
        if (tidal) {
            for (int i = 0; i < 36; ++i) {
                double angle = i * 2 * pi / 36;
                double reach = 150 + 20 * std::sin(angle * 3);
                s.addPoint({350 + reach * 1.55 * std::cos(angle), 240 + reach * std::sin(angle)});
            }
        } else {
            for (int y = 0; y < 5; ++y)
                for (int x = 0; x < 7; ++x) {
                    Point p{80.0 + x * 85 + (y % 2) * 18, 75.0 + y * 82};
                    if (G::distance(p, s.query()) > radius + 30)
                        s.addPoint(p);
                }
        }
    } else {
        if (tidal) {
            const Point centres[] = {{150, 150}, {350, 345}, {555, 155}};
            for (Point centre : centres)
                for (int i = 0; i < 16; ++i) {
                    double angle = i * 2.3999632297;
                    double radius = 15 * std::sqrt(i + 1.0);
                    s.addPoint({centre.x() + radius * std::cos(angle),
                                centre.y() + radius * 0.75 * std::sin(angle)});
                }
        } else {
            for (int y = 0; y < 5; ++y)
                for (int x = 0; x < 7; ++x)
                    s.addPoint(
                        {80.0 + x * 85 + (y % 2) * 20, 75.0 + y * 78 + 8 * std::sin(x * 0.9)});
        }
        if (mode == Mode::ClosestPair) {
            // A distinctive 3-4-5 pair sits between the larger landscape groups.
            s.addPoint({345, 220});
            s.addPoint({348, 224});
        } else if (mode == Mode::Geofence)
            s.setQuery(tidal ? Point{410, 240} : Point{380, 220});
    }
    return s;
}

// Names describe the input's purpose, rather than promising a particular result.
std::string sampleName(Mode mode, int variant) {
    if (variant < 0 || variant >= SampleVariantCount)
        throw std::invalid_argument("Unknown sample variant.");
    if (variant == 0)
        return "Reference scene";
    if (variant >= 4) {
        if (mode == Mode::Dijkstra || mode == Mode::AStar)
            return variant == 4 ? "Island bridges" : "Terrace switchbacks";
        if (mode == Mode::DBSCAN || mode == Mode::IndexedDBSCAN)
            return variant == 4 ? "Lagoon rings" : "Garden islands";
        if (mode == Mode::Geofence)
            return variant == 4 ? "Lagoon boundary" : "Terrace courtyard";
        if (mode == Mode::Collision)
            return variant == 4 ? "Tidal boardwalks" : "Mosaic joints";
        return variant == 4 ? "Tidal archipelago" : "Botanical terraces";
    }
    if (mode == Mode::Dijkstra || mode == Mode::AStar) {
        static const char *names[] = {"", "Campus deliveries", "Warehouse aisles", "Valley detour"};
        return names[std::clamp(variant, 1, 3)];
    }
    if (mode == Mode::DBSCAN || mode == Mode::IndexedDBSCAN) {
        static const char *names[] = {"", "Festival courtyards", "Curved sensor swarms",
                                      "Mixed-density survey"};
        return names[std::clamp(variant, 1, 3)];
    }
    if (mode == Mode::Geofence) {
        static const char *names[] = {"", "Campus courtyard", "Harbour perimeter",
                                      "Concave research zone"};
        return names[std::clamp(variant, 1, 3)];
    }
    if (mode == Mode::Collision) {
        static const char *names[] = {"", "Crossing corridors", "Touch and overlap",
                                      "Laser barriers"};
        return names[std::clamp(variant, 1, 3)];
    }
    static const char *names[] = {"", "Coastal survey", "Orbital sensors", "Orchard inspection"};
    return names[std::clamp(variant, 1, 3)];
}

Scene sampleScene(Mode mode, int variant) {
    if (variant < 0 || variant >= SampleVariantCount)
        throw std::invalid_argument("Sample variant must be between 0 and 5.");
    if (variant == 0)
        return referenceScene(mode);
    if (variant >= 4)
        return landscapeScene(mode, variant);
    Scene s;
    s.setQuery({325, 240});
    s.setRadius(82);
    s.setMinPts(4);
    // Explicit polygons include concavities; vertices remain in boundary order.
    if (variant == 1)
        s.setPolygon({{55, 65},
                      {650, 65},
                      {650, 415},
                      {430, 415},
                      {430, 315},
                      {270, 315},
                      {270, 415},
                      {55, 415}});
    else if (variant == 2)
        s.setPolygon({{60, 80},
                      {610, 70},
                      {665, 245},
                      {560, 420},
                      {390, 390},
                      {400, 220},
                      {260, 220},
                      {250, 410},
                      {80, 350}});
    else
        s.setPolygon({{60, 60},
                      {630, 60},
                      {630, 160},
                      {480, 160},
                      {480, 280},
                      {640, 280},
                      {640, 430},
                      {70, 430},
                      {70, 300},
                      {220, 300},
                      {220, 180},
                      {60, 180}});
    if (mode == Mode::Dijkstra || mode == Mode::AStar) {
        // Irregular junctions and closed links force meaningful detours. Every
        // open weight is at least its Euclidean length, so A* is admissible.
        const int columns = 6, rows = 4;
        for (int y = 0; y < rows; ++y)
            for (int x = 0; x < columns; ++x)
                s.addPoint({65.0 + x * 112 + (variant == 3 ? 17 * std::sin(y * 1.7) : 0),
                            65.0 + y * 113 + (variant == 1 ? 12 * std::sin(x * 1.4) : 0)});
        auto connect = [&](int a, int b, bool blocked) {
            const double length = G::distance(s.points()[a], s.points()[b]);
            s.addEdge({a, b, length * (1.0 + ((a + b + variant) % 3) * 0.12), blocked});
        };
        for (int y = 0; y < rows; ++y)
            for (int x = 0; x < columns; ++x) {
                int a = y * columns + x;
                if (x + 1 < columns)
                    connect(a, a + 1, x == 2 && y != variant % rows);
                if (y + 1 < rows)
                    connect(a, a + columns, variant == 2 && x == 4 && y == 1);
            }
        s.setGoal(columns * rows - 1);
    } else if (mode == Mode::DBSCAN || mode == Mode::IndexedDBSCAN) {
        if (variant == 2) {
            // Two curved, density-connected swarms challenge centroid intuition.
            for (int g = 0; g < 2; ++g)
                for (int i = 0; i < 22; ++i) {
                    double angle = i * 3.141592653589793 / 21;
                    s.addPoint({80 + g * 320 + 225 * i / 21.0,
                                130 + g * 105 + (g ? -1 : 1) * 95 * std::sin(angle)});
                }
            s.setRadius(35);
            s.setMinPts(3);
        } else {
            for (int g = 0; g < 3; ++g)
                for (int i = 0; i < 14 + g * 2; ++i) {
                    double angle = i * 2.3999632297;
                    double r = (variant == 3 && g == 2 ? 24 : 13) * std::sqrt(i + 1.0);
                    s.addPoint({130 + g * 205 + r * std::cos(angle),
                                180 + (g % 2) * 120 + r * std::sin(angle)});
                }
            s.setRadius(variant == 3 ? 40 : 43);
        }
        for (Point p : std::vector<Point>{{45, 405}, {360, 65}, {675, 395}, {350, 420}})
            s.addPoint(p);
    } else if (mode == Mode::Collision) {
        if (variant == 1)
            s.setPoints({{65, 80},
                         {615, 400},
                         {70, 390},
                         {620, 75},
                         {70, 245},
                         {640, 245},
                         {190, 60},
                         {190, 425},
                         {480, 70},
                         {480, 420},
                         {300, 390},
                         {390, 390}});
        else if (variant == 2)
            s.setPoints({{70, 90},
                         {300, 90},
                         {230, 90},
                         {430, 90},
                         {430, 90},
                         {600, 300},
                         {120, 320},
                         {300, 180},
                         {300, 180},
                         {440, 350},
                         {540, 370},
                         {540, 370}});
        else
            for (int i = 0; i < 8; ++i) {
                s.addPoint({65.0 + i * 75, 65});
                s.addPoint({630.0 - i * 70, 410});
            }
    } else {
        // Each point family uses a different spatial distribution. There is no
        // random state, so a saved sample always reproduces its trace.
        for (int i = 0; i < 36; ++i) {
            if (variant == 1)
                s.addPoint(
                    {65.0 + i * 16.0, 225 + 130 * std::sin(i * 0.21) + 40 * std::cos(i * 1.4)});
            else if (variant == 2) {
                double angle = i * 2.3999632297, r = 29 * std::sqrt(i + 1.0);
                s.addPoint({350 + r * 1.5 * std::cos(angle), 240 + r * std::sin(angle)});
            } else
                s.addPoint({80.0 + (i % 9) * 66 + 7 * std::sin(i * 2.1),
                            80.0 + (i / 9) * 102 + 9 * std::cos(i * 1.7)});
        }
        if (mode == Mode::ClosestPair) {
            // A close cross-strip pair makes the merge stage worth inspecting.
            s.addPoint({348, 252});
            s.addPoint({352, 255});
        }
    }
    return s;
}
} // namespace gv
