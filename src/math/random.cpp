/**
 * @file src/math/random.cpp
 * The 32-bit lcg random streams and random directions.
 */

#include "halo/core/crt.hpp"
#include "halo/math/math.hpp"
#include "halo/math/globals.hpp"

#include "tags.h"
#include "win32.h"
#include "halo/cseries/api.hpp"

namespace halo::math {

real random_real_range(real min, real max)
{
    return simulation_random().next_real(min, max);
}

real random_real()
{
    return simulation_random().next_real();
}

int32_t random_int_range(int16_t min, int16_t max)
{
    return simulation_random().next_int(min, max);
}

real random_range_real(real minimum, real maximum)
{
    return effect_random().next_real(minimum, maximum);
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

    scaled_a = (uint32_t)((counter_a.quad_part * 1000) / halo::cseries::globals().performance_frequency);
    scaled_b = (uint32_t)(counter_b.quad_part / halo::cseries::globals().performance_frequency);

    return rand_value ^ scaled_a ^ scaled_b;
}

real random_real_range_seeded(random_seed &seed, real min, real max)
{
    return random_stream(seed).next_real(min, max);
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

    random_stream rng(seed);

    index = (int16_t)rng.next_index((uint32_t)(int32_t)globals().sphere_point_table_count);
    sample = &globals().sphere_point_table[index];

    axis.i = sample->z * direction.y - sample->y * direction.z;
    axis.j = sample->x * direction.z - sample->z * direction.x;
    axis.k = sample->y * direction.x - sample->x * direction.y;

    length = vector3d_normalize_with_length(axis);
    if (0.0f < length) {
        angle = rng.next_real() * (hi - lo) + lo;
        vector3d_rotate_about_axis(*out, axis, (real)sin((double)angle), (real)cos((double)angle));
    }
    return out;
}

}  // namespace halo::math
