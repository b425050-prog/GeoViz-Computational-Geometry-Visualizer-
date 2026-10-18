# Geometry foundations and numerical discipline

**Status:** Mathematical and implementation guide. Tests and source code remain to be written.

## 1. Coordinate spaces

The project has several spaces that must not be mixed:

- **Screen coordinates:** pixels used by raylib. Panning/zooming changes only presentation.
- **Lab world coordinates:** bounded planar coordinates used by geometry algorithms.
- **Behaviour space:** normalized feature values with a versioned scale.
- **Geographic coordinates:** latitude/longitude angles on a sphere, with approximate or synthetic provenance.

Use separate types or explicit conversion functions. A mouse point is transformed to world space before editing a scene. A world-map pixel is not a geographic distance, and a normalized behavioural ε cannot be interpreted as metres.

## 2. Vectors and distance

For points A=(ax,ay) and B=(bx,by), the displacement is `(bx-ax, by-ay)`. Squared Euclidean distance is `(bx-ax)^2 + (by-ay)^2`. Compare squared distances when possible to avoid unnecessary square roots, provided the scene bounds prevent overflow.

The dot product `u.x*v.x + u.y*v.y` measures alignment and supports projection. The 2D cross product `u.x*v.y - u.y*v.x` gives signed area/orientation information. Reject NaN/infinity and excessive scene magnitudes before computation.

## 3. Orientation

Compute `cross(B-A, C-A)`:

- Positive: C is to the left of directed A→B in the chosen world convention.
- Negative: C is to the right.
- Zero/within the declared predicate tolerance: collinear.

Example: A=(0,0), B=(4,0), C=(2,3) gives cross=12, so C is left of the edge. D=(2,-1) gives -4. E=(2,0) is collinear.

raylib screen y typically increases downward; do not silently reuse screen orientation as mathematical y-up orientation. Transform coordinates or consistently document the convention.

## 4. Point on segment

A query Q lies on segment AB when it is collinear under the chosen policy and lies within the endpoints' bounded extent. A zero-length segment reduces to a point comparison.

Boundary classification comes before ray-casting membership. Otherwise, a point on an edge/vertex can be classified differently depending on floating-point rounding or edge direction.

## 5. Segment intersection

For segments AB and CD, compare orientations of C/D relative to AB and A/B relative to CD. Opposite sides in both tests indicate a proper crossing. Zero orientations require explicit endpoint/collinear handling through point-on-segment/bounding tests.

Return a meaningful result: none, single-point contact/crossing, or collinear overlap. A Boolean may suffice for rejecting polygon self-intersection but loses information needed in a visual explanation.

Example fixtures: perpendicular crossing, shared endpoint, disjoint collinear segments, partially overlapping segments, identical segments and zero-length segments. Document whether adjacent polygon edges may share their intended endpoint; that is different from a non-adjacent self-intersection.

## 6. Polygon validation

A geofence must have at least three usable vertices, no zero-length consecutive edges after the chosen cleanup, nonzero area and no non-adjacent self-intersections. A simple O(m²) edge-pair validation is acceptable for the small demo. Its cost is separate from an O(m) membership query.

Signed area uses the shoelace sum. Its sign indicates winding; its magnitude/2 gives area in squared world units. Reversed winding should not change inside/outside classification. Degenerate polygons produce a validation error rather than an arbitrary filled shape.

## 7. Floating-point policy

Use `double` for calculations. Define tolerances relative to the operation's units and scale. A cross product has squared-length units; blindly applying the same epsilon used for coordinate equality can be inappropriate.

Keep coordinate sorting strictly ordered by numeric coordinates and stable IDs where necessary. **Do not use approximate equality inside a sort comparator**: it can violate strict weak ordering and make behaviour undefined or inconsistent. Deduplication/tolerance policy is a separate explicit step.

For a bounded initial implementation, use carefully selected absolute/relative predicates and document their limits. Test nearly collinear points and very small/large permitted scales. If stronger robustness is needed later, use established adaptive/exact predicates rather than claiming double precision is exact.

## 8. Geographic distance

Haversine computes angular distance from latitude/longitude in radians:

```text
a = sin²((lat2-lat1)/2) + cos(lat1)*cos(lat2)*sin²((lon2-lon1)/2)
distance = 2*earth_radius*atan2(sqrt(a), sqrt(1-a))
```

Clamp roundoff in `a` to [0,1]. Use a documented Earth-radius constant and output unit. Coordinates close across the antimeridian should remain close, unlike naive longitude subtraction on a flat map. Real geolocation uncertainty may be much greater than arithmetic error.

Haversine is useful for geographic DBSCAN. It does not make a planar hull algorithm valid directly on latitude/longitude.

## 9. Local planar projection

For small local scenes, an explicitly bounded local approximation can convert angular offsets about an origin to metres. Record the origin, Earth model, valid extent and error policy. Reject/split unsuitable global or antimeridian-spanning scenes; do not draw a misleading polygon across the entire map.

Use a proper documented projection if the demonstration requires a larger extent. Validate projection round trips and a known distance fixture. Draw geographic hulls as **enclosing footprints**, not precise cluster borders or evidence of shared physical origin.

## 10. Implementation order and checks

Implement values/finite checks → vector arithmetic → orientation → segment membership/intersection → polygon validation → projection/distance helpers. Then build hull and region queries on this kernel. Keep functions independent of raylib, HTTP and storage.

Use hand-computable fixtures, reversed point order, duplicates and degenerate inputs. Test properties such as symmetry of distance and unchanged membership under polygon reversal, not merely whether one implementation calls another.

Next: [core algorithms](CORE_ALGORITHMS.md).
