#pragma once

#include <cstdint>

namespace halo::text {

/** Extreme 16 bit coordinates used to seed the running bounds of a text measurement. */
inline constexpr int16_t k_text_coordinate_max = 0x7fff;
inline constexpr int16_t k_text_coordinate_min = -0x7fff - 1;

/** A 16 bit text character carries a double byte lead in its high byte. */
inline constexpr uint32_t k_text_high_byte_mask = 0xff00;
inline constexpr uint32_t k_text_low_byte_mask = 0x00ff;

/** High byte value (the '|' escape lead) that introduces a formatting token. */
inline constexpr uint32_t k_text_escape_lead = 0x7c00;

/** Mask that selects the RGB bits of a packed ARGB color. */
inline constexpr uint32_t k_text_rgb_mask = 0x00ffffff;

}
