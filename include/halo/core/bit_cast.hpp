/**
 * @file include/halo/core/bit_cast.hpp
 * Bit-level reinterpretation between two trivially copyable types of equal size, for the engine records whose slots hold a float or an integer
 * depending on the reader.
 */
#pragma once

namespace halo {

/** Returns the bits of `from` as a value of type `To`. */
template <typename To, typename From>
constexpr To bit_cast(const From &from) noexcept
{
    static_assert(sizeof(To) == sizeof(From));
    return __builtin_bit_cast(To, from);
}

}  // namespace halo
