#pragma once
#include "workspace.hpp"
#include <filesystem>
#include <optional>

namespace gv {
// Owns local session-file policy independently of the desktop interface.
class SessionFiles final {
    std::filesystem::path root_;

  public:
    explicit SessionFiles(std::filesystem::path root) : root_(std::move(root)) {}
    static void verifyAssets(const std::filesystem::path &root);
    void ensureOutput() const;
    std::vector<std::string> readRecent() const;
    void writeRecent(const std::vector<std::string> &paths) const;
    std::optional<WorkspaceState> readRecovery() const;
    void writeRecovery(WorkspaceView view) const;
    void preserve(WorkspaceView view) const;
    static void writeSnapshot(WorkspaceView view, const std::filesystem::path &directory,
                              const std::string &name);
};
} // namespace gv
