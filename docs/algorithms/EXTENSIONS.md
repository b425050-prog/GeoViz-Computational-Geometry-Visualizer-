# Extension cookbook and algorithm selection

**Status:** Design guidance for the complete catalogue in the [specification](../PROJECT_SPECIFICATION.md#9-algorithm-catalogue). Required primitives are explained in the foundations/core/detection guides. Enhancements, optional features and research items below are not implemented or prerequisites for the first complete release.

For each addition, preserve a simple reference implementation, state units/assumptions, add a useful visual explanation and prove benefit before increasing product complexity.

## 1. Closest pair by divide and conquer — Enhancement

Sort by x once, divide the points, solve both halves, then examine the vertical strip narrower than the current best distance. Maintain y ordering during partition/merge so the algorithm remains O(n log n); re-sorting every recursive strip can increase the cost.

Return point IDs and distance; duplicates yield zero. Visualize recursion regions, candidates and the current best. Compare small random and degenerate fixtures with exhaustive pair search. A closest pair in feature space is not necessarily the geographically closest pair or an indication of abuse.

## 2. k-d tree — Enhancement

Partition low-dimensional Euclidean points by alternating dimensions around a median. Nearest/radius search visits the near side first and explores another region only when its distance bound permits an improvement/match.

A balanced median-based build can achieve expected O(n log n) under the chosen selection method. Queries can still visit every point. Rebuild/version the index after edits; do not reuse an index from a different run. Validate radius inclusivity at ε and duplicate handling against direct scan.

Use it for the behaviour plane or local projected coordinates. Raw latitude/longitude with Euclidean pruning is not automatically a correct Haversine index.

## 3. Uniform grid, quadtree and R-tree

A **uniform grid** is an enhancement for bounded local scenes: choose cell sizes in the metric's units, inspect intersecting cells, then apply the exact distance predicate. It is easy to explain and compare with brute force.

A **quadtree** is optional for adaptive subdivision/display queries. Cap depth and bucket coincident points to prevent endless splitting. An **R-tree** is optional for bounding-box/viewport queries; bounding boxes are candidates, not exact geometric results.

Choose an index for a demonstrated query/load. Indexing does not eliminate worst-case dense neighbourhood work or justify claiming universal subquadratic DBSCAN.

## 4. Alternative hull strategies — Optional

Graham scan sorts angularly and maintains a turn stack; typical cost O(n log n). Jarvis march wraps the boundary in O(nh), useful when the number h of hull vertices is small. QuickHull partitions around farthest points and can take O(n²) in the worst case.

Use a shared hull interface and identical duplicate/collinear policy. Compare cyclic-equivalent results rather than demanding the same first vertex. Show different decisions while checking the same final convexity/containment properties.

## 5. Winding number — Optional

Count signed upward/downward crossings around a query. Run the same explicit boundary test used by ray casting first. For validated simple polygons the methods should agree on membership independent of winding reversal. Do not silently expand to self-intersecting polygons without defining their semantics.

## 6. Clipping and triangulation — Optional

**Sutherland–Hodgman** clips a polygon progressively against a convex clipping region's half-planes. Use it for overlays or viewport clipping. A general nonconvex polygon Boolean operation needs additional machinery.

**Ear clipping** triangulates a validated simple polygon by removing a convex ear containing no other active vertex under the chosen boundary convention. A simple implementation is quadratic. Verify total triangle area, orientation and absence of invalid diagonals; handle degenerate ears explicitly.

## 7. Rotating calipers and simplification — Optional

Use **rotating calipers** on an ordered convex hull to compute diameter and, with the correct algorithm, oriented bounding rectangles. Compare small cases against exhaustive distance/edge-orientation checks.

Use **Douglas–Peucker** for display-only trail/outline simplification with a declared tolerance. Retain original evidence and avoid using simplified geometry silently for detection or access policy. Topology preservation is not automatic.

## 8. Delaunay, Voronoi and local contours

**Delaunay triangulation** and its **Voronoi dual** are optional lab modules for neighbourhood structure. Start with robust orientation/circumcircle predicates, then handle duplicates, collinearity, cocircular ties and bounded display cells. Compare topology/area on small known fixtures.

**Alpha shapes** or density-derived contours are research alternatives to broad convex hulls. Their boundaries depend on parameters and definitions; do not label them exact campaign borders. Geographic projection limits still apply.

## 9. EWMA and sustained changes

**EWMA** is an enhancement: `z_t = alpha*x_t + (1-alpha)*z_(t-1)`. State initialization, α and missing-bucket treatment determine lag. Use it to smooth rates, with a baseline comparison rather than as standalone proof.

**CUSUM** and **Page–Hinkley** are optional change detectors. Define reference level/drift/threshold, reset policy and false-alarm tests. Compare their delay and false-alert behaviour against median/MAD under legitimate trends and campaigns. Seasonal models remain research until sufficient representative history exists.

## 10. Indexed DBSCAN, ST-DBSCAN and ε guidance

**Indexed DBSCAN** is an enhancement only after the direct-scan implementation is correct. A compatible exact radius index must preserve core/border/noise behaviour and deterministic tie policy. Benchmark both sparse and dense datasets.

**ST-DBSCAN** is an enhancement with separate spatial and temporal neighbourhood thresholds. Clearly define density rules/variant before implementing it; concatenating unscaled time and longitude is not a valid substitute.

A **k-distance plot** is an enhancement that helps users inspect parameter scale. Explain the self-counting convention and neighbour rank. A visible elbow is a heuristic cue, not a theorem that a parameter is optimal.

## 11. Variable-density clustering — Research

**OPTICS** records reachability/ordering over a range of neighbourhood scales. **HDBSCAN** builds a density hierarchy and selects stable groups under its defined method. They address limitations of a single global ε but add substantial implementation/explanation/dependency work.

Evaluate them only after reference DBSCAN and labelled scenarios exist. Record what a label change means; do not compare two different methods as if they must return identical clusters.

## 12. Outliers and pattern similarity

**Isolation Forest** and **local outlier factor** are research tools for unusual observations. Outlier scores need separate evaluation and are not factual abuse labels. Do not train or choose thresholds with held-out labels.

Optional **entropy** summaries quantify concentration of targets/time bins. They depend on bin definitions and support. Optional **cross-correlation/sequence comparison** can identify aligned action patterns, but public events can synchronize legitimate users too.

Optional **TF-IDF and cosine similarity** can group templated report/comment text. Define C++ tokenization/normalization, language limits and access controls. Generic complaint templates can be legitimate. Text similarity is context, not automatic falsity detection.

## 13. Graph additions

Required BFS/DFS explores observed evidence relationships. Optional **union–find** supports incremental undirected connectivity when deletions are unnecessary; compare components with BFS.

Optional **degree, betweenness and PageRank** expose graph structure. Popular content and ordinary high-activity users can dominate these measures. Normalization and graph-construction choices must be visible.

**Louvain/Leiden or label propagation** are research community methods with stability/tie considerations. A community is not a campaign label. An optional **minimum spanning tree** is a compact similarity view, not a communication/causal tree.

Optional **bipartite matching/min-cost flow** can assign cases to eligible analysts when work units/capacities fit that model. Heterogeneous time-budget/skill constraints can require a generalized assignment/ILP model instead; do not assume every assignment problem reduces to simple flow.

## 14. Review-budget optimization — Optional

Given cases with estimated review time `t_i`, review utility `p_i` and budget B, select whole cases with binary `x_i`:

```text
maximize sum(p_i*x_i)
subject to sum(t_i*x_i) <= B
and x_i in {0,1}
```

This is integer linear programming. Priorities are utility estimates, not probabilities of guilt. A continuous LP instead allows fractional allocation and is appropriate only when partial review time has a defined value.

For small integer budgets, **0/1 knapsack dynamic programming** provides a useful exact baseline. A greedy priority/time ratio is fast but not generally optimal. Example: budget 10, cases (time=6,value=12), (5,9), (5,9). Ratio-greedy chooses the first for value 12; the two five-minute cases give 18.

Use a supported C++ solver API if adding ILP. Check tiny problems against exhaustive enumeration and show assumptions/remaining capacity rather than presenting the result as an ethical decision.

## 15. Operational algorithms

Required **token-bucket limiting** refills capacity over elapsed monotonic time and consumes tokens per permitted request. Define locking/concurrency, actor/source keys and burst/refill settings. Wall-clock jumps should not create unlimited capacity.

Required **hash maps, unique constraints, stable sorts and heaps** support lookup, correctness and ordering. A hash map is not an authorization layer. Database constraints still protect concurrent operations.

Optional bounded **LRU caches** must include authorization/version context or avoid caching protected results. Optional work/caching optimizations must preserve freshness and hard memory limits.

## 16. How to approve an extension

Write its purpose, input/units, assumptions, interface, complexity, trace events, limits, reference oracle, evaluation dataset and release impact. Demonstrate a real improvement or educational value. Keep optional algorithms out of the required completion checklist until explicitly selected.

See [evaluation](../evaluation/SCENARIOS_AND_METRICS.md), [OOP strategies](../implementation/OOP_MAP.md) and [references](../REFERENCES.md).
