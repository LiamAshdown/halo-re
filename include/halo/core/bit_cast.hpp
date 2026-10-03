<<<<<<< HEAD
=======
/**
 * @file include/halo/core/bit_cast.hpp
 * Bit-level reinterpretation between two trivially copyable types of equal size, for the engine records whose slots hold a float or an integer
 * depending on the reader.
 */
>>>>>>> worktree-agent-a49134934c41fcfa8
#pragma once

namespace halo {

<<<<<<< HEAD
/** Reinterprets the bits of a value as another type of the same size (std::bit_cast without the <bit> header). */
template <typename To, typename From>
constexpr To bit_cast(const From &from) noexcept {
=======
/** Returns the bits of `from` as a value of type `To`. */
template <typename To, typename From>
constexpr To bit_cast(const From &from) noexcept
{
    static_assert(sizeof(To) == sizeof(From));
>>>>>>> worktree-agent-a49134934c41fcfa8
    return __builtin_bit_cast(To, from);
}

}  // namespace halo
