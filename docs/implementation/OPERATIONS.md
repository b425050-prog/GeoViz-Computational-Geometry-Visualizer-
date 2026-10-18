# Local operation, recovery and maintenance

**Status:** Proposed operator runbook. CLI names/options and operational endpoints below are requirements to implement, not working tools available today.

## 1. Intended process roles

Run one Drogon server and one local database/media store. Start the console as an authenticated client. Start the simulator only during an explicitly enabled demonstration. The Algorithm Lab can run locally without changing website history.

Live processes are not retained by copying a project directory or publishing an environment snapshot. Dependencies/files may persist; services must restart using documented startup steps.

## 2. Planned command contract

The implementation should expose these commands or update this runbook to its actual verified alternatives:

```text
security_server --config <private-config-path>
security_console --server <https-server-origin>
security_simulator --server <https-server-origin> --scenario <scenario-path> --seed <integer>
```

Credentials should be obtained through a secure prompt/session mechanism rather than literal passwords in command-line arguments. Bootstrap/migration/reset interfaces must be explicit, administrator-controlled and documented after implementation. Do not invent executable output in the final report.

## 3. Startup checklist

1. Confirm configuration, schema compatibility, private paths and available disk space.
2. Run or verify pending migrations under the chosen controlled procedure.
3. Start the server and inspect redacted startup logs.
4. Check minimal liveness/readiness, then a representative functional request.
5. Authenticate a member and analyst separately; verify their permission differences.
6. Start the console, confirm dataset/generation, display timezone and fresh/empty state.
7. Enable/select a bounded scenario only when needed.

Liveness means a process can respond. Readiness means its configured prerequisites are usable. Neither demonstrates algorithm correctness or completes the end-to-end acceptance tests.

## 4. During a demonstration

Show a persistent live/replay/synthetic indicator, selected dataset and last ingestion time. Keep a fixed seed/configuration for comparisons. Display bounded simulator rate and allow pause/stop. Do not change rate limits secretly to make a campaign look larger.

When comparing configurations, create new immutable runs against the same snapshot. Comparing different live populations while changing ε makes the algorithm comparison ambiguous.

## 5. Graceful shutdown and recovery

Stop scenario producers first. Stop accepting new analysis jobs, allow bounded work to finish or mark cancellation/interruption, flush durable state and close private resources. The console should stop its transport worker and release graphics resources on its main thread.

After a crash, inspect unfinished job states, database integrity/migrations and any staged media. Do not label partial jobs completed. Retrying creates a linked new run. Rebuild a missing derived cache from authoritative data; never reconstruct missing events by guessing.

## 6. Backup and restore

Use a consistent SQLite backup mechanism, then include the matching protected media/snapshots and non-secret configuration versions. Keep secrets under their own secure recovery process. Record backup dataset/generation, checksums and source revision.

Restore into a separate local environment first. Verify migrations, sample authorized media, event counts and one saved run before calling the backup usable. A database-only backup may reference media that was never copied.

## 7. Reset and retention

Dataset reset is a privileged operation with explicit confirmation of the target, stopped producers and a backup/export option. Increment generation so old cursors/run references cannot be mistaken for new data. Do not delete repository documentation or unrelated directories.

Apply the deployment's documented retention policy to sessions, activity, snapshots, exports and backups. Keep a redacted audit of privileged cleanup where appropriate. Local synthetic fixtures can have a shorter lifecycle than investigation evidence, but neither should remain unmanaged indefinitely.

## 8. Operational troubleshooting

| Observation | Action |
| :--- | :--- |
| Console is stale | Check server connectivity, authentication expiry, request IDs and queue backlog. Preserve last consistent result. |
| Duplicate-looking reports | Compare raw events, current reports and canonical counts; inspect retries/constraints before blaming DBSCAN. |
| No baseline available | Show warm-up/insufficient history, generate permitted normal reference activity and rerun. |
| All points are noise | Inspect sample size, units, scaling, ε and MinPts; do not force a desired cluster. |
| Map looks wrong | Check provenance, coordinate order/ranges, projection extent and unknown-location handling. |
| New run differs | Compare snapshot hash, cutoff, version, tie rules, parameters and late data. |
| Queue/storage limit reached | Stop or reduce producers, inspect bounded rejection state and clean up through policy. |

The [user guide](../user/USER_GUIDE.md) covers normal operation; the [validation record](../testing/VALIDATION_RECORD.md) records what has actually been tested.
