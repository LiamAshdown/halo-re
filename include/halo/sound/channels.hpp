/**
 * @file include/halo/sound/channels.hpp
 * Logical playback channels: assignment, stealing and the channel parameter policies.
 */
#pragma once

#include "halo/sound/sound_types.hpp"

namespace halo::sound::channels {

/**
 * Runs the streaming buffer-fill update (sound_channel_stream_update) for every channel of type 0 currently
 * marked as streaming (the mono 3D block: the only channels whose reset keeps them streaming, see
 * sound_channel_reset).
 *
 * @address 0x00546b40
 */
void update_streaming(void);

/**
 * True when a driver channel type (sample rate, stereo, compressed) fits the requested sound format; for mono
 * channels the 3D flag must also match the request.
 *
 * @address 0x00548520
 */
uint8_t type_flags_match(int16_t compressed_requested, int16_t stereo_requested, uint16_t sample_rate_44khz_requested, uint16_t flags, int16_t requested_3d);

/**
 * Per-update pass over every live sound whose start_time has arrived: if its permutation is not yet resident in
 * the sound cache, either leaves it waiting (retried next pass) or, for classes whose discard_on_cache_miss is
 * set, gives up and stops it (remembering the skipped permutation so it is not retried). Otherwise requests a
 * playback channel for it, stealing whatever sound currently occupies the chosen channel.
 *
 * @address 0x0054c020
 */
void assign(void);

/**
 * Scans every logical playback channel for ones playing the exact same sound tag as `sound_handle` (recording
 * up to 16 in out->tag_matches), and among those, ones additionally owned by the same owner (recording up to 16
 * in out->owner_matches). Also copies the class's per-tag/per-object channel limits into the output.
 *
 * @address 0x0054c1d0
 */
void build_candidates(sound_channel_candidate_list *out, datum_index sound_handle);

/**
 * Chooses a playback channel for a sound that does not have one yet. Non-dialog sounds (and dialog sounds with
 * no owner) first try to steal a channel from another sound sharing their owner or tag, once the relevant
 * budget is exceeded; dialog sounds with an owner instead look for another currently-playing dialog sound from
 * the same owner and take over its channel (copying its location type). Falls back to
 * sound_find_lowest_priority_channel otherwise.
 *
 * @address 0x0054c2f0
 */
int16_t pick_for_instance(datum_index sound_handle);

/**
 * Scans every logical channel for one whose type flags match `sound_handle`'s spatialization, sample rate,
 * channel count and format needs: returns the first free one immediately, otherwise the occupied one whose
 * sound the candidate outranks by the widest margin (lowest class priority, then farthest), or -1.
 *
 * @address 0x0054c440
 */
int16_t find_lowest_priority(datum_index sound_handle);

/**
 * Among `candidate_channels[0..count)` (channel indices), returns the first whose current sound has played at
 * least the candidate's class's minimum_replace_time and whose distance is within 1.0 (squared) of the
 * candidate's own distance, or -1 if none qualify.
 *
 * @address 0x0054c5e0
 */
int16_t pick_replaceable(datum_index sound_handle, int16_t *candidate_channels, int16_t count);

/**
 * True if sound_a's class priority is more important (lower) than sound_b's, or if they tie and
 * distance_a_squared is less than a freshly recomputed distance for sound_a.
 *
 * @address 0x0054c6b0
 */
uint32_t compare_priority(datum_index sound_a, datum_index sound_b, float distance_a_squared);

/**
 * Queues `permutation` behind the current one on a logical channel, releasing the cache reference of any
 * permutation already queued, and forwards the new source to the audio device.
 *
 * @address 0x0054cd30
 */
void set_next_permutation(int16_t channel_index, SoundPermutation *permutation, int16_t unknown, int16_t sound_class, uint8_t streaming);

/**
 * Parameter policy used without EAX: applies the obstruction and occlusion attenuation to the gain of
 * absolute-position sounds, remembers the pitch on the first call and forwards the parameters to the audio
 * device.
 *
 * @address 0x0054ce50
 */
void apply_default_parameters(int16_t channel_index, sound_channel_parameters *parameters, uint8_t update, int16_t sound_class);

/**
 * Parameter policy used with EAX: boosts first-person gain for sound class 4 (capped at 0.6), remembers the
 * pitch on the first call and forwards the parameters to the audio device. Obstruction is left to the EAX
 * effect.
 *
 * @address 0x0054cf80
 */
void apply_eax_parameters(int16_t channel_index, sound_channel_parameters *parameters, uint8_t update, int16_t sound_class);

/**
 * Once the driver reports this channel's voice has fewer than 2 (then fewer than 1) sources queued, releases
 * the cached page reference for its queued/current permutation and promotes the queued one forward. Always
 * accumulates this channel's play_time by pitch * elapsed time. Returns the driver's channel state (clamped to
 * 0 if sound_cache_touch reports failure).
 *
 * @address 0x0054d020
 */
int16_t release_detail_buffers(int16_t channel_index);

/**
 * Releases the cache page references of a channel's next and current permutations (if any) and stops its driver
 * voice.
 *
 * @address 0x0054d0d0
 */
void release_permutations(int16_t channel_index);

}  // namespace halo::sound::channels
