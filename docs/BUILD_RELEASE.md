# Reproducible build and release

## Windows desktop

Supported local toolchain: GCC 15.2.0 x64, C++17, raylib 5.5, CMake 3.21+; the default installation
is C:/raylib/w64devkit. `build.ps1` delegates source lists and options to CMake. It builds the tests,
application and tools, runs all CTest groups, and copies the executable into the working root.

```powershell
./build.ps1 -Smoke
./build.ps1 -DebugBuild
```

A clean source build can instead fetch raylib at the pinned 5.5 revision:

```text
cmake -S . -B build/desktop -G Ninja -DGEOVIZ_FETCH_RAYLIB=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build/desktop
ctest --test-dir build/desktop --output-on-failure
cmake --build build/desktop --target run
cmake --install build/desktop --prefix dist/GeoViz
```

Linux desktop builds need X11/OpenGL development packages and a display (or Xvfb/Mesa). Native named
file dialogs target Windows; Linux desktop CI checks graphics and keyboard/canvas input. Engine-only
builds have no raylib/display dependency:

```text
cmake --preset engine-release
cmake --build build/engine-release
ctest --test-dir build/engine-release --output-on-failure
cmake --preset sanitizers
cmake --build build/sanitizers
ctest --test-dir build/sanitizers --output-on-failure
```

## Authoring environment

Node 24 and Python 3.13 are used in CI. npm's package-lock pins marked and Playwright; the Python
manifest pins Pillow, ReportLab and its text dependency. Desktop users need none of these tools.

```text
npm ci
python -m pip install -r requirements-docs.txt
build/desktop/geoviz_samples .
build/desktop/geoviz_showcase .
python tools/build_catalogue.py
python tools/build_media.py
node tools/build_docs.mjs
node tools/verify_docs.mjs
python tools/build_report.py
python tools/verify_release.py --source
```

On Windows append `.exe` to exporter names. On Linux install Playwright Chromium with
`npx playwright install --with-deps chromium`; on Windows QA uses installed Chrome by default.
`CHROME_PATH` can select another Chromium executable.

## Release provenance

Push a version tag such as v2.0.0 only after source checks pass. The verified-build workflow runs
GCC/Clang Debug/Release, ASan/UBSan, coverage, Linux graphics/physical input, Windows desktop smoke at
960x640 and 1440x900, and offline-document QA. It publishes only after those jobs succeed. Windows
CI builds the portable binary from the exact tag, records compiler, CMake, pinned raylib revision,
source commit and binary SHA-256, packages the assets/docs and uploads SHA256SUMS.txt.

The Windows CI graphics environment uses the pinned
[Mesa setup action](https://github.com/ssciwr/setup-mesa-dist-win) and Mesa 26.0.3 to support a software
OpenGL context. Mesa is test infrastructure; it is not included as an application dependency.

Local academic archives are produced with `python tools/package.py --destination PATH.zip` after
copying the built application to GeoViz.exe and running `python tools/provenance.py --compiler PATH`.
`package.py --manifest-only` generates hashes; `--check` checks without modifying them. The release
allowlist includes license and dependency manifests. Runtime evidence and build caches are excluded.
Binaries and generated hash manifests are release artifacts, not duplicated in source control.
