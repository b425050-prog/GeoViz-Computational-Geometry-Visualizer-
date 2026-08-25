# Validation Record

Validation date: **2026-08-25**  
Project: **GeoViz 1.0.1**  
Student: **Satyam Dhal (B425050)**

## Completed checks

| Check | Result |
|---|---|
| Core + tests compile as C++20 with GCC 13 | Pass |
| `-Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror` | Pass |
| 12 named regression suites | **12 passed, 0 failed** |
| AddressSanitizer and UndefinedBehaviorSanitizer test run | Pass, no diagnostics |
| GUI translation units checked against raylib 5.5 API surface | Pass under strict warning profile |
| Deterministic randomized stress test: 120 point clouds | Pass |
| Stress test: all four hull areas agree | Pass |
| Stress test: Delaunay empty-circumcircle property | Pass |
| Stress test: Voronoi cells cover viewport area | Pass |
| Stress test: 60 simple polygons conserve area after ear clipping | Pass |
| Five `.geoviz` presets parsed by production serializer | Pass |
| CMake preset JSON | Valid |
| Logo/dashboard SVG XML | Valid |
| Local Markdown links | No broken target |
| Interface preview visually inspected at 1440×900 | Pass |

## Regression output

```text
[PASS] Vector arithmetic and shape polymorphism
[PASS] Predicates, projections, and intersections
[PASS] All four convex hull strategies
[PASS] Bowyer-Watson Delaunay triangulation
[PASS] Voronoi cells and Delaunay dual
[PASS] Half-plane intersection
[PASS] Ear-clipping triangulation
[PASS] Closest pair and segment intersections
[PASS] Advanced polygon utilities
[PASS] Command pattern undo and redo
[PASS] Versioned scene serialization
[PASS] Polymorphic demo registry

12 passed, 0 failed.
```

## Continuous integration

The GitHub Actions workflow performs:

1. dependency-free core build and CTest execution on Ubuntu, Windows, and macOS; and
2. a complete native graphical build on Ubuntu after installing raylib's desktop prerequisites.

This separates fast mathematical regression from external GUI dependency validation.
