# Changelog

All notable changes to GeoViz are documented here. The project follows semantic versioning.

## [1.0.1] — 2026-08-25

### Fixed

- Removed the direct `<windows.h>` include from the native GUI entry point. This prevents MinGW/GCC conflicts between Win32 and raylib declarations such as `ShowCursor`, while preserving a console-free Windows application.

## [1.0.0] — 2026-08-25

### Added

- Native resizable raylib desktop interface with dark glass/neon design.
- Zoomable and pannable mathematical canvas with live point editing.
- Four convex hull strategies: Monotonic Chain, Graham Scan, Jarvis March, and QuickHull.
- Bowyer–Watson Delaunay triangulation and clipped Voronoi diagram with dual overlay.
- Half-plane intersection, ear clipping, closest pair, segment intersection, point-in-polygon, smallest enclosing circle, and rotating calipers labs.
- Shape hierarchy, visitor renderer, strategy registry, observer model, and undoable command system.
- Versioned `.geoviz` scene save/load and drag-and-drop loading.
- Deterministic animation traces with narration, counters, and playback controls.
- Dependency-free C++ regression suite, cross-platform CMake presets, and CI.
- Complete academic report, OOP evidence map, algorithm notes, user guide, and viva preparation.
