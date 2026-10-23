# User guide

## Start a run

1. Open `GeoViz.exe` from the repository root.
2. Select a mode. Click **Sample** to load a suitable input; repeat to cycle its six scenarios. Across ten modes, the catalogue contains 60 editable scenes, including tidal archipelago and terraced botanical families.
3. Click **Run** or press Enter. The engine validates input, calculates a result and checks it against a reference or invariant.
4. Use **Play**, **Step**, **Rewind**, or **Final**. The inspector explains the current decision and displays work counts.
5. Edit the input and run again. The previous result is retained for comparison; its trace is no longer current.

![Desktop algorithm workspace](media/lab.png)

## Controls

| Action | Control |
| --- | --- |
| Add/select a point | Left-click the canvas |
| Move a point | Drag it in Lab |
| Set the query | Right-click |
| Delete selected point | Delete |
| Pan / zoom / fit | Middle-drag / wheel / F or Fit |
| Next sample | Sample or N |
| Record / play-pause | Enter / Space |
| Previous-next step | Left / Right |
| First-final frame | Home / End |
| Slower-faster playback | `[` / `]` |
| Polygon edit tool | P; click vertices in boundary order; drag to adjust |
| Create graph edge | Shift-click two different existing points |
| Set route endpoints | Pick source / Pick destination, then click a point; or select and press R / T |
| Block selected edge | Click an edge, then B |
| Parameters / edge cost | Settings or O |
| Same-input algorithm comparison | Compare or C |
| Compare and export two retained runs | Prior or V |
| Select next-previous point | Period / comma |
| Save / reload session | S / L |
| Export evidence | E |
| Help / dismiss overlay | F1 / Esc |

Scroll the inspector for source records and instructions. Dense scenes omit crowded labels; select points or use period/comma to inspect their identities. Resize the window as needed; the minimum is 960 × 640.

Security mode provides nine scenarios through **Sample / N**. The canvas identifies the active dataset. The [security guide](SECURITY.md#csv-schema-and-samples) describes the scenario procedures.

## Laboratory workflows

**Routes:** load a Dijkstra/A* sample or choose Grid. Pick source and destination, run, and use Compare to check optimal-cost agreement. Blocking a link changes the input and requires a fresh run. Settings edits a selected edge's cost.

**Geofence:** use a sample or add polygon vertices with P. Return to the point tool with P, right-click a query and run. Inside, boundary and outside are distinct outcomes. Self-intersecting or zero-area polygons are rejected.

**Clustering:** choose scan or indexed DBSCAN. Set radius and MinPts in Settings. Filled circles are core points; squares are border points; crosses are noise. MinPts includes the point itself. Compare uses the same input and parameters for both implementations.

**Proximity and collision:** right-click a query for k-d nearest/radius modes. For collision, points 0-1 form one segment, 2-3 the next, and so on; an unmatched last point has no segment.

## Files and reproducibility

Drag a `.scene`, `.csv`, or `.security` file into the window to import it. Choose the matching algorithm after loading a scene. **Load** reloads `output/session.scene` or `output/session.security`; it is not a file picker.

**Save** writes the current input to `output/`. **Export** writes a checked summary and its exact scene, or full security evidence with both view scenes and a security snapshot. **Prior** exports previous/current runs separately. Input timestamps are UTC epoch seconds; window ends are excluded.

Scene snapshots also preserve polygon drafts, including one or two vertices and crossings that are still being edited. Reloading restores those vertices exactly. A geofence run requires a completed simple polygon and explains invalid geometry before calculating; other algorithms can use the remaining scene while the polygon is unfinished. Save and Load apply the same structural checks, including source/goal indices.

The plaintext `GEOVIZ 1` scene format contains point count/coordinates, polygon count/coordinates, edge count/rows, then query x/y, radius, MinPts, source, goal and grid flag. Edge rows are `endpointA endpointB weight blocked`. Boolean fields are 0 or 1. Coordinates use full-precision doubles; counts and trailing content are validated. See existing scenes before editing manually.

## Build from source

Use Windows, a C++17 compiler and a matching raylib installation. The tested installation uses GCC 15.2.0 and the headers/static library under `C:\raylib\w64devkit`. Raylib headers and binary must match the compiler architecture.

```powershell
.\build.ps1                 # Isolated tests, then the application
.\build.ps1 -Run            # Build, test and launch
.\build.ps1 -Smoke          # Build plus 70 offscreen UI captures
.\build.ps1 -DebugBuild     # Debug information and standard-library assertions
```

`-CompilerPath` and `-RaylibRoot` override the defaults. The script fails immediately if compilation or a required check fails. It copies the finished program to `GeoViz.exe`; intermediate executables stay in `build/`.

An engine-only build needs CMake and a C++17 compiler, with no raylib dependency:

```powershell
cmake -S . -B build/core -DGEOVIZ_BUILD_APP=OFF
cmake --build build/core --config Release
ctest --test-dir build/core -C Release --output-on-failure
```

On Windows, use the compiler's terminal environment so its build tool is on PATH. A full CMake desktop build also accepts `-DRAYLIB_ROOT=...`; the supplied PowerShell script is the shortest Windows path.

Documentation can be regenerated with `build.ps1 -Docs` after installing Python's Pillow package and Node's `marked` package (`python -m pip install Pillow`, `npm install --no-save marked`). This exports verified C++ trace recordings, then renders the offline guide and repository previews. The desktop application does not require Python or Node.

The printable report is generated separately with `python tools/build_report.py` and requires ReportLab. Run `node tools/verify_docs.mjs` first with Playwright and Chrome available; this verifies the guide and renders its diagrams for the report. Both Node tools accept an absolute dependency-module path as their first argument when using an external authoring environment.

Normal tests read the supplied data and write their scenes, reports and temporary inputs beneath a separate test-output directory. They do not rewrite `data/`. Use the explicit sample exporter when preparing a release, then `python tools/build_catalogue.py --source-root "C:\path\to\exports"` to read its checked `output/` reports and regenerate the catalogue and linked run snapshots. Omitting `--source-root` reads the project's own exports.

## Troubleshooting

| Symptom | Resolution |
| --- | --- |
| Program cannot find data or fonts | Keep the executable, `data/` and `assets/` together; extract the complete folder |
| Script execution is blocked | Use the process-only PowerShell invocation in the README |
| Invalid polygon | Remove crossings, repeated adjacent vertices or folded edges; use boundary order |
| No path | Inspect blocked links and chosen endpoints; an unreachable result is valid |
| Too many observations | Narrow time/user/device filters to at most 256 displayed observations |
| A* reports h=0 fallback | Costs are below geometric lower bounds; the safe result still matches Dijkstra |
| Nothing moves after an edit | Edits invalidate the old trace; Run again |
| Text or points seem outside the view | Fit; scroll the inspector for full explanations |
| Exports cannot be written | Place the folder in a writable location and keep `output/` writable |

The application runs offline and reads local input files. It does not require a service account or install a background process.
