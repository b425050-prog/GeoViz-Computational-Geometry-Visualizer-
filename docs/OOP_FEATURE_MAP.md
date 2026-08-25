# OOP and Modern C++ Feature Map

This document is a quick evidence index for the OOPS Lab evaluation. It maps each important course concept to a concrete, working part of GeoViz rather than adding unused demonstration classes.

## The four pillars

| Pillar | Evidence in GeoViz | Why it matters |
|---|---|---|
| Encapsulation | `Point`, `Vector2D`, `Circle`, `SceneModel`, `TracePlayer`, and `CommandManager` keep state private and expose validated operations. | A circle cannot acquire a negative radius; history cannot be mutated outside its API. |
| Abstraction | `Shape`, `ShapeVisitor`, `ConvexHullStrategy`, `DescribedComponent`, `TraceProducer`, `SceneCommand`, and `AlgorithmDemo` are abstract interfaces. | The UI asks for a trace without knowing the concrete algorithm. |
| Inheritance | Seven concrete shapes inherit `Shape`; four hull strategies inherit `ConvexHullStrategy`; commands and demos form additional hierarchies. | Common contracts eliminate repeated control logic. |
| Polymorphism | `Shape::accept`, `Shape::clone`, `ConvexHullStrategy::compute`, `SceneCommand::execute/undo`, and `AlgorithmDemo::buildTrace` use virtual dispatch. | Shapes render and algorithms switch at run time through base references/pointers. |

## Inheritance forms

| Form | Concrete example |
|---|---|
| Single inheritance | `Circle final : public Shape` |
| Hierarchical inheritance | `Point`, `Line`, `Ray`, `Segment`, `Circle`, `Triangle`, and `Polygon` all derive from `Shape`. |
| Multilevel inheritance | `AlgorithmDemo` derives from capability interfaces, then `DelaunayDemo`, `VoronoiDemo`, and other concrete demos derive from `AlgorithmDemo`. |
| Multiple inheritance | `AlgorithmDemo : public DescribedComponent, public TraceProducer` composes two pure interfaces without shared data or a diamond. |
| Interface inheritance | `ShapeVisitor`, `SceneCommand`, and `ConvexHullStrategy` contain pure virtual operations and virtual destructors. |

## Class relationships

| Relationship | Example | Interpretation |
|---|---|---|
| Composition | `Application` owns `SceneModel`, `CommandManager`, `DemoRegistry`, `TracePlayer`, and renderer values. | These collaborators share the application's lifetime. |
| Aggregation | `GeometryRenderer` stores references to `CanvasTransform` and `Theme`. | It uses objects owned by `Application` without owning them. |
| Association | `SceneCommand::execute(SceneModel&)` temporarily operates on a model. | Neither object owns the other. |
| Dependency | Demos call framework-independent algorithm classes to build traces. | Algorithms can change without changing the window lifecycle. |

## C++ language evidence

| Feature | Location | Example / purpose |
|---|---|---|
| Classes and objects | `Primitives.hpp` throughout | Domain objects model mathematical entities. |
| Access specifiers | Every non-trivial class | State is private; stable behavior is public. |
| Default constructor | `Point`, `Vector2D`, `AlgorithmTrace` | Safe zero/empty state. |
| Parameterized constructor | `Circle(Point, double)`, `Polygon(vector<Point>)` | Establish validated invariants. |
| Delegating constructor | `Line(Point, Point)`, `Ray(Point, Point)`, `HalfPlane(Point, Point)` | Reuses direction validation. |
| Virtual destructor | Every polymorphic base | Safe destruction through base pointers. |
| Constructor/destructor RAII | `Application`, streams, vectors, smart pointers | Resources are cleaned up deterministically on every exit path. |
| Deleted copy/move | `Application` | Prevents accidental duplication of a window-owning coordinator. |
| Function overloading | `intersect(...)` in `Predicates.hpp` | One semantic operation handles line/line, segment/segment, ray/segment, and circles. |
| Operator overloading | `Point`, `Vector2D`, `EdgeKey`, `QuantizedPoint` | Natural vector arithmetic, stream output, and ordered map keys. |
| Friend function | `operator<<(ostream&, const Point&)` | Controlled formatted access to encapsulated coordinates. |
| Runtime polymorphism | `Shape`, `ConvexHullStrategy`, `AlgorithmDemo`, `SceneCommand` | Behavior selected through virtual functions. |
| Compile-time polymorphism | `core::BoundedValue<T>` and `core::square(T)` | Constrained generic code is instantiated for the requested type. |
| C++20 concepts | `Arithmetic`, `std::totally_ordered` constraints | Invalid template types fail at compile time with a clear contract. |
| Templates | Custom utility templates plus STL containers/algorithms | Reusable code without sacrificing type safety. |
| Smart pointers | `unique_ptr<Shape>`, strategies, demos, commands | Explicit exclusive ownership and no manual `delete`. |
| STL containers | `vector`, `map`, `set`, `optional`, `array` | Appropriate data structures for algorithms and ownership. |
| STL algorithms | `sort`, `min_element`, `unique`, `clamp`, `none_of` | Expressive, tested operations instead of error-prone loops. |
| Iterators | Hull scans, erase operations, range constructors | Generic traversal independent of storage detail. |
| `std::span` | Public algorithm inputs | Non-owning, bounds-aware view accepts vectors and arrays. |
| `std::optional` | circumcenter, closest pair, circle, selection | Absence is represented explicitly rather than with magic values. |
| `enum class` | `ShapeKind`, `Orientation`, `DemoId`, result kinds | Scoped, strongly typed state. |
| Lambdas | sorting, observers, recursion helpers, predicates | Local behavior stays close to its use. |
| Recursive functions | QuickHull, closest pair, Douglas–Peucker | Natural expression of divide-and-conquer algorithms. |
| Move semantics | Trace frames, snapshots, presets, command ownership | Large vectors transfer without unnecessary copies. |
| Exception handling | `GeometryError`, file loading, app boundary | Invalid geometry and I/O errors have controlled recovery and UI feedback. |
| File I/O | `SceneSerializer.cpp` | Versioned, human-readable persistence with strict validation. |
| Filesystem library | scene save/load and tests | Cross-platform paths instead of platform-specific APIs. |
| Namespaces | `geoviz`, `geoviz::algorithms`, `geoviz::app`, `geoviz::io` | Prevents collisions and communicates module boundaries. |
| `constexpr` / `inline` | constants, accessors, generic utilities | Compile-time evaluation and one-definition safety. |
| `[[nodiscard]]` | queries and algorithm results | Compiler warns when meaningful results are ignored. |
| `noexcept` | safe getters and simple predicates | Documents and enables non-throwing operations. |
| Const-correctness | all read-only algorithms and render inputs | Mutability is explicit and minimized. |
| Structured bindings | Delaunay edge maps, statistics, test results | Clear decomposition of tuples and map entries. |
| Three-way comparison | internal Delaunay/Voronoi keys | Correct total ordering for associative containers. |
| Static assertions | `Utility.hpp` | Verifies template behavior at compile time. |
| Preprocessor portability | `main.cpp` | Native Windows GUI entry point and standard POSIX `main`. |

## Design pattern evidence

### Strategy

`ConvexHullStrategy` exposes `compute(span<const Point>)`. `MonotonicChainHull`, `GrahamScanHull`, `JarvisMarchHull`, and `QuickHull` implement different behavior behind that contract. `makeHullStrategy()` chooses one at run time.

### Visitor / double dispatch

Every `Shape` implements `accept(ShapeVisitor&)`. `GeometryRenderer` implements one `visit` overload per concrete shape. The first virtual dispatch selects `accept`; the overload selects the concrete rendering behavior. Domain types never include `raylib.h`.

### Command

`AddPointCommand`, `RemovePointCommand`, `MovePointCommand`, and `ReplaceSceneCommand` store enough state to implement both `execute` and `undo`. `CommandManager` transfers `unique_ptr` ownership between undo and redo stacks.

### Observer

`Application` subscribes to `SceneModel`. Each validated mutation increments a revision and notifies observers; the application marks the current animation trace dirty and rebuilds it before rendering.

### Factory and registry

`makeHullStrategy()` returns a concrete strategy through `unique_ptr<ConvexHullStrategy>`. `DemoRegistry` constructs and owns the 14 polymorphic sidebar modules, while callers see only `AlgorithmDemo&`.

## Suggested viva demonstration

1. Open `Primitives.hpp` and explain private data plus the abstract `Shape` contract.
2. Jump to `Renderer.cpp` and show that rendering is selected through `accept`/`visit` without a `dynamic_cast` chain.
3. Switch among all four hull labs; explain that the application uses the same `AlgorithmDemo` contract.
4. Add a point, undo, and redo; trace the `unique_ptr<SceneCommand>` movement.
5. Drag a point and show the observer-triggered algorithm rebuild.
6. Save a scene, open the human-readable `.geoviz` file, and reload it.
7. Run the tests to prove that the mathematical core does not require the graphical window.

