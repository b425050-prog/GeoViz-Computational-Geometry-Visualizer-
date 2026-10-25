# Review resolution - GeoViz 2.0

This checklist maps the original 40 review items to implemented changes and reproducible checks.
It describes an educational C++17 OOP project, not a production security platform or a promised
course grade. Items include optional teaching features because the requested scope was all 40.

The original engine suite remains intact alongside focused seeded regressions. Actual local test
reports are in verification/; CI artifacts and release provenance identify the exact source revision.
A configured gate is not evidence of a successful run: use the repository CI badge and the run for
the release tag to inspect the final outcome.

| # | Review item | Implementation | Verification / practical scope |
| --- | --- | --- | --- |
| 1 | Transactional saves/exports | AtomicFiles closes streams, stages every file, composes exports, backs up prior destinations and rolls back publication. | persistence_regressions: sidecar/unavailable path, short-write/flush, mid-publication rollback; engine security exports. |
| 2 | Exact Settings values | max_digits10 formatting; no-edit Apply preserves every parameter, including 1e-10. | geometry_regressions round trips; desktop coordinator no-edit Settings probes. |
| 3 | Translation-stable polygons | Vertex-relative long-double compensated area, with the existing world-unit tolerance. | Squares at origin and +/-9,999,999; reversed winding, boundary and membership checks. |
| 4 | Conservative A* and strict reference | No absolute epsilon in heuristic safety; edge metric and goal consistency checks; h=0 fallback. Relative cost equivalence preserves exact zero. | Zero-cost route identity across 1e-6..1e12 scales; deliberately wrong result fails Bellman-Ford/path verification. |
| 5 | Custom origin and atomic imports | CSV schema validation is origin-independent; PreparedSecurity prepares both views before commit; campus background is optional. | Other-origin snapshot import, absent background and rejected-import full-state preservation in desktop harness. |
| 6 | Independent Lab workspace | Lab scene, mode, view, selection and selected edge are retained across security views. | Desktop Lab-map-features-Lab roundtrip with edited graph and selections. |
| 7 | Undo/redo | Graphics-independent EditHistory implements a bounded Memento history, distinct from frozen completed runs. One drag commits on release. | transitions_regressions: deletion topology/settings restore and redo branching; production coordinator history checks. |
| 8 | Named sessions and recovery | Windows Open/Save As, recent paths, dirty marker, per-edit recovery and preserved generations before replacement. | Scene/security persistence; production coordinator failed-import rollback; CI OS save/recovery checks. Recovery excludes runtime data from delivery. |
| 9 | Verification strength | Explicit REFERENCE-VERIFIED, INVARIANT-CHECKED, TEST-COVERED and FAIL text in UI, reports and structured evidence. | Existing expected-case and runtime-oracle tests; collision is explicitly labelled without a separate oracle. |
| 10 | Adversarial numerical regressions | Seeded translation/scale, near-collinear/tie, small-cost, zero-cost cycle, path identity and no-trace equivalence cases. | geometry/routing/clustering_regressions plus original engine suite; seed 425050. |
| 11 | Parser and persistence failure paths | Bounded files/records; structural truncation, BOM rejection policy, malformed CSV, oversized fields and seeded parser fuzzing. | persistence_regressions and security_checks.hpp; 240 bounded random parser inputs, all structural scene/security newline boundaries. |
| 12 | Application transition checks | Production handlers reset route picking, edge starts, point/polygon drags and stale selections; failed imports commit no live state. | Dedicated app_smoke.hpp coordinator harness; samples, Grid/Clear, settings, history and import probes; pure PreparedSecurity/EditHistory tests. |
| 13 | Windows desktop CI | Pinned raylib source, full GCC/Windows desktop/resource build and portable packaging. | Verified builds desktop-windows job; actual job outcome is linked by the CI badge. |
| 14 | Compiler/configuration/sanitizers | GCC/Clang x Debug/Release matrix; Debug ASan+UBSan; retained JUnit and reports. | Verified builds engine matrix. Sanitizer results are CI evidence for the exact source commit, not inferred from Windows runs. |
| 15 | Desktop graphics/input gate | 77-state harness at 960x640 and 1440x900; separate retained screenshots; Linux Xvfb/xdotool sends OS keys/click/drag. | desktop-linux and desktop-windows jobs; desktop_input.sh validates saved geometry and recovery. |
| 16 | Focused tests and coverage | Named geometry, routing, clustering, persistence, security, transition and structured-evidence CTest cases; original broad suite retained. | CTest and gcovr HTML/XML artifacts. Assertion counts are never presented as coverage. |
| 17 | Pinned authoring environment | npm package-lock; pinned Pillow/ReportLab/text dependency; documented Node/Python/compiler/raylib versions. | Fresh npm ci; isolated Python environment; docs workflow regenerates and verifies output. |
| 18 | Unified build entry point | PowerShell delegates to CMake, with presets, asset-aware run and install targets. | Windows Release/Debug and engine-only presets; full fresh CI builds. |
| 19 | Versioned releases | Tag-triggered release publishes one portable archive, checksums and release notes only after all required jobs pass. | Verified builds publish job; GitHub releases/latest download link. |
| 20 | Exact source provenance | CI records source revision, dirty flag, compiler/CMake, pinned raylib revision and binary SHA-256. | provenance.py; packaged BUILD_PROVENANCE.json; release built from its tag. |
| 21 | Generated manifests and quality gates | package.py generates hashes or checks without mutation; explicit workflow permissions; source hygiene, compiler warnings and cppcheck. | verify_release.py, check_style.py, analysis job; intentionally updated delivery allowlist. |
| 22 | Focused engine interfaces | scene/geometry/trace/algorithm/persistence/security headers; geoviz.hpp is a short compatibility aggregate. | Graphics-independence/style gate and clean engine/desktop builds. |
| 23 | Smaller coordinator responsibilities | Entry point and UI support separated; command/input/inspector/render/smoke sections; domain history/view preparation are independent classes. | Pure workspace tests plus the separate production coordinator smoke harness. No framework or redundant class hierarchy added. |
| 24 | Security analysis separated from I/O | CSV/parser, projection/aggregation/evaluation and versioned security persistence have distinct translation units. | Focused origin/filter/aggregation/snapshot tests and existing nine-scenario checks. |
| 25 | Validated domain updates | Const SecuritySettings access; setSettings/updateSettings/replace validate candidates; event replacement clears stale reviews. Revisions invalidate caches. | Rejected-setting/event/fence update regressions; revision/cache invalidation checks. |
| 26 | Shared metadata | AlgorithmDescriptor supplies compatibility and purposes; engine sample names/count and security catalogue drive UI selection. | Picker uses descriptor compatibility; F4 skips incompatible security modes; original 60-scene/nine-scenario checks. |
| 27 | Direct sample selection | Named F2 picker with purpose and computed expected result; number shortcuts, recent sessions, N retained. | New smoke picker captures and Linux OS picker selection. |
| 28 | Contextual UTC Settings | Only applicable fields shown; separate feature/geographic radii; readable UTC with epoch preview; validation, caret, Tab and paste. | UTC valid/invalid date regressions; both Settings layouts in smoke; invalid candidate preserves edit buffer/live state. |
| 29 | Timeline and pseudocode | Bounded seek, recorded pseudocode line IDs, current explanation and full route g/f/parent plus queue inspection. | Playback bounds regression, trace line metadata assertions and intermediate/final smoke states. |
| 30 | Keyboard/accessibility | Tab/visible focus and core workflow shortcuts, adjustable text, source/edge selection, query/point edits and text alternatives. | OS input gate, enlarged-text capture, numerical text-contrast ratios in CONTRAST.json. Custom canvas is not a native screen-reader widget. |
| 31 | Explicit empty/disabled/truncated states | Unavailable playback/export controls disabled; empty filtered selection explains F7/Settings; capped trace announces skipped decisions. | Both empty-view captures; dense collision cap/final-frame regression; no-trace/run-state checks. |
| 32 | Comparison and presentation | Frozen side-by-side equivalent runs with proportional synchronized seeking; editing invalidates both; F5 hides editing bars and expands viewport. | Coordinator same-input/frozen comparison checks and dedicated comparison/presentation captures. |
| 33 | No discarded route/density trace state | No-trace snapshots return before queue/cost/vector/explanation work; truncated prefixes stop repeated snapshot construction. | Route/DBSCAN traced-vs-untraced identities and operation-count tests; benchmark trace/no-trace phases. |
| 34 | Workflow performance measurement | Sparse/dense/degenerate workloads through 256 points; index, checked execution, repeated verification, trace payload and frame latency recorded. | BENCHMARK.csv/environment JSON and smoke latency reports. Payload is a lower bound; checked execution includes index/validation/reference; phases are explicitly labelled. |
| 35 | Derived-data caches | Immutable observation/evaluation values cached behind controlled dataset/settings revisions. | Filter/revision cache invalidation regression; repeated evidence remains reproducible. |
| 36 | Structured evidence/reasons | Version/schema/source/units/parameters, canonical source hash, per-event candidate reasons, review notes/times and reproducible snapshots. | verify_evidence.py independently reconstructs input hash and validates candidate OR rule from structured output. |
| 37 | Sensitivity and review records | Radius/MinPts/scale comparison in inspector/export; notes and UTC review times are separate from synthetic labels. | Sensitivity exports; v1/v2 snapshot compatibility; quoted/multiline review note/time roundtrip regression. |
| 38 | Generated-guide deep links | Stable section-heading IDs; file-plus-fragment rewriting; HTML asset/fragment and navigation checks. | Playwright checks both reported broken links, every internal fragment, controls, images and mobile layout. |
| 39 | Repository entry/contribution path | CI badge, versioned download, changelog, demonstration script, contribution/build instructions; About/topics set during delivery. | README link checks and GitHub repository/release validation; GitHub Pages remains optional. |
| 40 | Project reuse terms | MIT project license, retained raylib/Lato notices; package/install include the same license. | Source/release allowlist, manifest and clean portable-package checks. |

## Important contracts

- Ordinary filesystem errors preserve the prior export through staging/rollback. This is not a
  power-loss journal, a distributed transaction or an authenticity/signature system.
- Recoverable workspace drafts are separate from validated security analysis state. Unfinished
  polygons can be recovered, but evidence requires a valid fence or an explicitly empty fence.
- Native named file dialogs target the Windows delivery. Linux desktop CI uses drag/drop and quick
  session paths for input testing. Custom-rendered widgets provide keyboard/text alternatives but
  do not expose a full native screen-reader tree.
- The portable archive carries executable/data/assets/docs/source once. Git excludes executable,
  generated hash/provenance copies, build caches, runtime recovery and temporary exports. The 60
  per-algorithm scene fixtures intentionally retain their stable catalogue identities.
- Performance timings depend on hardware/build and include the phases named in the CSV/environment
  report. Operation counts, check counts, coverage and wall time are distinct measurements.
