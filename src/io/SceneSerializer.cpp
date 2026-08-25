/**
 * @file SceneSerializer.cpp
 * @brief Human-readable .geoviz file format implementation using standard C++ streams.
 */

#include "geoviz/io/SceneSerializer.hpp"

#include <fstream>
#include <iomanip>
#include <limits>
#include <string>

namespace geoviz::io {
namespace {

inline constexpr std::size_t kMaximumItems = 10000U;

void requireStream(bool condition, const std::string& message) {
    if (!condition) throw GeometryError(message);
}

}  // namespace

void SceneSerializer::save(const app::SceneSnapshot& scene, const std::filesystem::path& path) {
    std::ofstream output(path);
    if (!output) throw GeometryError("Unable to open scene file for writing: " + path.string());

    output << "GEOVIZ 1\n" << std::setprecision(17);
    output << "POINTS " << scene.points.size() << '\n';
    for (const Point& point : scene.points) {
        output << point.x() << ' ' << point.y() << '\n';
    }
    output << "HALF_PLANES " << scene.halfPlanes.size() << '\n';
    for (const HalfPlane& halfPlane : scene.halfPlanes) {
        output << halfPlane.point().x() << ' ' << halfPlane.point().y() << ' '
               << halfPlane.direction().x() << ' ' << halfPlane.direction().y() << '\n';
    }
    if (!output) throw GeometryError("An error occurred while writing: " + path.string());
}

app::SceneSnapshot SceneSerializer::load(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input) throw GeometryError("Unable to open scene file: " + path.string());

    std::string magic;
    int version = 0;
    input >> magic >> version;
    requireStream(static_cast<bool>(input) && magic == "GEOVIZ" && version == 1,
                  "Unsupported or corrupt GeoViz scene header.");

    app::SceneSnapshot scene;
    std::string section;
    std::size_t count = 0U;
    input >> section >> count;
    requireStream(static_cast<bool>(input) && section == "POINTS" && count <= kMaximumItems,
                  "Invalid POINTS section in GeoViz scene.");
    scene.points.reserve(count);
    for (std::size_t index = 0U; index < count; ++index) {
        double x = 0.0;
        double y = 0.0;
        input >> x >> y;
        requireStream(static_cast<bool>(input), "Incomplete point data in GeoViz scene.");
        scene.points.emplace_back(x, y);
    }

    input >> section >> count;
    requireStream(static_cast<bool>(input) && section == "HALF_PLANES" && count <= kMaximumItems,
                  "Invalid HALF_PLANES section in GeoViz scene.");
    scene.halfPlanes.reserve(count);
    for (std::size_t index = 0U; index < count; ++index) {
        double x = 0.0;
        double y = 0.0;
        double directionX = 0.0;
        double directionY = 0.0;
        input >> x >> y >> directionX >> directionY;
        requireStream(static_cast<bool>(input), "Incomplete half-plane data in GeoViz scene.");
        scene.halfPlanes.emplace_back(Point{x, y}, Vector2D{directionX, directionY});
    }
    return scene;
}

}  // namespace geoviz::io

