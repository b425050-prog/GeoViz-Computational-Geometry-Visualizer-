# OOP design and C++ feature map

**Status:** Planned class responsibilities and academic explanations. File/class references become implementation evidence only after the corresponding code exists.

## 1. Why OOP is useful here

The project contains different kinds of objects with different lifetimes. An account enforces identity rules. A repository persists it. An algorithm transforms a snapshot. A renderer displays a result. Putting all of these responsibilities in one class makes correctness and testing difficult.

OOP provides boundaries. Encapsulation protects valid states; interfaces separate policy from infrastructure; polymorphism lets multiple algorithm strategies share the same user interaction. Composition keeps account, content and review policies reusable without forcing every object into a large inheritance tree.

## 2. Class families

| Family | Proposed types | What belongs here |
| :--- | :--- | :--- |
| Domain values | `AccountId`, `ContentId`, `Point2D`, `TimeWindow`, `AlgorithmSettings` | Validated identifiers, units and immutable inputs. |
| Entities | `Account`, `Content`, `Report`, `InvestigationCase` | Identity plus controlled state transitions. |
| Policies | `ContentAccessPolicy`, `ActionPolicy`, `AgePolicy` | Decisions shared by routes, media and exports. |
| Services | `AuthService`, `ReportService`, `AnalysisService`, `CaseService` | Use-case coordination and transaction boundaries. |
| Repositories | `IAccountRepository`, `IEventRepository`, `ICaseRepository` | Storage contracts and database/in-memory implementations. |
| Analysis | `WindowAggregator`, `FeatureExtractor`, `DbscanClusterer`, `EvidenceExplainer` | Pure transformations and explained outputs. |
| Lab | `IAlgorithmDemo`, `DemoRegistry`, `SceneModel`, `StepTrace` | Mode discovery, validated scenes and recorded decisions. |
| Presentation | `ConsoleScreen`, `CanvasRenderer`, `PlaybackController` | Input, rendering and local view state. |
| Infrastructure | `SystemClock`, `FakeClock`, `SqliteConnection`, `MediaStore` | Resource ownership and external capabilities. |

## 3. Encapsulation

Do not make account state, role flags or birth-date eligibility mutable public fields. Methods such as `deactivate(reason)` or a service command should validate the transition and record its consequences. A public `account.role = administrator` assignment would bypass the use case.

Likewise, construct a `TimeWindow` only when end is later than start. Reject nonfinite geometry coordinates. Validation near construction reduces the number of invalid states later algorithms must handle.

DTOs may be simple public-value structs after validation. Encapsulation does not mean wrapping every number in getters and setters.

## 4. Abstraction, inheritance and polymorphism

This is an **interface sketch**, not a library API already implemented:

```cpp
struct Point2D {
    double x;
    double y;
};

struct HullResult {
    std::vector<Point2D> vertices;
};

class IHullStrategy {
public:
    virtual ~IHullStrategy() = default;
    virtual HullResult compute(std::span<const Point2D> points) const = 0;
};
```

The sketch requires the appropriate standard headers. `MonotoneChain` can implement this interface; a later `GrahamScan` is another strategy. The UI uses the interface, while each implementation supplies its algorithm. A virtual destructor is necessary when derived objects are destroyed through a base pointer.

Do not claim inheritance merely because a class uses a vector. That is composition. Do not add a subclass unless there is a meaningful substitutable abstraction.

## 5. Patterns with concrete purposes

| Pattern | Use | What to avoid |
| :--- | :--- | :--- |
| Strategy | Replace hull/clustering implementations or lab modes behind compatible contracts. | Treating algorithms with incompatible units as interchangeable without validation. |
| Factory/registry | Construct the selected demo from a registered mode ID. | A giant UI switch containing every algorithm body. |
| Command | Undo/redo scene edits through reversible local commands. | Using undo to delete audit events or reverse server actions invisibly. |
| Observer | Notify linked views of validated scene/run changes. | Callbacks with dangling object lifetimes or feedback loops. |
| Repository | Isolate persistence and support test doubles. | Hiding arbitrary unrelated SQL in one universal repository. |
| Visitor, where useful | Draw heterogeneous geometry without embedding raylib in the domain. | Adding visitors to simple value structs without a real need. |

## 6. RAII, ownership and exceptions

Use automatic-storage objects and containers by default. Use `std::unique_ptr` for exclusive polymorphic ownership and `std::shared_ptr` only when shared lifetime is actually needed. A raw pointer/reference can be non-owning; document its lifetime.

Wrap files, database transactions, locks and raylib resources so cleanup happens on every exit path. A transaction object should roll back unless committed. A `std::lock_guard` releases its mutex even when an exception is thrown.

Avoid copying resource-owning objects unintentionally. Prefer the rule of zero when standard members already manage lifetime. Explicitly delete copying for exclusive resource wrappers; provide move support only when the underlying resource permits it.

## 7. STL and language features

| Feature | Meaningful project use |
| :--- | :--- |
| `std::vector` | Points, immutable event snapshots, trace events and adjacency lists. |
| `std::unordered_map` / `std::map` | ID lookup, feature aggregation and deterministic ordered outputs when needed. |
| `std::queue` | DBSCAN expansion or BFS. |
| `std::priority_queue` | Dijkstra/A* frontier and review ordering. |
| `std::sort` | Hull preparation and deterministic event/tie ordering. |
| `std::optional` | Missing coordinates, unknown historical values, absent routes. |
| `std::variant` | Typed trace events with distinct payloads. |
| `std::chrono` | Explicit durations and timestamps rather than mixed integer units. |
| Templates | Shared metric/query helpers when behaviour remains clear. |
| Operator overloading | Vector addition/subtraction where mathematical meaning is natural. |
| `const` and references | Immutable algorithm inputs and safe sharing without unnecessary copies. |

## 8. Academic evidence checklist

After implementation, add exact file/line references and a small example for each feature actually used. Explain its reason, the invariant it protects and the test demonstrating it. Do not list unused features, fabricate classes or describe a planned pattern as proven in source.

The [report](../academic/PROJECT_REPORT.md) summarizes this design; the [demonstration guide](../user/DEMONSTRATION.md) covers the product presentation.
