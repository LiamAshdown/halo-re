/**
 * @file include/halo/math/utility.hpp
 * Bit vectors, integer log2, qsort comparators, colour packing.
 * Declared for other modules through halo/math/api.hpp.
 */
#pragma once

#include "halo/math/math_types.hpp"

namespace halo::math {

/**
 * qsort comparator on a leading float: -1, 1 or 0 (also 0 when either is NaN).
 * @address 0x00405360
 */
int32_t float_compare_ascending(const void *a_value, const void *b_value);

/**
 * qsort comparator for ai_nearby_actor_candidate: records whose is_type_9 byte is clear sort first, then by
 * ascending distance_squared.
 *
 * Original register convention: stack -> (a, b).
 * @address 0x00433c70
 */
int object_sort_by_flag_then_distance(const void *a_record, const void *b_record);

/**
 * Packs an alpha and an r,g,b float triple (each 0..1) into a 0xAARRGGBB dword, rounding each channel*255 to
 * nearest; only the low 8 bits of each colour channel are kept.
 *
 * Original register convention: color_real_to_argb_pack(float alpha, float *rgb) -- cdecl.
 * @address 0x0044da60
 */
uint32_t color_real_to_argb_pack(float alpha, float *rgb);

/**
 * Index of the highest set bit (0 for 0 and 1).
 *
 * Original register convention: ECX -> value.
 * @address 0x004cb740
 */
int32_t uint32_log2_floor(uint32_t value);

/**
 * Bitwise-ORs bit array `a` into `b`, word by word, writing the result to `dst`; `bit_count` is rounded up to
 * whole 32-bit words.
 *
 * Original register convention: EAX -> a, ECX (low 16) -> bit_count, EDX -> b, stack -> dst.
 * @address 0x004cb760
 */
void bit_vector_or(uint32_t *a, int16_t bit_count, uint32_t *b, uint32_t *dst);

}  // namespace halo::math
