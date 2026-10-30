# Design and algorithms

## Modules

| Files | Responsibility |
| --- | --- |
| `include/scene.hpp`, `src/geometry.cpp` | Validated coordinates, editable scene topology, geometric predicates and playback |
| `include/algorithms.hpp`, `src/algorithms.cpp` | Algorithm hierarchy, spatial indexes, dispatch and runtime result checks |
| `include/trace.hpp` | Statistics, recorded frames, results, playback state and frozen runs |
| `include/security.hpp`, `src/security*.cpp`, `src/csv.cpp` | CSV ingestion, filtering, projection, aggregation, samples and security evidence |
| `include/persistence.hpp`, `src/io*.hpp`, `src/io.cpp` | Scene format and checked-run summaries |
| `include/atomic_files.hpp` | Staged multi-file saves with rollback on reported publication failures |
| `include/workspace.hpp` | Undo/redo snapshots and prepared security-view data |
| `src/main.cpp`, `src/app*.hpp` | Startup, commands, input, inspection and drawing |
| `src/ui*.hpp`, `src/canvas_art.hpp`, `src/file_dialogs.cpp` | Rendering, settings, decorative backgrounds and native file dialogs |
| `src/samples.cpp` | Six deterministic inputs per algorithm |
| `tests/` | Expected cases, reference comparisons, persistence failures and desktop checks |

## OOP concepts

| Concept | Concrete use |
| --- | --- |
| Encapsulation | `Point` and `Scene` keep state private; mutations validate input and update scene revision. Collections are exposed through const views. |
| Abstraction | `Algorithm`, `AlgorithmRunner`, `SpatialIndex` and `LogAdapter` expose interfaces to clients. |
| Inheritance | Concrete algorithms inherit from `Algorithm`; Dijkstra and A* specialize `RouteAlgorithm`. `LinearIndex` and `KDTree` implement `SpatialIndex`. |
| Runtime polymorphism | `Engine` owns algorithms through `unique_ptr<Algorithm>`; DBSCAN queries a `SpatialIndex` without depending on its implementation. |
| Composition | `Run` owns its `Scene` and `Result`; results own statistics and a trace; security workspaces adapt observations into ordinary scenes. |
| RAII | Smart pointers and graphics wrappers release owned resources. `AtomicFiles` cleans abandoned staging files and restores earlier files after a reported commit failure. |
| Template Method | `Algorithm::run` shares the validate → calculate → verify lifecycle. |
| Strategy | Linear scan and KD-tree neighbourhood queries are interchangeable within one DBSCAN implementation. |
| Memento | `EditHistory` stores up to 128 input states with undo/redo and discards a redo branch when a new edit is recorded. |

`SecuritySettings` is a detached edit buffer; related fields are validated together when
applied to `SecurityConsole`. A `Run` exposes const input/result access, so later edits
cannot change the recorded answer.

## Algorithms and boundaries

Visual references: [convex hull](media/hull.svg), [geofence](media/geofence.svg),
[routing](media/routing.svg), [DBSCAN](media/dbscan.svg), [closest pair](media/closest.svg),
[KD-tree](media/kdtree.svg), [segment collision](media/collision.svg),
[object model](media/classes.svg) and [security workflow](media/security-flow.svg).

The costs below describe the core calculation. Recording, input validation and runtime
result verification add work; these are included in application execution.

| Mode | Implementation and policy | Core cost |
| --- | --- | --- |
| Convex hull | Monotone chain; exact-coordinate duplicates removed; collinear side interiors omitted | O(n log n) |
| Geofence | Boundary test, then ray parity; returns boundary/inside/outside | O(p) query; simple-polygon validation O(p²) |
| Dijkstra | Undirected graph, nonnegative finite weights, lazy priority queue, blocked edges skipped | O((V + E) log V) |
| A* | Same route search, using Euclidean or grid Manhattan/60 estimates; incompatible edge weights use zero heuristic | Worst-case asymptotic bound as Dijkstra |
| DBSCAN scan | Inclusive radius, MinPts includes the point itself, deterministic expansion | O(n²) |
| Closest pair | Divide and conquer with merged y-order; duplicate points yield distance zero | O(n log n) |
| KD nearest | Balanced median splits; lower-bound box pruning; lowest input index wins exact-distance ties | Typically efficient; O(n) worst-case query |
| KD radius | Same traversal; boundary points included; matches sorted by input index | O(n + k log k) worst case including sorting |
| Indexed DBSCAN | Same expansion with KD radius queries; neighbourhoods sorted for stable labels | O(n² log n) worst case including neighbourhood sorting |
| Segment collision | Every unordered pair of consecutive-point segments; touching and overlapping segments count | O(m²), m = floor(n/2) |

A KD-tree build uses recursive median selection, with expected O(n log n) construction
and O(n) stored nodes. The indexed modes do not promise a speedup for every input.
Operation counters measure algorithm work, not elapsed time; index construction and
independent result checks are excluded from DBSCAN query-distance counts.

Route answers are checked against Bellman-Ford and their returned edges/cost are checked.
Closest pair and KD queries use exhaustive references. DBSCAN uses an independent
core-component/border reference; geofence uses winding number. Hull checks convexity and
containment. Segment collision reports **test-covered** because it has no independent
runtime oracle.

## Limits

- At most 256 points, 256 polygon vertices, 4,096 edges and 4,096 trace frames.
  A capped trace preserves its final frame; the complete answer is retained.
- Coordinates must be finite and within ±10,000,000; route weights are 0 through 10¹².
  Radius is positive; MinPts is 1 through 256.
- Security datasets hold at most 4,096 records; each filtered view is limited to
  256 observations. Geographic data is restricted to 50 km from the configured
  origin, whose latitude is within ±80°. Distances are local approximations.
- Windows uses native Open/Save dialogs and the system Segoe UI font when available.
  Other platforms use raylib's default font; the native session chooser is Windows-only.
- Saved files are local text formats. Staged publication handles reported I/O failures;
  it is not a power-loss journal. Export hashes are unsigned consistency checks.
