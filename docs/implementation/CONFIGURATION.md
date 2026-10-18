# Configuration contract

**Status:** Proposed configuration schema; no parser or configuration file is implemented. Validate it before starting the service. Values below are starting points for a small local demo, not measured production limits.

## 1. Settings and units

| Group / setting | Starting choice | Meaning |
| :--- | :--- | :--- |
| `server.bind` | Loopback address | Local-only demonstration by default. |
| `server.port` | 8443 | Example HTTPS listener, configurable. |
| `server.tls_certificate`, `tls_private_key` | Secure local paths | Never commit private keys. |
| `storage.database` | `runtime/security.sqlite3` | Server-owned persistent data. |
| `storage.media`, `snapshots`, `exports` | Private runtime subdirectories | Must not be publicly served as static directories. |
| `dataset.id`, `generation` | Explicit selection | Stored with events/runs; reset advances generation. |
| `policy.age_timezone` | `Asia/Kolkata` | Calendar zone for eligibility. |
| `policy.minimum_restricted_age` | 18 | Applies to restricted sample posts. |
| `session.idle_minutes` / `absolute_hours` | 30 / 8 | Proposed inactivity/absolute limits; actual policy must be tested. |
| `analysis.window_seconds` / `bucket_seconds` | 600 / 60 | Half-open event-time windows and UTC buckets. |
| `analysis.reference_buckets` / `minimum_reference_buckets` | 20 / 10 | Prior completed buckets for baseline/warm-up. |
| `analysis.minimum_bucket_reporters` / `surge_threshold` | 5 / 3.5 | Illustrative support/robust-score thresholds, subject to evaluation. |
| `analysis.scale_floor` | 1 count | Prevents division by zero when MAD is zero. |
| `features.reference_rate_per_minute` | 10.0 | Frozen normalization reference. |
| `features.minimum_actions` | 2 | Below this, mark insufficient support. |
| `behaviour_dbscan.epsilon` / `min_points` | 0.15 / 4 | Normalized Euclidean units; includes self. |
| `geo_dbscan.epsilon_metres` | Dataset preset | Positive distance chosen for the geographic scene, never copied from behavioural ε. |
| `limits.page_size` / `maximum_page_size` | 50 / 200 | Server caps query results. |
| `limits.analysis_points` / `trace_events` | 2,000 / 100,000 | Initial bounded core demo sizes. |
| `limits.analysis_workers` / `queued_jobs` | 1 / 8 | Conservative local concurrency. |
| `limits.upload_bytes` / `export_bytes` | Explicit required values | Set after sample media/disk inspection; fail startup if absent. |
| `limits.simulator_accounts` / `actions_per_second` | Explicit bounded preset | Operator-visible caps; no unrestricted generation. |
| `console.refresh_milliseconds` | 2,000 | Proposed polling interval with backoff on errors. |
| `demo.enabled` | False unless deliberately enabled | Gates scenario setup/control routes. |

Hashing parameters, rate-limit capacities/refill periods, retention lifetimes and media types also require documented explicit settings. Use a vetted password library's parameter profiles and benchmark memory/concurrency before selecting them.

## 2. Precedence and secret handling

Define one precedence order, such as validated defaults → configuration file → explicit deployment overrides. Log effective non-secret settings and source/version, but redact secrets and private paths where necessary.

Prefer injected secret values or restricted local configuration. Do not store credentials in an example file, command-line history, scenario fixture or Markdown. Public examples should use clearly named placeholders.

## 3. Validation rules

Reject missing storage permissions, invalid timezone, negative durations, zero/negative ε, MinPts below one, limits exceeding safe bounds and incompatible metric settings. Require an explicit local-only exception if using HTTP without secure cookies during development; do not quietly disable TLS/cookie protection on a remotely bound listener.

Validate every runtime request too. Startup validation cannot make a client-supplied analysis window safe or prove that a selected content item is accessible.

## 4. Versioning and reproducibility

Store the effective analysis settings in each run, including scaling and baseline windows. A later default change must not alter old results. Secrets and irrelevant operational settings are not copied into run exports.

Keep schema, feature, algorithm, rule, snapshot and API versions distinct. Record a source revision when available. Treat unknown/newer scene or snapshot versions as unsupported input rather than guessing their meaning.

See [contracts](../CONTRACTS.md), [operations](OPERATIONS.md) and [detection](../algorithms/DETECTION_PIPELINE.md).
