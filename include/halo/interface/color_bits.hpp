/**
 * @file include/halo/interface/color_bits.hpp
 * Packed colour access: the 4-byte colour structs of the HUD tags read and written as one ARGB dword.
 */
#pragma once

#include <stdint.h>
#include <string.h>

namespace halo::interface {

/** The packed ARGB dword of a 4-byte colour struct (ColorARGBInt and the tag colour fields built from it). */
template <typename C>
inline uint32_t color_bits(const C &color) {
    static_assert(sizeof(C) == sizeof(uint32_t), "color_bits needs a 4-byte colour");
    uint32_t bits;

    memcpy(&bits, &color, sizeof(bits));
    return bits;
}

/** Stores a packed ARGB dword into a 4-byte colour struct. */
template <typename C>
inline void set_color_bits(C &color, uint32_t bits) {
    static_assert(sizeof(C) == sizeof(uint32_t), "set_color_bits needs a 4-byte colour");
    memcpy(&color, &bits, sizeof(bits));
}

}  // namespace halo::interface
