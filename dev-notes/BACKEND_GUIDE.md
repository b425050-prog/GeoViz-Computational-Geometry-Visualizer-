# Building the Drogon C++ backend

> **TEMPORARY — REMOVE BEFORE SUBMISSION.** Personal preparation material. Retain during development; remove with the entire `dev-notes/` directory during final cleanup. See the [submission checklist](../docs/SUBMISSION_CHECKLIST.md).

**Status:** Implementation tutorial. Class names are proposed; framework integration must be checked against the pinned Drogon release. No generated controller or route is currently present.

## 1. Build a thin controller first

Create the server target, link the pinned Drogon dependency and return a small HTML response from a health-independent demonstration route. Confirm that the process starts and can stop cleanly before adding authentication.

Use Drogon's documented controller/route registration model for the selected version. Keep controller responsibilities limited to parsing the transport request, resolving trusted context, calling a service and translating its result into HTML/JSON/status codes. Do not put DBSCAN or database schema creation inside request handlers.

## 2. Introduce services before adding many routes

An `AuthService` handles credentials/session lifecycle. A `ContentService` controls publication and access. A `ReportService` handles valid report transitions. An `AnalysisService` builds jobs. A `CaseService` controls investigation state.

Pass repository, clock, password-hasher and policy dependencies explicitly. Constructor injection makes the service testable with a fake clock and in-memory repository. Avoid hidden global database clients that make every test need a running server.

An illustrative service boundary, not an existing implementation:

```cpp
struct ReportCommand {
    AccountId actor;
    ContentId target;
    ReportCategory category;
    std::string explanation;
    std::string requestKey;
};

// Result and domain identifiers are project types to define first.
class ReportService {
public:
    ReportOutcome submit(const ReportCommand& command,
                         const RequestContext& context);
};
```

The caller never chooses the authenticated actor independently of the session. Construct the command from trusted request context, not by blindly deserializing an arbitrary `actor` JSON property.

## 3. Authentication implementation sequence

1. Validate username/birth-date inputs and check uniqueness.
2. Hash passwords using a vetted library's high-level Argon2id string API; preserve parameters in its standard encoded representation.
3. Commit the account and registration event transactionally.
4. On login, verify the hash through the library, apply attempt limits and consider a parameter upgrade when supported.
5. Generate unpredictable session material through a cryptographic random source; store only a lookup digest server-side.
6. Set cookie attributes, rotate pre-login session/CSRF state and record the authentication outcome without secrets.
7. Check expiry, revocation, account state and privilege revision on subsequent requests.
8. Revoke sessions on logout, deactivation and sensitive changes.

Do not use `std::hash`, plaintext, reversible encryption or a fast general-purpose digest as password storage. Password hashing is intentionally expensive, so bound concurrent hashing to avoid exhausting memory/worker capacity.

## 4. Policy filters and business checks

Filters can reject missing authentication, missing roles or oversized requests early. The application service must still check object ownership, current account state and age access because these depend on the requested object and may also be used outside that particular route.

The same `ContentAccessPolicy` protects feed queries, detail responses, thumbnails, videos and exports. Keep its return value expressive: permitted, authentication required, age-restricted or unavailable, with client-safe messaging. Do not place private captions into the error object.

## 5. Repository and transaction integration

Use Drogon's supported SQLite client facilities for the chosen build or a dedicated C++ SQLite adapter behind repositories. Select one transaction owner for a use case. Calling two repositories that each silently commit their own transaction cannot guarantee atomic report-plus-event creation.

Avoid keeping borrowed database results or request objects beyond their lifetime. Asynchronous callbacks should capture owned values/shared application services with appropriate lifetime, rather than references to stack variables.

Apply backpressure to writes and expensive queries. Return a specific failure when database contention persists; infinite retry loops are not recovery.

## 6. Analysis worker

The request validates a run, stores a queued job and returns 202. A bounded worker materializes the snapshot, calculates counts/features, runs the algorithm and persists a completed result. Cancellation checks occur between bounded units of work.

Use an explicit state transition path and keep incomplete results private. On restart, identify previously running work and mark it interrupted or recover it under a tested policy. Never infer successful completion merely because the server process is alive.

## 7. Views and error rendering

Use Drogon views to emit HTML with escaped user data and CSRF hidden fields. Validate the selected view mechanism's escaping behaviour; raw user captions must not become HTML.

Use Post/Redirect/Get after successful mutations. On validation errors, preserve safe fields, clear passwords and display field-specific messages. HTML and JSON routes should map the same domain error to consistent meanings.

## 8. Observability and completion

Assign request/job IDs, log durations and redacted failure codes, and expose minimal liveness/readiness endpoints. Readiness checks required configuration and migrations; it does not certify algorithm correctness.

The backend slice is complete when a C++ integration test can register/login, read eligible content, submit one report, retry it, and observe exactly one first-submission event through an analyst query. Then add age/media negative tests before expanding the feed.

Read next: [website](WEBSITE_GUIDE.md), [API](../docs/implementation/API_REFERENCE.md), [database](../docs/implementation/DATABASE.md).
