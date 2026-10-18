# Engineering contracts

**Status:** Proposed version-1 contracts. These decisions make the implementation guides concrete; they do not represent implemented behaviour. Change a contract deliberately and update its dependants together. The [specification](PROJECT_SPECIFICATION.md) remains the scope authority.

## 1. Product and executable names

The product is **Spatial Security Analytics Console**. Planned CMake targets/executables are `security_server`, `security_console`, `security_simulator` and `security_tests`. These names are design decisions; no such binaries or build targets exist yet.

The console contains the Algorithm Lab as a separate workspace. The server owns business state and analysis results. The simulator creates controlled local scenarios. Desktop clients communicate through authorized HTTP APIs, never by opening the server database.

## 2. Identity, roles and account state

Use stable internal account IDs; do not use a mutable username as a foreign key. Public analysis uses a dataset-scoped pseudonymous actor ID, with its mapping stored behind restricted access.

Roles are a set: `member`, `creator`, `analyst`, `administrator`. Registration grants `member` only. A person may have multiple roles. An administrator is not automatically exempt from age/content policy. Content review requires an explicit `review_content` capability and adult eligibility when applicable.

Account states are `active`, `read_only`, `deactivated`. Read-only accounts may authenticate and browse eligible content but may not publish or submit engagement. Deactivated accounts cannot authenticate or use existing sessions. State/role/password changes invalidate affected sessions and append audit entries.

## 3. Time and age

- Persist timestamps as signed 64-bit Unix epoch milliseconds in UTC. Render APIs as RFC 3339 UTC strings with an explicit `Z` suffix.
- Default user-facing display timezone and age-policy timezone: `Asia/Kolkata`. Store the age-policy timezone with the dataset/run; never silently use a machine's local timezone.
- Birth dates are calendar dates, not timestamps. Adult eligibility begins on the eighteenth birthday in the configured policy zone. For a February 29 birth date whose eighteenth-birthday year is not a leap year, the demo uses March 1. This is a demonstration policy, not a claim about legal rules everywhere.
- Evaluate eligibility on every protected request using an injectable clock. UI display caches do not authorize media delivery.
- Windows are half-open: `start <= event_time < end`. Default demo analysis window: ten minutes. Default counting bucket: one UTC minute aligned to the epoch.
- Account age for a historical event is `event_time - created_at`, with invalid negative durations rejected or marked invalid. Current account age must not replace historical age.

Use calendar/timezone support whose availability is verified on the pinned toolchain. Never approximate eighteen years as `18 * 365` days.

## 4. Event and counting semantics

An event is an immutable observation with a stable event ID and monotonically ordered ingestion sequence. It contains the server-derived actor, action type, target if applicable, event/ingestion times, outcome, dataset and provenance. A state mutation and its event commit in one transaction.

| Event | When recorded | Analysis treatment |
| :--- | :--- | :--- |
| `account_created` | First successful registration. | Registration baseline; not a content action. |
| `login_succeeded`, `login_failed` | Authentication attempt outcome. | Separate authentication evidence; no passwords/tokens. |
| `content_view_counted` | Authorized detail-page request, at most once per actor/content/UTC-minute bucket. | Content action; not watch duration. |
| `reaction_changed` | Effective like/dislike/neutral transition. | One action per real state change; identical retry emits none. |
| `comment_created` | A new accepted comment. | Content action; text is protected separately. |
| `report_created` | First creation of actor/content/category report. | Report evidence; categories are collapsed where specified below. |
| `report_updated`, `report_withdrawn`, `report_reopened` | Valid state transition of the existing report. | Preserve history; do not create another first-submission count. |
| `access_denied`, `action_throttled` | Rejected access/action. | Diagnostic/context stream, excluded from successful-action features. |
| `content_published`, `content_hidden`, `account_restricted` | Authorized publishing/moderation transition. | Business/audit context, not member engagement. |

For a behavioural window, start with successful counted views, effective reaction changes, new comments and first-created reports. Collapse multiple report categories from the same actor on the same content within that window into **one canonical report action**, using the earliest event. Other qualifying events count once by event ID. Let `T_u` be all these canonical content actions by account u, and `R_u` their canonical report subset. Thus `0 <= R_u <= T_u`.

Raw events are never deleted to implement feature deduplication. Withdrawals do not erase historical submissions; current active-report counts are a separate metric. Effective reaction toggling remains visible behaviour and is constrained by rate limits, not silently rewritten as a single action.

For a selected post's report timeline, show distinct reporting actors per bucket. Distinct actors across the whole window are recomputed, not obtained by summing bucket distinct counts. Explicitly label raw events, current active reports, unique reporters and canonical actions as different quantities.

## 5. Analysis population and features

Select accounts with qualifying actions on the chosen target(s) in the selected window. Compute their behavioural features over **all authorized content activity in that same window**, retaining the selected-target concentration as supporting evidence. This supplies context beyond an account's behaviour on just one post. A run records both cohort selection and feature scope.

Feature version `behaviour_v1` uses:

- `activity_rate = T_u / window_minutes`.
- `report_fraction = R_u / T_u`; no value when `T_u = 0`.
- `x = clamp(log1p(activity_rate) / log1p(reference_rate), 0, 1)`.
- `y = report_fraction`.

Use a positive, frozen `reference_rate`; suggested demonstration starting value is 10 actions/minute. Store it with the run. Plot raw values as well as normalized coordinates, and identify clipped values.

Suggested starting parameters: minimum two canonical actions to enter the behavioural clustering population, ε = 0.15 in normalized Euclidean units and MinPts = 4 including self. These are **tuning defaults**, not validated optimal values. Preserve excluded accounts as insufficient-support observations; do not classify them as DBSCAN noise or benign.

Geographic DBSCAN is a separate run mode using Haversine distance and ε in metres. Missing coordinates are excluded only from that geographic computation and counted as unknown. Never append latitude/longitude to the two behavioural coordinates without a new metric/version.

## 6. Snapshot and analysis lifecycle

Each run has a dataset ID/generation, input snapshot ID/hash, event window, ingestion cutoff, settings, feature/rule/algorithm versions, clock policy, seed where needed, state and timestamps.

States: `queued`, `running`, `completed`, `failed`, `cancelled`, `interrupted`. Only `completed` exposes a complete result. A worker interrupted by process termination is not marked successful. A retry creates a new run linked to the earlier one.

Capture the event cutoff and required historical account/content metadata consistently in a database read transaction; materialize an immutable input snapshot. The cutoff alone does not freeze mutable profile/content rows. A late event may enter a later run but does not rewrite a completed run.

Case decisions reference immutable runs/evidence, not just whatever is currently on the screen. Historical snapshots containing protected information remain access-controlled and subject to retention rules.

## 7. Cases and decisions

Operational case state: `new`, `investigating`, `needs_context`, `closed`. A closed case has disposition `supported_concern`, `benign_explanation` or `inconclusive`. Together these represent the review states named in the specification.

Separate `content_policy_assessment` from `coordination_assessment`. A group can coordinate valid reports. Closing or reopening a case appends a decision; it never overwrites previous reasoning. Moderation is a separate authorized command with a reason and audit trail.

## 8. Limits and errors

Cap page sizes, upload sizes, analysis points, trace events, worker queues, simulator rates and export sizes. The configuration records exact caps. On overflow, return an explicit error or documented truncation marker; do not silently lose evidence.

Use consistent machine-readable error codes. Unauthorized credentials return 401; authenticated but forbidden operations return 403 except where 404 intentionally conceals inaccessible object existence. Return 409 for conflicting state/idempotency payloads, 413 for oversized input, 422 for invalid analysis parameters and 429 for throttling.

## 9. Shared evidence rules

Synthetic locations, scenario labels and inferred signals have distinct provenance. Ground-truth labels are evaluator-only. Priority is an explained ordering, never a probability of guilt. Country, age eligibility, young account age, a shared source, geographic clustering or DBSCAN noise alone cannot justify an automatic sanction.

These contracts are expanded in the [database](implementation/DATABASE.md), [API](implementation/API_REFERENCE.md), [detection](algorithms/DETECTION_PIPELINE.md) and [test](testing/TEST_PLAN.md) guides.
