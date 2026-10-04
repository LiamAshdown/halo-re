#pragma once

namespace halo::math {

/** Pi and its multiples, as the single-precision values the engine uses. */
inline constexpr float k_pi = 3.1415927f;
inline constexpr float k_two_pi = 6.2831855f;
inline constexpr float k_half_pi = 1.5707964f;

/** Degree/radian conversion factors (single precision). */
inline constexpr float k_degrees_to_radians = 0.017453292f;
inline constexpr float k_radians_to_degrees = 57.29578f;

/** Duration of one game tick in seconds (1/30), single precision. */
inline constexpr float k_seconds_per_tick = 0.033333335f;

}  // namespace halo::math
