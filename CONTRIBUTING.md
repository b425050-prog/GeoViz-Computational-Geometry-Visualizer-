# Contributing

Keep the project appropriate for a C++17 OOP course: prefer encapsulated models, existing Strategy,
Template Method, Memento and RAII boundaries. Do not introduce a service layer or framework for
simple value records. Engine interfaces must remain independent of raylib.

Run `cmake --preset engine-release`, `cmake --build build/engine-release`, and
`ctest --test-dir build/engine-release --output-on-failure`. Desktop changes also need the Windows
build and both smoke sizes. Run `python tools/verify_release.py --source`. Use clang-format 20+
with .clang-format; `python tools/check_style.py` checks formatting and unsafe persistence patterns.

Add a focused regression for changed numerical or persistence contracts. Seed generated tests and
include identity/path/topology assertions. Document measured phases and limits rather than treating
check counts as coverage. CI preserves tests, screenshots, coverage and benchmark artifacts.

Install authoring tools with `npm ci` and `python -m pip install -r requirements-docs.txt`.
Run the commands in docs/BUILD_RELEASE.md to regenerate guides and packages. Build products,
recovery files and evidence belong in ignored build/ and output/ directories. Version tags publish
CI-built release archives; do not commit executables or hand-maintained delivery hashes.
