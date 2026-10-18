# Project documentation

The project is currently in the specification stage. These documents describe planned functionality; application code and executable tests will follow during implementation.

| Document | Purpose |
| :--- | :--- |
| [Project overview](../README.md) | Product introduction, visual overview, technology choices and academic details. |
| [Complete specification](PROJECT_SPECIFICATION.md) | Full scope, access rules, data model, algorithms, OOP design, evaluation and implementation checklist. |
| [Submitted proposal](proposal/Computational_Geometry_Visualizer_Submission.pdf) | Original academic submission, preserved without modifying its contents. |

## Workflows and implementation references

- [System architecture and language contract](PROJECT_SPECIFICATION.md#2-language-and-architecture)
- [Photo/video website and age access](PROJECT_SPECIFICATION.md#3-video-and-photo-website)
- [Data model and event contract](PROJECT_SPECIFICATION.md#4-data-model-and-event-contract)
- [Detection and evidence pipeline](PROJECT_SPECIFICATION.md#6-detection-and-evidence-pipeline)
- [Console workflow](PROJECT_SPECIFICATION.md#7-console-workflow)
- [Algorithm workflow](PROJECT_SPECIFICATION.md#8-algorithm-workflow)
- [Algorithm catalogue](PROJECT_SPECIFICATION.md#9-algorithm-catalogue)
- [Demonstration scenarios](PROJECT_SPECIFICATION.md#11-demonstration-and-scenario-catalogue)
- [Validation and acceptance criteria](PROJECT_SPECIFICATION.md#12-validation-and-acceptance-criteria)
- [Implementation order and required-feature checklist](PROJECT_SPECIFICATION.md#13-implementation-order)

## Visual assets

| Animation | Static alternative | Contents |
| :--- | :--- | :--- |
| [Console overview](media/console-overview.gif) | [Console still](media/console-overview.png) | Website activity, timeline, geographic context, behavioural grouping and human review. |
| [Algorithm Lab overview](media/geometry-overview.gif) | [Algorithm Lab still](media/geometry-overview.png) | Convex hull, geofence, Dijkstra, A*, DBSCAN and playback concepts. |

These illustrations were generated for this repository using synthetic data. They are concept presentations, not screenshots or recordings of implemented software. Static alternatives are provided for readers who prefer reduced motion. The geographic panel is a schematic coordinate layout, not a detailed basemap.

Keep presentation assets in `docs/media/` and the original academic PDF in `docs/proposal/`. Future application media and map assets belong under the separately planned `assets/` directory. Add measured validation results and actual feature recordings as implementation becomes available.
