# GeoViz

### Computational Geometry Visualizer & Spatial Security Console

An offline C++17 desktop laboratory that makes algorithm decisions visible. Edit a scene, record a checked run, and inspect every step. The security console applies the same geometry engine to geographic events and device behaviour.

![Recorded geometry and clustering steps](docs/media/overview.gif)

**10 modes · 60 editable scenes · 9 security scenarios · reproducible evidence**

## Academic submission

| Field | Details |
| --- | --- |
| Student | Satyam Dhal |
| Student ID | b425050 |
| Branch / Section | Computer Science and Engineering (B) |
| Faculty | Dr. Srichandan Shobhanayak |
| Institution | [International Institute of Information Technology, Bhubaneswar](https://www.iiit-bh.ac.in/) |
| Course | Object-Oriented Programming |

## Run

Double-click **`GeoViz.exe`** in this folder. Keep `assets/` and `data/` beside it. No installation, account, or network connection is needed.

Choose an algorithm → **Sample** → **Run** → **Play**. **Sample / N** cycles through six inputs per algorithm, including tidal archipelago and terraced botanical scenarios. **Final** jumps to the answer; **Step** and **Rewind** inspect the reasoning.

To build from source on Windows with the matching raylib/GCC installation:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\build.ps1 -Run
```

The default toolchain is `C:\raylib\w64devkit`. See [build instructions](docs/USER_GUIDE.md#build-from-source) for configuration and the engine-only CMake build.

## Documentation

| Document | Purpose |
| --- | --- |
| [Interactive project guide](docs/index.html) | Offline recordings of all algorithms, with play, step, rewind and a timeline |
| [Printable project report](docs/GeoViz_Project_Report.pdf) | Formal cover and technical report covering implementation, algorithms, security and design |
| [Algorithms](docs/ALGORITHMS.md) | Reasoning, pseudocode, diagrams, costs and edge-case policies |
| [OOP & C++](docs/OOP_CPP.md) | Class relationships, feature usage and why each choice matters |
| [Security console](docs/SECURITY.md) | Event processing, units, filters, source linkage and evidence |
| [User guide](docs/USER_GUIDE.md) | Controls, building, file formats and troubleshooting |
| [Sample catalogue](docs/SAMPLES.md) | Every input, its purpose, parameters and checked result |
| [Verification](docs/VERIFICATION.md) | Test method, executed checks, limits and implementation coverage |

Open `docs/index.html` in a browser for interactive animation. GitHub renders the README's animated previews and diagrams; scripts and interactive HTML run when the guide is opened locally. Everything is bundled for offline use.

| Boundary construction | Goal-directed routing | Density expansion |
| --- | --- | --- |
| ![Hull recording](docs/media/hull.gif) | ![A-star recording](docs/media/routes.gif) | ![Indexed DBSCAN recording](docs/media/density.gif) |

## Application workspaces

![Campus event analysis](docs/media/security.png)

The **Lab** supports convex hull, geofence membership, Dijkstra, A*, DBSCAN, closest pair, k-d nearest/radius queries, indexed DBSCAN and segment collision. The **Security map** and **Security features** reuse these algorithms on filtered event observations. Click a point to inspect its original records; export a snapshot to reproduce the analysis.

![Verified security scenario recordings](docs/media/security.gif)

Nine security scenarios cover reference inputs, perimeter inspection, authentication bursts, device movement, low-activity telemetry, time-window boundaries and dense campus traffic. Their procedures and expected behaviour are documented in the [security guide](docs/SECURITY.md).

The campus layer and event records are synthetic study data. Geographic distances use local metres; behavioural distances use scaled rates. Review candidates support inspection and are not attack verdicts.

## Repository

```text
GeoViz.exe               Portable Windows application
include/                 Engine interfaces and domain models
src/                     Engine, sample generators and raylib interface
tests/                   Expected-case and deterministic reference checks
data/                    Scenes, event CSVs and offline geographic layer
docs/                    Targeted documentation and animated recordings
tools/                   Reproducible data, guide and packaging helpers
build.ps1                Build, test and optional launch
CMakeLists.txt           Desktop or engine-only build
```

Generated builds and runtime exports are ignored by Git.

The interface uses raylib and the Lato typeface. Their notices are included in [raylib license](assets/RAYLIB_LICENSE.txt) and [Lato license](assets/OFL.txt).
