# Temporary-file removal manifest

> **TEMPORARY — REMOVE BEFORE SUBMISSION.** This manifest is itself temporary and is removed with `dev-notes/`.

**Trigger:** Remove only when Satyam explicitly says to perform final cleanup. No submission date or scheduled action is required. Keep all preparation material until then.

## Remove these exact paths

| Path | Why it is temporary |
| :--- | :--- |
| `dev-notes/README.md` | Development-only navigation and cleanup notice. |
| `dev-notes/REMOVAL_MANIFEST.md` | This removal inventory. |
| `dev-notes/START_HERE.md` | Personal beginner walkthrough. |
| `dev-notes/IMPLEMENTATION_PLAYBOOK.md` | Personal coding sequence. |
| `dev-notes/BACKEND_GUIDE.md` | Backend construction tutorial. |
| `dev-notes/WEBSITE_GUIDE.md` | Website construction tutorial. |
| `dev-notes/CONSOLE_GUIDE.md` | Console construction tutorial. |
| `dev-notes/VIVA_PREPARATION.md` | Personal study questions and answers. |
| `dev-notes/media/console-overview.gif` | Pre-implementation concept animation. |
| `dev-notes/media/console-overview.png` | Pre-implementation static concept. |
| `dev-notes/media/geometry-overview.gif` | Pre-implementation Algorithm Lab animation. |
| `dev-notes/media/geometry-overview.png` | Pre-implementation Algorithm Lab still. |

The intended removal unit is the entire `dev-notes/` directory, including this manifest. Inspect it first: preserve any newly added source, measured results or other permanent material that does not belong here, then update the inventory. Do not use a broad deletion pattern outside this directory.

## Update these permanent files; do not delete them

- Root `README.md`: remove the development-only entry, temporary tree listing and concept-image references; use actual screenshots/GIFs for implemented features.
- `docs/README.md`: remove its temporary-preparation link/section and present the final as-built reading path.
- `docs/PROJECT_SPECIFICATION.md`: retain academic scope/provenance and accurately identify completed versus deferred requirements.
- `docs/academic/PROJECT_REPORT.md`: keep the report and fill implementation/evaluation evidence from real records.
- `docs/testing/VALIDATION_RECORD.md`: retain actual results, failures and limitations; remove obsolete boilerplate only when replaced by evidence.
- Permanent setup/API/architecture/configuration/user guides: reconcile them with the actual implementation, not the original intent alone.

## Do not remove as temporary

Application source, tests, reproducible configuration, sample scenes, dependency locks, licenses/attribution, the original proposal, final report, requirements, shared contracts, algorithm explanations, test/evaluation methodology, actual evidence and editable workflow diagrams belong to the submitted project.

Do not erase missing-feature disclosures or failed checks merely to make the repository look finished. Removing preparation material does not make an unimplemented feature complete. Normal deletion removes files from the submitted tree; it does not rewrite previous Git commits.

Follow the retained [submission checklist](../docs/SUBMISSION_CHECKLIST.md), verify all remaining links and publish the cleanup only according to the author's instruction at that time.
