# Project glossary

These definitions use the project's [engineering contracts](CONTRACTS.md). A term's ordinary meaning can be broader than the specific implementation choice here.

| Term | Meaning in this project |
| :--- | :--- |
| Account age | Time since registration at a specified event/reference time; unrelated to the person's age. |
| Age eligibility | Declared-birth-date calendar decision for restricted access; not verified identity/age. |
| Alert | A versioned lead produced by a rule, not a final verdict. |
| Analysis run | One computation over a frozen snapshot/settings, with explicit lifecycle and version. |
| Anomaly | Unusual measured behaviour relative to a stated reference. |
| Baseline | Earlier comparable observations used as a reference. |
| Behaviour space | Coordinates derived from scaled activity features, not geography. |
| Border point | Non-core DBSCAN point reached from a core neighbourhood. |
| Canonical action | A qualifying event counted under the defined feature deduplication rules. |
| Capability | Fine-grained permission such as reviewing content; separate from age eligibility. |
| Case | Human investigation containing evidence, decisions and audit references. |
| Cluster | Density-connected observations under a chosen metric/parameters. |
| Cohort | Selected population of accounts/observations for a run. |
| Consistent heuristic | A* lower-bound estimate satisfying the edge triangle inequality and h(goal)=0. |
| Content policy | Rules used to judge whether content/report claims are justified. |
| Coordination evidence | Observations supporting a shared pattern; not proof of communication or malicious intent. |
| Core point | DBSCAN point with at least MinPts neighbours, including self in this design. |
| CSRF | Forged browser-originated state-changing request made using a victim's session context. |
| Cutoff | Highest permitted ingestion sequence in a snapshot; does not alone freeze mutable metadata. |
| Dataset generation | Version distinguishing a reset/recreated dataset from earlier references. |
| Deduplication | Preventing repeated representations from being counted as distinct intended activity. |
| Disposition | Human conclusion on a closed case: supported concern, benign explanation or inconclusive. |
| DTO | Transport data-transfer object; validated before entering domain computation. |
| Epsilon / ε | DBSCAN neighbourhood radius in its declared metric/units. |
| Event time | When an observation/action occurred under the trusted event source. |
| Feature | Measured/transformed input to analysis, such as rate or report fraction. |
| Geofence | Analyst-defined polygon used for region membership; not automatically a forbidden area. |
| Ground truth | Evaluator-only labels for controlled scenarios, never detector input. |
| Half-open window | Includes start and excludes end: `[start,end)`. |
| Haversine | Spherical point-distance formula for latitude/longitude. |
| Hull | Convex enclosure of points; can contain empty space. |
| Idempotency | Repeating the same intended operation does not repeat its effect. |
| Ingestion time | When the server durably receives/stores an event. |
| Jaccard similarity | Intersection size divided by union size for nonempty sets. |
| Label leakage | Expected outcomes/future information improperly entering detector inputs or tuning. |
| Late event | Observation arriving after an earlier ingestion cutoff/window analysis. |
| MAD | Median absolute deviation from the median; a robust spread summary. |
| Metric | Distance rule with defined units; must match clustering/index assumptions. |
| MinPts | Minimum neighbourhood count for DBSCAN core status. |
| Noise | DBSCAN nonmember under one run; not an abuse/innocence label. |
| Outbox | Durable pending-event/message record committed with business state; a separate broker is unnecessary for the initial direct-database worker. |
| Precision | Share of predicted positives that match the defined positive truth. |
| Projection | Mapping coordinates into another space; geographic planar assumptions must be stated. |
| Pseudonym | Replacement identifier with controlled linkage; not a guarantee of complete anonymity. |
| RAII | C++ resource cleanup tied to object lifetime. |
| Recall | Share of actual positives recovered by the defined predictor. |
| Revision | Version of a mutable record, used to detect conflicting updates. |
| Role | Named collection of permissions; multiple roles can coexist. |
| Snapshot | Immutable materialized observations/metadata used by an experiment. |
| Stale view | Previously valid result that is no longer fresh or does not match live availability. |
| Strategy | Interface allowing compatible alternative implementations. |
| Synthetic | Generated/fixture data explicitly labelled as such. |
| Trace | Typed record of algorithm decisions used for explanation/replay. |
| Transaction | Atomic database unit whose changes all commit or all roll back. |
| View | Deduplicated authorized content-page request in this project; not watch duration. |
| WAL | SQLite write-ahead log mode; supports concurrent readers but not unlimited writers. |
| Warm-up | Too little reference history to apply the intended baseline reliably. |

[Documentation index](README.md) · [Engineering contracts](CONTRACTS.md)
