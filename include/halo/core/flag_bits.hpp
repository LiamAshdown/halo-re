#pragma once

#include <type_traits>
#include "halo/core/flags.hpp"

namespace halo {

/** True when any of the bits of `bit` is set in the raw integer field `raw` (a tag or record field typed as a plain integer). */
template <bit_flags E, typename T>
constexpr bool test_flag(T raw, E bit) noexcept {
    return (static_cast<std::make_unsigned_t<std::underlying_type_t<E>>>(raw) & static_cast<std::make_unsigned_t<std::underlying_type_t<E>>>(bit)) != 0;
}

/** Sets the bits of `bit` in the raw integer field `raw`. */
template <bit_flags E, typename T>
constexpr void set_flag(T &raw, E bit) noexcept {
    raw = static_cast<T>(raw | static_cast<std::underlying_type_t<E>>(bit));
}

/** Clears the bits of `bit` in the raw integer field `raw`. */
template <bit_flags E, typename T>
constexpr void clear_flag(T &raw, E bit) noexcept {
    raw = static_cast<T>(raw & ~static_cast<std::underlying_type_t<E>>(bit));
}

/** Sets or clears the bits of `bit` in the raw integer field `raw`. */
template <bit_flags E, typename T>
constexpr void assign_flag(T &raw, E bit, bool value) noexcept {
    if (value) {
        set_flag(raw, bit);
    } else {
        clear_flag(raw, bit);
    }
}

}  // namespace halo
