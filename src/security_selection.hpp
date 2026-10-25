#pragma once
#include "security.hpp"
namespace gv {
inline bool selected(const Event &e, const SecuritySettings &s) {
    return e.timestamp() >= s.start() && e.timestamp() < s.end() &&
           (s.user() == "*" || s.user() == e.user()) &&
           (s.device() == "*" || s.device() == e.device());
}

} // namespace gv
