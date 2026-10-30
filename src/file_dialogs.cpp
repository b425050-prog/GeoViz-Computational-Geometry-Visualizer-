#include "file_dialogs.hpp"
#include <algorithm>
#include <cctype>
#include <stdexcept>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <filesystem>
// clang-format off
#include <windows.h>
#include <commdlg.h>
// clang-format on
namespace gv {
std::string chooseSessionFile(bool save, bool security, const std::string &initial) {
    wchar_t buffer[32768]{};
    auto path = std::filesystem::u8path(initial).wstring();
    path.copy(buffer, std::min(path.size(), std::size(buffer) - 1));
    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.lpstrFile = buffer;
    dialog.nMaxFile = static_cast<DWORD>(std::size(buffer));
    dialog.lpstrFilter =
        save ? (security ? L"Security snapshot\0*.security\0" : L"Lab scene\0*.scene\0")
             : L"GeoViz sessions\0*.scene;*.security\0Event CSV\0*.csv\0All files\0*.*\0";
    dialog.lpstrDefExt = security ? L"security" : L"scene";
    dialog.Flags = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST |
                   (save ? OFN_OVERWRITEPROMPT : OFN_FILEMUSTEXIST);
    if (!(save ? GetSaveFileNameW(&dialog) : GetOpenFileNameW(&dialog))) {
        if (CommDlgExtendedError())
            throw std::runtime_error("Windows file dialog failed.");
        return {};
    }
    const auto selected = std::filesystem::path(buffer);
    auto extension = selected.extension().u8string();
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (save && extension != (security ? ".security" : ".scene"))
        throw std::runtime_error(security ? "Save security snapshots with a .security extension."
                                          : "Save Lab scenes with a .scene extension.");
    return selected.u8string();
}
} // namespace gv
#else
namespace gv {
std::string chooseSessionFile(bool, bool, const std::string &) {
    throw std::runtime_error("Use drag/drop to open; named dialogs require Windows.");
}
} // namespace gv
#endif
