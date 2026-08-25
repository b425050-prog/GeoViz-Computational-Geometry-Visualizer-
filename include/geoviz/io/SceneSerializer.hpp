#pragma once

/**
 * @file SceneSerializer.hpp
 * @brief Versioned, dependency-free scene persistence.
 */

#include "geoviz/app/SceneModel.hpp"

#include <filesystem>

namespace geoviz::io {

class SceneSerializer final {
public:
    static void save(const app::SceneSnapshot& scene, const std::filesystem::path& path);
    [[nodiscard]] static app::SceneSnapshot load(const std::filesystem::path& path);
};

}  // namespace geoviz::io

