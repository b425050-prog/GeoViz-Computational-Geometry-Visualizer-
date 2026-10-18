# Website, console and Algorithm Lab user guide

**Status:** Guide to the intended interface. These screens/actions are not implemented yet. Final button names and screenshots must be checked against the actual application before release.

## 1. Choose your role

A member uses the photo/video website. A creator can publish permitted media. An analyst investigates activity through the desktop console. An administrator controls roles and local scenarios. Roles may coexist, but every operation still follows content, age and account-state policy.

The local demonstration uses synthetic accounts/history/locations. Do not treat a map marker as a verified person's physical position.

## 2. Create an account and log in

Open Create account. Enter a unique username, display name, password/confirmation and declared birth date. Review field errors and resubmit if needed. New accounts receive member privileges only.

Log in with the credentials. A successful login starts a revocable session. Use Log out when finished. Session expiry, account deactivation or a privilege/password change may require another login.

The website does not promise email verification/recovery. The demo uses a controlled administrator-assisted recovery process until a separate feature is implemented.

## 3. Browse and interact

The feed shows eligible published photo/video posts, newest first, with pagination. Choose a creator profile or a following feed to narrow the view. Open a post for detail, comments and report controls.

Photos have descriptions/alternative text. Videos use native HTML controls. A counted view is a deduplicated page request, not a claim about how long you watched.

Choose Like, Dislike or Clear reaction. There is one current reaction per account/post. Add a plain-text comment or a categorized report. Repeated identical submissions should not multiply state. Withdraw a report through its explicit action; previous activity remains in the historical record.

Dislikes are a feature of this original demo, not a statement about another platform's current interface.

## 4. Restricted posts

General-audience and 18+ sample posts use benign media. Restricted content requires authentication and adult eligibility under the stated calendar policy. Ineligible requests display a generic restricted/unavailable result, including direct thumbnails/media.

Declared birth date is not verified age assurance. You cannot simply toggle eligibility through a hidden field. Administrator or creator status alone does not bypass the access policy.

## 5. Publish as a creator

Use Publish to select an approved photo/video, caption and age classification. The server validates type/size and ownership before publication. An error identifies what to fix; it must not expose private storage paths.

Edit/delete actions operate only on authorized content. Historical revisions relevant to investigations remain governed by the retention/audit policy.

## 6. Begin an analyst session

Start the desktop console and authenticate with analyst permissions. Check the context bar before interpreting any chart:

- Dataset and generation.
- Live versus replay/synthetic mode.
- Target(s), time window and displayed timezone.
- Run/snapshot ID and feature/algorithm version.
- Last refresh and stale/loading/error status.

An empty dataset is different from a detector that found no concern. A stale snapshot is different from a current result.

## 7. Investigate a candidate alert

1. Select a target/window in Overview or open an alert.
2. Inspect the timeline, raw/support counts and baseline warm-up/status.
3. Open Behaviour Explorer. Read feature units, ε, MinPts, excluded support and core/border/noise labels.
4. Select a group to link the account list, history and geographic view to that same run.
5. Inspect creation timing, prior targets and synchronization. Check missing data and benign alternatives.
6. Review content/report-policy context only if authorized; valid reports can be coordinated too.
7. Compare another parameter run against the same snapshot if needed.
8. Create/update a case, record reasoning and uncertainty, and save the decision.

Noise means no density membership under that run, not “attacker.” A dense cluster is not automatically malicious. New accounts or a shared region/source alone are insufficient conclusions.

## 8. Map and region tools

World Activity displays source/uncertainty labels and an explicit unknown count. Geographic DBSCAN uses metres; behavioural DBSCAN uses normalized units. Their settings are not interchangeable.

Draw an analyst region to classify selected observations as inside/outside/boundary. Read the boundary-inclusion rule. A hull is an enclosing projected footprint; it may cover empty space and may be unavailable for unsuitable global extents.

## 9. Cases and moderation

Cases move through new, investigating, needs context and closed. A closed disposition records supported concern, benign explanation or inconclusive. Content-policy and coordination assessments are separate.

Saved decisions retain analyst, reason, evidence/run references and history. Another analyst's conflicting edit produces a revision conflict. A moderation action requires its own permission/reason and is not triggered automatically by a cluster or case score.

Export an authorized case/run summary to preserve settings and redacted evidence. A failed export is not a complete report.

## 10. Use the Algorithm Lab

Choose a mode, load a preset or edit a scene, then run validation. Hull mode accepts points; membership mode needs a simple polygon/query; routing needs a graph/start/goal; DBSCAN needs points and valid radius/support settings.

Use play/pause, next/previous, first/last and speed controls. Read the narration and operation counters. Editing a scene invalidates the old trace; undo/redo affects local scenes only. Save/load retains the scene version/settings; it does not alter website event history.

Proposed keyboard shortcuts, to verify during implementation: Space for play/pause, arrows for stepping, Home/End for first/last, Ctrl+Z/Ctrl+Y for scene undo/redo and Ctrl+S/Ctrl+O for lab save/load. Visible controls remain available; shortcuts must not activate while typing in an input field.

## 11. Recovery and help

If a view is stale, inspect connection/session state and retry. If all points are noise, inspect support, scaling and parameters instead of forcing a desired result. If baseline history is missing, collect/replay reference activity and show warm-up. If a decision times out, check its saved status before resubmitting with different input.

For setup/reset see [operations](../implementation/OPERATIONS.md). For a structured presentation see [demonstration guide](DEMONSTRATION.md).
