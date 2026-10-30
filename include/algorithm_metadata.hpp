#pragma once
#include "trace.hpp"
namespace gv {
struct AlgorithmDescriptor {
    Mode mode;
    bool routing;
    bool security;
    const char *purpose;
};
inline const std::vector<AlgorithmDescriptor> &algorithmDescriptors() {
    static const std::vector<AlgorithmDescriptor> values = {
        {Mode::Hull, false, true, "Outer boundary"},
        {Mode::Geofence, false, true, "Ray crossings"},
        {Mode::Dijkstra, true, false, "Least cost"},
        {Mode::AStar, true, false, "Heuristic route"},
        {Mode::DBSCAN, false, true, "Density scan"},
        {Mode::ClosestPair, false, true, "Closest distance"},
        {Mode::KDNearest, false, true, "Nearest query"},
        {Mode::KDRadius, false, true, "Radius query"},
        {Mode::IndexedDBSCAN, false, true, "Indexed density"},
        {Mode::Collision, false, true, "Segment intersections"}};
    return values;
}
inline std::string pseudoStep(Mode mode, const std::string &explanation) {
    if (mode == Mode::Dijkstra || mode == Mode::AStar) {
        if (explanation.find("Initialize") != std::string::npos)
            return "1: g[source]=0; push source";
        if (explanation.find("Settle") != std::string::npos)
            return "2: pop least (f,g,id); settle vertex";
        if (explanation.find("Relax") != std::string::npos)
            return "3: if g[u]+w<g[v], update parent and push";
        return "4: reconstruct goal path or report unreachable";
    }
    if (mode == Mode::DBSCAN || mode == Mode::IndexedDBSCAN) {
        if (explanation.find("Too few") != std::string::npos)
            return "2: neighbours<MinPts => provisional noise";
        if (explanation.find("Core point") != std::string::npos)
            return "3: core point starts a new cluster";
        if (explanation.find("Expand") != std::string::npos)
            return "4: pop queue; add core neighbours; assign border";
        return "1/5: query neighbours; publish labels and kinds";
    }
    if (mode == Mode::Hull)
        return "sort points; lower/upper: pop non-left turns; join chains";
    if (mode == Mode::Geofence)
        return "boundary test; count half-open ray crossings; parity";
    if (mode == Mode::Collision)
        return "for each unordered segment pair: orientation + endpoints";
    if (mode == Mode::ClosestPair)
        return "split x-order; merge y-order; compare bounded strip";
    return "median split; visit nearer region; prune by box lower bound";
}
} // namespace gv
