#include "io_detail.hpp"
namespace gv {
using namespace io_detail;

// Write a versioned input snapshot using full-precision doubles and checked counts.
void SceneIO::save(const Scene &s, const std::string &path, AtomicFiles *external) {
    validateSnapshot(s);
    AtomicFiles localTransaction;
    auto &transaction = external ? *external : localTransaction;
    auto out = openOutput(transaction.stage(path));
    out << "GEOVIZ 1\n" << s.points().size() << '\n';
    for (auto p : s.points())
        out << p.x() << ' ' << p.y() << '\n';
    out << s.polygon().size() << '\n';
    for (auto p : s.polygon())
        out << p.x() << ' ' << p.y() << '\n';
    out << s.edges().size() << '\n';
    for (auto e : s.edges())
        out << e.a() << ' ' << e.b() << ' ' << e.weight() << ' ' << e.blocked() << '\n';
    out << s.query().x() << ' ' << s.query().y() << ' ' << s.radius() << ' ' << s.minPts() << ' '
        << s.source() << ' ' << s.goal() << ' ' << s.grid() << '\n';
    if (!out)
        throw std::runtime_error("Scene write failed.");
    AtomicFiles::close(out);
    if (!external)
        transaction.commit();
}
// Parse into a local Scene, reject trailing content, then validate snapshot structure.
Scene SceneIO::load(const std::string &path) {
    auto in = openInput(path);
    std::string magic;
    int version = 0;
    in >> magic >> version;
    if (magic != "GEOVIZ" || version != 1)
        throw std::runtime_error("Unsupported scene format/version.");
    Scene s;
    auto n = count(in, MAX_POINTS);
    for (std::size_t i = 0; i < n; ++i)
        s.addPoint(readPoint(in));
    n = count(in, MAX_POINTS);
    for (std::size_t i = 0; i < n; ++i)
        s.addPolygonVertex(readPoint(in));
    n = count(in, MAX_EDGES);
    for (std::size_t i = 0; i < n; ++i)
        s.addEdge(readEdge(in));
    s.setQuery(readPoint(in));
    s.setRadius(readValue<double>(in));
    s.setMinPts(readValue<int>(in));
    s.setSource(readValue<int>(in));
    s.setGoal(readValue<int>(in));
    s.setGrid(readValue<bool>(in));
    endInput(in);
    validateSnapshot(s);
    return s;
}
// Pair a readable checked result with the exact input required to reproduce it.
void SceneIO::summary(const Run &run, const std::string &path, AtomicFiles *external) {
    AtomicFiles localTransaction;
    auto &transaction = external ? *external : localTransaction;
    auto out = openOutput(transaction.stage(path));
    auto &r = run.result();
    out << "GEOVIZ CHECKED RUN\nAlgorithm: " << modeName(r.mode()) << "\nResult: " << r.summary()
        << "\n"
        << r.check() << "\nOperations: " << r.stats().operations()
        << "\nRange queries: " << r.stats().queries()
        << "\nVisited/expanded: " << r.stats().expanded()
        << "\nScene revision: " << run.input().revision() << "\nRadius: " << run.input().radius()
        << "\nMinPts: " << run.input().minPts() << "\nTrace frames: " << r.trace().frames().size()
        << "\nTrace truncated: " << r.trace().truncated()
        << "\nTie policy: ascending input index; heap (f,g,index); strict coordinate "
           "sort.\nTolerance: 1e-9 scale-aware orientation; exact radius inclusion.\nValue: "
        << number(r.value(), 12) << "\nAnswer indices:";
    for (int id : r.answer())
        out << ' ' << id;
    out << "\nCluster labels:";
    for (int label : r.labels())
        out << ' ' << label;
    out << "\nKinds (0 noise,1 border,2 core):";
    for (int kind : r.kind())
        out << ' ' << kind;
    out << "\nReproduce: load the adjacent .scene snapshot, select this algorithm, and Run.\n";
    if (!out)
        throw std::runtime_error("Summary write failed.");
    SceneIO::save(run.input(), path + ".scene", &transaction);
    AtomicFiles::close(out);
    if (!external)
        transaction.commit();
}

} // namespace gv
