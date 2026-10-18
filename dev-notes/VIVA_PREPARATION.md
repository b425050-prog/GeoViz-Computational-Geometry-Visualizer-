# Viva preparation

> **TEMPORARY — REMOVE BEFORE SUBMISSION.** Personal preparation material. Retain during development; remove with the entire `dev-notes/` directory during final cleanup. See the [submission checklist](../docs/SUBMISSION_CHECKLIST.md).

## 1. Questions about the product

**What is the main project?** A visual security-investigation console backed by an original photo/video website. The Algorithm Lab supports the academic algorithm requirements and shared explanation engine.

**Does it detect false reports?** It flags patterns for review. Content and policy assessment are needed to decide whether a report is justified.

**Why not use only a world map?** Regional audiences can be legitimate and coordinated users can be distributed. Behaviour, history and timing add context.

**Why is a new account not automatically suspicious?** A genuine user may have just joined; older accounts can also coordinate. Creation time is supporting evidence.

**Why keep genuine mass complaints in the evaluation?** They test a major false-positive risk and the separation between coordination and report validity.

**Is declared birth date verified age?** No. It demonstrates a calendar/access rule, not identity verification.

## 2. Questions about algorithms

**Why DBSCAN?** It explains density-connected groups and noise without choosing a number of clusters, but depends on metric, scale and parameters.

**Does MinPts include the point itself?** Yes in this design. That convention is reflected in examples, radius queries and tests.

**Can noise later become border?** A point visited before a core neighbourhood can initially be marked noise and then be assigned as border during expansion.

**Can every pair in a cluster be far apart?** Density connectivity can link through intermediate core points; cluster membership does not require every pair to be within ε.

**Why normalize features?** Otherwise a large-unit rate/distance could dominate a fraction. Scaling is frozen/versioned so runs are interpretable and reproducible.

**Why not run a planar hull directly on latitude/longitude?** Longitude wraparound and nonuniform distance make that misleading. Use a suitable local projection and extent policy.

**Why test polygon boundary first?** Ray crossings at edges/vertices are ambiguous without a declared boundary and endpoint rule.

**What does Dijkstra require?** Non-negative finite costs. Negative edges invalidate its ordinary settling logic.

**What makes A* valid?** Its heuristic is a compatible lower bound; the initial implementation uses consistency and h(goal)=0. Arbitrary screen coordinates are not a proof.

**Why include routing if it is not the main detector?** The proposal explicitly requires it, and the lab gives it a meaningful comparison without claiming that shortest paths prove coordination.

## 3. Questions about engineering and OOP

**Where is encapsulation?** Entities/policies prevent invalid state transitions; callers cannot freely assign roles or bypass age access.

**Where is polymorphism?** Compatible algorithm/demo/repository interfaces allow different implementations behind shared contracts.

**Why a virtual destructor?** A derived strategy may be destroyed through its base interface. Destruction must be correct for the dynamic type.

**What does RAII do?** Resource lifetimes are tied to object lifetimes, so files, transactions, locks and graphics resources are cleaned up on normal and exceptional paths.

**Why composition rather than inheritance everywhere?** Accounts having policies/history is a has-a relationship, not an is-a hierarchy. Inheritance is reserved for meaningful substitutable interfaces.

**Why immutable events and snapshots?** Current state answers what is true now; events explain how it changed; snapshots preserve what a particular analysis actually saw.

**Why an ingestion cutoff is not enough?** It freezes visible events but not mutable account/content rows. Required metadata must also be materialized or versioned.

**How does the website work without JavaScript?** HTML forms submit to Drogon, which validates and renders/redirects. Native HTML handles media controls; the desktop C++ client handles console refresh.

**Why SQLite?** It simplifies local deployment and fixtures. Its single-writer limitations are acknowledged and tested.

**What is an atomic event operation?** The business mutation and its event commit together; a failure rolls both back.

## 4. Questions about results

**Why not report accuracy alone?** Class imbalance can hide failure. Precision/recall, false alerts, delay and coverage provide clearer task-specific evidence.

**How do you avoid label leakage?** Separate evaluator truth from detector inputs, freeze features/rules on tuning data and score held-out conditions.

**What does a seeded simulator guarantee?** A repeatable logical schedule under its defined generator/configuration. Real network timing still requires preserving the actual input snapshot.

**What is not finished?** Answer from the current [validation record](../docs/testing/VALIDATION_RECORD.md), not the desired roadmap. Explain limitations rather than claiming unrun checks passed.
