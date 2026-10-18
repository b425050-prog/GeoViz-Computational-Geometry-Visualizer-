# Start here: understand and build the project

> **TEMPORARY — REMOVE BEFORE SUBMISSION.** Personal preparation material. Retain during development; remove with the entire `dev-notes/` directory during final cleanup. See the [submission checklist](../docs/SUBMISSION_CHECKLIST.md).

**Document status:** Implementation handbook for the agreed design. The application does not exist yet. Examples describe intended behaviour; they are not screenshots, working endpoints or test results.

## What you are building

Imagine that a video receives many reports in a short time. The website records who acted, which content they acted on and when. The server groups that activity into a window, compares it with earlier activity, and looks for accounts with similar behaviour. The analyst opens a desktop console to inspect the surge, account histories and content-policy context before deciding what it means.

There are three connected products, not three unrelated assignments:

1. **The website** creates authentic demo interactions: accounts, photos, videos, views, likes/dislikes, comments and reports.
2. **The security console** explains activity using statistics, DBSCAN, geometry and human investigation.
3. **The Algorithm Lab** teaches and validates the algorithms through editable scenes and recorded decisions.

All application logic is C++. Drogon serves the backend and HTML views. raylib draws the console/lab. The website uses HTML/CSS forms and native video controls, without JavaScript. SQLite stores the local demonstration data.

## A useful mental picture

Think of the system as a laboratory notebook. The website produces observations. The analysis engine performs an experiment using explicitly recorded settings. The console shows those observations and experimental results. The analyst writes a conclusion. Saving a run must preserve the observations and settings, even when the live website later changes.

An algorithm does not identify a false report by itself. DBSCAN identifies density in a chosen space. A timeline identifies volume changes. A hull encloses points. Their outputs become evidence only after their assumptions and the content context are considered.

## Read in this order

| Your question | Read |
| :--- | :--- |
| What exactly must be delivered? | [Specification](../docs/PROJECT_SPECIFICATION.md) and [requirements](../docs/requirements/SRS.md). |
| Which definitions must every module share? | [Engineering contracts](../docs/CONTRACTS.md). |
| What should I implement first? | [Implementation playbook](IMPLEMENTATION_PLAYBOOK.md). |
| How do the parts communicate? | [Architecture](../docs/implementation/ARCHITECTURE.md), [database](../docs/implementation/DATABASE.md), [API](../docs/implementation/API_REFERENCE.md). |
| How do I write the actual modules? | [Backend](BACKEND_GUIDE.md), [website](WEBSITE_GUIDE.md), [console](CONSOLE_GUIDE.md). |
| How do the algorithms work? | [Foundations](../docs/algorithms/GEOMETRY_FOUNDATIONS.md), [core algorithms](../docs/algorithms/CORE_ALGORITHMS.md), [detection](../docs/algorithms/DETECTION_PIPELINE.md). |
| How do I prove it works? | [Test plan](../docs/testing/TEST_PLAN.md), [scenarios and metrics](../docs/evaluation/SCENARIOS_AND_METRICS.md), [validation record](../docs/testing/VALIDATION_RECORD.md). |
| What goes into the submission? | [Academic report](../docs/academic/PROJECT_REPORT.md), [demonstration guide](../docs/user/DEMONSTRATION.md) and [viva preparation](VIVA_PREPARATION.md), [OOP map](../docs/implementation/OOP_MAP.md). |

## Learn the minimum prerequisites

For C++, understand classes, constructors, `const`, references, `std::vector`, `std::string`, `std::optional`, enums, exceptions, RAII and ownership before adding threads. Learn `std::sort`, `std::queue` and `std::priority_queue` when implementing algorithms. Introduce virtual interfaces where two implementations actually need the same contract.

For web development, understand a request, response, status code, cookie, session, HTML form and redirect. A server-rendered form submits to Drogon, which validates it, updates state, then redirects to a page. A browser does not need JavaScript for this flow.

For persistence, understand primary keys, foreign keys, unique constraints, transactions and prepared statements. SQLite is a database; a C++ repository wraps database operations so domain code does not depend on SQL everywhere.

For analysis, distinguish an event from a current count, a feature from a label, a cluster from an accusation, and event time from arrival time. These distinctions prevent more errors than adding extra algorithms.

## Your first working slice

Build the smallest complete route through the system:

1. A Drogon server returns a static HTML page.
2. A migration creates one account/content schema in a temporary database.
3. A registration form creates a properly hashed account.
4. Login produces a revocable session.
5. One seeded photo is visible to an eligible account.
6. One report is recorded together with its durable event.
7. An analyst-only query returns the event without credentials or private birth-date data.
8. A simple C++ client displays that query result.

Do not start with every console panel or a large simulator. First prove that a real action moves correctly from browser to database to authorized client. Then add independent analysis and richer views.

## How to use this documentation while coding

Create a branch for a vertical slice. Read its requirements and exit checks. Implement the domain rule, persistence operation, route and small UI in that order. Run the corresponding tests. Record the real command and outcome in the validation ledger. Only then mark its checklist entry complete.

The report narrative, technical explanations and demonstration structure are already drafted. The parts that must still come from the application are source-file references, dependency pins, real commands, screenshots, measured results and any institutional signatures. Those cannot truthfully be completed before the code exists.

Use [the glossary](../docs/GLOSSARY.md) when a term is unfamiliar. Keep the [documentation index](../docs/README.md) as your main navigation page.
