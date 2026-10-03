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

/** `value` with its low byte replaced (the value a function returns in EAX when it only sets AL). */
constexpr uint32_t replace_low_byte(uint32_t value, uint8_t low) noexcept
{
    return (value & ~0xffu) | low;
}

/**
 * Packs three unit-range channels into an opaque D3DCOLOR the way the model and environment passes build their
 * texture factor: each channel is truncated to an integer and merged into the 0xff.. pattern, so out-of-range values
 * spill into the higher channels exactly as they do in the original code.
 */
inline uint32_t pack_opaque_color(float red, float green, float blue) noexcept
{
    uint32_t color = ~0xffu | static_cast<uint32_t>(static_cast<int32_t>(red * 255.0f));

    color = (color << 8) | (static_cast<uint32_t>(static_cast<int32_t>(green * 255.0f)) & 0xffu);
    color = (color << 8) | (static_cast<uint32_t>(static_cast<int32_t>(blue * 255.0f)) & 0xffu);
    return color;
}

}  // namespace halo::rasterizer
