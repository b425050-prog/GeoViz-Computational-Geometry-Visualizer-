# Final submission and repository hygiene checklist

**Purpose:** Retained release checklist for turning the development repository into a professional, accurate submission. This document is not a claim that the application is complete.

**Cleanup trigger:** The author will explicitly request removal. No deadline or automatic deletion is scheduled. Temporary preparation stays available until that request.

## 1. Permanent and temporary boundaries

Keep product/reference documentation under `docs/`. Temporary author tutorials, viva notes, planning walkthroughs and concept artwork are grouped under `dev-notes/` and visibly marked for removal. While it exists, its exact inventory is in `dev-notes/REMOVAL_MANIFEST.md`.

Keep the original proposal, report, requirements, API/database/architecture/security documentation, user/demo guides, algorithm explanations, tests, evaluation methods, real evidence, diagram sources and attribution. Review permanent documents for accuracy; do not delete useful documentation merely because its first version was written during planning.

## 2. Before removing preparation material

- [ ] Wait for the author's explicit cleanup instruction.
- [ ] Inspect the worktree and `dev-notes/` inventory; preserve unrelated changes and any permanent work accidentally placed there.
- [ ] Preserve a recoverable copy of any uncommitted preparation content if the author still needs it; do not rewrite Git history.
- [ ] Capture actual website/console/lab screenshots and GIFs where implemented; store published product media in `docs/media/` with truthful labels.
- [ ] Replace the root README's temporary concept placements with those real assets, or remove the placements if no verified recording exists.
- [ ] Update the permanent setup, API, schema, configuration, architecture and user instructions against the actual code.
- [ ] Fill report/source references and measured results from actual validation records; identify unresolved requirements honestly.

## 3. Cleanup transaction

- [ ] Remove the entire inspected `dev-notes/` directory, including its own README, manifest, tutorials, viva notes and four concept assets.
- [ ] Remove development-only links/sections and the temporary directory entry from root/documentation navigation.
- [ ] Search the remaining Markdown/image references for `dev-notes/`, old tutorial names and removed concept paths; repair each dependency.
- [ ] Retain this checklist as a release record, updating obsolete instructions to record the actual cleanup outcome.
- [ ] Keep permanent workflow diagrams synchronized with the final architecture; retain editable sources.

## 4. Final application and documentation checks

- [ ] Run the actual clean build and required C++ unit/integration/scenario checks; preserve underlying runner outcomes and test counts.
- [ ] Verify login/create-account, age-policy access paths, media playback, unique interaction/event counts and authorized analyst operations.
- [ ] Demonstrate a real website-to-analysis-to-case workflow and all required lab modes.
- [ ] Record evaluation datasets/configuration, false positives, failures, unrun checks and limitations.
- [ ] Resolve local file/section links and decode all published images/GIFs/PDFs.
- [ ] Check dependency versions, media/map licenses and attribution; choose/retain the repository license only with the owner's authorization.
- [ ] Exclude secrets, private runtime databases, sessions, uploads, local configuration and compiler/build outputs from the submission tree.
- [ ] Make the README lead with the implemented product, actual screenshots and verified setup/usage, while identifying deferred work accurately.
- [ ] Check the staged diff contains intended source/documentation changes only, then commit/push or prepare the submission artifact as requested.

## 5. Cleanup record

| Field | Current value |
| :--- | :--- |
| Author's removal instruction | Not yet given. |
| Temporary directory | Retained during development. |
| Date / source revision of cleanup | Pending explicit request. |
| Removed paths / link repairs | Pending. |
| Application checks and evidence | See the validation record; no application implementation yet. |
| Submission publication | Not performed by this checklist. |

Record actual outcomes here at cleanup time. A visually tidy repository must still be honest about what works and what remains incomplete.
