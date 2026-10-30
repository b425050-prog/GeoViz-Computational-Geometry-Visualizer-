#include "app.hpp"

int main(int argc, char **argv) {
    bool smoke = false;
    try {
        std::filesystem::path root = std::filesystem::absolute(GetApplicationDirectory());
        if (!std::filesystem::exists(root / "data/security_sample.csv"))
            root = root.parent_path();
        int width = 1440, height = 900;
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "--smoke")
                smoke = true;
            else if (arg == "--smoke-size" && i + 2 < argc) {
                width = std::stoi(argv[++i]);
                height = std::stoi(argv[++i]);
                if (width < 960 || height < 640 || width > 7680 || height > 4320)
                    throw std::runtime_error("Smoke dimensions must be 960x640 to 7680x4320.");
            } else if (arg == "--root" && i + 1 < argc)
                root = argv[++i];
            else
                throw std::runtime_error("Usage: geoviz [--root PROJECT_DIRECTORY] "
                                         "[--smoke] [--smoke-size W H]");
        }
        SetTraceLogLevel(LOG_WARNING);
        gv::ui::App app(std::filesystem::absolute(root), smoke, width, height);
        app.loop();
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "GeoViz: " << e.what() << '\n';
        if (!smoke)
            gv::showApplicationError(e.what());
        return 1;
    }
}
