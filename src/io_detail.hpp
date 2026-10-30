#pragma once
#include "algorithms.hpp"
#include "atomic_files.hpp"
#include "persistence.hpp"
#include "security.hpp"
#include <fstream>
#include <iomanip>
#include <sstream>
namespace gv::io_detail {

template <class T> T readValue(std::istream &in) {
    T value{};
    in >> value;
    if (!in)
        throw std::runtime_error("Malformed or incomplete file.");
    return value;
}
inline std::string readQuoted(std::istream &in) {
    std::string value;
    in >> std::quoted(value);
    if (!in)
        throw std::runtime_error("Malformed quoted value.");
    return value;
}
inline Point readPoint(std::istream &in) {
    const double x = readValue<double>(in);
    const double y = readValue<double>(in);
    return {x, y};
}
inline Edge readEdge(std::istream &in) {
    const int a = readValue<int>(in), b = readValue<int>(in);
    const double weight = readValue<double>(in);
    const bool blocked = readValue<bool>(in);
    return {a, b, weight, blocked};
}
inline std::ifstream openInput(const std::string &path) {
    std::ifstream in(std::filesystem::u8path(path), std::ios::binary);
    if (!in)
        throw std::runtime_error("Cannot open: " + path);
    in.seekg(0, std::ios::end);
    if (in.tellg() > 8 * 1024 * 1024)
        throw std::runtime_error("Input exceeds 8 MiB limit.");
    in.seekg(0);
    return in;
}
inline std::ofstream openOutput(const std::string &path) {
    std::ofstream out(std::filesystem::u8path(path), std::ios::binary);
    if (!out)
        throw std::runtime_error("Cannot write: " + path);
    out << std::setprecision(17);
    return out;
}
inline std::size_t count(std::istream &in, std::size_t maximum) {
    long long n = -1;
    in >> n;
    if (!in || n < 0 || static_cast<unsigned long long>(n) > maximum)
        throw std::runtime_error("Invalid or oversized record count.");
    return static_cast<std::size_t>(n);
}
inline void endInput(std::istream &in) {
    if (!in)
        throw std::runtime_error("Malformed or incomplete file.");
    in >> std::ws;
    if (!in.eof())
        throw std::runtime_error("Unexpected trailing file content.");
}
inline double asDouble(const std::string &text) {
    std::size_t n = 0;
    double v = std::stod(text, &n);
    if (n != text.size() || !std::isfinite(v))
        throw std::runtime_error("Invalid numeric CSV value: " + text);
    return v;
}
// Snapshots preserve editable drafts. Polygon validity belongs to algorithms
// that consume a completed polygon, while structural checks apply to both I/O paths.
inline void validateSnapshot(const Scene &s) {
    Engine::validate(s);
    if (!s.points().empty() &&
        (s.source() < 0 || s.goal() < 0 || s.source() >= static_cast<int>(s.points().size()) ||
         s.goal() >= static_cast<int>(s.points().size())))
        throw std::runtime_error("Saved source/goal index is invalid.");
}
inline std::int64_t asInteger(const std::string &text) {
    std::size_t n = 0;
    auto v = std::stoll(text, &n);
    if (n != text.size())
        throw std::runtime_error("Invalid integer CSV value: " + text);
    return v;
}

} // namespace gv::io_detail
