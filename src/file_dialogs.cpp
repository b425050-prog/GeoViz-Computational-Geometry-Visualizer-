#include "file_dialogs.hpp"
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
    dialog.lpstrFilter = L"GeoViz sessions\0*.scene;*.security\0Event CSV\0*.csv\0All files\0*.*\0";
    dialog.lpstrDefExt = security ? L"security" : L"scene";
    dialog.Flags = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST |
                   (save ? OFN_OVERWRITEPROMPT : OFN_FILEMUSTEXIST);
    if (!(save ? GetSaveFileNameW(&dialog) : GetOpenFileNameW(&dialog))) {
        if (CommDlgExtendedError())
            throw std::runtime_error("Windows file dialog failed.");
        return {};
    }
    return std::filesystem::path(buffer).u8string();
}
} // namespace gv
#else
namespace gv {
std::string chooseSessionFile(bool, bool, const std::string &) {
    throw std::runtime_error("Use drag/drop to open; named dialogs require Windows.");
}
} // namespace gv
#endif
