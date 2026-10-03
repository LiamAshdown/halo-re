/**
 * @file include/halo/sound/sound_definitions.hpp
 * Sound tag queries: promotion, audibility, distance, permutation and pitch choice.
 */
#pragma once

#include "halo/sound/sound_types.hpp"

namespace halo::sound::definitions {

/**
 * Effective maximum audible distance for a Sound tag: its own maximum_distance when nonzero, otherwise its
 * sound class's default_maximum_distance.
 *
 * @address 0x00545460
 */
float maximum_distance(datum_index sound_definition);

/**
 * Chooses the pitch range that covers `target_pitch`: the current range when it still fits, otherwise the
 * populated range whose bend bounds are closest. Returns -1 when no range qualifies.
 *
 * @address 0x005454a0
 */
int16_t pick_pitch_range(int16_t pitch_range_index, Sound *tag, float target_pitch);

/**
 * Picks the next permutation to play for a pitch range: a previously discarded index takes priority; then (for
 * tags with Sound flags bit 0x02) an explicit chained permutation's next_permutation_index;
 *
 * @address 0x00545590
 */
int16_t pick_permutation(int16_t pitch_range_index, int16_t explicit_permutation_index, Sound *tag);

/**
 * Randomized pitch multiplier: a random value within the tag's random_pitch_bounds, scaled by the
 * zero_pitch_modifier..one_pitch_modifier blend at the sound's current distance fraction.
 *
 * @address 0x0054aec0
 */
float compute_random_pitch(float pitch_bounds_min, float pitch_bounds_max, float zero_pitch_modifier, float one_pitch_modifier, float distance_scale);

/**
 * True if the sound tag has at least one pitch range whose permutations are loaded, and its sound class is not
 * currently muted.
 *
 * @address 0x0054af10
 */
uint32_t has_audible_permutations(TagID sound_tag_id);

/**
 * Rate-limits (re)triggering `sound_tag_id`: accumulates a decaying counter of recently requested permutation
 * time against the tag's promotion_count * longest_permutation_length budget. Returns 0 to play normally, 1 to
 * play the promotion sound instead (and resets the counter), or 2 to refuse outright (no promotion sound
 * configured; the counter is rolled back by this call's own increment so the next call sees the same budget
 * again).
 *
 * @address 0x0054b050
 */
int16_t check_promotion(TagID sound_tag_id);

/**
 * Picks a random distance within detail->distance_bounds; if nonzero, also picks a random pitch and yaw and
 * converts (yaw, pitch, distance) to a Cartesian direction vector. A zero distance returns the zero vector
 * instead (no detail sound placement needed).
 *
 * @address 0x0054e4d0
 */
void random_detail_direction(SoundLoopingDetail *detail, real_vector3d *out);

}  // namespace halo::sound::definitions
