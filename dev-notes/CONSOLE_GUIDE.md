# Building the raylib console and Algorithm Lab

> **TEMPORARY — REMOVE BEFORE SUBMISSION.** Personal preparation material. Retain during development; remove with the entire `dev-notes/` directory during final cleanup. See the [submission checklist](../docs/SUBMISSION_CHECKLIST.md).

**Status:** Planned screen behaviour and C++ implementation approach. There is no running UI yet.

## 1. Start with a reliable application loop

Initialize a raylib window, load resources on the main thread, process input, update local UI state, draw and clean up through RAII wrappers. Keep server transport and analysis out of the draw loop.

Begin with a static layout and a single authorized event table. Add linked charts only after authentication, DTO parsing and stale/error handling work. A visually rich dashboard displaying inconsistent data is not an acceptable milestone.

## 2. Layout and navigation

Use a left navigation rail, top context bar, central workspace and optional inspector panel. The top bar always identifies dataset/generation, selected window, target, live/replay state, run ID and freshness.

Proposed workspaces:

| Workspace | Main view | Inspector / controls |
| :--- | :--- | :--- |
| Overview | Counts, baseline and timeline. | Candidate alerts and data quality. |
| Activity Explorer | Paginated/filterable event rows. | Event provenance and linked actor/target. |
| World Activity | Offline coordinate/map view with unknown count. | Geographic metric, hull/region settings and selection. |
| Behaviour Explorer | Feature plane and cluster roles. | ε, MinPts, support, raw features and explanations. |
| Accounts / Content | Historical timeline and allowed metadata. | Creation age, targets, report categories and policy review. |
| Evidence Graph | Bounded account/content relationships. | Edge definition, time scope and supporting observations. |
| Cases | Triage list and case detail. | Decisions, audit, moderation confirmation and exports. |
| Algorithm Lab | Editable scene and trace. | Mode, settings, narrative and operation counters. |
| Demo Manager | Scenario controls/progress. | Administrator-only bounded configuration. |

Use readable labels instead of unexplained icons. The long-form [user guide](../docs/user/USER_GUIDE.md) should use the same workspace names.

## 3. State model and linked selection

Create a `ViewContext` containing dataset, window, target filters, run ID, generation and selected point/account/cluster. A selection updates linked views through explicit messages/observers. A run-local cluster selection cannot be reused blindly after a new run arrives.

Separate server state from local presentation state. Panning, zoom, scroll and playback do not mutate events. A case decision is a server command, not a local checkbox that the UI pretends was saved.

## 4. Transport and threading

A C++ HTTP worker authenticates, polls bounded endpoints and returns owned DTOs through a bounded queue. Use request generation IDs so an older response cannot overwrite a newer filter selection. On failure, display the last consistent snapshot with a stale indicator and bounded retry/backoff.

Do not create textures, draw or manipulate graphics resources on a transport thread. Do not hold a UI mutex during a network request. Cancellation stops unnecessary future adoption/work; it cannot undo a server transaction already committed.

Session expiry returns the user to authentication while preserving harmless local navigation state. Never silently retry a moderator command with a changed body or assume a timeout means the operation did not commit; use idempotency/status checks.

## 5. Chart contracts

Timeline axes identify timezone, bucket length, metric and distinct/raw meaning. Display warm-up, missing denominator and baseline range. Compare matching snapshots/filters.

The feature plane displays normalized axes and raw values on selection. Core, border, noise and insufficient-support observations use different shapes/text, not colour alone. A higher-dimensional future projection is labelled explicitly.

The map identifies synthetic/approximate data, unknown locations and projection limits. Hulls are enclosing footprints. Region queries show boundary inclusion policy. A worldwide coordinate layout is not a metre-accurate planar calculation surface.

Graph edges identify relationship type/weight. Limit expansion to avoid freezing the interface; provide an explicit “more relationships available” state rather than silently pretending the visible subset is complete.

## 6. Lab editing and commands

Represent scene edits as commands: add/move/delete point, change edge, replace scene and edit query. Commands update a validated scene model and invalidate its trace. Undo/redo affects only local lab scenes.

Use world coordinates and stable IDs. Convert mouse coordinates through the current camera before editing. Panning/zooming should not change algorithm input.

Load sample scenes with format/version validation and size limits. A routing mode needs non-negative finite edge costs; a geofence needs a simple polygon; DBSCAN needs a positive radius and valid MinPts. Show validation errors before launching computation.

## 7. Trace playback

The engine records typed events such as orientation tests, queue changes, edge relaxation and membership transitions. The player advances a display state according to events; it does not rerun the algorithm according to frame rate.

Provide play/pause, next/previous, first/last, speed and scrub controls. Rewind reconstructs from a bounded checkpoint and applies events forward. Test that the final replayed result agrees with the computed result and that revisiting a step is deterministic.

Enforce event/memory caps. Explain a truncated/disabled detailed trace without claiming the algorithm result itself is incomplete if the engine completed safely under a separate policy.

## 8. Usability and completion checks

Handle resize, small windows, long labels, keyboard focus, reduced motion, loading/empty/error states and readable colour contrast. Never hide missing evidence behind decorative charts.

A completed console slice can authenticate, request a versioned run, inspect all linked data consistently, survive stale/out-of-order responses and submit an authorized decision with visible save status. Actual screenshots/GIFs are recorded only after these behaviours work.
