# Scenario design and evaluation methodology

**Status:** Evaluation protocol, not experimental findings. All cohort sizes/rates below are illustrative starting designs and must be bounded by the implemented simulator configuration.

## 1. Separate the questions being evaluated

Evaluate these tasks separately:

1. **Volume change:** did the count deviate from the declared baseline?
2. **Coordination candidate:** did the evidence rules identify a group worth investigating?
3. **Report validity:** after content review, was the complaint justified under the demo policy?
4. **System integrity:** were events, access decisions, snapshots and exports correct?

A legitimate viral surge can be a correct volume alert and an incorrect coordination alert if the latter claims an unsupported pattern. A coordinated reporting group can still submit valid reports. Do not combine these truths into one ambiguous attack label.

## 2. Scenario components

Define each scenario with a versioned ID, dataset generation, seed, logical start time, cohort sizes, account-history fixtures, target choices, action distributions, delays, locations/provenance, limits and evaluator labels.

Use a C++ pseudorandom generator with a documented algorithm/seed. Generate the logical schedule deterministically. Network timing and server-assigned IDs may vary during real execution, so preserve the actual accepted events and snapshot used for evaluation; a seed alone does not recreate arbitrary concurrent wall-clock effects.

Privileged fixture setup creates synthetic historical accounts/events under controlled import rules. Ordinary actions then pass through normal authentication/validation/rate limits. The console labels this historical/synthetic context explicitly.

## 3. Minimum scenario suite

| ID | Construction | What to inspect |
| :--- | :--- | :--- |
| S01 Normal | Varied accounts, creators, media, action mix and timing. | Baseline stability and unsupported alerts. |
| S02 Viral | A sudden audience increase with diverse histories and targets. | Correct volume signal without automatic campaign verdict. |
| S03 Genuine complaint | Many fixture users report a seeded policy/classification issue. | Separate report validity from coordination evidence. |
| S04 New-account cohort | A bounded registration burst followed by repeated target reports in aligned periods. | Timing, creation history and target overlap. |
| S05 Distributed cohort | S04-like behaviour with geographically spread synthetic locations. | Behavioural detection should not require regional density. |
| S06 Regional benign | Normal/viral activity concentrated in a target's usual region. | Geographic context alone must not justify abuse. |
| S07 Older-account cohort | Established fixture accounts change behaviour on repeated targets. | Account age is insufficient to dismiss evidence. |
| S08 Low-and-slow | Repeated small actions below a single spike threshold. | Document baseline limits and value of historical comparison. |
| S09 Mixed | Benign and campaign actors overlap in the feature plane. | False positives, uncertainty and parameter sensitivity. |
| S10 Sparse/unknown | Too little reference history, excluded feature support and missing locations. | Honest insufficient-data states. |
| S11 Access/integrity | Age-boundary requests, retries, report withdrawal, late events and restarts. | Policy/count/durability checks, not campaign accuracy. |

Use original/benign sample media. A policy-violation label can be a simulated classification problem; explicit content is unnecessary. Do not generate traffic against real external platforms.

## 4. Ground truth and split discipline

Store the intended synthetic cohort/relationship labels separately from detector inputs. Detector repository interfaces must not return labels or scenario names that reveal the expected outcome. Feature settings are chosen on tuning scenarios, then frozen for held-out seeds/settings.

Vary more than the random seed: action rates, histories, cohort size, geographic distribution, content popularity and benign overlap. Otherwise, a detector may learn the simulator's easiest pattern rather than the intended phenomenon.

Specify the positive target precisely, such as membership in an intentionally coordinated synthetic cohort. This does not establish real-world maliciousness or factual falsity. Keep valid-complaint labels separate.

## 5. Define predictions before counting results

For account-level evaluation, define whether an account is predicted positive by a specific versioned evidence rule, not merely by appearing in any DBSCAN cluster. A dense benign cluster is not automatically a positive prediction.

For campaign-level evaluation, define matching by target scope, time overlap and actor-set overlap. Select thresholds before held-out scoring. Use one-to-one matching so repeated alerts on one campaign cannot become multiple true positives. Unmatched predicted campaigns are false positives; unmatched labelled campaigns are false negatives.

Record how insufficient-support/unrun cases are handled and report their number. Do not silently exclude difficult cases to improve metrics.

## 6. Metrics and a hand-worked example

```text
precision = TP / (TP + FP)
recall    = TP / (TP + FN)
F1        = 2 * precision * recall / (precision + recall)
```

When denominators are zero, report N/A and the counts. If both precision and recall are defined as zero with nonzero relevant denominators, F1 is zero under the documented convention.

Illustration only: 20 labelled cohort accounts, with 16 correctly flagged and four missed, plus four benign accounts incorrectly flagged. TP=16, FN=4, FP=4. Precision=0.8, recall=0.8 and F1=0.8. These numbers explain the formulas; they are **not project results**.

Also report false alerts per defined window/hour, supported population coverage and detection delay. A delay in accelerated logical event time differs from wall-clock UI refresh latency; measure/label both rather than subtracting incompatible clocks.

For clustering, inspect core/border/noise agreement on unambiguous reference scenes. For alert quality, evaluate the full evidence rules. Cluster similarity metrics alone do not prove a useful investigation workflow.

## 7. Comparisons and sensitivity

Where implemented, compare volume-only, geography-only and combined behavioural/evidence approaches on the same snapshots. The expected benefit is a hypothesis to test, not a claim that combined logic must always win.

Sweep ε/MinPts, baseline thresholds and minimum support on tuning data. Publish the chosen values and held-out results. Ablations remove one evidence family at a time to inspect contribution; avoid interpreting two correlated aliases as independent improvements.

## 8. Performance experiment

Record hardware, operating system, compiler/build mode, dependency/source versions and dataset sizes. Measure ingestion throughput, request latency, analysis time, queue backlog, memory, trace size and console refresh/frame time.

Use bounded load tiers. State whether timing includes snapshot I/O, algorithm work, trace recording and rendering. Repeat measurements and summarize variability. A screenshot of an open window is not a performance result.

## 9. Reproducibility package

Each evaluation artifact includes dataset/scenario manifest, split membership, actual snapshot hash, settings/versions, prediction output, evaluator version, matching rules, confusion counts, timings and limitations. Exports exclude credentials and unnecessary personal fields.

Keep raw metrics separate from the prose report. A future C++ evaluator can emit CSV/JSON and Markdown summaries directly, reducing manual transcription; it must be implemented and checked before any generated report is treated as evidence.

Record actual runs in [validation](../testing/VALIDATION_RECORD.md). The [academic report](../academic/PROJECT_REPORT.md) references those records rather than inventing percentages.
