/**
 * @file include/halo/rasterizer/pixel_formats.hpp
 * Conversions from the packed 16 bit pixel formats the bitmap and detail object code reads to A8R8G8B8.
 */
#pragma once

#include <cstdint>

namespace halo::rasterizer {

/** The alpha channel of an opaque A8R8G8B8 pixel. */
inline constexpr uint32_t k_alpha_opaque = 0xffu << 24;

/** Widens a 5 bit channel to 8 bits by replicating its top bits. */
constexpr uint32_t expand_5_bits(uint32_t channel) noexcept
{
    return (channel << 3) | (channel >> 2);
}

/** Widens a 6 bit channel to 8 bits by replicating its top bits. */
constexpr uint32_t expand_6_bits(uint32_t channel) noexcept
{
    return (channel << 2) | (channel >> 4);
}

/** R5G6B5 to opaque A8R8G8B8. */
constexpr uint32_t unpack_r5g6b5(uint32_t pixel) noexcept
{
    return k_alpha_opaque | (expand_5_bits((pixel >> 11) & 31) << 16) | (expand_6_bits((pixel >> 5) & 63) << 8) |
           expand_5_bits(pixel & 31);
}

/** A1R5G5B5 to A8R8G8B8 (the alpha bit becomes 0 or 255). */
constexpr uint32_t unpack_a1r5g5b5(uint32_t pixel) noexcept
{
    return (((pixel >> 15) & 1) != 0 ? k_alpha_opaque : 0u) | (expand_5_bits((pixel >> 10) & 31) << 16) |
           (expand_5_bits((pixel >> 5) & 31) << 8) | expand_5_bits(pixel & 31);
}

/** A4R4G4B4 to A8R8G8B8 (each nibble is repeated into both halves of its byte). */
constexpr uint32_t unpack_a4r4g4b4(uint32_t pixel) noexcept
{
    return (((pixel >> 12) & 15) * 17u << 24) | (((pixel >> 8) & 15) * 17u << 16) | (((pixel >> 4) & 15) * 17u << 8) |
           ((pixel & 15) * 17u);
}

}  // namespace halo::rasterizer
