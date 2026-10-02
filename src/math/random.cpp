/**
 * @file src/math/random.cpp
 * The 32-bit lcg random streams and random directions.
 * The original author notes and decompiles are in docs/original/math/.
 */

#include "halo/math/math.hpp"

#include "tags.h"
#include "win32.h"

extern "C" {
extern random_seed random_seed_global;
extern random_seed effect_random_seed;
extern int64_t performance_frequency;
extern int rand(void);
extern double cos(double x);
extern double sin(double x);
extern real_point3d *sphere_point_table;
extern int16_t sphere_point_table_count;
}

namespace halo::math {

real random_real_range(real min, real max)
{
    random_seed_global = random_seed_global * k_random_multiplier + k_random_increment;
    return (max - min) * (real)(random_seed_global >> k_random_value_shift) * 1.5259022e-05f + min;
}

real random_real()
{
    random_seed_global = random_seed_global * k_random_multiplier + k_random_increment;
    return (real)(random_seed_global >> k_random_value_shift) * 1.5259022e-05f;
}

int32_t random_int_range(int16_t min, int16_t max)
{
    int32_t range;

    random_seed_global = random_seed_global * k_random_multiplier + k_random_increment;
    range = (int32_t)max - (int32_t)min;
    return (int32_t)(((uint32_t)range * (random_seed_global >> k_random_value_shift)) >> 16) + min;
}

real random_range_real(real minimum, real maximum)
{
    effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
    return (maximum - minimum) * (real)(effect_random_seed >> k_random_value_shift) * 1.5259022e-05f + minimum;
}

uint32_t random_seed_generate()
{
    large_integer counter_a;
    large_integer counter_b;
    uint32_t rand_value;
    uint32_t scaled_a;
    uint32_t scaled_b;

    QueryPerformanceCounter((LARGE_INTEGER *)&counter_a);
    QueryPerformanceCounter((LARGE_INTEGER *)&counter_b);
    rand_value = (uint32_t)rand();

    scaled_a = (uint32_t)((counter_a.quad_part * 1000) / performance_frequency);
    scaled_b = (uint32_t)(counter_b.quad_part / performance_frequency);

    return rand_value ^ scaled_a ^ scaled_b;
}

real random_real_range_seeded(random_seed &seed, real min, real max)
{
    seed = seed * k_random_multiplier + k_random_increment;
    return (max - min) * (real)(seed >> k_random_value_shift) * 1.5259022e-05f + min;
}

real_vector3d * vector3d_randomize_direction(const real_point3d &direction, real_vector3d *out, random_seed &seed, real lo, real hi)
{
    real_vector3d axis;
    real_point3d *sample;
    int16_t index;
    real length;
    real angle;

    out->i = direction.x;
    out->j = direction.y;
    out->k = direction.z;

    seed = seed * k_random_multiplier + k_random_increment;
    index = (int16_t)(((seed >> k_random_value_shift) * (uint32_t)(int32_t)sphere_point_table_count) >> 16);
    sample = &sphere_point_table[index];

    axis.i = sample->z * direction.y - sample->y * direction.z;
    axis.j = sample->x * direction.z - sample->z * direction.x;
    axis.k = sample->y * direction.x - sample->x * direction.y;

    length = vector3d_normalize_with_length(axis);
    if (0.0f < length) {
        seed = seed * k_random_multiplier + k_random_increment;
        angle = (real)(seed >> k_random_value_shift) * 1.5259022e-05f * (hi - lo) + lo;
        vector3d_rotate_about_axis(*out, axis, (real)sin((double)angle), (real)cos((double)angle));
    }
    return out;
}

}  // namespace halo::math
