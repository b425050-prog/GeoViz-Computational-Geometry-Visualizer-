# Verification

Local verification date: **30 October 2026**.

## Executed checks

| Check | Result |
| --- | --- |
| Original GCC Release build and eight CTest suites | Passed before refactoring |
| Final GCC 15.2.0 Release build with strict compiler warnings | Passed |
| Final eight CTest suites | Passed |
| Engine expected/reference checks, fixed seed 425050 | 1,766 checks passed |
| Application command/state checks | 333 checks, zero failures |
| Desktop smoke render at 1440×900 | 77 states completed |
| Desktop smoke render at 960×640 | 77 states completed |
| Independent Clang Debug engine build and eight suites | Passed; Windows static-link warnings are a toolchain limitation |
| Documentation paths, controls, formats, workflow and OOP claims | Checked against retained source; all local Markdown links resolve |

The CTest suites cover geometry, routing, clustering, persistence, security, workspace
transitions, broader engine cases and structured evidence. Python 3 is needed only for
the structured-evidence suite; without it, CMake registers the seven C++ suites.

Tests exercise every built-in Lab input, all nine security scenarios, scan/indexed
agreement, deterministic ties, boundary and collinear cases, unreachable/blocked routes,
small weights, trace limits, CSV errors, transactional-save rollback, undo/redo,
immutable completed runs and failed-setting/import preservation.

Desktop checks call the production application commands and render all modes, both
security views, scenarios, empty selections, intermediate algorithm frames, dialogs,
comparison, presentation layout and enlarged text. Captured images and test exports
are generated under ignored `output/` or `build/` directories.

## Fixes and simplifications

- KD nearest/radius share traversal, pruning, counters and trace recording.
- Dijkstra/A* retain one route search; DBSCAN retains one expansion loop with two index strategies.
- Result initialization/final frames, security-view publication and workspace snapshot
  writing share implementations.
- Final-frame navigation seeks directly instead of repeatedly stepping.
- Manual stepping resets the playback interval before playback resumes; a regression
  check verifies that an old partial interval cannot skip a frame on resume.
- Accessor bodies and class comments are concise; private state and validated mutation remain.
- Duplicate generated scene files, HTML/report output, documentation generators,
  authoring package dependencies and old verification logs are removed.
- The tested Windows executable is tracked as `GeoViz.exe`. The five original GIFs,
  four workspace/sample screenshots and nine SVG diagrams are retained under `docs/media/`.
- The UI loads Windows fonts at runtime; bundled fonts and license files are removed.
  Graphics remain provided by the external raylib dependency.

## Reproduce

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\build.ps1 -Smoke
```

For the larger render size after building:

```powershell
.\GeoViz.exe --root . --smoke --smoke-size 1440 900
```

GitHub Actions is configured for GCC/Clang Debug/Release engine checks, Linux sanitizers
in Debug, and a Linux desktop build/smoke run. This configuration is not itself evidence
of a successful remote run; inspect the Actions result for the published commit.

## Verification limits

Automated checks and screenshot inspection cover the cases above; they are not exhaustive
physical mouse/keyboard testing or proof of correctness for every possible input.
Hull uses invariants; collision has expected-case tests rather than an independent runtime
oracle. Synthetic security scenarios do not establish real-world detection accuracy.
Unsigned evidence hashes check consistency, not authenticity.
