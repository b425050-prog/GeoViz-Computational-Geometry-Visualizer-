# GeoViz Viva Preparation

## Project and architecture

### 1. What problem does GeoViz solve?

It turns computational-geometry algorithms from final-output programs into interactive, narrated state transitions. A user edits geometry and watches orientation decisions, hull updates, triangulation cavities, clipping, and recursive merges.

### 2. Why is the algorithm core independent of raylib?

It follows separation of concerns and dependency inversion. Mathematical code can be tested without a window, reused by another interface, and reasoned about without rendering state.

### 3. What is an `AlgorithmTrace`?

It is ordered metadata plus `VisualFrame` objects. Each frame holds semantic geometry—accepted/candidate/rejected edges, points, triangles, polygons, guide circle, narration, and counters. The renderer consumes it later.

### 4. Why use C++20?

The project benefits from concepts, `std::span`, strong scoped enums, smart pointers, filesystem support, STL algorithms, move semantics, and efficient value types alongside classic OOP.

## OOP

### 5. Where is encapsulation used?

Geometry coordinates/radii, model state, playback state, and command stacks are private. Public methods validate state transitions; for example, `Circle` rejects a negative radius and `TracePlayer` clamps speed.

### 6. Where is abstraction used?

`Shape`, `ShapeVisitor`, `ConvexHullStrategy`, `AlgorithmDemo`, `TraceProducer`, and `SceneCommand` specify behavior without exposing implementation.

### 7. Explain runtime polymorphism in GeoViz.

`unique_ptr<ConvexHullStrategy>` may refer to any of four algorithms, but `compute` invokes the correct override. Similarly, shapes and commands execute through base interfaces with virtual functions.

### 8. Is multiple inheritance used?

Yes. `AlgorithmDemo` inherits two pure capability interfaces: `DescribedComponent` and `TraceProducer`. They contain no shared data, so this avoids ambiguous state and the diamond problem.

### 9. What type of inheritance does the Shape hierarchy show?

Hierarchical inheritance: seven concrete classes derive from the same abstract base. Each also demonstrates single inheritance.

### 10. Why must a polymorphic base have a virtual destructor?

Deleting a derived object through a base pointer otherwise has undefined behavior and may skip the derived destructor. All polymorphic GeoViz bases have virtual destructors.

### 11. Explain the Visitor pattern here.

Every shape implements `accept(ShapeVisitor&)`, and the renderer implements `visit` overloads. This selects rendering by concrete type without putting raylib calls inside geometry classes or using a long `dynamic_cast` chain.

### 12. Explain the Command pattern here.

Each edit stores enough data for `execute` and `undo`. The manager owns commands with `unique_ptr`; undo moves one to the redo stack, and redo executes and moves it back.

### 13. Explain the Observer pattern here.

`Application` subscribes to `SceneModel`. Mutations increment the revision and notify observers. The application marks its trace dirty and rebuilds the selected algorithm before drawing.

### 14. What is composition in GeoViz?

`Application` contains its model, command manager, registry, player, transform, and renderer. Their lifetime is a structural part of the application.

### 15. What is aggregation in GeoViz?

`GeometryRenderer` stores references to an externally owned `CanvasTransform` and `Theme`. It uses them but does not control their lifetime.

## Modern C++

### 16. Why use `unique_ptr` instead of raw owning pointers?

It states exclusive ownership, automatically destroys objects, transfers safely through move semantics, and prevents memory leaks and double deletion.

### 17. Where is compile-time polymorphism used?

`BoundedValue<T>` and `square(T)` are templates constrained by C++20 concepts. The compiler generates type-specific implementations and rejects types outside the contract.

### 18. Why use `std::span` for algorithm input?

It is a non-owning, bounds-aware view. The same function accepts a vector, array, or compatible contiguous data without copying or taking ownership.

### 19. Why use `std::optional`?

Some results legitimately do not exist: collinear triangles have no finite circumcenter, fewer than two points have no closest pair, and an empty input has no enclosing circle. `optional` makes absence explicit.

### 20. Give examples of operator overloading.

Points add/subtract vectors, subtracting two points returns a vector, vectors scale and divide, point stream output is overloaded, and internal edge keys use three-way comparison.

### 21. Why use `enum class` rather than ordinary enum?

Values are scoped and do not implicitly convert to integers, preventing collisions and accidental comparisons between unrelated state types.

### 22. Where are exceptions used?

Invalid geometry and corrupt files throw `GeometryError`. The application catches errors at a stable boundary and turns them into a toast or explanatory trace rather than crashing.

### 23. What is RAII in this project?

Vectors, streams, smart pointers, and application objects acquire resources in constructors and release them in destructors. Early returns or exceptions still clean up correctly.

### 24. Why are read-only methods `const` and often `[[nodiscard]]`?

`const` prevents hidden mutation and allows use through const references. `[[nodiscard]]` warns if a meaningful result, such as an intersection or computed trace, is accidentally ignored.

## Geometry

### 25. What is the orientation test?

It is the sign of the 2-D cross product of vectors `B-A` and `C-A`. Positive is counter-clockwise, negative clockwise, and near zero collinear.

### 26. Why not compare floating point directly with zero?

Arithmetic introduces rounding. GeoViz uses scale-aware tolerances so a determinant is interpreted relative to the magnitudes involved.

### 27. Compare the four hull algorithms.

Monotonic Chain and Graham are `O(n log n)` and sort first. Jarvis is output-sensitive `O(nh)`. QuickHull is fast on average but can be quadratic. They use different invariants but return the same boundary.

### 28. What is the Delaunay empty-circumcircle property?

For every retained triangle, no input site lies strictly inside its circumcircle. This tends to avoid skinny triangles and is dual to the Voronoi diagram.

### 29. Explain one Bowyer–Watson insertion.

Find all triangles whose circumcircles contain the new point, remove them, count their edges to find the cavity boundary, then connect the new point to every boundary edge.

### 30. How is Voronoi built?

For each site, start with the canvas rectangle. For every other site, clip to the half-plane closer to the current one. The remaining polygon is its bounded visible cell.

### 31. Why is the Voronoi complexity `O(n³)` here?

There are `n` cells, each clipped against `n-1` neighbours, and a polygon can have `O(n)` vertices. This is intentionally clear and stable; Fortune's algorithm would be `O(n log n)`.

### 32. How does half-plane direction work?

For a boundary from A to B, the feasible side is the left side. A candidate is feasible when `cross(B-A, candidate-A) >= 0` within tolerance.

### 33. What makes a polygon vertex an ear?

The previous-current-next turn is convex and no other active vertex is inside or on the boundary of that triangle. Clipping it preserves the remaining simple polygon.

### 34. Explain closest-pair divide and conquer.

Sort by x, solve both halves, take best distance `d`, and inspect only a y-sorted strip within `d` of the split. Points farther apart vertically cannot improve the answer.

### 35. How does point-in-polygon avoid double-counting vertices?

It counts only edges that straddle the query's y-coordinate using a half-open comparison, and checks boundary membership separately before parity toggling.

### 36. What does rotating calipers find?

On a convex polygon, it advances antipodal support points while an edge rotates. The farthest pair (diameter) must occur among these antipodal candidates.

## Testing and engineering

### 37. How is correctness tested without the GUI?

The test executable links only `geoviz_core`. It verifies output topology, counts, areas, distances, persistence, command state, and that every demo builds a safe trace.

### 38. Describe a bug caught by testing.

An ear candidate was initially allowed when another vertex lay on its diagonal. The triangle count was correct but total triangle area exceeded polygon area, proving overlap. Treating boundary containment as blocked fixed it.

### 39. Why pin raylib 5.5?

Pinning makes builds reproducible. Fetching a moving development branch could introduce breaking API or build changes between evaluations.

### 40. What would you improve next?

Adaptive exact predicates, Fortune Voronoi, Bentley–Ottmann intersections, constrained triangulation, polygon holes, and in-app frame export are the highest-value extensions.

