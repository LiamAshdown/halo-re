/**
 * @file src/math/utility.cpp
 * Bit vectors, integer log2, qsort comparators, colour packing.
 */

#include "halo/core/crt.hpp"
#include "halo/math/math.hpp"

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "objects.h"
#include "ai.h"

namespace halo::math {

int32_t float_compare_ascending(const void *a_value, const void *b_value)
{
    const real *a = (const real *)a_value;
    const real *b = (const real *)b_value;

    if (*a < *b) {
        return -1;
    }
    if (*b < *a) {
        return 1;
    }
    return 0;
}

int object_sort_by_flag_then_distance(const void *a_record, const void *b_record)
{
    const ai_nearby_actor_candidate *a = (const ai_nearby_actor_candidate *)a_record;
    const ai_nearby_actor_candidate *b = (const ai_nearby_actor_candidate *)b_record;

    if (a->is_type_9 != b->is_type_9) {
        return (int)(a->is_type_9 != 0) * 2 - 1;
    }
    if (a->distance_squared < b->distance_squared) {
        return -1;
    }
    if (b->distance_squared < a->distance_squared) {
        return 1;
    }
    return 0;
}

namespace {
inline int32_t round_to_int(float x) { return static_cast<int32_t>(std::lrint(x)); }
}

uint32_t color_real_to_argb_pack(float alpha, float *rgb)
{
    return ((uint32_t)round_to_int(rgb[2] * 255.0f) & 0xff) |
           (((uint32_t)round_to_int(rgb[1] * 255.0f) & 0xff) << 8) |
           (((uint32_t)round_to_int(rgb[0] * 255.0f) & 0xff) << 0x10) |
           ((uint32_t)round_to_int(alpha * 255.0f) << 0x18);
}

int32_t uint32_log2_floor(uint32_t value)
{
    int32_t result;

    result = 0;
    if (value != 0) {
        while (value != 1) {
            result = result + 1;
            value = value >> 1;
        }
    }
    return result;
}

void bit_vector_or(uint32_t *a, int16_t bit_count, uint32_t *b, uint32_t *dst)
{
    int16_t dword_count;
    int16_t i;

    dword_count = (int16_t)((bit_count + 0x1f) >> 5);
    i = (int16_t)(dword_count - 1);
    while (-1 < i) {
        dst[i] = b[i] | a[i];
        i = (int16_t)(i - 1);
    }
}

}  // namespace halo::math
