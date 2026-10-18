# Validation record and evidence ledger

**Current application status: NOT IMPLEMENTED.** This file is intentionally honest about missing execution. Documentation completeness is not proof of application correctness.

## 1. Readiness by capability

| Capability | Status | Evidence |
| :--- | :--- | :--- |
| Project specification and implementation handbooks | Documentation prepared | Repository Markdown and linked concept diagrams. |
| Website/server/database implementation | Not implemented | No source/build/migration targets exist yet. |
| Core algorithm tests | Not run | No C++ test suite yet. |
| Auth/age/media HTTP tests | Not run | No running application. |
| Detection/scenario metrics | Not measured | No executed detector or labelled evaluation output. |
| Graphical usability/performance | Not run | Concept artwork is not a working UI. |
| Production deployment/security assurance | Not assessed | Outside current evidence. |

The repository's original proposal PDF and documentation media have been checked during preparation. The documentation checks below do not constitute an application test pass.

### Documentation preparation checks before repository reorganization

Checked on **2026-10-18** against the locally prepared documentation:

| Check | Observed outcome |
| :--- | :--- |
| Markdown navigation and structure | 31 Markdown documents; all 216 local file/section links resolve; code fences and table column counts are consistent. |
| Documentation index coverage | Every other Markdown guide under `docs/` is linked from the handbook. |
| Requirement/test identifiers | 36 functional and nine non-functional requirements; 66 unique planned test cases, with all mapped families represented. These are specifications, not executed tests. |
| JSON examples | Both documented JSON payloads parse. |
| Standalone C++ interface excerpt | Hull-strategy example passes C++20 syntax checking with its required standard headers. This does not compile the application or the deliberately partial service sketch. |
| Worked numerical examples | DBSCAN neighbourhood counts and normalized feature-distance arithmetic agree with the guides. |
| Existing assets | All nine image/GIF assets decode and remain unchanged; original six-page proposal PDF remains unchanged. |

No application build, HTTP integration, scenario evaluation or graphical usability test was executed. External reference links are supplied for implementation research; the local-link check does not establish that every external site is reachable.

### Repository organization checks

After separating retained documentation from temporary preparation on **2026-10-18**:

- All 236 local file/section links in 35 Markdown documents resolve; fences, table columns and whitespace checks pass.
- The exact removal manifest covers 12 temporary files: eight marked Markdown documents and four concept assets.
- Retained documents link to temporary material only from the root README and documentation index, where cleanup instructions explicitly identify the references.
- All nine visual assets decode. The four moved concept files and original six-page proposal PDF retain their original bytes.
- The 66 planned test-case IDs remain unique. These checks validate documentation organization, not application implementation.

## 2. Per-run evidence record

Copy this structure when a real check is executed:

```text
Record ID:
Requirement/test IDs:
Source revision:
Platform / compiler / dependencies:
Working directory:
Exact command or manual steps:
Dataset generation / snapshot hash / configuration hash:
Start/end time and timezone:
Intended target and observed test count:
Passed / failed / skipped / unrun:
Exit status:
Observed behaviour and limitations:
Artifact/log/checksum references:
Reviewer and follow-up:
```

Fields are intentionally blank until there is an actual run. A failure remains recorded after a later fix; link the follow-up passing record rather than replacing history.

## 3. Detection-results table

| Dataset/split | Snapshot/config | TP/FP/FN/TN | Precision/recall/F1 | Detection delay | Status |
| :--- | :--- | :--- | :--- | :--- | :--- |
| No application evaluation yet | Not available | Not measured | Not measured | Not measured | Pending implementation |

Document account-level and campaign-level results separately. Undefined metrics are N/A with a denominator explanation, not an invented zero or perfect score.

## 4. Implementation evidence to attach

- Actual clean build and meaningful C++ test output.
- HTTP/auth/age-policy checks including direct restricted media requests.
- Dataset/scenario manifest and held-out evaluation report.
- Real console/website screenshots with mode/provenance labels.
- All five lab modes with reference outcomes and replay checks.
- Resource/latency measurements on a named reference machine.
- Known defects, optional/unrun checks and unresolved setup limitations.

## 5. Academic report handover

The [report](../academic/PROJECT_REPORT.md) already contains the narrative and design explanation. Replace its evidence-pending sections with references to real records here. Do not backfill fictitious measurements, faculty approval, signatures or implementation claims.
