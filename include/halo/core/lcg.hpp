#pragma once

#include <cstdint>

namespace halo {

/** Multiplier and increment of the engine's linear congruential generator (random_seed_global, effect_random_seed). */
inline constexpr uint32_t k_random_multiplier = 0x0019660d;
inline constexpr uint32_t k_random_increment = 0x3c6ef35f;

/** The high half of the seed is the random value: seed >> k_random_high_shift. */
inline constexpr uint32_t k_random_high_shift = 16;

/** One step of the generator. */
constexpr uint32_t advance_random_seed(uint32_t seed) noexcept { return seed * k_random_multiplier + k_random_increment; }

/** 1 / 65536: maps a 16-bit value (the high half of a seed, or a fixed-point coordinate) onto [0, 1). */
inline constexpr float k_unit_word_scale = 1.5259022e-05f;

}  // namespace halo
