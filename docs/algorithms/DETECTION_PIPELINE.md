# Detection pipeline and evidence interpretation

**Status:** Version-1 analytical design. All numerical examples are hand-worked illustrations; no detector has been executed on application data.

## 1. What the pipeline decides

The pipeline answers: did activity change, which accounts look similar under a declared feature/metric, and what evidence should an analyst inspect? It does not decide factual report validity or malicious intent automatically.

Use separate outputs for volume anomaly, clustering, historical context, review priority and human disposition. Combining all of them into a single unexplained score would obscure both errors and legitimate alternatives.

## 2. Freeze the experiment

Select target(s), action/cohort filters and `[start,end)` in event time. Capture a dataset generation, ingestion cutoff and required historical metadata consistently. Materialize the bounded input snapshot and hash it before computation.

Record baseline bounds, feature scope, metric, scaling, ε, MinPts, support limits, rule version and tie policy. Historical features use only information available before the reference event/window. Late events enter later snapshots, not completed results.

## 3. Aggregate activity correctly

For a selected post, a report bucket counts distinct actors submitting first-created reports in that bucket, collapsing categories for that metric. Also show raw submissions and active reports separately. Registration, dislikes and comments use explicitly labelled independent series.

Whole-window distinct counts are recomputed over the union. If one account appears in three minute buckets, summing their distinct counts would misleadingly suggest three accounts.

Compare action rates to eligible views/audience only when the denominator is meaningful. Missing/very small denominators produce an insufficient-support result. Access-denied requests are not successful views.

## 4. Rolling median/MAD surge detector

Use prior completed bucket counts for the same target/metric. Let `b` be their median. Let `MAD` be the median of `abs(count-b)`. A robust scale is `s=max(scale_floor, 1.4826*MAD)`. Define `z=(current-b)/s`.

A starting rule triggers a candidate volume anomaly only when enough reference buckets exist, current count meets an absolute support floor and z exceeds its configured threshold. Parameters in [configuration](../implementation/CONFIGURATION.md) are starting values that need evaluation.

### Hand-worked example

Prior counts: `2,1,2,3,2,1,2,3,2,2`. Median=2. Absolute deviations are `0,1,0,1,0,1,0,1,0,0`, so MAD=0. With scale floor 1, s=1. A new bucket count of 14 gives z=12. With a minimum count of 5 and threshold 3.5, this opens a candidate investigation.

If the reference counts are all zero and current count is two, the scale floor prevents division by zero and the minimum count prevents a tiny cohort from becoming a strong alert. If too little history exists, show warm-up, not an invented score.

The constant 1.4826 relates MAD to standard deviation under a normal reference model; it does not make discrete traffic normally distributed or turn z into a calibrated probability. The method is a transparent change indicator.

Exclude the tested bucket/window from its own baseline. Freeze/reference-version the baseline so a persistent campaign does not silently redefine normality inside the same experiment. A legitimate viral surge can still trigger; that is expected and requires context.

## 5. Build behavioural features

Take accounts participating on the selected target(s). Calculate context across all their qualifying content actions in the same window, as defined in [contracts](../CONTRACTS.md).

For each u, compute canonical action count `T_u`, canonical report count `R_u`, activity rate `T_u/window_minutes` and report fraction `R_u/T_u`. Accounts below the minimum support remain visible as excluded/insufficient support, not fake noise points.

Normalize activity with a stored positive reference rate:

```text
x = clamp(log1p(activity_rate) / log1p(reference_rate), 0, 1)
y = report_fraction
```

This tames the effect of a large rate range while keeping the fraction interpretable. Reference rate is frozen for the run, not fitted to scenario labels or changed to create desired clusters. Clipping is displayed; a saturated x coordinate must not conceal the raw rate.

### Hand-worked example

In a ten-minute window, account A has 20 actions and eight canonical reports: rate=2/min, y=0.4, and x≈0.458 when the reference rate is 10/min. B has 18 actions and seven reports: rate=1.8/min, y≈0.389 and x≈0.429. Their normalized distance is approximately 0.031.

This indicates similar measured behaviour. It does not prove coordination: genuine reviewers can share those values. Histories and target/timing evidence are still necessary.

## 6. Behavioural and geographic DBSCAN

Run behavioural DBSCAN on exactly the shown scaled coordinates for version 1. Display ε/MinPts, raw values, support exclusions, role labels and population size. Larger future feature vectors require a new version and projection labels in the 2D UI.

Run geographic DBSCAN separately, using distance in metres and labelled approximate/synthetic coordinates. Missing locations do not disappear from behavioural analysis. Dense regional audiences and shared networks are legitimate alternatives.

Do not tune ε until the expected group appears and call that a successful detector. Use parameter sweeps on tuning data, then report held-out results and sensitivity. A cluster is a density-connected structure under a metric, not a fixed social entity.

## 7. Historical and relational evidence

| Signal | Calculation/interpretation | Caveat |
| :--- | :--- | :--- |
| Account age | Duration from creation to the analysed event/window reference. | New accounts may be legitimate; old accounts can coordinate. |
| Creation burst | Histogram of participating accounts' creation times. | An actual launch or promotion can explain a burst. |
| Target concentration | Share of an account's canonical actions directed to the selected target(s). | A fan/viewer may naturally focus on one creator. |
| Shared targets | Jaccard similarity: intersection size divided by union size of prior target sets, with minimum support. | One shared popular post is weak evidence; empty sets produce unknown/no evidence. |
| Synchronization | Spread/IQR of comparable first-action timestamps and repeated alignment across targets. | A public event can synchronize genuine reactions. |
| History shift | Compare current action mix with a preceding supported reference window. | Missing history is unknown; do not treat it as zero-risk or guilt. |
| Geographic spread | Distribution with provenance/unknown counts and target audience comparison. | IP location is approximate; distributed campaigns can be coordinated. |

The evidence graph connects observed actors and content. Projected similarity edges store the measure, threshold, supporting events and time scope. A graph path expresses a chain of defined relationships, not communication or causation.

## 8. Explained review priority

Start with transparent rules, not a learned guilt score. A plausible rule family uses supported volume change, a sufficiently populated behavioural group, repeated shared targeting and synchronization. Account age/context appears as supporting evidence rather than an independent automatic sanction trigger.

For example, a high-priority candidate could require a supported surge plus a behavioural group and supported repeated-target/timing evidence. A surge alone remains a volume lead. Exact thresholds must be configurable/versioned and evaluated; this example is not an empirically validated rule.

Group related contributions to avoid counting account age and a new-account flag as two independent facts. Explain which facts are observed, derived or missing. Display at least one plausible benign alternative and leave content-policy assessment to review.

## 9. Correlate alerts across windows

Cluster IDs are scoped to their run. Link candidate alerts using explicit target/time overlap and actor-set overlap, with an explainable versioned threshold. A later cluster with ID 2 is not automatically the earlier cluster with ID 2.

Treat ambiguous correlations as proposed links. Analysts can split/merge cases with an audit trail. Compare related alerts by immutable snapshots, not mutable live selection.

## 10. Failure and limitation states

Show separate states for no observations, too little baseline, insufficient feature support, all-noise, normal volume, missing location, unsupported projection, failed analysis and stale UI. Do not collapse all of these to “no attack.”

Dense benign groups, distributed adversaries, slow campaigns, account compromise, synthetic-data bias and threshold sensitivity are limitations to measure. Confidence comes from reference checks, honest false-positive reporting and inspectable evidence, not a dramatic animation.

## 11. Implementation/testing sequence

Implement pure aggregation and feature functions first. Verify the hand examples. Add DBSCAN reference tests, then frozen snapshots and rule explanations. Integrate live events only after these agree with expected values. Finally evaluate scenarios and compare volume-only/geography-only/combined approaches where implemented.

See [scenario evaluation](../evaluation/SCENARIOS_AND_METRICS.md), [test plan](../testing/TEST_PLAN.md) and [extensions](EXTENSIONS.md).
