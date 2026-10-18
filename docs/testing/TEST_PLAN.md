# Test strategy and acceptance matrix

**Status:** Planned tests. No application test has run because application source/build targets do not yet exist. Actual outcomes belong in the [validation record](VALIDATION_RECORD.md).

## 1. Test layers

| Layer | What to execute | Why it matters |
| :--- | :--- | :--- |
| Core unit | Pure C++ domain, geometry, clustering, statistics and traces. | Fast reference checks without a server/display. |
| Persistence integration | Real temporary SQLite database and migrations. | Verifies constraints, transactions, retries and snapshots. |
| HTTP integration | Real Drogon process/test server with a C++ client. | Verifies routes, auth, CSRF, object/age access and serialization. |
| Scenario/evaluation | Seeded inputs, frozen snapshots, separate ground truth. | Measures the complete detector and false positives. |
| UI/manual | Browser without JavaScript and actual raylib client. | Verifies media, keyboard, resize, labels and linked views. |
| Recovery/load | Bounded producer rates, faults and restarts. | Verifies durability, limits and explicit failure states. |

Use temporary datasets/private storage per test. Inject clocks for age/window boundaries. Do not let one test depend on another's account IDs or real current date. Integration tests must use production validation/policy paths, not a permissive test-only route.

## 2. Identity and website tests

| ID | Check and expected behaviour |
| :--- | :--- |
| AUTH-01 | Valid registration stores an Argon2id hash and one member role/event; no plaintext password. |
| AUTH-02 | Invalid/duplicate usernames and malformed birth dates fail safely without partial accounts. |
| AUTH-03 | Login rotates session identity; prior tokens fail; logout/expiry/revocation deny reuse. |
| AUTH-04 | Anonymous/member/wrong-role requests cannot access analyst/admin operations. |
| AUTH-05 | Password/role/state changes invalidate affected sessions; deactivated accounts cannot authenticate. |
| AUTH-06 | Missing/invalid CSRF blocks state changes, including relevant native cookie-backed routes. |
| WEB-01 | Feeds/profiles/search paginate deterministically and omit unauthorized content. |
| WEB-02 | Captions, names and comments containing markup are displayed as text. |
| WEB-03 | Follow/comment owner actions work; modifying another object's ID does not grant ownership. |
| WEB-04 | Forms and native photo/video controls work with browser JavaScript disabled. |
| MEDIA-01 | Approved sample photo/video types work; spoofed/oversized/executable/traversal uploads fail. |
| MEDIA-02 | Protected Range/HEAD/full requests authorize correctly and return consistent status/headers. |
| MEDIA-03 | Interrupted staging/publishing recovers without exposing partial or unrelated files. |
| AGE-01 | Day before/on/after eighteenth birthday produces the documented result in the policy timezone. |
| AGE-02 | February 29, timezone-day boundaries and invalid/future birth dates follow explicit policy. |
| AGE-03 | Restricted feed/detail/thumbnail/media/API/export paths expose no protected data to denied users. |
| AGE-04 | Cached responses or role/URL changes cannot bypass eligibility; analyst review needs capability too. |

## 3. Events, APIs and persistence

| ID | Check and expected behaviour |
| :--- | :--- |
| EVENT-01 | Report and durable event commit together; forced event failure rolls back the business change. |
| EVENT-02 | Retry/concurrent identical report/reaction requests preserve unique state and transition counts. |
| EVENT-03 | Reaction changes, report categories/withdrawal/reopening and view buckets follow contracts. |
| EVENT-04 | Whole-window distinct actors are not the sum of per-bucket distinct counts. |
| EVENT-05 | Event/ingestion time, half-open boundaries and late events are handled explicitly. |
| EVENT-06 | Cross-dataset links/invalid foreign keys fail; reset generation prevents stale reference reuse. |
| API-01 | Status/content type/error envelopes match the contract and do not reveal secrets/internal SQL. |
| API-02 | Pagination is bounded/deterministic; malformed cursors do not bypass query permissions. |
| API-03 | Idempotency body conflicts and case revision conflicts return 409, not silent overwrite. |
| API-04 | Request/body/job limits return defined errors and no partial successful mutation. |

## 4. Analytical and algorithm tests

| ID | Check and expected behaviour |
| :--- | :--- |
| DETECT-01 | Hand-worked median/MAD example returns median=2, MAD=0, scale=1, score=12 for count=14. |
| DETECT-02 | Warm-up, zero MAD, tiny counts and missing denominator have explicit outcomes. |
| DETECT-03 | Feature counts, report collapse, scaling/clipping and insufficient support match contracts. |
| DETECT-04 | Historical account age/features do not use future data or current replacement values. |
| DETECT-05 | Priority contributions explain the rule and do not double-count aliases of one signal. |
| CLUSTER-01 | Six-point worked fixture has four core points, one border and one noise at stated settings. |
| CLUSTER-02 | Include-self, exact ε, duplicates, all-noise/one-cluster and provisional-noise→border work. |
| CLUSTER-03 | Ambiguous borders use the documented stable order; repeated runs are deterministic. |
| CLUSTER-04 | Geographic distance/units and unknown exclusions differ correctly from behavioural mode. |
| GEO-01 | Orientation, segment degeneracy/overlap and simple-polygon validation match known results. |
| GEO-02 | Hull is convex, contains input, handles duplicate/collinear/small scenes and returns correct corners. |
| GEO-03 | Membership returns inside/outside/boundary on edges, vertices and reversed winding. |
| GEO-04 | Haversine/projection/antimeridian policies handle known geographic fixtures and reject unsuitable hulls. |
| GRAPH-01 | Dijkstra returns route S→A→B→G at cost 5 on the documented fixture. |
| GRAPH-02 | A* with the documented consistent heuristic returns equal optimal cost. |
| GRAPH-03 | Unreachable/zero-cost graphs work; negative/nonfinite costs and invalid heuristics are rejected. |
| GRAPH-04 | Evidence graph edge supports/time scopes and bounded BFS/DFS match actual observations. |
| TRACE-01 | Replaying/rewinding typed events reconstructs the same state/final result. |
| TRACE-02 | Edit/undo/redo invalidates obsolete traces; stable input versions prevent stale adoption. |
| TRACE-03 | Playback speed leaves computed result and operation counts unchanged. |
| TRACE-04 | Trace caps/cancellation/checkpoints produce explicit outcomes and bounded memory. |

Use independent expected geometry/results and exhaustive small references where appropriate. A test that merely compares an implementation with the same helper logic can repeat its bug. Randomized property tests need fixed seeds and invariant checks, not a claim of exhaustive proof.

## 5. Investigation, evaluation and recovery

| ID | Check and expected behaviour |
| :--- | :--- |
| CASE-01 | Review transitions append decisions, preserve prior reasons and enforce permissions/revisions. |
| CASE-02 | Valid reports can coexist with coordination evidence; detection alone cannot sanction. |
| CASE-03 | Alert correlation uses target/time/actor overlap, not identical run-local cluster numbers. |
| CASE-04 | Moderation/reversal are separate authorized audited actions. |
| EXPORT-01 | Export includes reproducible settings/IDs/checksums and excludes credentials/private birth dates. |
| EXPORT-02 | Download rechecks authorization and age policy; failure/truncation is not labelled complete. |
| SCENARIO-01 | Same seed/configuration produces the defined logical schedule and valid fixture provenance. |
| SCENARIO-02 | Ordinary users cannot seed/backdate/select locations or start/reset scenarios. |
| EVAL-01 | Labels are inaccessible to feature/detector code; tuning/held-out sets are separate. |
| EVAL-02 | Account/campaign matching and duplicate-alert rules produce the expected confusion counts. |
| EVAL-03 | Legitimate surge/genuine complaints and distributed/older-account scenarios are evaluated. |
| RECOVERY-01 | Immutable snapshot survives later state/late-event changes and reloads consistently. |
| RECOVERY-02 | Crash/restart marks incomplete jobs interrupted; failed transaction leaves no half-state. |
| RECOVERY-03 | Backup/restore includes matching media/snapshots and reproduces an authorized result. |
| RECOVERY-04 | Queue/storage/producer limits apply without hidden dropped work or unbounded retries. |
| UI-01 | Out-of-order network responses cannot replace a newer selection/run. |
| UI-02 | Stale/empty/failed/insufficient states and units/provenance are visible. |
| UI-03 | Keyboard, resize, reduced motion and non-colour labels work in real clients. |

## 6. Execution and recording

Use CTest or the chosen C++ runner after implementation. Preserve the underlying exit status, record test counts and verify any output file belongs to the current run. Distinguish passed, failed, skipped, expected-failure and unrun. A zero-test success does not validate the project.

Record source revision, toolchain/dependency versions, dataset/snapshot/configuration hashes, command/working directory, timestamps, counts, failures and artifact paths. Use separate records for core, database, HTTP, scenario and graphical checks.

## 7. Completion gates

Required tests execute successfully or have a diagnosed, explicitly accepted limitation that prevents the corresponding readiness claim. Optional-extension tests apply only when that extension is selected. No percentage-accuracy or production-readiness claim belongs in the report without evidence.

See [requirements](../requirements/SRS.md), [evaluation definitions](../evaluation/SCENARIOS_AND_METRICS.md) and [validation ledger](VALIDATION_RECORD.md).
