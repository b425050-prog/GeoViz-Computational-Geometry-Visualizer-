#pragma once
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <stdexcept>
#include <vector>
namespace gv {
// RAII transaction: all streams close successfully before any destination changes.
// Existing generations are renamed to backups and restored on publication failure.
// This protects against reported I/O failures; it is not a power-loss journal.
class AtomicFiles final {
    struct File {
        std::filesystem::path target, staged, backup;
        bool backed = false, published = false;
    };
    std::vector<File> files_;
    inline static std::atomic<unsigned long long> serial_{0};

  public:
    AtomicFiles() = default;
    AtomicFiles(const AtomicFiles &) = delete;
    AtomicFiles &operator=(const AtomicFiles &) = delete;
    std::string stage(const std::string &name) {
        std::filesystem::path p = std::filesystem::absolute(std::filesystem::u8path(name));
        for (const auto &f : files_)
            if (f.target == p)
                throw std::runtime_error("Duplicate export destination.");
        if (std::filesystem::exists(p) && !std::filesystem::is_regular_file(p))
            throw std::runtime_error("Export destination is not a regular file: " + name);
        auto token = std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) +
                     "-" + std::to_string(serial_++);
        auto staged = p;
        staged += ".stage-" + token;
        auto backup = p;
        backup += ".backup-" + token;
        files_.push_back({p, staged, backup});
        return staged.u8string();
    }
    static void close(std::ofstream &out) {
        out.flush();
        if (!out)
            throw std::runtime_error("Export flush failed; prior files preserved.");
        out.close();
        if (!out)
            throw std::runtime_error("Export close failed; prior files preserved.");
    }
    void commit(const std::function<void(std::size_t)> &beforePublish = {}) {
        for (const auto &f : files_) {
            if (!std::filesystem::is_regular_file(f.staged))
                throw std::runtime_error("Missing staged export.");
            if (std::filesystem::exists(f.target) && !std::filesystem::is_regular_file(f.target))
                throw std::runtime_error("Destination changed during export.");
        }
        try {
            std::size_t index = 0;
            for (auto &f : files_) {
                if (beforePublish)
                    beforePublish(index++);
                if (std::filesystem::exists(f.target)) {
                    std::filesystem::rename(f.target, f.backup);
                    f.backed = true;
                }
                std::filesystem::rename(f.staged, f.target);
                f.published = true;
            }
        } catch (...) {
            bool restored = true;
            for (auto i = files_.rbegin(); i != files_.rend(); ++i) {
                std::error_code ec;
                if (i->published) {
                    std::filesystem::remove(i->target, ec);
                    restored &= !ec;
                    i->published = false;
                }
                if (i->backed) {
                    ec.clear();
                    std::filesystem::rename(i->backup, i->target, ec);
                    restored &= !ec;
                    if (!ec)
                        i->backed = false;
                }
            }
            if (!restored)
                throw std::runtime_error(
                    "Publication failed; recovery backups retained beside destination.");
            throw;
        }
        for (auto &f : files_) {
            std::error_code ec;
            std::filesystem::remove(f.backup, ec);
            f.backed = false;
        }
        files_.clear();
    }
    ~AtomicFiles() {
        for (const auto &f : files_) {
            std::error_code ec;
            std::filesystem::remove(f.staged, ec);
        }
    }
};
} // namespace gv
