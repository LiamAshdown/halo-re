#pragma once

#include <type_traits>

namespace halo {

/**
 * Opt-in trait that turns an `enum class` into a bit-flag type. Specialise it as
 * `template <> struct enable_bit_flags<my_flags> : std::true_type {};` to enable the operators below.
 */
template <typename E>
struct enable_bit_flags : std::false_type {};

template <typename E>
inline constexpr bool enable_bit_flags_v = enable_bit_flags<E>::value;

template <typename E>
concept bit_flags = std::is_enum_v<E> && enable_bit_flags_v<E>;

template <bit_flags E>
constexpr E operator|(E a, E b) noexcept {
    using U = std::underlying_type_t<E>;
    return static_cast<E>(static_cast<U>(a) | static_cast<U>(b));
}

template <bit_flags E>
constexpr E operator&(E a, E b) noexcept {
    using U = std::underlying_type_t<E>;
    return static_cast<E>(static_cast<U>(a) & static_cast<U>(b));
}

template <bit_flags E>
constexpr E operator^(E a, E b) noexcept {
    using U = std::underlying_type_t<E>;
    return static_cast<E>(static_cast<U>(a) ^ static_cast<U>(b));
}

template <bit_flags E>
constexpr E operator~(E a) noexcept {
    using U = std::underlying_type_t<E>;
    return static_cast<E>(~static_cast<U>(a));
}

template <bit_flags E>
constexpr E &operator|=(E &a, E b) noexcept { return a = a | b; }

template <bit_flags E>
constexpr E &operator&=(E &a, E b) noexcept { return a = a & b; }

template <bit_flags E>
constexpr E &operator^=(E &a, E b) noexcept { return a = a ^ b; }

/** True when every bit of `bit` is set in `value`. */
template <bit_flags E>
constexpr bool has(E value, E bit) noexcept { return (value & bit) == bit; }

/** True when any bit is set in `value`. */
template <bit_flags E>
constexpr bool any(E value) noexcept { return static_cast<std::underlying_type_t<E>>(value) != 0; }

/** The raw integer value of a flag set. */
template <bit_flags E>
constexpr std::underlying_type_t<E> to_bits(E value) noexcept { return static_cast<std::underlying_type_t<E>>(value); }

}  // namespace halo
