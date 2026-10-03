#pragma once

namespace halo {

/** Reinterprets the bits of a value as another type of the same size (std::bit_cast without the <bit> header). */
template <typename To, typename From>
constexpr To bit_cast(const From &from) noexcept {
    return __builtin_bit_cast(To, from);
}

}  // namespace halo
