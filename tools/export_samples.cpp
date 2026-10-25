// Explicit authoring tool: normal tests never rewrite the distributed scenes.
#include "geoviz.hpp"
#include <filesystem>
#include <iostream>

int main(int argc, char **argv) {
    try {
        if (argc != 2)
            throw std::invalid_argument("Usage: geoviz_samples <destination-root>");
        const auto root = std::filesystem::absolute(argv[1]);
        std::filesystem::create_directories(root / "data/gallery");
        std::filesystem::create_directories(root / "output/gallery");
        gv::Engine engine;
        int exported = 0;
        for (int variant = 0; variant < gv::SampleVariantCount; ++variant) {
            for (int mode = 0; mode < 10; ++mode) {
                const auto selected = static_cast<gv::Mode>(mode);
                const auto scene = gv::sampleScene(selected, variant);
                const gv::Run run{scene, engine.execute(scene, selected)};
                if (!run.result().checked())
                    throw std::runtime_error("Sample check failed: " + run.result().check());
                const std::string name =
                    variant == 0 ? "sample_" + std::to_string(mode)
                                 : "mode_" + std::to_string(mode) + "_" + std::to_string(variant);
                const auto folder = variant == 0 ? std::filesystem::path{} : "gallery";
                gv::SceneIO::save(scene, (root / "data" / folder / (name + ".scene")).string());
                gv::SceneIO::summary(run, (root / "output" / folder / (name + ".txt")).string());
                ++exported;
            }
        }
        std::cout << "PASS: exported " << exported << " checked scenes and reports to "
                  << root.string() << '\n';
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
