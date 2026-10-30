# Usability review

Reviewed on **31 October 2026**, using the Windows application in an isolated
extraction folder containing spaces and Ω. The user chose automated and assistant
UI review in place of a three-person novice test. No human usability study is claimed.

## Repeatable task script and observed results

| Task | Steps and acceptance condition | Assistant UI result |
| --- | --- | --- |
| First run | Start the complete extracted app; inspect the Lab scene; Enter runs the hull | Scene and recorded result displayed |
| Keep input across algorithms | Select another algorithm without loading a sample | Input retained; caption/status corrected during review |
| Find a sample | F2, Tab, Enter; confirm the selected input replaces the scene | Keyboard focus visible; selected input loaded |
| Correct a setting | Open DBSCAN Settings; Ctrl+A, enter radius 0, Apply; correct to 40 | Invalid value rejected without changing input; valid value applied |
| Navigate fields | Tab and Shift+Tab between DBSCAN settings | Focus moved in both directions |
| Save and reopen | Ctrl+S; type a long descriptive filename; save; Ctrl+O reopens it | Exact file written and reopened in the Unicode extraction folder |
| Recover input | Close the app and restart it | Saved recovery geometry and parameters restored; UNSAVED displayed |
| Review a source record | Security map; `.` selects an observation; Benign opens a note; type text; Ctrl+A; Tab; Enter | Selection preserved the text; keyboard Save committed the note |
| Export the review | E in Security; inspect the generated evidence | Verdict, note and review time present separately from synthetic truth |
| Present and get help | F5; F1; Esc | Presentation layout and scrollable help displayed; Esc returned to the workspace |

Review findings were addressed: Ctrl+A selects instead of clearing; fields and review
notes share caret/selection behavior; invalid-settings feedback clears when the value
is edited; long paths fit their buttons; and changing algorithms retains an accurate
input caption. Original fonts and visual assets were retained.

## Automated companion checks

The portable-package test validates fresh extraction, three rendering sizes, assets,
startup failures and documentation links. Production command tests exercise rejected
imports/settings, undo/redo, failed saves, recovery failures, and closing with an open
Settings or review draft. See [Verification](VERIFICATION.md) for counts and commands.

This review establishes that these tasks worked under assistant control on the tested
Windows setup. It does not measure how quickly an unfamiliar person learns them,
assistive-technology compatibility, or behavior on every display/driver combination.
