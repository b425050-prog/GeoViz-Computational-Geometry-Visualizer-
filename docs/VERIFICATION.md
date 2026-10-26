# Verification and scope

Existing-feature audit: **26 October 2026**. Verification applies to the supplied build configuration and documented input limits. The original compiler/instrumentation checks below are retained as historical evidence; the current audit reran Windows GCC Debug/Release, all eight CTest suites, desktop smoke and offline-guide checks.

## Executed checks

| Layer | Method | Result |
| --- | --- | --- |
| Release build | GCC 15.2.0, C++17, `-Wall -Wextra -Wpedantic -Werror`, static link | Passed |
| Engine suite | 1766 assertions, fixed seed 425050 | Passed |
| Second compiler | Clang, warnings treated as errors and library assertions | Passed |
| Undefined-behaviour instrumentation | Clang `-fsanitize=undefined -fsanitize-trap=all` on the engine suite | Passed, no trap |
| Independent build path | Full desktop CMake build, reusable engine target and isolated CTest | Passed |
| Application state | Run/history, edits, settings transactions, six-sample cycling, filters, source review, queries, snapshots, exports and endpoint picking | Passed |
| Rendered UI | 77 states at 960×640 and 1440×900 | Passed; 960×640 contact sheets visually reviewed |
| Existing-feature regressions | 333 checks: all 60 Lab samples, all nine security datasets in both views, drafts, imports, filters, selection and coordinate limits | Passed |
| Offline guide | 24 recordings, step/play/rewind/final, timeline, navigation and narrow layout | Passed |
| Delivery | Clean-folder launch, source/executable hashes, local links and archive integrity | Passed |

Concise logs are supplied in `docs/verification/`. Build scripts stop on compiler errors and failed engine assertions. The CI workflow runs the engine suite on repository pushes and pull requests; its remote result is available only after a push.

## What the engine suite checks

- Geometry: crossing, endpoint touch, overlap, disjoint and zero-length segments; inside/outside/edge/vertex queries; invalid and folded polygons.
- Hull/proximity: empty, singleton, duplicate and collinear inputs; nearest ties and inclusive radius boundaries; exhaustive pair/query comparisons.
- Routes: weighted alternatives, blocking, unreachable goals, source=goal, negative-cost rejection, safe A* fallback and grid optimality.
- Density: self-counting, core/border/noise, provisional-noise reassignment and direct/indexed agreement.
- Reproducibility: one-vertex, two-vertex and self-crossing polygon drafts round-trip without loss; geofence execution rejects incomplete regions. Precise scene and security snapshot round trips, frozen historical inputs, operation counts, query locations and analyst verdicts.
- Inputs: CSV quoting/rejection, duplicate identities, invalid windows/scales and projection extent.
- Bounds: capped traces preserve the final result; the maximum 256-point input is checked; a 257th point is rejected; 128 mutually crossing segments retain 8,128 pair intersections with unique highlights and a capped trace.
- Samples: all 60 scenes are verified; added gallery scenes are reloaded and rerun; extended security datasets check window exclusion, aggregation and source linkage.
- Test isolation: ordinary and CTest runs write to a separate output root; a repeat run preserves every source-data hash, and the source root is rejected as the output destination. An explicit exporter handles release fixtures.
- Security scenarios: all nine presets are checked in both views. Tests cover corner/edge membership, device trajectories, zero counts, unknown labels, quoted filters, one-second and partial windows, 256/257 observations, 4096/4097 source records, invalid scales/radii, CSV rejection and preservation of existing evidence after a rejected export.
- Design contracts: abstract interfaces, virtual destructors and read-only collection/snapshot access are checked at compile time.

Eighty seeded scenes exercise multiple algorithms against references, including random weighted graphs. Tests use a fixed seed so failures can be reproduced. References use Bellman-Ford, exhaustive distance/query scans, winding classification, hull invariants and an independent DBSCAN core-component construction. Segment collision uses expected cases rather than claiming a separate runtime oracle.

## Implementation coverage

| Component group | Delivered implementation and evidence |
| --- | --- |
| Geometry kernel, hull, geofence and collision demonstrations | `GeometryKernel`, concrete algorithm classes; expected cases, invariant/reference checks and recordings |
| Dijkstra, A*, weighted/blocked graphs and grid navigation | Shared route Strategy with heuristic specializations; same-input optimal-cost/exploration comparisons |
| Direct and indexed DBSCAN | Shared expansion with `LinearIndex` / `KDTree`; queue/core/border/noise recordings and agreement checks |
| Divide-and-conquer closest pair | x-sort, recursive ranges, merged y order and strip comparisons; exhaustive verification |
| Balanced k-d construction, nearest and radius queries | Alternating median splits, visited/pruned regions, inclusive queries and fresh rebuilds |
| Editing, playback, rewind, comparisons and persistence | `Scene`, `Run`, `Trace`, `Playback`, `SceneIO`; edit invalidation and saved-input tests |
| Geographic and behavioural security views | Offline geographic layer, local projection, scaled device rates, common filters and source-linked inspector |
| Event ingestion and review evidence | Abstract log adapter plus local CSV implementation; labelled samples, separate analyst verdicts and reproducible exports |
| Bounded input, accessible labels and checks | Explicit limits, shape/line cues, readable inspector, validation and capped-trace disclosure |
| OOP/C++ design and deliverables | Encapsulation, polymorphic Strategies, RAII, source/test separation; focused design guide and portable executable |

The console reads local exports through the CSV adapter. It does not integrate a live production service.

## Presentation and maintainability

`SceneCanvas` consumes a const frame context and separates rendering into surface, region, topology, highlight, point, query and annotation layers. `SettingsPanel` owns modal input and validates detached candidate models. `App` coordinates commits and history. The 77-state smoke sequence covers both components, failed settings transactions and every sample.

## Practical limits

Floating-point geometry uses documented tolerances rather than exact predicates. Geographic distances are approximate within a local study area. Runtime reference checks add work beyond the displayed algorithm counts. A k-d tree can visit all points; indexed DBSCAN can still be quadratic. Trace storage can omit middle decisions when capped, while retaining the final result.

UI verification combines programmatic state checks and offscreen rendering; physical mouse/keyboard sequences and display configurations are not exhaustively covered. The supplied executable targets Windows x64. Synthetic security labels demonstrate reproducibility and false-positive accounting, not production detection accuracy.

## Version 2 verification gates

The 40-item review response is in [REVIEW_RESOLUTION.md](REVIEW_RESOLUTION.md). The focused CTest
cases cover geometry, routing, clustering, persistence, security, edit transitions and structured
evidence; the original broad suite remains. The desktop harness now captures 77 states at 960x640
and 1440x900, including new sample/settings/review/comparison/presentation/text layouts. See
[small-window report](verification/DESKTOP_960.txt), [normal-window report](verification/DESKTOP_1440.txt),
[text contrast ratios](verification/CONTRAST.json), [benchmark phases](verification/BENCHMARK.csv),
and [measurement environment](verification/BENCHMARK_ENVIRONMENT.json).

The [existing-feature audit](FEATURE_AUDIT.md) records the fixes and scope; its
[regression result](verification/FEATURE_CHECKS.txt) is generated by the desktop smoke run.
Superseded 70-state UI logs were removed. Matching scene files for paired algorithms remain
intentional fixtures: the catalogue and reproducibility tests refer to each mode's named input.

The [CI workflow](../.github/workflows/engine.yml) runs GCC/Clang Debug/Release, supported ASan/UBSan,
coverage, cppcheck, Windows desktop/resource builds, Linux graphics and OS input, and pinned guide
regeneration. It preserves logs/screenshots and publishes release assets only after required jobs
succeed. Local Windows checks do not imply Linux sanitizer success; inspect CI for the exact commit.

Regression checks assert path identities, topology, cache invalidation, reference failures, complete
export rollback and hash reconstruction. The bounded parser campaign uses seed 425050 and does not
claim exhaustive fuzz coverage. Collision is test-covered without a distinct runtime oracle; hull
is invariant-checked; other listed modes declare their independent runtime reference checks.
