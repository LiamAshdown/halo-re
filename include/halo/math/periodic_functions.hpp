/**
 * @file include/halo/math/periodic_functions.hpp
 * Periodic (wave) and transition (easing) function tables and evaluators.
 * The C symbols other modules link against are the wrappers in src/math/math_c_api.cpp.
 */
#pragma once

#include "halo/math/math_types.hpp"

namespace halo::math {

/** The two phases a wave is sampled at while its table is built: i*28/1024 and the noise-warped noise[i]*28. */
struct wave_phase {
    real linear;
    real warped;
};

/**
 * One periodic (wave) function as a Strategy: the table builder asks the wave registered for a selector for each of
 * its 1024 samples. Waves are immutable objects in static storage, never in game state.
 */
class periodic_wave {
public:
    /** The unquantised sample at `phase`; `seed` is the builder's copy of random_seed_global (only noise draws). */
    virtual real sample(const wave_phase &phase, random_seed &seed) const = 0;

protected:
    ~periodic_wave() = default;
};

/** One transition (easing) curve on [0, 1] as a Strategy, registered per transition_function_type. */
class transition_curve {
public:
    /** The curve at t in [0, 1]. */
    virtual real value(real t) const = 0;

protected:
    ~transition_curve() = default;
};

/** The wave registered for `type`, or nullptr outside the twelve selectors (the builder then repeats its last sample). */
const periodic_wave *periodic_wave_for(periodic_function_type type);

/** The curve registered for `type`, or nullptr outside the six selectors. */
const transition_curve *transition_curve_for(transition_function_type type);


/**
 * Allocates and builds the twelve periodic and six transition tables, seeding random_seed_global to 0x20f3f660
 * first so the tables are reproducible.
 * @address 0x004cc8d0
 */
void periodic_function_tables_init();

/**
 * Frees the periodic and transition tables if they were built.
 * @address 0x004cc960
 */
void periodic_function_tables_free();

/**
 * Samples periodic function `type` at `time` (seconds; 25.6 samples per unit), lerping two neighbouring table
 * entries. Type 0 is always 1; every type reads 0 before the tables exist.
 *
 * Original register convention: AX (low half of EAX) -> type, stack -> time.
 * @address 0x004cc9b0
 */
real periodic_function_evaluate(periodic_function_type type, double time);

/**
 * Samples transition table `type` at `phase` in [0,1], lerping two neighbouring entries; reads 0 before the
 * tables exist.
 *
 * Original register convention: CX (low half of ECX) -> type, stack -> phase.
 * @address 0x004ccac0
 */
real transition_function_evaluate(transition_function_type type, real phase);

/**
 * Fills 1024 floats with a normalised cumulative noise curve used to warp the phase of the variable-period
 * waves; advances random_seed_global.
 *
 * Original register convention: EDX -> table.
 * @address 0x004ccbb0
 */
void periodic_function_build_noise_table(real *table);

/**
 * Builds one 1024-byte transition (easing) table: entry i = curve(i/1023) * 255, clamped to 0..255.
 *
 * Original register convention: EBX (low half, sign-extended) -> type, stack -> table.
 * @address 0x004cccb0
 */
void periodic_function_build_transition_table(transition_function_type type, uint8_t *table);

/**
 * Builds one 1024-byte periodic function table: samples the wave at 1024 phases and quantises it to 0..255,
 * rescaled to its own min/max except for the two slide waves.
 * @address 0x004ccdb0
 */
void periodic_function_build_table(periodic_function_type type, uint8_t *out);

}  // namespace halo::math
