# Build, dependencies and development setup

**Status:** Setup plan for the future implementation. This repository currently contains documentation, not CMake/application source. The build/run commands in this guide become runnable only after the foundation milestone creates the named files and targets.

## 1. Tools and dependencies

| Dependency | Purpose | Integration decision |
| :--- | :--- | :--- |
| C++20 compiler | Application, algorithms and tests. | Pin a supported GCC/Clang/MSVC version after a clean build. |
| CMake + Ninja | Reproducible builds. | Minimum CMake version must support the chosen configuration/presets. |
| Drogon + required Trantor/JSON/TLS dependencies | HTTP, views, async networking and database integration. | Pin release and transitive resolution. |
| raylib | Native console drawing/input. | Match supported desktop/platform dependencies. |
| SQLite | Local persistence. | Verify foreign-key and selected journaling support. |
| libsodium or equivalent vetted password library | Argon2id, secure random and token-digest operations. | Use supported APIs; do not replace verification with custom cryptography. |
| C++ test framework or small C++ test runner | Unit/integration/scenario checks. | Nonzero meaningful test execution is required. |

Package manager tools and CMake configuration are build infrastructure, not application logic. No Node/npm browser runtime or Python backend is needed.

## 2. Platform preparation

On Ubuntu/Debian, typical prerequisite packages include `build-essential`, `cmake`, `ninja-build`, `git`, `pkg-config`, OpenSSL, JSON, SQLite, zlib and libsodium development packages. raylib additionally needs the documented X11/OpenGL/audio development libraries. Package names and versions must be checked against the actual distribution.

On Windows, use one consistent toolchain: Visual Studio with a pinned C++ dependency manager/toolchain, or a documented compatible MSYS2 environment. Do not combine arbitrary MSVC libraries with a MinGW binary. On macOS, use a supported Clang toolchain and matching native libraries.

Select one reproducible dependency strategy: an explicitly pinned package-manager manifest or checksum/tag-pinned CMake fetching. Do not mix system and fetched copies accidentally. Do not disable TLS/signature/checksum verification to get an install through.

Before installation, inspect existing tool versions and repository instructions. Setup must not overwrite tracked files, user configuration or a dependency lockfile unexpectedly.

## 3. Foundation build design

Create a root `CMakeLists.txt` and define separate targets:

- `security_core`: domain, geometry, algorithms, analysis and typed traces; no raylib/Drogon headers in its public algorithm interface.
- `security_server`: Drogon controllers/views, storage adapters and application services.
- `security_console`: raylib UI, HTTP client and local lab workspace.
- `security_simulator`: scenario loader, deterministic scheduler and C++ client.
- `security_tests`: meaningful registered C++ tests, with separate integration labels where helpful.

Use target-scoped include paths, compile features and warning flags. Add build options for core/tests independently of the graphical client so headless core CI does not need a display. Do not make core correctness depend on a graphics context.

Pin dependencies in the same change that introduces them. Record license, source URL, exact version/commit and verification mechanism. A local dependency cache is useful only if its artifacts remain valid for the selected platform/toolchain.

## 4. Intended development commands

**Not runnable in the current documentation-only repository.** Once the build exists, the intended standard flow is:

```sh
cmake -S . -B build/debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build/debug --parallel 4
ctest --test-dir build/debug --output-on-failure --no-tests=error
```

The actual executable path/configuration may differ for multi-configuration generators. Document it from a real build. Four workers is a starting point, not a requirement; inspect available memory/cores before larger compilation or sanitizer runs.

Define and test a core-only configure option during implementation, then document its exact spelling. Do not publish a command using an option that CMake silently ignores.

## 5. Local configuration and first startup

Create a non-secret example configuration from the [configuration contract](CONFIGURATION.md). Copy it to an ignored local path; supply secrets securely. Create private runtime directories with restricted permissions. Select a dataset and use the server's migration/bootstrap process.

Start the server, verify a functional authorized request, then start the console and authenticate. Keep client/server protocols and effective schema versions compatible. A listening port alone does not prove that registration, media access or analysis works.

The proposed CLI contract is described in [operations](OPERATIONS.md). It must be implemented and verified before it is advertised as a working command.

## 6. Headless and graphical checks

Unit tests for geometry, clustering, statistics, persistence and policy can run without a display. Browser/form integration can use a C++ HTTP test client and manual browser review. raylib rendering tests and keyboard/resize checks need a compatible graphics/display environment.

If a cloud machine has no display, record graphical validation as unrun rather than claiming a window opened. A virtual display can support a deliberate smoke test once its dependencies are installed, but it does not replace real usability review.

## 7. Common setup diagnoses

| Symptom | First diagnosis |
| :--- | :--- |
| CMake cannot find Drogon/raylib | Check selected toolchain and package configuration paths before reinstalling everything. |
| SQLite support is unavailable | Inspect dependency build features and link configuration. |
| Server cannot read keys/media/database | Check paths, ownership and required configuration; do not make private files world-readable. |
| Video fails while images work | Check MIME type, authorization, Range handling and supported sample codec. |
| Date/timezone operations fail | Verify the pinned C++ timezone support and timezone database. |
| Graphical client fails on startup | Check display/OpenGL availability and platform libraries. |
| Build uses stale paths/toolchain | Reconfigure a separate build directory; do not delete user data. |

Record successful versions, commands and limitations in [validation](../testing/VALIDATION_RECORD.md). At present, dependency pinning and all application build checks remain pending.
