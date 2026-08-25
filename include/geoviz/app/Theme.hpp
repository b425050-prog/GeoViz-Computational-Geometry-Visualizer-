#pragma once

/**
 * @file Theme.hpp
 * @brief Centralized visual tokens for GeoViz's dark glass/neon interface.
 */

#include <raylib.h>

#include <array>

namespace geoviz::app {

struct Theme final {
    Color background{7, 13, 25, 255};
    Color backgroundRaised{11, 20, 36, 255};
    Color panel{16, 28, 48, 244};
    Color panelHover{24, 41, 66, 255};
    Color border{46, 67, 94, 210};
    Color gridMinor{29, 47, 68, 135};
    Color gridMajor{47, 68, 93, 180};
    Color text{232, 242, 255, 255};
    Color textMuted{139, 157, 184, 255};
    Color cyan{54, 226, 209, 255};
    Color violet{145, 105, 255, 255};
    Color coral{255, 103, 128, 255};
    Color amber{250, 197, 74, 255};
    Color sky{75, 169, 255, 255};
    Color mint{94, 234, 162, 255};
    Color danger{255, 86, 104, 255};

    [[nodiscard]] Color accent(int index) const noexcept {
        const std::array<Color, 7> accents{cyan, violet, sky, mint, coral, amber,
                                           Color{225, 113, 255, 255}};
        const int normalized = index < 0 ? 0 : index % static_cast<int>(accents.size());
        return accents[static_cast<std::size_t>(normalized)];
    }
};

}  // namespace geoviz::app

