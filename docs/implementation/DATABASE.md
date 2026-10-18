# Database design and persistence guide

**Status:** Schema contract to implement as versioned migrations. No database or migration exists yet. SQL here explains persistence rules; application orchestration remains C++.

## 1. Storage principles

Use one server-owned SQLite database for a local environment. Every record belongs to a dataset where appropriate; normal users cannot select arbitrary datasets. Media bytes live in private filesystem storage, referenced by generated IDs rather than user-controlled paths.

Represent UTC timestamps with `INTEGER` epoch milliseconds, birth dates with validated `YYYY-MM-DD` text, identifiers with stable keys and flags with constrained integers. Do not store decimal timestamps in floating-point columns.

Enable foreign keys on **every** connection. Choose WAL intentionally and document checkpoint/backup handling. Short transactions and a bounded busy timeout matter more than increasing worker count. Use prepared statements/bound parameters; never concatenate form values into SQL.

## 2. Entity relationships

- An account has roles, capabilities and sessions; a creator owns content.
- Content has one or more media references, reactions, comments and reports.
- Accounts follow accounts through a unique relationship.
- Business operations append activity events in the same transaction.
- Analysis runs consume immutable snapshots and produce features, memberships and alerts.
- Cases link alerts/runs and append review decisions/moderation actions.
- Audit entries record privileged actions independently of UI state.
- Scenario truth belongs to evaluator-only storage and never joins detector queries.

## 3. Identity and access tables

| Table | Fields and constraints |
| :--- | :--- |
| `datasets` | ID, name, generation, provenance, created time, schema/config versions, enabled state. A reset creates a new generation. |
| `accounts` | ID, dataset ID, normalized username, display name, biography, password hash, birth date, created/updated times, state, policy revision. Unique dataset/username; username normalization is deterministic. |
| `account_roles` | Account ID, role enum, assigned time, assigning administrator. Unique account/role. |
| `account_capabilities` | Account ID, capability name, assignment metadata. Explicitly controls content review and other fine-grained permissions. |
| `sessions` | ID, account ID if authenticated, token digest, CSRF token/digest as designed, created/last-used/idle-expiry/absolute-expiry times, revoked time, authentication/privilege revision. Unique token digest. |
| `profile_revisions` | Account ID, revision, changed fields permitted in historical analysis, effective time. Never duplicate password hashes into analysis history. |

Hash unpredictable session tokens before persisting them. This is a random-token lookup digest, not a replacement for Argon2id password hashing. Never store raw cookie tokens in events or logs. An anonymous session can support registration/login CSRF protection before authentication.

Avoid a single scalar `role` column if an account can be both creator and analyst. Resolve permissions through a policy service; the database mapping alone does not decide access.

## 4. Website tables

| Table | Fields and constraints |
| :--- | :--- |
| `content` | ID, dataset, creator, kind (`photo`/`video`), caption, age label, publication state, created/published/updated times, current revision. |
| `content_revisions` | Content ID, revision, caption/classification/publication state and effective time; used for historical review where retention permits. |
| `media_assets` | ID, owner/content, generated storage key, approved MIME type, byte size, dimensions, duration if actually validated, checksum, thumbnail reference, state. No arbitrary external URL. |
| `follows` | Follower/followed IDs, created time; unique pair, no self-follow. |
| `reactions` | Actor/content IDs, state (`like`, `dislike`, `neutral`), updated time, revision; unique actor/content. |
| `comments` | ID, actor/content, plain text, created/updated/deleted times, moderation state. Apply length limits before insertion. |
| `reports` | ID, actor/content/category, explanation, state, created/updated/withdrawn times; unique actor/content/category. Reopening changes the existing row. |
| `view_buckets` | Actor/content, UTC minute bucket, first-view event ID; unique actor/content/bucket. |

Use explicit constrained values rather than arbitrary status strings. Record soft deletion/state changes where historical integrity requires it; do not rely on cascading deletes that erase investigation evidence unexpectedly.

If accounts/content are dataset-scoped, enforce same-dataset relationships in services and with suitable composite foreign keys. Referenced composite keys need matching primary/unique constraints; independent foreign keys do not by themselves prevent cross-dataset links.

## 5. Durable events

`activity_events` is the analysis source of truth for observed actions:

| Column | Meaning |
| :--- | :--- |
| `seq` | Increasing ingestion sequence, using an intentional non-reuse policy. Gaps are allowed. |
| `event_id` | Stable unique public/internal event identifier. |
| `dataset_id`, `dataset_generation` | Exact data population. |
| `actor_id` / pseudonymous actor reference | Server-resolved actor; nullable when an unauthenticated failure has no account identity. |
| `content_id` | Optional target. |
| `event_type`, `outcome` | Constrained action and success/denial/throttle result. |
| `event_time_ms`, `ingested_at_ms` | When it occurred and when it was stored. |
| `source_kind` | Browser, simulator, fixture import, moderation or system; not arbitrary client text. |
| `location_context_id` | Optional reference with provenance/uncertainty. |
| `business_revision` | Relevant content/report/account revision when applicable. |
| `request_id`, `idempotency_reference` | Traceability without credentials. |
| `payload_version`, minimal payload | Versioned extra facts; do not dump entire requests or private account rows. |

Use a sequence design such as an SQLite `INTEGER PRIMARY KEY AUTOINCREMENT` when non-reuse is needed; it still does not promise gap-free numbering. A snapshot cutoff means `seq <= cutoff`, plus dataset/window filters.

The event table is append-only during ordinary operation. Retention/reset is a separate privileged, audited lifecycle process; append-only must not become an accidental promise to retain personal data forever.

`location_contexts` stores approximate/synthetic latitude/longitude, source, accuracy/uncertainty and observation time. Missing coordinates remain null, not `(0,0)`. Validate geographic ranges.

## 6. Analysis and investigation tables

| Table | Contents |
| :--- | :--- |
| `analysis_snapshots` | Dataset/cutoff/window, frozen input metadata, snapshot format version, checksum, size and private storage reference or bounded serialized payload. |
| `analysis_runs` | Snapshot ID, algorithm/feature/rule versions, metric, parameters/scales, state, timestamps, failure code, result checksum and optional parent retry ID. |
| `feature_rows` | Run/account, raw/support counts, scaled coordinates, eligibility/exclusion reason and clipping flags. Unique run/account. |
| `cluster_memberships` | Run/point, run-local cluster ID, role (`core`, `border`, `noise`) and related neighbourhood facts. Excluded points do not get a fake noise row. |
| `alerts` | Run, target, rule/version, priority contributions, status and evidence references. |
| `cases` | ID, dataset, owner, operational state, latest disposition, created/updated times and revision for optimistic concurrency. |
| `case_evidence` | Case, immutable run/event/snapshot references and analyst annotation. |
| `case_decisions` | Case, analyst, prior/new state, disposition, separate coordination/content assessments, reason and timestamp. Append-only. |
| `moderation_actions` | Target, authorized actor, action, reason, prior/new state and optional case. Reversal is a new action. |
| `audit_log` | Actor, privileged action, target, request/job ID, timestamp and redacted details. |
| `exports` | Owner/permission scope, snapshot/run/case IDs, format, state, checksum, private storage reference and expiry. |
| `idempotency_records` | Actor, operation, key, canonical request digest, result reference and expiry. Unique actor/operation/key. |

Ground-truth cohort membership and desired outcomes belong to separate evaluator files/storage. Analysis repository APIs must not expose them, even when they share the same local machine.

## 7. Transactions that matter

### Creating a report

Begin a transaction; validate current access and state; insert the report under its unique constraint; append `report_created` only if a new report was created; persist the idempotency result; commit. On an existing report, return the documented existing state or a separate update transition. Do not blindly append another first-submission event.

If event insertion fails, the report insertion must roll back. Returning a success page before commit would allow a visible report with no corresponding durable event.

### Changing a reaction

Read/lock the effective state under the chosen transaction strategy. If the requested state equals it, return unchanged with no transition event. Otherwise update the state/revision and append `reaction_changed` atomically. Counter caches, if later added, update inside the same transaction or are derived from authoritative rows.

### Closing a case

Check actor permission and expected case revision. Append the decision and update the case's summary state in the same transaction. A conflicting revision returns 409 so another analyst's work is not silently overwritten.

## 8. Query indexes

Create indexes to match actual queries, then inspect query plans:

- Events: `(dataset_id, content_id, event_time_ms, seq)` for target windows; `(dataset_id, actor_id, event_time_ms, seq)` for history; `(dataset_id, seq)` for ingestion traversal.
- Reports: unique `(actor_id, content_id, category)` plus content/state/time for active report queues.
- Reactions: unique actor/content, plus content/state for counts.
- Content: dataset/publication/published-time/ID, creator/publication/time and age-policy filters as needed.
- Sessions: unique token digest and expiry/revocation lookup.
- Features/memberships: run/account and run/cluster.
- Cases: state/priority/updated-time/ID; decisions: case/time/ID.

Do not index every payload field without evidence. Indexes consume write time/storage; a full-text search extension is optional, not necessary for the first few demo posts.

## 9. Migrations, backup and reset

Maintain a `schema_migrations` table with ordered IDs and applied checksums. Run migrations under a controlled startup/admin path before serving traffic. Migration failures stop readiness instead of leaving a partly upgraded service.

Back up the database using a supported consistent mechanism such as SQLite's backup API. Copying only the main file while WAL writes are active can miss committed state. Include protected media and snapshot references in a tested backup/restore procedure.

A demo reset requires administrator authorization, an explicit target dataset/generation, stopped scenario producers and a recovery/export option. It must not wipe the repository or unrelated environments. Preserve the submitted PDF and project documentation independently of runtime data.

## 10. Persistence verification

Test constraints directly, two simultaneous identical actions, a simulated failure between mutation and event insertion, unknown foreign keys, cross-dataset links, session revocation, migration repeatability, snapshot recovery and backup restore. See the [test plan](../testing/TEST_PLAN.md).
