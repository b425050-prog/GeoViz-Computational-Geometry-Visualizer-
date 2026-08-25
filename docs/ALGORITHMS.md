# Algorithm Notes

## Predicate foundation

Most planar algorithms depend on one signed determinant:

\[
\operatorname{cross}(A,B,C)=(B_x-A_x)(C_y-A_y)-(B_y-A_y)(C_x-A_x).
\]

- positive: counter-clockwise turn;
- negative: clockwise turn;
- approximately zero: collinear.

GeoViz compares against a scale-aware tolerance rather than exact zero. Segment containment adds a bounding test; line/ray/segment intersections return `None`, `Point`, `Overlap`, or `TwoPoints` explicitly.

## Convex hull family

### Monotonic Chain

1. Sort distinct points lexicographically.
2. Scan left-to-right, popping while the last two hull points and candidate do not make a strict left turn.
3. Repeat in reverse for the upper chain.
4. Join both chains without duplicate endpoints.

Invariant: each open chain contains only strict counter-clockwise turns. Complexity is `O(n log n)` for sorting and `O(n)` for both scans.

### Graham Scan

1. Choose the lowest point (then lowest x) as pivot.
2. Sort remaining points by polar angle; nearer collinear points appear first.
3. Maintain a stack of strict left turns. A later farther point removes a nearer point on the same ray.

Complexity: `O(n log n)`.

### Jarvis March

Starting at an extreme point, inspect every point to choose the next supporting edge. Repeat until returning to the start. If the hull contains `h` vertices, complexity is `O(nh)`. This is attractive when `h` is small and makes an especially clear animation.

### QuickHull

The leftmost/rightmost baseline divides the points. For each side, the farthest point forms a triangle; points inside that triangle are discarded, and two outside subsets recurse. Average complexity is `O(n log n)`; adversarial distributions can reach `O(n²)`.

## Bowyer–Watson Delaunay triangulation

For every inserted site:

1. Find triangles whose circumcircles contain the site.
2. Their union is a cavity.
3. Count cavity edges; edges occurring once form its boundary.
4. Remove bad triangles and connect the new site to each boundary edge.
5. After all insertions, remove triangles incident to the temporary super-triangle.

GeoViz includes cocircular points in the cavity with a scaled tolerance. Final triangles satisfy the empty-circumcircle property subject to floating-point precision. The implementation performs a naive triangle search for animation clarity, so its worst case is quadratic even though spatial indexing gives expected `O(n log n)` implementations.

## Voronoi diagram

For site \(s\) and neighbour \(n\), points closer to \(s\) satisfy:

\[
\|x-s\|^2 \le \|x-n\|^2,
\]

which simplifies to a linear half-plane bounded by the perpendicular bisector. Each site begins with the canvas rectangle and is clipped against the closer side of every other site's bisector. This constructs exact cells inside the viewport and naturally represents otherwise unbounded cells.

GeoViz also computes Delaunay triangulation and overlays its edges, making the primal/dual relationship visible. The educational clipped-cell method is `O(n³)` in the straightforward bound; Fortune's sweep is a roadmap item.

## Half-plane intersection

A `HalfPlane(point, direction)` keeps points to the **left** of its directed boundary:

\[
\operatorname{cross}(direction, candidate-point) \ge 0.
\]

Starting with the visible rectangle, GeoViz applies Sutherland–Hodgman-style clipping for each constraint. At every polygon edge:

- inside → inside: emit current endpoint;
- inside → outside or outside → inside: emit the boundary intersection;
- outside → outside: emit nothing.

For `k` constraints and at most `v` active vertices, the visual construction is `O(kv)`. An empty or zero-area polygon means the system is infeasible within the visible universe.

## Ear-clipping triangulation

GeoViz normalizes polygon orientation to counter-clockwise. An active vertex is an ear if:

1. its previous/current/next triple is convex; and
2. no other active vertex lies inside **or on the boundary** of that triangle.

The boundary condition matters: allowing a vertex on the ear diagonal can create overlapping triangles. A simple `n`-vertex polygon yields exactly `n-2` triangles. Complexity is `O(n²)`.

## Closest pair

1. Sort by x.
2. Recursively solve left and right halves.
3. Let `d` be the better distance.
4. Build a strip containing points within `d` of the split and sort it by y.
5. Compare only following strip points whose y difference is below `d`.

The current implementation sorts each merge strip and therefore runs in `O(n log² n)`. Carrying y-sorted lists through recursion would reach the optimal `O(n log n)`.

## Segment intersection

Two non-collinear segments intersect when each pair of endpoints straddles the other's supporting line. GeoViz additionally handles:

- shared endpoints;
- zero-length segments;
- parallel disjoint segments;
- partially and fully overlapping collinear segments.

The interactive module examines every segment pair: `O(m²)`. Each pair is animated as a candidate and all unique intersection points remain highlighted.

## Point in polygon

Boundary containment is checked first. Otherwise a horizontal ray toggles an `inside` flag whenever a polygon edge straddles the query's y-coordinate and crosses to its right. The half-open straddle rule avoids double-counting vertices. Complexity: `O(n)`.

## Smallest enclosing circle

The deterministic incremental algorithm maintains a valid disk. When a point is outside, it must lie on the boundary of the next disk. Nested loops add a second and, when needed, third boundary point. Two points define a diameter circle; three non-collinear points define a circumcircle. Complexity: `O(n³)`.

## Rotating calipers diameter

After constructing a counter-clockwise convex hull, a second pointer advances while triangle area relative to the current edge increases. These antipodal candidates include the farthest vertex pair. Hull construction is `O(n log n)` and the caliper walk is `O(h)`.

## Additional library algorithms

| Routine | Method | Complexity |
|---|---|---:|
| Polygon clipping | Sutherland–Hodgman against convex window | `O(nm)` |
| Minkowski sum | Pairwise sums followed by monotonic hull | `O(nm log(nm))` |
| Douglas–Peucker | Recursive farthest distance from baseline | worst `O(n²)` |
| Centroid | shoelace-weighted coordinate sum | `O(n)` |
| Circumcenter | determinant formula | `O(1)` |
| Projection | dot product parameterization | `O(1)` |
| Circle intersections | quadratic geometry | `O(1)` |

## Degenerate input policy

- Exact/near duplicate points are removed where algorithm invariants require distinct sites.
- Fewer than the required number of points produces an explanatory frame, not a crash.
- All-collinear hull input becomes an endpoint hull.
- All-collinear Delaunay input returns no final triangles.
- A self-intersecting polygon causes ear clipping to throw an actionable `GeometryError`.
- Parallel line intersections and collinear overlaps use explicit result kinds.
- Invalid finite bounds, zero directions, negative radii, and non-finite coordinates are rejected.

