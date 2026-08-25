#pragma once

/**
 * @file Utility.hpp
 * @brief Small constrained templates shared by independent GeoViz modules.
 */

#include <algorithm>
#include <concepts>
#include <stdexcept>

namespace geoviz::core {

/** @brief Compile-time generic square operation restricted to arithmetic types. */
template <typename T>
concept Arithmetic = std::integral<T> || std::floating_point<T>;

template <Arithmetic T>
[[nodiscard]] constexpr T square(T value) noexcept {
    return value * value;
}

/**
 * @brief Reusable class template that guarantees a value remains in [min, max].
 *
 * TracePlayer uses BoundedValue<float> for animation speed.  This provides a
 * concrete example of C++20 concepts and compile-time polymorphism in addition
 * to the project's runtime Shape/Strategy polymorphism.
 */
template <std::totally_ordered T>
class BoundedValue final {
public:
    constexpr BoundedValue(T value, T minimum, T maximum)
        : value_(value), minimum_(minimum), maximum_(maximum) {
        if (maximum_ < minimum_) {
            throw std::invalid_argument("BoundedValue maximum is below minimum.");
        }
        value_ = std::clamp(value_, minimum_, maximum_);
    }

    constexpr void set(T value) noexcept { value_ = std::clamp(value, minimum_, maximum_); }
    [[nodiscard]] constexpr const T& get() const noexcept { return value_; }
    [[nodiscard]] constexpr const T& minimum() const noexcept { return minimum_; }
    [[nodiscard]] constexpr const T& maximum() const noexcept { return maximum_; }

private:
    T value_;
    T minimum_;
    T maximum_;
};

static_assert(square(5) == 25);
static_assert(square(2.5) == 6.25);

}  // namespace geoviz::core

