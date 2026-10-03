/**
 * @file include/halo/math/random.hpp
 * The 32-bit lcg random streams and random directions.
 * Declared for other modules through halo/math/api.hpp.
 */
#pragma once

#include "halo/math/globals.hpp"
#include "halo/math/math_types.hpp"

namespace halo::math {

/**
 * A view of one of the engine's 32-bit linear congruential streams: each draw does
 * seed = seed * 0x19660d + 0x3c6ef35f and uses the high 16 bits. The stream does not own its state; it advances
 * random_seed_global, effect_random_seed or a caller's seed in place, in the original draw order.
 */
class random_stream {
public:
    explicit constexpr random_stream(random_seed &state) noexcept : state(state) {}

    /** Advances the stream and returns the new state. */
    constexpr random_seed advance() noexcept
    {
        state = state * k_random_multiplier + k_random_increment;
        return state;
    }

    /** Draws a float in [0, 1): high16 * (1/65536). */
    real next_real() noexcept { return (real)(advance() >> k_random_value_shift) * 1.5259022e-05f; }

    /** Draws a float in [minimum, maximum): (maximum - minimum) * high16 * (1/65536) + minimum, in that order. */
    real next_real(real minimum, real maximum) noexcept
    {
        const random_seed drawn = advance();
        return (maximum - minimum) * (real)(drawn >> k_random_value_shift) * 1.5259022e-05f + minimum;
    }

    /** Draws an integer in [minimum, maximum): (range * high16) >> 16 as unsigned 32-bit arithmetic, plus minimum. */
    int32_t next_int(int16_t minimum, int16_t maximum) noexcept
    {
        const random_seed drawn = advance();
        const int32_t range = (int32_t)maximum - (int32_t)minimum;
        return (int32_t)(((uint32_t)range * (drawn >> k_random_value_shift)) >> 16) + minimum;
    }

    /** Draws an index in [0, count): (high16 * count) >> 16. */
    uint32_t next_index(uint32_t count) noexcept { return ((advance() >> k_random_value_shift) * count) >> 16; }

private:
    random_seed &state;
};

/** The deterministic simulation stream (random_seed_global): game state, reproducible from its seed. */
inline random_stream simulation_random() noexcept { return random_stream(globals().random_seed_global); }

/** The non-deterministic effects stream (effect_random_seed): visuals only, never game state. */
inline random_stream effect_random() noexcept { return random_stream(globals().effect_random_seed); }


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
