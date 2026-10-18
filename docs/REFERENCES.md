# Technical and academic references

Use these authoritative starting points when implementing and checking assumptions. Pin actual dependency versions in the build and record what was used. Listing a reference does not claim that a particular version/API has been compiled or that every external site was reachable during documentation preparation.

## Frameworks and language

- [Drogon official repository and documentation wiki](https://github.com/drogonframework/drogon/wiki): controllers, views, filters, sessions, asynchronous networking and database facilities. Verify exact APIs against the selected release.
- [raylib official repository](https://github.com/raysan5/raylib) and [API cheatsheet](https://www.raylib.com/cheatsheet/cheatsheet.html): supported platforms, drawing, input and resource lifetime.
- [CMake documentation](https://cmake.org/cmake/help/latest/) and [CTest manual](https://cmake.org/cmake/help/latest/manual/ctest.1.html): target-based builds, configurations and test execution.
- [C++ reference](https://en.cppreference.com/w/): standard containers, chrono, smart pointers, ordering requirements and exceptions. This is a reference site, not the ISO standard itself.

## Persistence and security

- [SQLite foreign keys](https://www.sqlite.org/foreignkeys.html), [transactions](https://www.sqlite.org/lang_transaction.html), [WAL](https://www.sqlite.org/wal.html) and [backup API](https://www.sqlite.org/backup.html).
- [libsodium password hashing](https://doc.libsodium.org/password_hashing/default_phf): supported high-level password storage/verification and parameter guidance.
- [OWASP Cheat Sheet Series](https://cheatsheetseries.owasp.org/): authentication, session management, CSRF, authorization, file uploads, XSS prevention and logging. Select relevant controls and test them in the actual architecture.
- [HTML standard](https://html.spec.whatwg.org/): forms, media elements and browser behaviour. Native controls do not imply measured watch time.

## Algorithms

1. Ester, M.; Kriegel, H.-P.; Sander, J.; Xu, X. (1996). **A Density-Based Algorithm for Discovering Clusters in Large Spatial Databases with Noise.** Proceedings of KDD-96, pp. 226–231. [Original paper](https://cdn.aaai.org/KDD/1996/KDD96-037.pdf). Use it for density/core/border/noise definitions; document the project's deterministic border convention.
2. Dijkstra, E. W. (1959). **A note on two problems in connexion with graphs.** Numerische Mathematik 1, pp. 269–271. [DOI](https://doi.org/10.1007/BF01386390).
3. Hart, P. E.; Nilsson, N. J.; Raphael, B. (1968). **A Formal Basis for the Heuristic Determination of Minimum Cost Paths.** IEEE Transactions on Systems Science and Cybernetics 4(2), pp. 100–107. [DOI](https://doi.org/10.1109/TSSC.1968.3001369). Distinguish admissibility, consistency and implementation reopening rules.
4. Andrew, A. M. (1979). **Another efficient algorithm for convex hulls in two dimensions.** Information Processing Letters 9(5), pp. 216–219. [DOI](https://doi.org/10.1016/0020-0190(79)90072-3).
5. [CGAL manual](https://doc.cgal.org/latest/Manual/packages.html): robust geometry and spatial-searching background. CGAL is a reference/possible later dependency, not a required library in the initial design.
6. [Boost Graph Library documentation](https://www.boost.org/doc/libs/release/libs/graph/): graph concepts and algorithms for comparison/reference, not a claim that the project uses Boost Graph.

## Project-owned sources and evidence

- [Original proposal](proposal/Computational_Geometry_Visualizer_Submission.pdf): academic baseline, preserved unchanged.
- [Specification](PROJECT_SPECIFICATION.md): expanded agreed scope.
- [Contracts](CONTRACTS.md): version-1 engineering definitions.
- [Evaluation protocol](evaluation/SCENARIOS_AND_METRICS.md): how synthetic results will be measured.
- [Validation record](testing/VALIDATION_RECORD.md): actual execution evidence, initially unrun.

## Citation and asset practice

Distinguish an original implementation of a published algorithm from a new algorithmic contribution. Cite sources used for code/design, respect library and media licenses, and record original/third-party sample media attribution. Do not assign a repository-wide license or claim permission for external assets without the owner's decision/evidence.

Concept diagrams/GIFs currently in this repository are project illustrations using synthetic data. Future map/media assets need their own source/license register before distribution. An institution's certification/signature must come from the institution, not from this draft.
