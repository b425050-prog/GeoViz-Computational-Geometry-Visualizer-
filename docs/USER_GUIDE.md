# User guide

## Build and run

The Windows build requires CMake 3.21+, Ninja and a GCC toolchain matching the installed
raylib 5.5 library. The default is `C:\raylib\w64devkit`; CMake and Ninja must be on PATH.

```powershell
# Build the desktop application, run automated tests, and copy GeoViz.exe to the project root
powershell -NoProfile -ExecutionPolicy Bypass -File .\build.ps1

# Build, test and launch
powershell -NoProfile -ExecutionPolicy Bypass -File .\build.ps1 -Run

# Build and run the hidden desktop smoke test
powershell -NoProfile -ExecutionPolicy Bypass -File .\build.ps1 -Smoke

# Debug configuration or engine-only tests
powershell -NoProfile -ExecutionPolicy Bypass -File .\build.ps1 -DebugBuild
powershell -NoProfile -ExecutionPolicy Bypass -File .\build.ps1 -CoreOnly

# A different compatible compiler/raylib installation
.\build.ps1 -CompilerPath 'D:\raylib\bin\g++.exe' -RaylibRoot 'D:\raylib'
```

The engine can be built without graphics:

```sh
cmake -S . -B build/core -G Ninja -DGEOVIZ_BUILD_APP=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build/core --parallel 4
ctest --test-dir build/core --output-on-failure
```

For a desktop build without installed raylib, use `-DGEOVIZ_FETCH_RAYLIB=ON` in the CMake
configure command. This downloads and builds the pinned raylib 5.5 revision and requires
network access plus the platform's graphics development libraries. CMake presets also
provide `engine-release`, `engine-debug` and Linux `sanitizers` configurations.

Run `GeoViz.exe` beside `assets/` and `data/`. To specify another data root:

```powershell
.\GeoViz.exe --root 'C:\path\to\project'
```

The minimum desktop size is 960×640. Startup creates `output/`, reads the synthetic
security catalogue and restores valid recovery snapshots when available.
An invalid recovery snapshot produces a status message while the default inputs remain usable.

## Controls

| Action | Control |
| --- | --- |
| Run selected algorithm | Run / Enter |
| Play or pause recorded frames | Play / Space |
| Previous or next frame | Step buttons / Left or Right |
| First or last frame | Rewind / Home; Final / End |
| Slow down or speed up playback | [ / ] |
| Select a sample; next sample | Samples / F2; N |
| Cycle workspace; available algorithm | F3; F4 |
| Help; presentation layout | F1; F5 |
| Open applicable settings | Settings / O |
| Save current input to its path or the default session file | S |
| Save As; Open | Ctrl+S; Ctrl+O |
| Reload the default session file | L |
| Undo; redo input edits | Ctrl+Z; Ctrl+Y |
| Export checked result/evidence | Export / E |
| Compare paired algorithms on the same input | Compare / C |
| Export summaries of prior and current runs | Prior / V |
| Fit the view | Fit / F |
| Increase or decrease text size | Ctrl+plus / Ctrl+minus |
| Cycle point selection; edge selection | . / , ; F6 |
| Reset security identity filters | F7 |
| Toggle security parameter-sensitivity summary | F8 |
| Close an overlay or cancel endpoint picking | Esc |
| Exit and save recovery data | Ctrl+Q / window Close |

In Lab, click blank canvas to add a point, click a point to select it, and drag to move it.
Delete removes the selected point and its incident edges. Shift-click two existing points
to connect them; select an edge and press B to block/unblock it. R and T set the selected
point as source and goal. Settings edits route endpoints and a selected edge's weight.

Right-click sets the query. P switches the polygon tool; click vertices in boundary order
and drag to move them. Clear polygon removes the fence. The feature workspace has no polygon.
The mouse wheel zooms around the pointer, and middle-button dragging pans.
Ctrl+I inserts a point or polygon vertex at the query; Ctrl+arrows moves a selected Lab
point by one world unit. Shift+arrows moves the query by ten world units.

Security points inspect source observations rather than edit CSV records. The inspector's
review controls store a verdict and optional note for all source records linked to that
observation. The sidebar and help panel scroll when their content exceeds the visible area.
Windows supports dragging a scene, security snapshot or event CSV onto the window.

## Security inputs

`data/security_samples.csv` lists nine scenarios with their parameters and fence policy.
The remaining event CSVs and `campus_map.csv` are synthetic study data. Built-in Lab
inputs are constructed by `sampleScene`; saving one creates an editable `.scene` file.

Event CSVs require this exact header:

```csv
event_id,timestamp,user_id,device_id,latitude,longitude,requests,failures,label,summary
```

Timestamps are integer UTC seconds. IDs must be nonempty and event IDs unique.
Requests and failures are nonnegative integers, failures cannot exceed requests,
and requests cannot exceed 1,000,000,000. Truth labels are `benign`, `suspicious`
or `unknown`. CSV supports quoted commas and doubled quotes, with one record per line.

Settings accepts a UTC window, exact user/device filters (`*` means all), radii,
MinPts and feature scales. UTC fields accept epoch seconds or
`YYYY-MM-DD HH:MM:SS`. The window is [start, end), positive and at most 31 days.

The security map shows one projected event per point in metres. Feature coordinates are:

```text
window_minutes = (end - start) / 60
x = (device_requests / window_minutes) / request_scale
y = (device_failures / window_minutes) / failure_scale
```

Records are filtered before aggregation; devices with no selected records have no point.
Truth labels and analyst verdicts do not enter these coordinates.

## Save, recovery and exports

| File | Contents |
| --- | --- |
| `.scene` | `GEOVIZ 1` header; points, polygon, edges, query, radius, MinPts, source, goal and grid flag |
| `.security` | `GEOVIZ_SECURITY 2` header; settings, fence, source events and analyst reviews; loader also accepts version 1 |
| `output/run.txt` and `run.txt.scene` | Lab result summary and its exact input |
| `output/evidence.txt`, `.json`, `.security`, `.geo.scene`, `.feature.scene` | Security evidence and adjacent snapshots; suffixes are appended to `evidence.txt` |
| `output/security_selected_run.txt` and adjacent `.scene` | Selected security algorithm's result, when a completed run exists during export |
| `output/previous.txt`, `current.txt` and adjacent `.scene` files | Prior and current completed-run summaries |
| `output/recovery.*` and `recovery-history/` | Current recovery snapshots and generations retained before unsaved work is replaced |
| `output/recent.txt` | Recently opened session paths |

Save stores editable input; Run creates a result. Editing invalidates the current run.
Compare pairs Dijkstra/A* or scan/indexed DBSCAN on the same scene and synchronizes the
two frozen traces; other modes show a prior-run summary when one is available.

Do not edit saved file structure manually unless following the serializers in
`src/io.cpp` and `src/security_io.cpp`. Malformed, oversized or trailing input is rejected.
If a loaded file is rejected, the existing workspace remains available.
If too many security observations are selected, narrow the time window or identity filters.
If graphics initialization fails, check the graphics driver and matching raylib/toolchain.
