/**
 * @file main.cpp
 * @brief Native entry point for the GeoViz desktop application.
 */

#include "geoviz/app/Application.hpp"

#include <exception>
#include <iostream>

namespace {

[[nodiscard]] int runGeoViz() {
    try {
        geoviz::app::Application application;
        return application.run();
    } catch (const std::exception& error) {
        std::cerr << "GeoViz fatal error: " << error.what() << '\n';
        return 1;
    }
}

}  // namespace

#if defined(_WIN32)
// Declare the Windows GUI entry point without including <windows.h>.  raylib
// intentionally exposes convenient names such as ShowCursor; Win32 exposes
// functions with the same names, so including both headers in one C++
// translation unit creates conflicting C-linkage declarations.  The Win32
// handle types are opaque pointers, which is all this entry point needs.
extern "C" int __stdcall WinMain(void*, void*, char*, int) {
    return runGeoViz();
}
#else
int main() {
    return runGeoViz();
}
#endif
