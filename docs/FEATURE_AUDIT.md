# Existing-feature audit — 26 October 2026

Scope: repair and verify the existing second-year BTech OOP project. No new end-user features.

## Repairs

- Opening Settings with O no longer inserts `o` into its first numeric field. Modal-opening Enter is consumed instead of submitting a new dialog.
- Clearing a Lab polygon no longer clears the security fence. The feature view disables geographic-only polygon actions and geofence analysis.
- Settings, refreshes and view changes preserve unfinished geographic fence drafts. Security analysis, saving and export reject an invalid draft instead of silently using an older fence.
- Evaluation and sensitivity displays are invalidated when their input, sample, filters or reviews change.
- Resetting filters prepares the new view before committing; oversized results preserve the prior model and display.
- Imports preserve each workspace's named save destination and restore the Lab mode. Uppercase Windows extensions are accepted.
- Restored selections are checked against the restored scene. Selecting an edge clears a selected point, and vice versa.
- Fit, pan, zoom and grid rendering handle the documented coordinate limits, including a scene spanning the entire supported range.
- Frozen comparison canvases cannot accidentally edit the full-width scene using mismatched coordinates.
- Route endpoint buttons accept keyboard activation independently of mouse location. Help takes precedence over underlying modal input; review dialogs block timeline changes.
- Save As uses the matching format filter, rejects mismatched extensions before writing, and preserves the prior destination after a failed save.
- User-guide file controls and smoke counts now match the application.
- The Linux OS-input harness separates gestures across frames and holds a click long enough to be observed. Its old zero-duration click was missed, and consecutive F3 events could collapse within one frame. Snapshot assertions remain, with an additional check immediately after point insertion.

## Verification

- GCC 15.2.0 C++17 Debug and Release desktop builds; compiler warnings treated as errors.
- All eight CTest suites passed, including the 1,766-assertion engine suite and structured evidence validation.
- 333 application regression checks passed. These run production command handlers for all 60 Lab samples and all supported algorithms across nine security scenarios in both views.
- Existing application smoke checks passed for history, undo/redo, imports, settings rejection, review persistence, evidence exports and workspace transitions.
- 77 UI states rendered at 960×640 and 1440×900; all 960×640 captures inspected as contact sheets.
- All 24 offline-guide recordings and controls passed, including navigation, links, narrow layout and JavaScript-error checks.
- Native Windows interaction reproduced and verified the Settings shortcut repair, algorithm run, playback, comparison and recovery. Automated command coverage is broader than manual interaction coverage.

## Repository cleanup

Removed three obsolete 70-state UI logs, replaced by current desktop reports. Generated build caches, executable, runtime exports and packaging manifests remain ignored by Git. Per-mode scenes with identical geometry are retained because they are named, referenced comparison fixtures; removing them would break the existing sample catalogue and reproducibility checks. The offline HTML guide, printable report and their source documents serve different delivery purposes.

## Limits

Passing these checks does not prove every possible input or physical mouse/keyboard sequence is bug-free. Windows testing here does not substitute for the Linux/compiler/sanitizer jobs in GitHub Actions. Use the workflow result for the pushed commit to assess those platforms. The documented floating-point and bounded-input limitations remain.
