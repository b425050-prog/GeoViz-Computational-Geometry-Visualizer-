# Implementation playbook: build one working slice at a time

> **TEMPORARY — REMOVE BEFORE SUBMISSION.** Personal preparation material. Retain during development; remove with the entire `dev-notes/` directory during final cleanup. See the [submission checklist](../docs/SUBMISSION_CHECKLIST.md).

**Status:** Development sequence with concrete exit checks. All steps are unimplemented unless the [validation record](../docs/testing/VALIDATION_RECORD.md) later says otherwise.

## 1. Prepare your working method

Read [contracts](../docs/CONTRACTS.md) before coding. Keep the website, server, console and simulator on the same data/API versions. Create small branches/commits and leave the original proposal unchanged.

For each slice, implement a domain rule, a storage/service boundary, its route or client view, then a meaningful test. Update only the affected guide/status evidence. Do not begin by generating every planned directory with empty files or claiming empty targets are a completed application.

## 2. Slice A: build foundation

Create the root CMake configuration and target separation in [setup](../docs/implementation/BUILD_AND_SETUP.md). Add simple core types and one genuine unit test for a domain invariant, such as a valid half-open window or geometry orientation. Start a minimal Drogon server and raylib window separately.

**Exit:** a clean configure/build works, a nonzero meaningful test executes, the server returns a representative response and the window opens on a supported graphical environment. Record any unavailable graphical check separately.

## 3. Slice B: identity

Implement migrations for datasets, accounts, roles and sessions. Add `Clock`, password-hasher adapter and account repositories. Build registration/login/logout through `AuthService`, then HTML views and native authentication contract.

**Exit:** two test accounts have different IDs, stored passwords are hashes, invalid input is rejected, registration grants member only, session rotation/revocation works, and anonymous/member access cannot call analyst routes.

Do not implement password reset through a hardcoded password or a publicly accessible fixture endpoint.

## 4. Slice C: one protected photo and video

Seed original/approved general and restricted sample media. Implement content rows, private storage and shared access policy before a complex feed. Add detail pages, native video playback and authorized thumbnail/range delivery. Then add feed/profile filtering and creator publishing.

**Exit:** adult/ineligible/anonymous clients receive the expected content access across detail, feed, thumbnail, direct media and API. Date-boundary tests and invalid media tests pass. Captions/comments are escaped.

## 5. Slice D: one report and its durable event

Add unique report/reaction/view state, event storage and idempotency. Submit one browser report, retry it, query the result through analyst authentication, and force an event-insertion failure to verify rollback.

**Exit:** state and events agree after retries and concurrent requests; an ordinary member cannot read the investigation API. Counts distinguish first-created reports, active reports and distinct reporters.

Now extend to reaction transitions, comments, report withdrawal and follows. Do not create charts before defining their count semantics.

## 6. Slice E: reproducible snapshots and scenarios

Create the C++ simulator with bounded parameters, seed and actor identities. Separate privileged fixture preparation from ordinary interaction requests. Create a small normal scenario and a manually triggered reporting sequence.

Implement snapshot materialization with event cutoff and required historical metadata. Keep evaluator labels elsewhere.

**Exit:** the same scenario configuration/seed yields the expected logical schedule; two analyses of one snapshot see the same input despite later website changes. No member can backdate accounts or assign locations.

## 7. Slice F: independent detection

Implement window aggregation, distinct counts, median/MAD, feature extraction and direct-scan DBSCAN as pure C++ modules. Start with hand-worked examples, then scenario inputs. Add explainable historical/target/timing summaries and priority rules.

**Exit:** reference results agree; zero MAD/denominators, missing history, all-noise and insufficient support have explicit outcomes. Labels are not accessible to detector code. A legitimate surge is not automatically classified as malicious.

## 8. Slice G: console vertical path

Implement transport/authentication, event table, one timeline and one feature plane. Add account inspection and a simple case decision. Introduce map/geographic DBSCAN, hull/region tools and evidence graph after consistent run selection works.

**Exit:** a real website interaction appears in a versioned run, an analyst explains it through linked views and records a durable case decision/export. Test stale/out-of-order responses and session expiry.

## 9. Slice H: Algorithm Lab

Implement the geometry kernel and five modes with independent result tests. Introduce typed traces, world-coordinate editing, command history, playback/checkpoints and save/load. A routing fixture validates Dijkstra/A* cost agreement before decorative animation.

**Exit:** each mode handles normal/degenerate/invalid inputs, replay final state matches computation, edits invalidate traces and operation counts do not change with playback speed.

## 10. Slice I: complete and evaluate

Add the full scenario catalogue, held-out evaluation, negative security tests, recovery/limits, keyboard/resize review and reproducible exports. Choose a reference machine and record real timings/metrics.

Use the [demo script](../docs/user/DEMONSTRATION.md), capture real feature screenshots/GIFs, update the [report evidence sections](../docs/academic/PROJECT_REPORT.md) and [validation ledger](../docs/testing/VALIDATION_RECORD.md). Keep known failures visible.

**Exit:** an independent person can follow the actual setup guide, create/login accounts, demonstrate access policy, run an investigation, replay the lab and reproduce recorded checks.

## 11. Extensions come last

Select an enhancement from the [cookbook](../docs/algorithms/EXTENSIONS.md) only when it solves a measured bottleneck or adds a clear demonstration. Implement a reference comparison and show the benefit. The core project does not need every research algorithm to be complete.

## 12. Definition of done for every slice

- Inputs, units, role/age rules and error cases are documented.
- The underlying behaviour works through a real public/service interface.
- Meaningful tests execute and their actual outcomes are recorded.
- Limits, retries and recovery are addressed for the state changed.
- No credentials or private runtime files are committed.
- UI/docs identify what is implemented, planned, failed or unrun.

The [requirements IDs](../docs/requirements/SRS.md) and [test families](../docs/testing/TEST_PLAN.md) provide traceability. The report is drafted in advance, but only working source and recorded execution can supply implementation evidence.
