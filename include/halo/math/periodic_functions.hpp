/**
 * @file include/halo/math/periodic_functions.hpp
 * Periodic (wave) and transition (easing) function tables and evaluators.
 * The C symbols other modules link against are the wrappers in src/math/math_c_api.cpp.
 */
#pragma once

#include "halo/math/math_types.hpp"

namespace halo::math {

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
real periodic_function_evaluate(periodic_function_t type, double time);

/**
 * Samples transition table `type` at `phase` in [0,1], lerping two neighbouring entries; reads 0 before the
 * tables exist.
 *
 * Original register convention: CX (low half of ECX) -> type, stack -> phase.
 * @address 0x004ccac0
 */
real transition_function_evaluate(transition_function_t type, real phase);

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
void periodic_function_build_transition_table(transition_function_t type, uint8_t *table);

/**
 * Builds one 1024-byte periodic function table: samples the wave at 1024 phases and quantises it to 0..255,
 * rescaled to its own min/max except for the two slide waves.
 * @address 0x004ccdb0
 */
void periodic_function_build_table(periodic_function_t type, uint8_t *out);

}  // namespace halo::math
