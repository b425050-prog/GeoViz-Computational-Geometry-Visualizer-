#include "geoviz.hpp"
#include "numeric_text.hpp"
#include "session_files.hpp"
#include "text_cursor.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>

using namespace gv;
namespace {
int checks = 0;
void expect(bool condition, const std::string &message) {
    ++checks;
    if (!condition)
        throw std::runtime_error(message);
}
template <class F> void rejects(F action, const std::string &message) {
    bool rejected = false;
    try {
        action();
    } catch (const std::exception &) {
        rejected = true;
    }
    expect(rejected, message);
}
void editing() {
    TextCursor cursor;
    std::string value = "0.8";
    cursor.selectAll(value);
    expect(value == "0.8" && cursor.selectedText(value) == value, "select all preserves text");
    expect(cursor.replace(value, "12", 2) && value == "12",
           "paste replaces selection within limit");
    expect(!cursor.replace(value, "345", 2) && value == "12", "oversized paste preserves text");
    cursor.selectAll(value);
    cursor.left(value);
    expect(cursor.position() == 0 && !cursor.selected(), "left collapses to selection start");
    cursor.right(value, true);
    expect(cursor.selectedText(value) == "1", "shift right extends selection");
    cursor.backspace(value);
    expect(value == "2", "backspace removes selection");
    cursor.erase(value);
    expect(value.empty(), "delete removes the next character");
    value = u8"AéΩ";
    cursor.reset(value.size());
    cursor.backspace(value);
    expect(value == u8"Aé", "backspace preserves UTF-8 boundaries");
    cursor.left(value);
    cursor.erase(value);
    expect(value == "A", "delete preserves UTF-8 boundaries");
    expect(exactNumber(0.8) == "0.8", "short round-trip setting text");
}
void history(const std::filesystem::path &root) {
    auto lab = sampleScene(Mode::Hull);
    auto console = loadSecuritySample((root / "data").u8string(),
                                      securitySamples((root / "data").u8string()).front());
    std::vector<Event> events;
    for (int i = 0; i < 4096; ++i) {
        auto event = console.events().front();
        event.setId("history-" + std::to_string(i));
        events.push_back(std::move(event));
    }
    console.setEvents(std::move(events));
    auto draft = console.fence();
    EditHistory edits;
    expect(edits.observe({lab, console, draft}), "first workspace is recorded");
    const auto begin = std::chrono::steady_clock::now();
    bool stable = true;
    for (int i = 0; i < 10000; ++i)
        stable = !edits.observe({lab, console, draft}) && stable;
    expect(stable && !edits.canUndo(), "idle observations create no history entries");
    std::cout << "10000 unchanged 4096-event observations: "
              << std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - begin)
                     .count()
              << " ms\n";
    lab.touch();
    expect(!edits.observe({lab, console, draft}) && !edits.pending({lab, console, draft}),
           "equivalent state refreshes stamps without recording");
    auto replacement = sampleScene(Mode::Hull);
    replacement.movePoint(0, {1, 2});
    lab = replacement;
    expect(edits.observe({lab, console, draft}),
           "replacement models have distinct revision tokens");
    const auto changed = workspaceKey(edits.current());
    auto restored = edits.undo();
    lab = restored.lab;
    console = restored.security;
    draft = restored.fenceDraft;
    expect(!edits.observe({lab, console, draft}) && edits.canRedo(), "undo does not destroy redo");
    expect(workspaceKey(edits.redo()) == changed, "redo restores the frozen input");
    draft[0] = {3, 4};
    expect(edits.observe({lab, console, draft}), "draft-only fence changes are observed");
    expect(!edits.canRedo(), "branching edit discards redo");
}
void sessions(const std::filesystem::path &root, const std::filesystem::path &scratch) {
    SessionFiles::verifyAssets(root);
    rejects([&] { SessionFiles::verifyAssets(scratch); }, "missing assets report an error");
    const auto folder = scratch / std::filesystem::u8path(u8"Session Ω with spaces");
    SessionFiles files(folder);
    files.ensureOutput();
    auto lab = sampleScene(Mode::Dijkstra);
    auto console = loadSecuritySample((root / "data").u8string(),
                                      securitySamples((root / "data").u8string()).front());
    auto fence = console.fence();
    const WorkspaceView view{lab, console, fence};
    files.writeRecovery(view);
    auto recovered = files.readRecovery();
    expect(recovered && workspaceKey(*recovered) == workspaceViewKey(view),
           "all recovery components round-trip through Unicode paths");
    const auto before = workspaceViewKey(view);
    std::ofstream(folder / "output/recovery.fence.scene") << "invalid snapshot";
    rejects([&] { files.readRecovery(); }, "corrupt fence prevents partial recovery");
    expect(workspaceViewKey(view) == before, "failed recovery preserves existing input");
    const auto scene = folder / "output/recovery.scene";
    files.writeRecent({scene.u8string(), scene.u8string(), (folder / "missing.scene").u8string()});
    expect(files.readRecent() == std::vector<std::string>{scene.u8string()},
           "recent paths omit duplicates and missing files");
    const auto blocked = scratch / "blocked";
    std::filesystem::create_directories(blocked);
    std::ofstream(blocked / "output") << "an output file, not a directory";
    SessionFiles unavailable(blocked);
    rejects([&] { unavailable.ensureOutput(); }, "unwritable output is reported");
    rejects([&] { unavailable.writeRecovery(view); }, "failed snapshot is rejected");
    expect(workspaceViewKey(view) == before, "failed save preserves the live workspace");
}
} // namespace
int main(int argc, char **argv) {
    try {
        if (argc != 3)
            throw std::runtime_error("product_checks ROOT OUTPUT");
        const auto root = std::filesystem::u8path(argv[1]);
        const auto scratch =
            std::filesystem::u8path(argv[2]) /
            std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        std::filesystem::create_directories(scratch);
        editing();
        history(root);
        sessions(root, scratch);
        std::cout << "PASS: " << checks << " product reliability checks.\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
