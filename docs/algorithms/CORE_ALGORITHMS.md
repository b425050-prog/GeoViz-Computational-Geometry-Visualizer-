# The five proposal algorithms, explained

**Status:** Algorithm specifications and worked examples, not recorded execution results. [Geometry foundations](GEOMETRY_FOUNDATIONS.md) and [contracts](../CONTRACTS.md) define units, tolerances and deterministic ordering.

## 1. Monotone chain convex hull

### Purpose

Find the smallest convex polygon containing the input points, subject to the duplicate/collinear policy. In the console it summarizes a suitable locally projected footprint; in the lab it exposes orientation decisions.

### Procedure

1. Validate finite bounded coordinates and sort lexicographically by x, then y, with stable point IDs for ties.
2. Remove exact coordinate duplicates under the documented initial policy, retaining identity mapping for display.
3. Scan left to right to build the lower chain. While its last two vertices and the candidate fail the required left turn, remove the last vertex.
4. Scan right to left to build the upper chain using the same rule.
5. Concatenate chains without repeating endpoints.

Choose the initial policy **retain only extreme endpoints on a collinear boundary**. Empty input gives an empty hull; one unique point gives one point; a fully collinear set gives its two extremes. Do not return a fake area-bearing polygon for these cases.

### Worked example

Points: (0,0), (2,0), (2,2), (0,2), (1,1). Sorted order: (0,0), (0,2), (1,1), (2,0), (2,2). Orientation tests remove interior/non-boundary candidates from the partial chains. The final hull is the square's four corners; (1,1) remains an input point but is not a hull vertex.

Show the current candidate, accepted chain and popped vertices. Record events such as `CandidateConsidered`, `OrientationTested`, `VertexRemoved` and `VertexAccepted`.

Sorting costs O(n log n); chain scans cost O(n), with O(n) working/result storage excluding traces. Check convexity, input containment, order/reversal conventions and degenerate cases. A hull may cover empty space between groups.

## 2. Ray-casting polygon membership

### Purpose and assumptions

Classify a query as inside, outside or boundary of a validated simple polygon. The query is O(m) after polygon validation; validating self-intersection is separate work.

### Procedure

1. Test whether the point lies on any polygon edge. If so, return boundary.
2. Cast a horizontal ray toward increasing x.
3. For each edge A→B, consider a crossing only when `(Ay > Qy) != (By > Qy)`. This half-open endpoint convention avoids counting a vertex twice.
4. Compute the crossing x only for such non-horizontal straddling edges. Count it if it lies to the right of Q.
5. Odd crossing count means inside; even means outside.

### Worked example

For square (0,0), (4,0), (4,4), (0,4): Q=(2,2) has one rightward crossing and is inside. Q=(5,2) has zero and is outside. Q=(4,2) and Q=(4,4) are boundary before parity is considered.

Animate tested edges and accepted crossings. Preserve the explicit boundary result in filters; an analyst may choose inclusion/exclusion of boundary points, and the chosen policy must be stored. Check reversed polygon winding, horizontal edges and vertex-height queries.

## 3. Dijkstra's shortest path

### Purpose and assumptions

Find the least total-cost route from a start vertex on a graph with **non-negative** edge costs. Directed/undirected interpretation is part of the scene. Reject negative or nonfinite costs; do not silently turn them positive.

### Procedure

Initialize start distance to zero and all others to infinity. Push the start into a min-priority queue. Pop the least tentative distance; ignore stale entries. For each outgoing edge, relax it if the path through the current node is strictly better. Store predecessor and new distance, then push the updated candidate. Continue until the goal is settled or the frontier is empty.

If the goal is unreachable, return an explicit no-route result. Do not reconstruct by following uninitialized predecessors. Stable tie rules make equivalent routes repeatable; avoid epsilon-based heap comparators.

### Worked example

Edges: S→A=2, S→B=5, A→B=1, A→G=6, B→G=2. Settling S gives A=2 and B=5. Settling A improves B to 3 and sets G=8. Settling B improves G to 5. Final route: S→A→B→G, total cost 5.

Show frontier entries, distances, relaxed edges and predecessor changes. A simple graph with a binary heap has the familiar O((V+E) log V) bound; implementation details for duplicate heap entries should be documented. Working graph/distance storage is O(V+E), excluding trace expansion.

## 4. A* search

### Purpose and heuristic

A* prioritizes `f(v)=g(v)+h(v)`, where g is known path cost and h is a lower bound on remaining cost. Use a **consistent** heuristic in the initial implementation: `h(u) <= cost(u,v)+h(v)` for each edge, and h(goal)=0.

For a unit-cost four-neighbour grid use Manhattan distance. For geometric graphs use Euclidean distance only when each edge cost is at least the corresponding geometric distance in compatible units. Travel-time costs need a time lower bound, such as distance divided by an actual maximum-speed bound.

Zero is always a safe non-negative lower-bound heuristic and reduces A* to Dijkstra's objective. An arbitrary account-graph layout is not a heuristic proof.

### Procedure and worked example

Use the Dijkstra relaxation structure but order by f, maintain g, and terminate when the goal is correctly popped under the chosen consistent-heuristic policy. Discovering the goal as a neighbour is not enough to stop.

On the previous graph choose h(S)=4, h(A)=3, h(B)=2, h(G)=0; these satisfy consistency. Initial f(S)=4. After S, A has g=2/f=5 and B has g=5/f=7. Expanding A improves B to g=3/f=5 and introduces G at g=8/f=8. B improves G to g=5/f=5. The optimal cost remains 5.

This tiny example does not promise fewer expansions. Compare both algorithms on the same snapshot/graph/start/goal, with operation counts separate from animation speed. With suitable consistent heuristics the standard binary-heap graph-search bound is comparable to Dijkstra; practical expansion counts depend on the scene. If later allowing inconsistent heuristics, implement/retest reopening rather than reusing an invalid closed-set assumption.

## 5. DBSCAN

### Definitions

For radius ε, `N(p)` contains all points at distance ≤ ε from p, **including p itself**. A core point has at least MinPts neighbours. A border point is reachable from a core neighbourhood but is not itself core. Other points are noise under that run's parameters.

### Procedure

1. Visit points in stable ID order; skip already visited points.
2. Run a radius query. If the neighbourhood is too small, provisionally mark noise.
3. Otherwise start a cluster and expand a queue of neighbours.
4. For each newly visited core point, add its neighbourhood to the expansion queue with duplicate protection.
5. Assign eligible unassigned/provisional-noise points to the cluster. A provisional noise point can become border.

Use deterministic first-reached assignment for border points reachable from multiple clusters; document that ambiguity. Do not claim every member pair in a density-connected cluster is within ε.

### Worked example

Euclidean points A=(0,0), B=(0.1,0), C=(0,0.1), D=(0.1,0.1), E=(0.24,0.1), F=(1,1). Choose ε=0.16 and MinPts=4.

The four square points see each other, so A–D are core. E sees itself and D but lacks four neighbours; it becomes border when reached from D. F sees only itself and remains noise. If E is visited before the core group it can transition from provisional noise to border.

In the console's behavioural run, distances are in normalized feature units. In geographic mode they are Haversine metres. The same ε value is not interchangeable across these metrics.

Direct scanning takes O(n²) distance work. On-demand queries can keep working storage near O(n) apart from outputs, while caching all neighbourhoods may require O(n²) storage. A recorded full trace can also grow quadratically; enforce a separate trace limit.

## 6. Shared trace and verification rules

Store stable input IDs, event types, relevant values, versions and enough state/checkpoints for rewind. Never record only the final drawing position; screen coordinates change under zoom.

Test mathematical results independently of animation. Replay verifies the final state matches the computed result; it does not replace algorithm reference tests. Geometry tests include degeneracies, routes include unreachable/zero-cost fixtures, and DBSCAN tests include exact-radius boundaries, duplicates, all-noise and ambiguous borders.

See [detection integration](DETECTION_PIPELINE.md), [optional extensions](EXTENSIONS.md) and [test plan](../testing/TEST_PLAN.md).
