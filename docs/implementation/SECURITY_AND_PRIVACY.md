# Security, access and privacy design

**Status:** Controls to implement and verify. This is not a security certification or a claim that the unbuilt application is secure.

## 1. Trust boundaries

Treat all browser/native-client input as untrusted, including hidden fields, IDs, claimed MIME types and client timestamps. The server derives identity from its session, validates access to each object and owns business timestamps. The simulator is a privileged local client; its controls must not become a registration backdoor.

The desktop console is not trusted merely because it is written in C++. It must authenticate and use the same authorization rules as other clients. Private media/database/snapshot files are not public static assets.

## 2. Permission matrix

Roles can coexist. `review_content` is an explicit capability and does not bypass adult eligibility.

| Operation | Member | Creator | Analyst | Administrator |
| :--- | :--- | :--- | :--- | :--- |
| Browse eligible published content | Yes | Yes | Yes | Yes |
| React/comment/report while active | Yes | Yes | Yes | Yes |
| Publish/edit own content | No | Yes | Only if also creator | Only with explicit publishing authorization |
| Read investigation events/features | No | No | Yes | With appropriate analysis permission |
| Review restricted media | No special access | Owner rules still apply | Capability + adult eligibility | Capability + adult eligibility |
| Record case decisions | No | No | Yes | With analyst permission |
| Moderate accounts/content | No | No | Explicit moderation capability | Explicit moderation capability |
| Assign roles / run/reset scenarios | No | No | No | Yes, demo controls require enabled demo mode |

`read_only` state blocks engagement/publishing. Deactivation denies sessions. Implement the matrix through shared policies and tests, not duplicated UI conditions.

## 3. Threats and controls

| Threat | Control | Verification |
| :--- | :--- | :--- |
| Password disclosure | Vetted Argon2id hashing, bounded verification workers and no secret logging. | Inspect storage and log output; test invalid credentials. |
| Session theft/fixation | Random tokens, server digest, rotation, expiry/revocation and HTTPS cookie flags. | Old-token reuse fails after login rotation/logout. |
| Cross-site request forgery | Synchronizer tokens for browser and cookie-backed API mutations; origin checks as additional protection. | Missing/incorrect token cannot change state. |
| SQL injection | Bound parameters and typed repository queries. | Malicious strings remain data. |
| Stored/reflected script injection | Context-appropriate escaping, plain-text comments, restrictive CSP. | Captions/usernames/comments cannot introduce executable markup. |
| Broken object access | Authorize target ownership/visibility on every request. | Changing an ID cannot expose another user's protected object. |
| Age-policy bypass | Protect metadata, thumbnails, direct media, range responses, exports and caches. | Cross-surface negative tests with denied users. |
| Upload/path attacks | Approved content validation, generated paths, size/dimension limits and staging. | Traversal, spoofed/executable/oversized files fail safely. |
| Duplicate business evidence | Constraints, state-transition logic, idempotency and transactional events. | Retries/concurrent requests do not inflate counts. |
| Unbounded resource use | Input/queue/hash/trace/upload/export limits and rate limiting. | Overload yields defined rejections, not uncontrolled growth. |
| Ground-truth leakage | Evaluator-only label storage and separate APIs. | Detectors run without access to label files/queries. |
| Unsupported sanctions | Manual review with reasons and audit. | Detection alone cannot invoke moderation. |

No remote-media import is required, reducing the need for arbitrary outbound fetching. Do not later add URL fetching without a separate design for destination validation and server-side request forgery.

## 4. Age handling

Self-declared birth dates demonstrate an access policy; they do not verify identity or legal age. Do not collect identity documents for the local academic demo. Use synthetic profiles in scenario datasets.

The policy must be central and date-correct. Updating a birth date is controlled/audited. Invalidate relevant session/cache state after eligibility changes. A video range response and thumbnail are separate access paths and both require checks.

Report validity is independent of the reporter's age. A denied request is not automatically abuse evidence, and analyst visibility should not expose unnecessary personal details.

## 5. Data minimization

Keep raw credentials out of logs, events, screenshots and exports. Export pseudonymous IDs, necessary timestamps, coarse eligibility/account-age information and provenance rather than full birth dates or raw tokens.

Coordinates in the demo are synthetic by default. If approximate geolocation is later added, retain uncertainty/source and use only necessary precision. An unknown value is not zero. Do not infer exact user location from incomplete data.

Separate raw report/comment text from feature summaries. Allow analysts only the fields needed for their role. A shared-network identifier is sensitive context and must not be treated as a reliable person identity.

## 6. Retention and deletion

Define configurable lifetimes for sessions, raw activity, snapshots, case evidence, exports and backups. The chosen values must appear in a deployment's configuration and data inventory; this document does not invent a legal retention mandate.

An account deactivation, data erasure request and full demo reset are different operations. Decide which identifiers/evidence may remain pseudonymized and which records must be removed. Apply retention to derived snapshots and backups too, rather than deleting only the account row.

Append-only audit history means ordinary application operations cannot rewrite decisions. It does not remove the need for a controlled retention lifecycle.

## 7. Local environment and secrets

Store session keys, database/private media and runtime configuration outside committed public assets. Use restricted filesystem permissions and secret injection for actual credentials. `.gitignore` reduces accidental commits but is not a security boundary or a remedy for a secret already published.

Avoid default administrator passwords. Bootstrap privileged access through an explicit local process; record the assignment without logging the supplied secret. Demo mode and remote binding are explicit settings.

## 8. Release review

Complete the negative test families in the [test plan](../testing/TEST_PLAN.md), inspect authorization on every route/media path, exercise concurrent requests, review exports and dependency licenses, then record actual evidence in the [validation record](../testing/VALIDATION_RECORD.md). Describe unresolved risks plainly rather than claiming production readiness.
