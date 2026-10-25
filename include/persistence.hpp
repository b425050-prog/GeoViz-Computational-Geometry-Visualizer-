#pragma once
#include "atomic_files.hpp"
// The engine deliberately has no raylib dependency. Coordinates here are doubles.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "trace.hpp"

namespace gv {
class SceneIO {
  public:
    static void save(const Scene &scene, const std::string &path, AtomicFiles *external = nullptr);
    static Scene load(const std::string &path);
    static void summary(const Run &run, const std::string &path, AtomicFiles *external = nullptr);
};
// Six deterministic inputs per mode, shared by the engine, interface and documentation.
// Keeping samples in the engine lets tests, documentation and the GUI use identical data.
constexpr int SampleVariantCount = 6;
Scene sampleScene(Mode mode, int variant = 0);
std::string sampleName(Mode mode, int variant);
Scene gridScene();

} // namespace gv
