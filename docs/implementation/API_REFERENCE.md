# HTTP API and form contract

**Status:** Proposed API version 1; no endpoint exists yet. Examples are illustrative payloads. Routes in the specification are expanded here, not presented as a deployed service.

## 1. Common behaviour

Website routes return server-rendered HTML. A successful form mutation returns **303 See Other** to a GET page; the redirected page displays the result. JSON clients use `/api/` routes with the same application services and permissions.

Use HTTPS, bounded bodies and explicit content types. Timestamps are RFC 3339 UTC; IDs are stable opaque strings in external payloads. Responses containing restricted/private material use appropriate private/no-store caching. Never put credentials, CSRF tokens or session tokens in a query string.

### Authentication

Use the same opaque server-side session model for website and native client. A session cookie is HttpOnly, Secure under HTTPS and SameSite=Lax or a stricter compatible policy. Server-side idle/absolute expiry and revocation are authoritative.

The native C++ client maintains its cookie jar in memory. It uses JSON authentication routes backed by the same `AuthService`; it does not receive a permanent all-powerful API key. A CSRF bootstrap endpoint establishes a bounded anonymous session and returns a synchronizer token. Authenticated mutations send a fresh session-associated token. Login rotates both session identity and CSRF state.

The browser obtains CSRF tokens in hidden fields rendered by the server; the C++ client sends `X-CSRF-Token`. Login/registration need protection too. Define origin checks as defence in depth, not as a replacement for tokens. Rate-limit token/session bootstrap to avoid unbounded anonymous-session creation.

### Error envelope

```json
{
  "error": {
    "code": "ANALYSIS_INVALID_PARAMETERS",
    "message": "epsilon must be positive",
    "request_id": "request-example-001",
    "fields": {"epsilon": "must be greater than zero"}
  }
}
```

`fields` is optional. Messages never contain password hashes, tokens, SQL text or stack traces. Distinguish 400 malformed input, 401 authentication required, 403 forbidden, intentional 404 concealment, 409 conflict, 413 too large, 422 semantically invalid parameters, 429 throttled and 500 unexpected failure.

### Pagination and concurrency

Use bounded `limit` and an opaque cursor based on deterministic `(time, ID)` or sequence ordering. Return `next_cursor` and the query's snapshot/cutoff where consistency matters. A cursor is validated against the requesting actor/query; it is not an authorization token.

For native business mutations, accept `Idempotency-Key`. HTML forms carry an equivalent hidden request key. Reusing a key with different meaningful input returns 409. Database uniqueness remains necessary after key expiry. Use `expected_revision` for case/profile edits that must detect concurrent changes.

## 2. Authentication, accounts and forms

| Method and route | Input / output | Permission |
| :--- | :--- | :--- |
| `GET /signup` | Registration form and anonymous CSRF context. | Public, rate-limited. |
| `POST /signup` | Username, display name, password/confirmation, birth date, CSRF. Creates member only. | Public, validated. |
| `GET /login` / `POST /login` | Login form / credential validation then session rotation. | Public; bounded attempts. |
| `POST /logout` | Revoke current session; redirect. | Session and CSRF. |
| `GET /api/auth/csrf` | Establish/read CSRF context; returns token and current authentication state. | Bounded bootstrap. |
| `POST /api/auth/login` | JSON credentials + CSRF; returns redacted profile, new CSRF context and session cookie. | Same auth policy. |
| `POST /api/auth/logout` | Revokes native session. | Session and CSRF. |
| `GET /api/me` | ID, display name, roles, capabilities and relevant access flags; no password/birth-date dump. | Authenticated. |
| `POST /settings/profile`, `/password`, `/deactivate` | Validated profile change / authenticated password change / controlled deactivation. | Owner and CSRF. |
| `POST /profiles/{id}/follow` | Desired following state; idempotent relationship change. | Active member, CSRF. |

Normalize usernames by the documented ASCII policy. Do not trim or normalize passwords silently. Invalid login responses should avoid unnecessary account-existence disclosure while still being usable.

## 3. Website and media

| Method and route | Contract |
| :--- | :--- |
| `GET /feed` | Chronological/following feed, filters, cursor; only permitted content. |
| `GET /profiles/{id}` | Public profile and permitted posts, with pagination. |
| `GET /posts/{id}` | Content detail or generic unavailable/restricted response; authorized counted-view policy applies. |
| `POST /posts` | Multipart publishing request; caption, kind, age label, file and CSRF; creator capability required. |
| `POST /posts/{id}/edit` / `/delete` | Ownership, expected revision and policy checks; no arbitrary storage paths. |
| `GET /media/{id}` | Authorization before image/video/thumbnail bytes, including Range/HEAD semantics; protected assets are not static public files. |
| `POST /posts/{id}/reaction` | Desired state `like`, `dislike` or `neutral`; identical state returns unchanged. |
| `POST /posts/{id}/comments` | Escaped plain-text comment with limits. |
| `POST /posts/{id}/reports` | Category/explanation; create or return existing report under the defined uniqueness rule. |
| `POST /reports/{id}/withdraw` | Owner transition; preserves history and does not create another first report. |

Native equivalents, if needed, use explicit `/api/` routes that call the same services. Do not implement a permissive JSON route that bypasses age policy already applied to HTML.

## 4. Analyst queries and runs

| Route | Contract |
| :--- | :--- |
| `GET /api/events` | Authorized event filters: target, actor, types, window and cursor; no credential data. |
| `GET /api/accounts/{id}/history` | Historical account context and prior events within permitted scope. |
| `GET /api/content/{id}/summary` | Redacted content/activity summary; protected media still requires separate content access. |
| `POST /api/analysis/runs` | Validate configuration, materialize bounded immutable input and enqueue work. Returns 202 with run ID/status URL. |
| `GET /api/analysis/runs/{id}` | Job state, snapshot/version metadata and complete results only after completion. |
| `POST /api/analysis/runs/{id}/cancel` | Cooperative cancellation; does not erase website events. |
| `GET /api/analysis/runs/{id}/points` | Paginated raw/scaled features, exclusions and memberships for that run. |
| `GET /api/alerts` | Authorized queue with consistent filters, case linkage and evidence contributions. |

Example planned run request:

```json
{
  "mode": "behaviour_dbscan",
  "target_ids": ["content-example-001"],
  "window": {"start": "2026-10-18T10:00:00Z", "end": "2026-10-18T10:10:00Z"},
  "feature_version": "behaviour_v1",
  "parameters": {"epsilon": 0.15, "min_points": 4, "minimum_actions": 2},
  "scale": {"reference_rate_per_minute": 10.0}
}
```

Times are synthetic examples, not a scheduled demonstration. Dataset comes from authorized server context, not unrestricted client selection. Store the actual ingestion cutoff and materialized snapshot ID in the accepted run response. Geographic mode has a different metric and an explicit `epsilon_metres` field.

## 5. Cases, exports and demo controls

`GET/POST /api/cases` lists/creates cases. `POST /api/cases/{id}/decision` requires expected revision, desired state/disposition, separate content/coordination assessments and a reason. `POST /api/moderation/actions` is independently authorized and audited; closing a case does not automatically sanction anyone.

`POST /api/exports` requests a bounded export; `GET /api/exports/{id}` returns status and an authorized download when complete. Recheck permissions at download time and include a checksum. Do not leak the private filesystem path.

Administrator-only `/api/demo/scenarios/{id}/start`, `/pause`, `/stop`, `/reset` are unavailable unless demo mode is explicitly enabled. Fixture seeding/backdating is separately controlled. A normal website request cannot assign synthetic coordinates or historical creation dates.

## 6. Contract verification

For every route, test anonymous access, wrong-role access, wrong-object access, invalid/missing CSRF where applicable, oversized input, malformed fields, retries and concurrent requests. Verify response status/content type and absence of protected bytes. See [security](SECURITY_AND_PRIVACY.md) and the [test plan](../testing/TEST_PLAN.md).
