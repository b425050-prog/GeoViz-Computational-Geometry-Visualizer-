# GeoViz User Guide

## Starting the app

Use the platform instructions in the main README. On first CMake configuration, an installed raylib 5.5 is used when available; otherwise the pinned official source is fetched automatically.

The Windows build uses the GUI subsystem, so opening `GeoViz.exe` launches only the application window.

## Layout

- **Header:** project and student identity.
- **Algorithm Atlas:** grouped list of fourteen labs. Scroll when necessary.
- **Canvas:** editable world coordinates; x points right and y points upward.
- **Toolbar:** Smart, Random, Grid, Ring, and Clear presets.
- **Inspector:** active algorithm, complexity, input rules, statistics, playback, and narration.
- **Footer:** status and shortcut reminder.

## Editing the canvas

| Interaction | Result |
|---|---|
| Left-click empty canvas | Add a point at the world coordinate. |
| Left-drag a point | Move it; releasing creates one undoable move command. |
| Right-click near a point | Remove it. |
| Middle-drag | Pan. |
| Wheel | Zoom at cursor. |
| Home | Restore 100% zoom and centred pan. |

The current algorithm rebuilds after every model mutation. While dragging, this gives continuous visual feedback.

## Playback

The five inspector buttons jump to first, step backward, play/pause, step forward, and jump to last. `-` and `+` adjust playback from 0.5 to 12 frames per second. The progress bar, step number, headline, explanation, and comparison count always match the visible frame.

## Input conventions by lab

### Point-set labs

Primitive Lab, all hulls, Delaunay, Voronoi, closest pair, smallest circle, and rotating calipers accept an unordered point set.

### Ordered polygon labs

Ear clipping expects boundary order. Click clockwise or counter-clockwise around a simple polygon. Do not cross the boundary. Point-in-polygon uses every point except the last as the polygon and treats the final point as the query.

### Paired-point labs

Segment Intersections treats points `(0,1)`, `(2,3)`, and so on as separate segments. An unpaired final point is ignored.

Half-Plane Intersection uses the same pairing, but each pair is an arrow. The **left** side of that directed boundary is feasible. Use Smart preset to see a correctly oriented example.

## Presets

- **Smart:** tailored to the active algorithm's input convention.
- **Random:** twenty random points.
- **Grid:** staggered regular point set, useful for degeneracy testing.
- **Ring:** wavy boundary plus interior points.
- **Clear:** empty scene.

Presets are commands, so `Ctrl+Z` restores the previous scene.

## Save and load

`Ctrl+S` writes `geoviz_scene.geoviz` in the current working directory. `Ctrl+O` loads it. A `.geoviz` file may also be dragged directly onto the window.

Ready-made files in `assets/presets/` can be copied to the working directory, renamed `geoviz_scene.geoviz`, or drag-dropped into the app.

## Troubleshooting

### CMake is not recognized

Install the MSYS2 UCRT64 CMake package and start the **UCRT64** terminal, not the plain MSYS terminal:

```bash
pacman -S --needed mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja
```

### raylib download fails

Check internet access to GitHub, then delete the incomplete build directory and configure again. Alternatively install raylib 5.5 so `find_package` resolves it locally.

### Linux linker/X11 error

Install the development libraries listed in the README's Ubuntu section. raylib builds desktop window and audio backends from those system packages.

### Ear clipping reports a self-intersection

The points probably do not follow one simple boundary. Press `R` for a valid smart preset, then drag its existing vertices rather than adding crossing edges.

### Delaunay displays no triangle

At least three distinct non-collinear sites are required. A grid row or a line alone is degenerate.

### Half-plane result is empty

Check arrow directions. Reversing a point pair reverses its feasible side. Start from the Smart preset and edit one boundary at a time.

### UI feels too dense

Resize the window larger than its minimum, or use 100% operating-system scaling. The canvas remains responsive and the sidebar scrolls.

## Running only tests

```bash
cmake --preset core-only
cmake --build --preset core-only
ctest --test-dir build/core-only --output-on-failure
```

This path does not fetch or compile raylib.

