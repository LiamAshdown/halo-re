/**
 * @file include/halo/rasterizer/constants.hpp
 * Named constants of the rasterizer: float bit patterns, fixed-function formats and the numeric ids it compares.
 */
#pragma once

#include <cstdint>

namespace halo::rasterizer {

/** Bit pattern of -1.0f; the light tags use it to mark an omni light (no falloff cone). */
inline constexpr uint32_t k_float_bits_minus_one = 0xbf800000u;
/** Bit pattern of 1.0f. */
inline constexpr uint32_t k_float_bits_one = 0x3f800000u;

}  // namespace halo::rasterizer
