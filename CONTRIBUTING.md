# Contributing to GeoViz

Thank you for helping improve GeoViz. Contributions should preserve the project's two central promises: mathematically correct behavior and a clear educational animation.

## Development workflow

1. Create a focused branch from `main`.
2. Configure the dependency-free validation build:

   ```bash
   cmake --preset core-only
   cmake --build --preset core-only
   ctest --test-dir build/core-only --output-on-failure
   ```

3. Make a small, reviewable change.
4. Add a regression test for new algorithms and every discovered degeneracy.
5. Build the graphical target and inspect animation playback at multiple window sizes.
6. Run formatting and commit with a descriptive message.

## Architecture rules

- `geometry/`, `algorithms/`, and `animation/` must not include `raylib.h`.
- Algorithms return mathematical results plus `AlgorithmTrace`; they never call drawing functions.
- Ownership must be explicit. Prefer values and `std::unique_ptr`; avoid owning raw pointers.
- Validate public inputs and throw `GeometryError` with an actionable message.
- Use `double` in the geometry core and convert to `float` only at the renderer boundary.
- Compare floating-point values with the scale-aware helpers in `Predicates.hpp`.

## C++ style

- C++20, four-space indentation, braces on the same line.
- Types use `PascalCase`; functions and variables use `camelCase`; constants begin with `k`.
- Mark values `const` and methods `[[nodiscard]]` wherever ignoring a result is likely a bug.
- Add a Doxygen file comment and explain invariants or non-obvious mathematical decisions.
- Keep functions focused; name geometric roles (`candidate`, `pivot`, `halfPlane`) instead of using single letters outside formulas.

## Adding a visual algorithm

1. Put the framework-independent declaration in `include/geoviz/algorithms/`.
2. Implement the result and meaningful trace frames under `src/algorithms/`.
3. Add an `AlgorithmDemo` adapter and metadata in `AlgorithmDemo.cpp`.
4. Add a smart preset when the input is not a generic point set.
5. Document its invariant, complexity, and edge cases in `docs/ALGORITHMS.md`.
6. Add at least one normal and one degenerate test.

## Pull requests

Explain the problem, the mathematical approach, complexity, screenshots for UI changes, and test evidence. Keep unrelated refactors in separate pull requests.

