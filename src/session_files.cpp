#include "session_files.hpp"
#include "persistence.hpp"
#include <chrono>
#include <fstream>

namespace gv {
void SessionFiles::verifyAssets(const std::filesystem::path &root) {
    for (const char *name : {"assets/Lato-Regular.ttf", "assets/Lato-Bold.ttf", "assets/geoviz.png",
                             "data/security_samples.csv"}) {
        const auto path = root / name;
        if (!std::filesystem::is_regular_file(path))
            throw std::runtime_error("Required file is missing: " + path.u8string() +
                                     ". Extract the complete GeoViz ZIP and keep assets/ and "
                                     "data/ beside GeoViz.exe.");
    }
}
void SessionFiles::ensureOutput() const {
    try {
        std::filesystem::create_directories(root_ / "output");
    } catch (const std::exception &error) {
        throw std::runtime_error("Cannot use " + (root_ / "output").u8string() +
                                 ". Save As to a writable folder or extract GeoViz to one. " +
                                 error.what());
    }
}
std::vector<std::string> SessionFiles::readRecent() const {
    std::ifstream in(root_ / "output/recent.txt");
    std::vector<std::string> paths;
    std::string path;
    while (paths.size() < 8 && std::getline(in, path)) {
        std::error_code error;
        if (path.size() <= 32768 &&
            std::filesystem::is_regular_file(std::filesystem::u8path(path), error) &&
            std::find(paths.begin(), paths.end(), path) == paths.end())
            paths.push_back(path);
    }
    return paths;
}
void SessionFiles::writeRecent(const std::vector<std::string> &paths) const {
    ensureOutput();
    AtomicFiles files;
    std::ofstream out(
        std::filesystem::u8path(files.stage((root_ / "output/recent.txt").u8string())),
        std::ios::binary);
    if (!out)
        throw std::runtime_error("Cannot write the recent-session list.");
    for (const auto &path : paths)
        out << path << '\n';
    AtomicFiles::close(out);
    files.commit();
}
std::optional<WorkspaceState> SessionFiles::readRecovery() const {
    const auto lab = root_ / "output/recovery.scene", security = root_ / "output/recovery.security";
    if (!std::filesystem::exists(lab) && !std::filesystem::exists(security))
        return std::nullopt;
    auto input = SceneIO::load(lab.u8string());
    auto console = SecurityConsole::load(security.u8string());
    const auto draft = root_ / "output/recovery.fence.scene";
    auto fence = std::filesystem::exists(draft) ? SceneIO::load(draft.u8string()).polygon()
                                                : console.fence();
    return WorkspaceState{std::move(input), std::move(console), std::move(fence)};
}
void SessionFiles::writeRecovery(WorkspaceView view) const {
    writeSnapshot(view, root_ / "output", "recovery");
}
void SessionFiles::preserve(WorkspaceView view) const {
    writeSnapshot(view, root_ / "output/recovery-history",
                  std::to_string(std::chrono::system_clock::now().time_since_epoch().count()));
}
void SessionFiles::writeSnapshot(WorkspaceView view, const std::filesystem::path &directory,
                                 const std::string &name) {
    try {
        std::filesystem::create_directories(directory);
        AtomicFiles files;
        SceneIO::save(view.lab, (directory / (name + ".scene")).u8string(), &files);
        view.security.save((directory / (name + ".security")).u8string(), &files);
        Scene draft;
        draft.setPolygon(view.fenceDraft);
        SceneIO::save(draft, (directory / (name + ".fence.scene")).u8string(), &files);
        files.commit();
    } catch (const std::exception &error) {
        throw std::runtime_error("Could not save workspace in " + directory.u8string() +
                                 ". Current input remains in memory; use Save As to a writable "
                                 "folder. " +
                                 error.what());
    }
}
} // namespace gv
