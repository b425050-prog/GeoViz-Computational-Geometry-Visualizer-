#pragma once
#include "scene.hpp"
#include "security.hpp"
#include <iomanip>
#include <sstream>
namespace gv {
// Memento pattern: input history is independent of immutable completed Runs.
struct WorkspaceState {
    Scene lab;
    SecurityConsole security;
    std::vector<Point> fenceDraft;
    WorkspaceState(Scene labInput = {}, SecurityConsole console = {}, std::vector<Point> draft = {})
        : lab(std::move(labInput)), security(std::move(console)), fenceDraft(std::move(draft)) {}
};
inline std::string workspaceKey(const WorkspaceState &state) {
    std::ostringstream out;
    out << std::setprecision(17);
    const auto &s = state.lab;
    for (auto p : s.points())
        out << p.x() << ',' << p.y() << ';';
    out << '|';
    for (auto p : s.polygon())
        out << p.x() << ',' << p.y() << ';';
    out << '|';
    for (auto e : s.edges())
        out << e.a() << ',' << e.b() << ',' << e.weight() << ',' << e.blocked() << ';';
    out << '|' << s.query().x() << ',' << s.query().y() << ',' << s.radius() << ',' << s.minPts()
        << ',' << s.source() << ',' << s.goal() << ',' << s.grid();
    out << "draft:";
    for (auto p : state.fenceDraft)
        out << p.x() << ',' << p.y() << ';';
    const auto &c = state.security;
    const auto &t = c.settings();
    out << '|' << t.start() << ',' << t.end() << ',' << std::quoted(t.user()) << ','
        << std::quoted(t.device()) << ',' << t.requestScale() << ',' << t.failureScale() << ','
        << t.featureRadius() << ',' << t.geoRadius() << ',' << t.minPts() << ',' << t.originLat()
        << ',' << t.originLon() << ',' << t.geoQuery().x() << ',' << t.geoQuery().y() << ','
        << t.featureQuery().x() << ',' << t.featureQuery().y();
    for (auto p : c.fence())
        out << p.x() << ',' << p.y() << ';';
    for (const auto &e : c.events())
        out << std::quoted(e.id()) << e.timestamp() << std::quoted(e.user())
            << std::quoted(e.device()) << e.lat() << ',' << e.lon() << ',' << e.requests() << ','
            << e.failures() << std::quoted(e.label()) << std::quoted(e.summary());
    for (const auto &v : c.reviews())
        out << std::quoted(v.first) << std::quoted(v.second);
    for (const auto &v : c.reviewNotes())
        out << std::quoted(v.first) << std::quoted(v.second);
    for (const auto &v : c.reviewTimes())
        out << std::quoted(v.first) << v.second;
    return out.str();
}
class EditHistory final {
    std::vector<WorkspaceState> states_;
    std::size_t cursor_ = 0;

  public:
    bool record(const WorkspaceState &state) {
        if (!states_.empty() && workspaceKey(states_[cursor_]) == workspaceKey(state))
            return false;
        if (!states_.empty())
            states_.erase(states_.begin() + static_cast<std::ptrdiff_t>(cursor_ + 1),
                          states_.end());
        states_.push_back(state);
        if (states_.size() > 128)
            states_.erase(states_.begin());
        cursor_ = states_.size() - 1;
        return true;
    }
    bool canUndo() const { return cursor_ > 0; }
    bool canRedo() const { return cursor_ + 1 < states_.size(); }
    const WorkspaceState &current() const { return states_.at(cursor_); }
    const WorkspaceState &undo() {
        if (canUndo())
            --cursor_;
        return current();
    }
    const WorkspaceState &redo() {
        if (canRedo())
            ++cursor_;
        return current();
    }
};
// Prepare both unit systems and optional background before publishing live UI state.
struct PreparedSecurity {
    Scene scene;
    std::vector<Observation> observations;
    std::vector<MapLine> map;
    static PreparedSecurity prepare(const SecurityConsole &console, bool feature,
                                    const std::string &mapPath) {
        console.validate();
        console.observations(!feature);
        PreparedSecurity out{console.scene(feature), console.observations(feature), {}};
        if (!feature)
            try {
                out.map = console.loadMap(mapPath);
            } catch (const std::exception &) {
            }
        return out;
    }
};
} // namespace gv
