# Verification

Local verification date: **31 October 2026**.

## Executed checks

| Check | Result |
| --- | --- |
| GCC 15.2 Release desktop build with strict compiler warnings | Passed |
| Nine CTest suites | Passed |
| Engine expected/reference checks, fixed seed 425050 | 1,766 checks passed |
| Product reliability checks | 26 checks passed |
| Production application command/state checks | 346 checks, zero failures |
| Fresh portable ZIP at 960×640, 1440×900 and 1920×1200 | 77 states completed at each size |
| Extraction path containing spaces and Ω | Passed, including original fonts/icons and recovery |
| Missing required font and malformed CSV startup | Rejected with an actionable error |
| Independent Clang 23 Debug engine build and nine suites | Passed; the local Windows static linker reports duplicate-section warnings |
| Assistant-driven Windows UI tasks | Completed; script and observations are in [Usability review](USABILITY.md) |
| Local Markdown links, controls, file formats and OOP claims | Checked against the retained source |

The suites cover geometry, routing, clustering, persistence, security, workspace
transitions, broader engine cases, product reliability and structured evidence.
Python 3 enables the structured-evidence suite; without it CMake registers eight
C++ suites. No Python is needed to run GeoViz.

Tests exercise all 60 built-in Lab inputs and all nine security scenarios,
scan/indexed agreement, deterministic ties, boundary and collinear cases,
unreachable/blocked routes, small weights, trace limits, CSV errors, transactional
save rollback, immutable completed runs, undo/redo, rejected settings/imports and
preservation of selections after a failed sample change.

Product checks cover bounded selection/paste, UTF-8 deletion, replacement models
with distinct revision tokens, equivalent edits, recent-session filtering, Unicode
recovery paths and rejection of incomplete/corrupt recovery. Desktop checks cover
failed save checkpoints, refused unsaved close when recovery fails, and preservation
of unapplied Settings and review drafts.

The packaged desktop checks call production commands and render every mode, both
security views, every scenario, empty selections, intermediate frames, dialogs,
comparison, presentation layout and enlarged text. Generated images, exports and
reports stay outside the release contents.

## Final product changes

- Shared route search, DBSCAN expansion, KD traversal and snapshot writing retain
  their OOP interfaces and avoid duplicate implementations.
- `App` has a declaration header and separately compiled implementation files;
  `SessionFiles`, `EditHistory` and the shared text editor own focused concerns.
- Idle history checks borrow models and inspect revision tokens. In one local
  Release run, 10,000 unchanged observations of a 4,096-event workspace took less
  than 1 ms. This measures that check, rather than overall application frame time.
- Save/import/recovery failures preserve input. Close failures explain how to save
  elsewhere, and open dialog drafts remain available.
- Text editing supports a caret, selection and clipboard shortcuts; long labels
  are shortened visually. Input captions remain accurate across algorithm changes.
- The tracked `GeoViz.exe`, five original GIFs, four screenshots, nine SVG diagrams,
  original Lato fonts, icons, theme, artwork and all ten modes are retained.
- Generated documentation tools, HTML guides and old build/test output are excluded.
  Dependency notices stay with bundled third-party files.
- The Windows icon build handles source paths containing spaces. The executable
  retains GeoViz 2.0.0 metadata and embeds UTF-8 process configuration.

## Reproduce

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\build.ps1 -Smoke

# OutputDirectory must be new and should be outside the source folder.
powershell -NoProfile -ExecutionPolicy Bypass -File .\tests\portable_checks.ps1 `
  -BuildDirectory .\build\Release -OutputDirectory 'C:\Temp\GeoViz-package-check'
```

The package check builds a portable ZIP from the fresh build, extracts it and tests
three render sizes. GitHub Actions defines six jobs: GCC/Clang Debug/Release engine
checks, Linux desktop build/smoke, and a Windows build/package check. Linux Debug
jobs use AddressSanitizer and UndefinedBehaviorSanitizer. Windows CI fetches pinned
raylib 5.5, compiles in a path containing spaces, runs CTest and validates a fresh ZIP
using software OpenGL. Its ZIP, checksum and reports are uploaded as workflow artifacts.
Check the [Actions result](https://github.com/b425050-prog/GeoViz-Computational-Geometry-Visualizer-/actions)
for the commit being reviewed; a workflow definition alone is not a passing result.

## Limits

The user selected automated and assistant UI review in place of unfamiliar human
testers. These checks do not establish novice-user success rates or exhaustive
physical interaction coverage. See [Usability review](USABILITY.md).

Hull uses invariants; collision has expected-case tests rather than an independent
runtime oracle. Synthetic security scenarios do not establish real-world detection
accuracy. Evidence hashes are unsigned consistency checks. Staged saves handle
reported I/O failures and do not promise recovery from a power loss during publication.