# Documentation handbook

This documentation explains how to understand, implement, operate, demonstrate and report the **Spatial Security Analytics Console**. It contains a detailed design and academic report draft. The application, executable build and tests have not been implemented; planned interfaces/commands are labelled accordingly.

Use the [user guide](user/USER_GUIDE.md) for operation, [architecture](implementation/ARCHITECTURE.md) for the system design and [project report](academic/PROJECT_REPORT.md) for the academic narrative. Personal construction tutorials are separated from retained reference documentation.

## 1. Understand the project

| Document | What you will learn |
| :--- | :--- |
| [Project overview](../README.md) | Main product, visuals, roadmap and academic details. |
| [Complete specification](PROJECT_SPECIFICATION.md) | Every agreed feature, full algorithm catalogue, workflows and completion scope. |
| [Engineering contracts](CONTRACTS.md) | Shared roles/states, event counts, timestamps, age policy, features and snapshots. |
| [Requirements specification](requirements/SRS.md) | Stable functional/non-functional requirement IDs and acceptance stories. |
| [Glossary](GLOSSARY.md) | Definitions of technical and project-specific terms. |
| [References](REFERENCES.md) | Official framework/security sources and academic algorithm citations. |
| [Original proposal](proposal/Computational_Geometry_Visualizer_Submission.pdf) | Submitted academic baseline, preserved unchanged. |

## 2. Engineering reference

| Guide | Contents |
| :--- | :--- |
| [Build and setup](implementation/BUILD_AND_SETUP.md) | Toolchain/dependencies, target separation, future build flow and platform diagnosis. |
| [Architecture](implementation/ARCHITECTURE.md) | Layers, ownership, concurrency, transaction flow and engineering trade-offs. |
| [Database](implementation/DATABASE.md) | Tables/fields, keys, indexes, event consistency, migrations, backup and reset. |
| [API reference](implementation/API_REFERENCE.md) | Routes, sessions/CSRF, request/response rules, errors, pagination and examples. |
| [Security and privacy](implementation/SECURITY_AND_PRIVACY.md) | Permissions, threats, safeguards, data minimization and retention. |
| [Configuration](implementation/CONFIGURATION.md) | Proposed keys, units, validation, safe defaults and reproducibility. |
| [Operations](implementation/OPERATIONS.md) | Intended startup/shutdown, recovery, backup, reset and troubleshooting. |
| [OOP feature map](implementation/OOP_MAP.md) | Class responsibilities, patterns, ownership and C++ implementation evidence. |

## 3. Understand and implement the algorithms

| Guide | Contents |
| :--- | :--- |
| [Geometry foundations](algorithms/GEOMETRY_FOUNDATIONS.md) | Coordinates, vectors, orientation, intersections, polygon validation and numeric limits. |
| [Five core algorithms](algorithms/CORE_ALGORITHMS.md) | Procedures, hand-worked examples, trace events, assumptions and complexity. |
| [Detection pipeline](algorithms/DETECTION_PIPELINE.md) | Windows, median/MAD example, feature formulas, DBSCAN, history and explained priority. |
| [Extension cookbook](algorithms/EXTENSIONS.md) | How/when to add the complete catalogue's spatial, statistical, graph and optimization techniques. |

The required proposal modes are convex hull, polygon membership, Dijkstra, A* and DBSCAN. Their outputs are separate from a human judgement of report validity. Optional/research additions do not silently become required work.

## 4. Verify and evaluate

| Document | Purpose |
| :--- | :--- |
| [Test plan](testing/TEST_PLAN.md) | Meaningful unit/integration/UI/recovery test cases with stable IDs. |
| [Scenarios and metrics](evaluation/SCENARIOS_AND_METRICS.md) | Scenario construction, label separation, held-out evaluation, precision/recall and performance methodology. |
| [Validation record](testing/VALIDATION_RECORD.md) | Actual outcomes, evidence templates and explicitly unrun application checks. |

The sample calculations teach the mathematics; they are not measured project results. Populate the validation ledger from real execution, preserving failures, skipped and unrun checks.

## 5. Use, demonstrate and submit

| Document | Purpose |
| :--- | :--- |
| [User guide](user/USER_GUIDE.md) | Intended website, analyst and lab workflows, with common recovery steps. |
| [Demonstration guide](user/DEMONSTRATION.md) | Retained product and algorithm demonstration runbook. |
| [Submission checklist](SUBMISSION_CHECKLIST.md) | Final cleanup, as-built documentation and release verification. |
| [Academic project report](academic/PROJECT_REPORT.md) | Drafted abstract, background, scope, architecture, OOP, methods, evaluation, limitations and conclusion. |

The report narrative is prepared. Final source references, actual setup versions/commands, institutional certification where required, screenshots and measurements must come from the completed work. No document claims faculty approval or fabricates signatures/results.

## 6. Visuals and workflows

| Retained workflow | Image |
| :--- | :--- |
| System architecture | [Architecture diagram](media/system-architecture.png) |
| Console overview | [Console workflow](media/console-workflow-overview.png) |
| Algorithm overview | [Algorithm workflow](media/algorithm-workflow-overview.png) |

[Diagram sources and image index](diagrams/README.md) covers the architecture and overview/detailed workflows. PNGs display without Mermaid support. Update these diagrams against the final implementation; retain their editable sources.

## 7. Keep documentation consistent

Use the specification for scope, contracts for shared definitions and the validation ledger for observed reality. When a contract changes, update affected modules, guides, requirements/tests and run versions together. Do not mark planned features implemented based only on a diagram or a successful zero-test command.

Keep documentation/media in `docs/`; future application media belongs in `assets/`, private runtime data in ignored storage and application logic in C++. The original proposal remains unchanged for academic traceability.

<!-- BEGIN TEMPORARY DEVELOPMENT LINKS -->
## Development-only material — remove before submission

The [temporary preparation hub](../dev-notes/README.md) contains the beginner walkthrough, coding tutorials, viva questions and concept artwork. These are explicitly marked **REMOVE BEFORE SUBMISSION**, grouped in `dev-notes/` and retained until the author asks for cleanup. Follow the [submission checklist](SUBMISSION_CHECKLIST.md) when that instruction arrives. Remove this development-only section during cleanup.
<!-- END TEMPORARY DEVELOPMENT LINKS -->
