/**
 * @file include/halo/math/random.hpp
 * The 32-bit lcg random streams and random directions.
 * The C symbols other modules link against are the wrappers in src/math/math_c_api.cpp.
 */
#pragma once

#include "halo/math/math_types.hpp"

namespace halo::math {

/**
 * Advances random_seed_global and returns a float in [min, max).
 * @address 0x00401050
 */
real random_real_range(real min, real max);

/**
 * Advances random_seed_global and returns a float in [0, 1).
 * @address 0x004019f0
 */
real random_real();

/**
 * Advances random_seed_global and returns an integer in [min, max).
 *
 * Original register convention: ECX -> min, stack -> max.
 * @address 0x00405320
 */
int32_t random_int_range(int16_t min, int16_t max);

/**
 * Advances effect_random_seed (the non-deterministic stream) and returns a float in [minimum, maximum).
 *
 * Original register convention: stack -> (minimum, maximum).
 * @address 0x00444af0
 */
real random_range_real(real minimum, real maximum);

/**
 * Returns rand() XOR two QueryPerformanceCounter readings scaled by performance_frequency: a non-reproducible
 * seed.
 * @address 0x004cd070
 */
uint32_t random_seed_generate();

/**
 * Advances the caller's seed and returns a float in [min, max).
 *
 * Original register convention: ECX -> seed, stack -> (min, max).
 * @address 0x004cd170
 */
real random_real_range_seeded(random_seed &seed, real min, real max);

/**
 * Rotates `direction` by a random angle in [lo, hi) about a random perpendicular axis taken from
 * sphere_point_table, advancing `seed`. Returns `out`.
 *
 * Original register convention: EAX -> direction, EBX -> out, EDI -> seed, stack -> (lo, hi).
 * @address 0x004cd1b0
 */
real_vector3d * vector3d_randomize_direction(const real_point3d &direction, real_vector3d *out, random_seed &seed, real lo, real hi);

}  // namespace halo::math
