# Spatial Security Analytics Console

## OOP LAB project report

| Academic detail | Value |
| :--- | :--- |
| Student | Satyam Dhal |
| Student ID | b425050 |
| Branch | Computer Science and Engineering |
| College | International Institute Of Information Technology, Bhubaneswar |
| Subject | OOP LAB |
| Course Faculty | Dr. Srichandan Shobhanayak |

**Report status:** Substantive design/report draft prepared before implementation. The system is not implemented. Architecture, algorithms and evaluation methods below describe the agreed design. Implementation evidence, measurements, final screenshots and any institutional certification must be added only after they exist. The [validation record](../testing/VALIDATION_RECORD.md) is the evidence authority.

## Abstract

Photo and video platforms can experience rapid increases in dislikes, complaints and reports. Such increases can result from legitimate popularity, genuine content-policy concerns, or coordinated engagement campaigns. Volume alone does not distinguish these explanations, and geographic concentration is unreliable when audiences are regional or coordinated accounts are distributed.

This project proposes a C++ visual investigation system combining an original photo/video demonstration website, an analyst console and an Algorithm Lab. A Drogon backend will authenticate users, enforce content/age policies and record durable activity events. A raylib desktop console will combine recent-window statistics, behavioural and geographic DBSCAN, account histories, repeated-target evidence and human case review. Convex hull and point-in-polygon tools support spatial exploration, while Dijkstra and A* remain explicit routing demonstrations from the original proposal.

The design emphasizes separation of computation and presentation, deterministic snapshots, explainable results and measurable validation. A C++ simulator will supply controlled scenarios with evaluator-only labels. Evaluation will measure false positives, precision/recall, detection delay and resource use without treating clusters or anomalies as proof of malicious intent. All application logic is C++; website presentation uses server-rendered HTML/CSS without JavaScript.

## 1. Introduction

Engagement systems aggregate actions from many accounts, but aggregate counts hide important context. Fourteen reports may be a major change for one post and ordinary activity for another. Newly created accounts may be suspicious in one scenario but expected after a platform launch. A geographically dense audience may reflect a regional creator rather than coordinated abuse.

An investigation tool should make these distinctions visible. This project therefore focuses on explainable triage: identifying an unusual pattern, preserving the relevant observations and settings, and helping an analyst evaluate alternative explanations. It is not designed to infer factual falsity from a report count.

The academic contribution is the integration of object-oriented design, geometry, graph search, clustering, web services and reproducible evaluation into one understandable application. The system remains small enough for a controlled local demonstration.

## 2. Problem statement

Design a system that captures interactions from a photo/video demo website and helps an authorized analyst inspect possible coordinated reporting or engagement. It must detect recent changes, group similar behaviour, display temporal/geographic context, inspect account histories and preserve a documented decision.

The system must also satisfy the original algorithm-visualization commitment: convex hull, geofence queries, Dijkstra, A* and DBSCAN must be executable, traceable and independently testable. Their mathematical outputs must not depend on animation speed.

## 3. Objectives and scope

The required objectives are:

1. Implement account registration/login, roles and server-side age/media access.
2. Provide original photo/video content, reactions, comments and categorized reports.
3. Record consistent, immutable events alongside successful state changes.
4. Analyse activity with explicit baselines, features, clustering and evidence rules.
5. Provide linked console views and human case review with audit/export.
6. Demonstrate all five proposal algorithms with editable scenes and playback.
7. Evaluate correctness, security, false positives, performance and reproducibility.

The first release uses a local SQLite database, synthetic scenario histories/locations and an offline map asset. It excludes real social-platform automation, verified identity/age services, automatic sanctions and production-scale deployment. Optional algorithms are selected only after the complete core workflow works.

## 4. Background and related techniques

**DBSCAN** discovers density-connected groups with explicit noise and does not require a preselected number of clusters. Its usefulness depends on metric, scaling and density parameters; one global radius can struggle with variable densities. The original method is described by Ester and colleagues in the [references](../REFERENCES.md).

**Robust activity baselines** use median and median absolute deviation to compare recent counts with reference activity. A scale floor, support threshold and warm-up policy are necessary for sparse or constant counts. An anomaly indicator is not a calibrated probability.

**Computational geometry** supplies orientation, intersection, hull construction and region membership. Convex hulls enclose empty space and planar operations require an appropriate coordinate system. These assumptions are exposed rather than hidden behind decorative maps.

**Graph search** demonstrates least-cost routing under stated edge/heuristic conditions. Dijkstra and A* operate on the same lab scenes for a fair comparison. A route in an evidence graph would not independently establish communication or intent.

The project does not claim a novel clustering algorithm or superiority over deployed moderation systems. Its contribution is an inspectable educational integration and a reproducible evaluation framework.

## 5. Requirements and feasibility

The [SRS](../requirements/SRS.md) assigns stable IDs to website, analysis, review and lab requirements. Non-functional requirements cover security, bounded resource use, accessible presentation and repeatable results.

A small local dataset makes a direct-scan DBSCAN reference feasible to investigate without distributed infrastructure. That is an engineering expectation to measure, not a benchmark result. Scope is controlled by separating required functions from enhancement/optional/research work.

Key risks include implementation breadth, weak synthetic realism, numeric degeneracy, confusing age with account age, accidental future-data leakage and interpreting legitimate reporting as abuse. Layered tests, shared contracts, labelled uncertainty and a staged implementation plan address these risks.

## 6. System architecture

![Planned architecture](../media/system-architecture.png)

The browser submits ordinary HTML forms to a Drogon C++ service. The service validates permissions and records state plus events transactionally. A bounded analysis worker consumes frozen inputs and stores versioned results. A raylib C++ console authenticates through server APIs and renders linked views. The Algorithm Lab shares the core/trace library while keeping scene edits separate from live website history.

Domain and algorithm modules do not depend on raylib or HTTP. Repository interfaces isolate storage, and presentation adapters translate validated service outcomes into HTML, JSON or drawing state. This separation permits independent tests and reduces duplicated policy.

## 7. Database and event model

The database stores accounts/roles/sessions, content/media, follows, reactions, comments, reports, durable events, snapshots/runs, alerts/cases, decisions, moderation actions and exports. Unique constraints prevent duplicate state; transactions prevent a successful action from losing its event.

Event time and ingestion time remain distinct. Analysis windows are half-open and recorded with a dataset generation/cutoff. A run materializes required historical metadata because a cutoff on the event table alone does not freeze mutable content/account rows.

Ground truth belongs only to evaluation. Credentials, raw session tokens and unnecessary birth-date fields do not enter analysis exports. The detailed [database guide](../implementation/DATABASE.md) defines constraints, indexes, recovery and retention.

## 8. Detection methodology

The analysis pipeline selects a target/time population, aggregates validated events, compares activity with a historical baseline, extracts features and runs DBSCAN. It then attaches explainable history/target/timing context for review.

Version-1 behavioural features use canonical activity rate and report fraction with frozen scaling. Supporting information includes account age at the event, creation bursts, prior target sets, synchronization and geographic provenance. Geographic DBSCAN is a separate metric/distance analysis; raw longitude and behavioural fractions are not concatenated.

Review rules combine supported evidence while showing missing data and benign alternatives. The analyst separately assesses content-policy validity and coordination. A genuine mass complaint may be valid even when participants act similarly.

## 9. Algorithm Lab

| Mode | Principle | Verification |
| :--- | :--- | :--- |
| Monotone chain | Sorted lower/upper hull construction using orientation. | Convexity, containment, duplicate/collinear fixtures. |
| Polygon membership | Boundary test followed by ray-crossing parity. | Explicit edge/vertex cases and winding reversal. |
| Dijkstra | Settle least tentative cost on non-negative graphs. | Known routes, unreachable and zero-cost cases. |
| A* | Prioritize known cost plus a valid lower bound. | Cost agreement with Dijkstra under stated heuristic rules. |
| DBSCAN | Core-neighbourhood expansion with border/noise roles. | Hand-labelled density fixtures, self-counting and deterministic borders. |

Typed trace events and bounded checkpoints support pause, step and rewind. Operation counters measure engine work rather than rendered frames. Scene edits invalidate the previous trace. The [algorithm guide](../algorithms/CORE_ALGORITHMS.md) gives procedures and worked examples.

## 10. Object-oriented design

Encapsulated entities protect valid states. Policy objects centralize access rules. Services coordinate use cases, and repository interfaces separate persistence. Strategy interfaces support compatible algorithm variants; a registry creates demos; Command handles local undo/redo; observers coordinate linked views.

RAII manages files, transactions, locks and graphical resources. STL containers provide point storage, adjacency, queues, heaps and lookups. Geometry values can use meaningful operator overloads while ownership and polymorphism remain explicit.

The [OOP map](../implementation/OOP_MAP.md) explains each planned class/pattern and the source evidence to attach after implementation. It avoids fabricated inheritance hierarchies added solely to satisfy a checklist.

## 11. Website, access and user experience

The website includes create-account/login pages, feeds/profiles, original photo/video posts and engagement/reporting forms. Native HTML video controls provide playback without JavaScript. Restricted sample posts use benign media and server-side eligibility checks across metadata, thumbnails and direct media/range access.

The console provides timelines, geographic/feature views, account/content inspection, an evidence graph, cases, comparisons and the Algorithm Lab. Colour is reinforced with labels/shapes; loading, stale, empty and insufficient-data states are visible. All synthetic data is identified.

Declared age is a demonstration mechanism, not verified age assurance. An adult's newly created account and an older account owned by an ineligible user are distinct cases.

## 12. Testing and evaluation plan

Unit tests cover domain invariants, geometry, graphs, clustering, statistics and replay. Database/HTTP integration tests cover transactions, constraints, sessions, authorization, age access, idempotency, uploads and exports. Recovery/load tests verify bounded resources and interrupted jobs.

Scenarios include normal activity, legitimate virality, genuine policy complaints, new/older-account cohorts, distributed locations, low-and-slow patterns, mixed populations and sparse history. Separate tuning and held-out conditions reduce overfitting to the simulator.

Report account- and campaign-level confusion counts separately. Define alert matching/duplicate handling in advance, then calculate precision, recall, F1, false-alert rate, coverage and delay. Measure resource use on a named machine. See the [evaluation protocol](../evaluation/SCENARIOS_AND_METRICS.md).

## 13. Results and implementation evidence

**No application results are available yet.** The following evidence must come from actual execution:

| Evidence | Current status | Final source |
| :--- | :--- | :--- |
| Source modules and dependency versions | Not implemented/pinned | Repository source and build manifest. |
| Build/test counts and outcomes | Not run | Validation records and runner output. |
| Detection metrics and latency | Not measured | Held-out scenario/evaluator artifacts. |
| Website/console screenshots | Not captured | Actual running features with provenance labels. |
| Performance/resource measurements | Not measured | Named-machine benchmark records. |
| Defects and residual limitations | Design risks documented; runtime defects unknown | Validation/issue history. |

The concept GIFs illustrate intended workflows and cannot serve as implementation screenshots. The report's final evidence update should reference actual [ledger records](../testing/VALIDATION_RECORD.md), preserving failures and unrun checks.

## 14. Limitations and future work

Synthetic evaluation cannot establish effectiveness on real social networks. Legitimate coordination, shared networks, approximate geography, missing history and adversarial adaptation complicate interpretation. DBSCAN and threshold choices influence outcomes; self-declared age does not verify identity.

Enhancements include closest-pair queries, spatial indexes, spatiotemporal clustering and parameter exploration. Optional work includes alternative geometry, advanced graph summaries and review-budget ILP. Research methods need separate justification and validation before entering product decisions.

## 15. Conclusion

The design establishes a coherent C++ project in which a real demonstration website supplies events, a console supports evidence-based investigation and an Algorithm Lab makes the mathematics inspectable. Its completion will depend on correct implementation, independent checks and honest evaluation rather than the number of visual effects or algorithms listed.

The current deliverable is the detailed specification and documentation. A final implementation conclusion must be updated from measured evidence after the system is built.

## References and appendices

- [Technical and academic references](../REFERENCES.md)
- [Complete scope and catalogue](../PROJECT_SPECIFICATION.md)
- [Requirements](../requirements/SRS.md) and [engineering contracts](../CONTRACTS.md)
- [Test matrix](../testing/TEST_PLAN.md) and [validation record](../testing/VALIDATION_RECORD.md)
- [User guide](../user/USER_GUIDE.md) and [demonstration guide](../user/DEMONSTRATION.md)
- [Original submitted proposal](../proposal/Computational_Geometry_Visualizer_Submission.pdf)
