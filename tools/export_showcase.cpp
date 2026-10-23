// Export real engine traces for the offline documentation. The browser only
// replays these states; it does not contain a second algorithm implementation.
#include "geoviz.hpp"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>

namespace {
using namespace gv;
// JSON strings need explicit escaping; std::quoted alone is not JSON encoding.
void string(std::ostream &out, const std::string &s) {
    out << '"';
    for (unsigned char c : s) {
        if (c == '"' || c == '\\')
            out << '\\' << c;
        else if (c < 32)
            out << "\\u00" << "0123456789abcdef"[c / 16] << "0123456789abcdef"[c % 16];
        else
            out << c;
    }
    out << '"';
}
template <class T> void array(std::ostream &out, const std::vector<T> &values) {
    out << '[';
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i)
            out << ',';
        out << values[i];
    }
    out << ']';
}
void points(std::ostream &out, const std::vector<Point> &p) {
    out << '[';
    for (std::size_t i = 0; i < p.size(); ++i) {
        if (i)
            out << ',';
        out << '[' << p[i].x() << ',' << p[i].y() << ']';
    }
    out << ']';
}
void boxes(std::ostream &out, const std::vector<Box> &b) {
    out << '[';
    for (std::size_t i = 0; i < b.size(); ++i) {
        if (i)
            out << ',';
        out << '[' << b[i].x0() << ',' << b[i].y0() << ',' << b[i].x1() << ',' << b[i].y1() << ']';
    }
    out << ']';
}
void item(std::ostream &out, const Scene &s, Mode mode, const std::string &title, int variant = 0) {
    const Result r = Engine{}.execute(s, mode);
    if (!r.checked())
        throw std::runtime_error(r.check());
    out << "{\"title\":";
    string(out, title);
    out << ",\"variant\":" << variant << ",\"mode\":" << static_cast<int>(mode) << ",\"summary\":";
    string(out, r.summary());
    out << ",\"check\":";
    string(out, r.check());
    out << ",\"operations\":" << r.stats().operations() << ",\"expanded\":" << r.stats().expanded();
    out << ",\"points\":";
    points(out, s.points());
    out << ",\"polygon\":";
    points(out, s.polygon());
    out << ",\"query\":[" << s.query().x() << ',' << s.query().y() << "]";
    out << ",\"radius\":" << s.radius() << ",\"minPts\":" << s.minPts() << ",\"edges\":[";
    for (std::size_t i = 0; i < s.edges().size(); ++i) {
        if (i)
            out << ',';
        const auto &e = s.edges()[i];
        out << '[' << e.a() << ',' << e.b() << ',' << e.weight() << ',' << e.blocked() << ']';
    }
    out << "],\"frames\":[";
    for (std::size_t i = 0; i < r.trace().frames().size(); ++i) {
        if (i)
            out << ',';
        const auto &f = r.trace().frames()[i];
        out << "{\"text\":";
        string(out, f.explanation());
        out << ",\"focus\":" << f.focus() << ",\"radius\":" << f.radius();
        out << ",\"chain\":";
        array(out, f.chain());
        out << ",\"active\":";
        array(out, f.active());
        out << ",\"labels\":";
        array(out, f.labels());
        out << ",\"kind\":";
        array(out, f.kind());
        out << ",\"regions\":";
        boxes(out, f.regions());
        out << ",\"pruned\":";
        boxes(out, f.pruned());
        out << '}';
    }
    out << "]}";
}
} // namespace

int main(int argc, char **argv) {
    try {
        const std::filesystem::path root = argc > 1 ? argv[1] : ".";
        std::filesystem::create_directories(root / "docs/media");
        std::ofstream out(root / "docs/media/traces.js");
        if (!out)
            throw std::runtime_error("Cannot write trace data.");
        out << std::setprecision(17) << "window.GEOVIZ_TRACES=[";
        for (int m = 0; m < 10; ++m) {
            if (m)
                out << ',';
            const auto mode = static_cast<Mode>(m);
            item(out, sampleScene(mode, 1), mode, modeName(mode) + " / " + sampleName(mode, 1));
        }
        SecurityConsole sec;
        sec.setEvents(CsvLogAdapter{}.load((root / "data/security_campus.csv").string()));
        sec.setFence({{-300, -250}, {350, -250}, {350, 300}, {-300, 300}});
        for (bool feature : {false, true}) {
            out << ',';
            item(out, sec.scene(feature), Mode::IndexedDBSCAN,
                 feature ? "Security / scaled device behaviour" : "Security / campus geography");
        }
        for (const auto &[mode, variant] :
             std::vector<std::pair<Mode, int>>{{Mode::Hull, 4},
                                               {Mode::AStar, 4},
                                               {Mode::DBSCAN, 4},
                                               {Mode::Hull, 5},
                                               {Mode::KDRadius, 5},
                                               {Mode::Collision, 5}}) {
            out << ',';
            item(out, sampleScene(mode, variant), mode,
                 modeName(mode) + " / " + sampleName(mode, variant), variant);
        }
        const auto catalogue = securitySamples((root / "data").string());
        for (std::size_t i = 3; i < catalogue.size(); ++i) {
            auto sample = loadSecuritySample((root / "data").string(), catalogue[i]);
            const bool feature = i == 4 || i == 6 || i == 7;
            const Mode mode = i == 3 ? Mode::Geofence : i == 5 ? Mode::Hull : Mode::IndexedDBSCAN;
            if (i == 7) {
                sample.settings().setUser("lab, \"night\"");
                sample.settings().setStart(1800000300);
            }
            out << ',';
            item(out, sample.scene(feature), mode, "Security / " + catalogue[i].name);
        }
        out << "];\n";
        if (!out)
            throw std::runtime_error("Trace write failed.");
        std::cout << "Exported " << 18 + catalogue.size() - 3 << " verified engine recordings.\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
