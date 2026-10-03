#pragma once

#include <cstdint>

namespace halo::game {

/** Game ticks per second. */
inline constexpr int32_t k_ticks_per_second = 30;

/** Game ticks in one minute (the multiplayer clock and score-limit unit). */
inline constexpr int32_t k_ticks_per_minute = 0x708;

/** Game ticks in fifteen seconds (the stock respawn growth, suicide penalty and respawn time step). */
inline constexpr int32_t k_ticks_per_fifteen_seconds = 0x1c2;

/** Packed vehicle-set word the built-in game types store: every 3-bit slot from index 2 up set to 1. */
inline constexpr uint32_t k_default_vehicle_set = 0x249240;

/** Bit pattern of 1.0f, for the places that move floats through integer slots. */
inline constexpr uint32_t k_float_one_bits = 0x3f800000;

/** XOR key of the data iterator signature ('iter'). */
inline constexpr uint32_t k_iterator_signature_key = 0x69746572;

}  // namespace halo::game
