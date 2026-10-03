/**
 * @file include/halo/sound/gain.hpp
 * Gain, millibel and attenuation conversions.
 */
#pragma once

#include "halo/sound/sound_types.hpp"

namespace halo::sound::gain {

/**
 * Converts a linear gain to a DirectSound attenuation in hundredths of a decibel, 2000 * log10(gain) offset by
 * `maximum`, clamped to [k_sound_minimum_volume, maximum].
 *
 * @address 0x00545710
 */
int32_t linear_to_attenuation(float gain, int32_t maximum);

/**
 * Converts a linear gain to hundredths of a decibel with a caller-supplied floor and ceiling; returns `minimum`
 * for zero or too-quiet gains.
 *
 * @address 0x0054cda0
 */
int32_t linear_to_millibels_clamped(int32_t minimum, float gain, int32_t bias_and_maximum);

/**
 * Converts a DirectSound-style attenuation (hundredths of a decibel) into a linear gain via
 * 10^(attenuation*0.0005), clamped to [minimum, maximum]; the sentinel k_sound_minimum_volume passes `minimum`
 * straight through as the result instead of evaluating the curve.
 *
 * @address 0x0054cdf0
 */
float evaluate_volume_curve(int32_t attenuation, int32_t minimum, int32_t maximum);

/**
 * Limits `gain` to within `ratio` times `compare` (above it) or `compare` divided by `ratio` (below it);
 * returns the gain unchanged for a zero ratio or equal values.
 *
 * @address 0x0054e660
 */
float clamp_by_ratio(float gain, float compare, float ratio);

/**
 * Converts a linear gain to a DirectSound volume in hundredths of a decibel, clamped to `maximum` and floored
 * at k_sound_minimum_volume.
 *
 * @address 0x0054ee70
 */
int32_t to_directsound_volume(float gain, int32_t maximum);

/**
 * Converts the complement of a gain, 1 - gain, to hundredths of a decibel, clamped to the range
 * [k_sound_minimum_volume, 0].
 *
 * @address 0x0054eec0
 */
int to_millibels(float gain);

/**
 * Maps a reverb size to the scale value the EAX environment uses: 1000 below 20, 20000 from 20000, 5000
 * unchanged, otherwise a linear scale.
 *
 * @address 0x00550c10
 */
float reverb_size_scale(float value);

}  // namespace halo::sound::gain
