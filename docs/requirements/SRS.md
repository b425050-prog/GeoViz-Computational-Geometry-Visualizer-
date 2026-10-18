# Software requirements specification

**Status:** Requirements baseline, not an implementation or acceptance report. Read alongside the [full scope](../PROJECT_SPECIFICATION.md) and [engineering contracts](../CONTRACTS.md).

## Purpose and stakeholders

The system supports a controlled investigation of potentially coordinated engagement/reporting on an original photo/video website. The developer needs reproducible inputs and isolated algorithms; members need understandable access and interaction rules; analysts need inspectable evidence; administrators need safe local scenario controls; faculty need visible OOP design and verified algorithm behaviour.

The first deployment is a single-machine academic demonstration with several browser sessions and one desktop analyst client. Production social-network scale is not a promised requirement.

## Functional requirements

Each ID is stable and should appear in implementation issues and test names. The associated test family is defined in the [test plan](../testing/TEST_PLAN.md).

| ID | Required behaviour | Verification family |
| :--- | :--- | :--- |
| FR-01 | Register an ordinary member using validated username, password and declared birth date. | AUTH |
| FR-02 | Log in/out with revocable sessions; rotate session identity after authentication and privilege changes. | AUTH |
| FR-03 | Enforce role/capability and account-state rules on every route. | AUTH |
| FR-04 | Support profile editing, password change and controlled deactivation/reset. | AUTH |
| FR-05 | Display paginated chronological and following feeds, profiles and content detail pages. | WEB |
| FR-06 | Support original photo/video media and native HTML video controls. | WEB |
| FR-07 | Allow authorized creators to publish/edit/delete with validated uploads and ownership checks. | MEDIA |
| FR-08 | Restrict 18+ content across every metadata, thumbnail, media and export path. | AGE |
| FR-09 | Support one current like/dislike/neutral reaction per actor/content. | EVENT |
| FR-10 | Support escaped plain-text comments and controlled deletion/moderation. | WEB |
| FR-11 | Support categorized reports, updates and withdrawal without duplicate inflation. | EVENT |
| FR-12 | Record validated actions and outcomes as durable, immutable events. | EVENT |
| FR-13 | Expose authorized, redacted, paginated event/history APIs. | API |
| FR-14 | Preserve UTC event/ingestion times, provenance and synthetic-location labels. | EVENT |
| FR-15 | Aggregate configurable recent windows with distinct-actor counts. | DETECT |
| FR-16 | Detect volume changes with median/MAD, support thresholds and warm-up handling. | DETECT |
| FR-17 | Extract reproducible behavioural features with frozen scaling and insufficient-support states. | DETECT |
| FR-18 | Execute reference behavioural DBSCAN with explained core/border/noise roles. | CLUSTER |
| FR-19 | Execute separate geographic DBSCAN with a geographic distance metric. | CLUSTER |
| FR-20 | Link timeline, map, feature plane, accounts and content to a consistent run. | UI |
| FR-21 | Inspect creation timing, earlier activity, repeated targets and synchronization evidence. | DETECT |
| FR-22 | Draw projected hulls and classify analyst region queries explicitly. | GEO |
| FR-23 | Build a bounded account–content evidence graph with defined edge semantics. | GRAPH |
| FR-24 | Produce explained review priorities and alternative explanations. | DETECT |
| FR-25 | Correlate overlapping alerts without treating run-local cluster IDs as permanent identity. | CASE |
| FR-26 | Create/update/reopen cases and retain append-only review decisions. | CASE |
| FR-27 | Perform manual authorized moderation with reasons and audit history. | CASE |
| FR-28 | Export redacted, reproducible run/case summaries. | EXPORT |
| FR-29 | Demonstrate monotone chain convex hull and polygon membership. | GEO |
| FR-30 | Demonstrate Dijkstra and A* on identical valid graph/grid scenes. | GRAPH |
| FR-31 | Demonstrate DBSCAN neighbourhoods, expansion queue and role transitions. | CLUSTER |
| FR-32 | Edit/save/load scenes and provide undo/redo, trace playback and deterministic rewind. | TRACE |
| FR-33 | Compare results and operation counts independently of animation speed. | TRACE |
| FR-34 | Generate reproducible local scenarios with an authorized C++ simulator. | SCENARIO |
| FR-35 | Isolate scenario ground truth from detector input and evaluate held-out seeds. | EVAL |
| FR-36 | Support bounded jobs, cancellation, failure recovery and explicit freshness states. | RECOVERY |

## Non-functional requirements

| ID | Requirement | Acceptance approach |
| :--- | :--- | :--- |
| NFR-01 | C++ application logic; Drogon backend; no-JavaScript HTML/CSS website; raylib desktop. | Repository/dependency review and browser test with scripting disabled. |
| NFR-02 | Business state and events remain consistent under retries, errors and restarts. | Database-backed integration and fault-injection tests. |
| NFR-03 | Authentication, authorization and age policy are enforced server-side. | Negative route/media tests, not just screenshots. |
| NFR-04 | Computation and presentation are separate and algorithm results are repeatable. | Framework-free core tests plus saved-snapshot replay. |
| NFR-05 | Queues, uploads, scenes, traces, exports and simulator load are bounded. | Boundary and overload tests with explicit rejection states. |
| NFR-06 | UI presents units, provenance, uncertainty, empty/error states and accessible labels. | Structured usability checklist and keyboard review. |
| NFR-07 | Personal/credential data is minimized and exports are redacted. | Data-field/retention review and export regression tests. |
| NFR-08 | Performance is measured on a named machine and dataset. | Timings, memory and refresh latency recorded without invented thresholds/results. |
| NFR-09 | Setup, OOP design, limitations and evaluation evidence are documented. | Traceable documentation/release checklist. |

## External interfaces

The browser uses HTML forms over HTTPS. The desktop client uses authenticated JSON HTTP and protected media responses. SQLite and private filesystem storage are server-owned. Local scenario files use a versioned structured format. The offline map asset is licensed and bundled; public tile services are unnecessary.

No API credentials for third-party websites are required for the first release. Future integrations must be separately designed; registering on the demo does not create real Instagram/YouTube accounts.

## Representative acceptance stories

**Registration:** Given an unused valid username and permitted birth date, submitting the form creates one member account, stores only a password hash, and produces one registration event. A retry does not create a second account. An invalid request provides a field error without echoing passwords.

**Protected content:** Given an ineligible or unauthenticated requester, direct access to a restricted thumbnail/media URL returns no protected bytes. Changing the URL, requesting a video range, reading an API, or opening a cached page must not bypass policy.

**Analysis:** Given a frozen input snapshot and configuration, two runs produce the same feature rows and compatible deterministic cluster assignments. A newly arriving late event affects a later snapshot, not the completed result.

**Investigation:** Given an alert, an analyst can see supporting and missing evidence, inspect a legitimate alternative, record a disposition and export settings/evidence. The system does not infer factual falsity from cluster membership.

**Routing lab:** Given a graph with non-negative costs and an appropriate heuristic, Dijkstra and A* return equal optimal costs, or both identify an unreachable goal. Expanded-node counts may differ but improvement is not required on every graph.

## Scope control

Optional algorithms in the specification are not silently promoted to required work. An addition must identify a user need, input contract, implementation owner, verification method, dependency impact and documentation change. Avoid enlarging scope while the first complete website-to-investigation path is still broken.

Implementation status belongs in the [validation record](../testing/VALIDATION_RECORD.md), not in optimistic wording in this requirements document.
