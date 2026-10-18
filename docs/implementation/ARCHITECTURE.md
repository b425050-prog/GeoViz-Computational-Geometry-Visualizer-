# Architecture and engineering decisions

**Status:** Detailed design for implementation. [Shared contracts](../CONTRACTS.md) define names, time, metrics and states.

![Planned architecture](../media/system-architecture.png)

## 1. Boundaries and dependency direction

Separate four layers:

1. **Domain:** accounts, content, events, policies, cases, geometry and algorithm values. This layer does not include Drogon or raylib headers.
2. **Application services:** commands such as register, report content, start analysis and close a case. Services coordinate domain rules and repository interfaces.
3. **Infrastructure:** SQLite repositories, private media storage, cryptographic adapters, clocks and HTTP transport.
4. **Presentation:** Drogon controllers/views and raylib screens. These translate input/output; they do not duplicate domain policy.

Dependencies point inward. A `ReportController` calls `ReportService`; the service uses `IReportRepository` and an event writer. `SqliteReportRepository` implements the interface. The DBSCAN engine receives feature points, not an HTTP request or SQL connection.

This separation makes it possible to test whether a report is duplicated without drawing a window, or to test clustering without starting a server.

## 2. Ownership of state

| State | Owner | Consumers |
| :--- | :--- | :--- |
| Accounts, roles, credentials and active sessions | Server | Authorized services only; clients receive a redacted profile. |
| Current content/reaction/report state | Server/database | Website and authorized console APIs. |
| Immutable events | Server/database | Snapshot builder and authorized event queries. |
| Frozen inputs and computed runs | Analysis service | Console and exports. |
| Local screen selection, zoom and playback position | Console | Render/input layer only. |
| Lab scene edits and command history | Algorithm Lab | Trace engine after validation/snapshotting. |
| Scenario labels | Evaluator | Metrics only, never detectors. |

Do not give the console direct SQLite access. That would bypass authorization, couple it to schema migrations and make a future separate-machine client difficult.

## 3. A report request, end to end

1. Browser submits a CSRF-protected report form containing the target, category, explanation and a request key.
2. Drogon checks request limits and resolves the session.
3. The service checks account state, target visibility, age access, report category and input limits.
4. A transaction creates or transitions the report under its unique constraint.
5. Only an effective transition appends the corresponding activity event. The transaction commits both records.
6. The server returns a redirect. A second identical submission does not multiply the state or events.
7. The analysis scheduler later snapshots committed observations. The console queries a completed versioned result.

Because events are stored in the same database transaction, the first version needs no separate message broker. A polling worker uses committed ingestion sequences. In-memory notifications may reduce latency, but they must not be the only record of work.

## 4. Concurrency model

Drogon request handlers should perform bounded parsing and asynchronous service/database coordination. Expensive password hashing and analysis must use bounded worker capacity. Never block an event-loop thread waiting for another callback on the same loop.

SQLite permits concurrent readers but serializes writes. Use short transactions, a documented busy timeout, foreign keys enabled on every connection and an intentional journaling mode. WAL can improve reader/writer coexistence, but does not create multiple simultaneous writers.

An analysis job captures a consistent read transaction, copies its bounded inputs, then releases the read transaction before long computation. Keeping a transaction open throughout rendering/analysis can hold resources and prevent healthy WAL checkpointing.

raylib drawing and GPU resource creation stay on the console's main thread. A transport worker posts completed DTOs into a bounded thread-safe queue. The UI adopts a result only if its request/run generation still matches the selected context. A slow response to an older filter selection must not replace a newer view.

## 5. Immutable runs and historical truth

An ingestion cutoff freezes which events are visible; it does not freeze account/profile/content rows that can later change. The snapshot builder must also capture required historical metadata and classification versions. Store a content-policy revision reference when it is needed to understand a report.

A run records snapshot hash, settings, metric, feature scale, versions and provenance. A case points to that immutable result. Current moderation may change the website while the historical investigation remains reproducible, subject to access/retention controls.

Use stable point IDs and deterministic ordering to break algorithm ties. Saved settings alone are insufficient if a different database state is silently loaded later.

## 6. Failure handling

| Failure | Required behaviour |
| :--- | :--- |
| Validation or policy rejection | No business mutation; return a defined error and optionally a diagnostic event. |
| Event insertion fails | Roll back the associated business mutation. |
| Client disconnects after commit | Safe retry finds existing state/idempotency result. |
| Analysis queue is full | Reject new work explicitly; retain existing durable events. |
| Worker crashes | Mark unfinished work interrupted on recovery; no partial result presented as complete. |
| Console loses connectivity | Keep the last consistent snapshot, visibly stale, and offer retry. |
| Export generation fails | Record failure; never offer a truncated file as a completed export. |

An expected domain error should become a typed service result. Unexpected exceptions are logged with a request/job ID and redacted context, then converted to a safe error response. Do not expose stack traces or database contents to members.

## 7. Important design decisions

| Decision | Reason | Trade-off |
| :--- | :--- | :--- |
| Server-rendered HTML/CSS | Respects C++-only application logic. | Form actions reload pages; no JavaScript live feed. |
| raylib desktop console | Matches proposal and gives direct C++ drawing/input. | More widget/accessibility work than a browser framework. |
| SQLite first | Simple local deployment and reproducible fixtures. | Write throughput is intentionally limited. |
| Direct-scan DBSCAN first | Transparent reference behaviour. | Quadratic work; use small bounded inputs before adding an index. |
| Separate geographic/behavioural metrics | Units and interpretations remain meaningful. | Two analysis modes must be explained in the UI. |
| Human review | Metadata does not prove report validity or intent. | The demonstration needs a review workflow, not merely alerts. |
| Routing in the lab | Gives Dijkstra/A* a valid, testable purpose. | They are not presented as the main security detector. |

## 8. Integration checklist

Before joining modules, agree on DTO versions, actor IDs, UTC units, error envelope, content-access policy, snapshot/run IDs and counting semantics. Then test one complete report flow. Do not begin by wiring every optional algorithm into a single controller.

Next: [database design](DATABASE.md) and [API reference](API_REFERENCE.md).
