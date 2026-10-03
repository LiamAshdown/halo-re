/**
 * @file include/halo/sound/looping_sounds.hpp
 * Looping sound table: tracks, detail sounds and the per-update state machine.
 */
#pragma once

#include "halo/sound/sound_types.hpp"

namespace halo::sound::looping {

/**
 * For every track of a SoundLooping definition whose "loop" sound has exactly one pitch range with loaded
 * permutations, touches the sound cache to keep it resident.
 *
 * @address 0x00544000
 */
void predict(datum_index looping_definition);

/**
 * hs sound_looping_start: (re)starts the scripted instance of a looping sound, optionally on an object, with
 * script-controlled gain.
 *
 * @address 0x00544090
 */
void start(datum_index definition_index, datum_index object_index, float scale);

/**
 * Detaches the game_looping_sound datum currently referenced by a SoundLooping tag's runtime_scripting_sound
 * back-reference: requests it stop and clears the scripted flag, then clears the tag's own reference.
 *
 * @address 0x00544120
 */
void stop(datum_index looping_definition);

/**
 * Sets the playback gain (clamped to [0, 1]) of the looping-sound datum currently referenced by a SoundLooping
 * tag's runtime_scripting_sound back-reference.
 *
 * @address 0x00544180
 */
void set_scale(datum_index looping_definition, float gain);

/**
 * Sets or clears the alternate-loop option flag on the looping-sound datum currently referenced by a
 * SoundLooping tag's runtime_scripting_sound back-reference.
 *
 * @address 0x00544200
 */
void set_alternate(datum_index looping_definition, uint8_t alternate);

/**
 * Creates a script-gain game_looping_sound for `definition_index` with gain `scale`.
 *
 * @address 0x00544250
 */
datum_index start_ambient(datum_index object_index, datum_index definition_index, float scale);

/**
 * True if any track of `looping_definition` has a "loop" sound belonging to the music sound class.
 *
 * @address 0x00544c10
 */
uint32_t definition_has_music_loop(datum_index looping_definition);

/**
 * While the sound system is active, finds the looping_sound datum whose stored reference matches `reference`
 * and stamps its update_toggle with the current frame's double-buffer flag, keeping it alive for this frame.
 *
 * @address 0x00549f50
 */
void touch(int32_t reference);

/**
 * Drives one looping sound from a game looping sound's request: creates the looping_sound for `owner` when
 * needed, updates its location, then starts, continues, crossfades or stops its track sounds for the requested
 * state and fade duration. Returns 1 when the looping sound is finished and the caller may delete its record.
 *
 * @address 0x00549fa0
 */
uint8_t set_state(int32_t owner, datum_index definition_index, sound_location *location, int16_t state, uint8_t alternate, float fade_duration);

/**
 * Allocates a new looping_sound datum for a (SoundLooping tag, owner) pair, if sound is currently enabled.
 * Seeds detail_next_time[] for every detail sound with a randomized future time: random_period_bounds (seconds)
 * scaled by the zero/one detail period lerp at location->scale.
 *
 * @address 0x0054d140
 */
datum_index state_new(datum_index definition_index, int32_t owner, sound_location *location);

/**
 * Per-update pass over every live looping_sound: if it was touched this frame (update_toggle matches the global
 * frame toggle), triggers any detail sounds whose next_time has arrived and reseeds their next_time; otherwise
 * (not touched this frame -- finished) sweeps its SoundLooping definition's tracks for cache pages nothing
 * needs any more and deletes the datum.
 *
 * @address 0x0054d270
 */
void update_states(void);

/**
 * Creates a new one-shot detail sound for `definition_index`, owned by looping_sound `owner`, starting at
 * `track_index`/`play_state`. Returns k_datum_index_none if the definition has no loaded, unmuted permutations,
 * or is not currently audible from the owner's location.
 *
 * @address 0x0054d9f0
 */
datum_index create_detail_sound(datum_index owner, datum_index definition_index, int16_t track_index, int16_t play_state);

/**
 * sound_location_proc of looping track sounds (start / loop / end parts): the sound simply sits wherever its
 * looping sound is.
 *
 * @address 0x0054dc10
 */
uint8_t track_location_proc(datum_index owner, void *callback_data, sound_location *location);

/**
 * Predicted-resource location callback for a detail sound spawned by a looping sound: places it at a
 * caller-supplied offset from the owning looping sound's own location (when that location is absolute),
 * inheriting its orientation, leaf/cluster and obstruction/occlusion. Returns 0 if the owning looping_sound
 * handle is no longer valid. Matches the sound_location_proc typedef exactly (void *callback_data), which is
 * really a pointer to 3 floats here;
 *
 * @address 0x0054dc70
 */
uint8_t detail_location_proc(datum_index owner, void *callback_data, sound_location *location);

/**
 * Computes the current gain/pitch/cone parameters for the sound playing on `channel_index` (scaled by
 * `external_gain_multiplier`), pushes them to the driver, and drives the looping track's playback state
 * machine: starting the channel the first time it is assigned, crossfading in a successor detail sound when the
 * pitch range needs to change mid-loop, and otherwise picking a new permutation (or applying a queued
 * definition switch) once the current one is exhausted.
 *
 * @address 0x0054deb0
 */
void update_gain(int16_t channel_index, float external_gain_multiplier);

/**
 * Linear search for the looping_sound datum whose owner field equals `owner`, or k_datum_index_none if there is
 * none.
 *
 * @address 0x0054e5d0
 */
datum_index find_by_owner(int32_t owner);

/**
 * Audibility-gate hook of the looping sound start path: when the audibility check is enabled and the location
 * is absolute it inspects the first loop track and the detail sounds of the definition for explicit distances,
 * and returns without changing any state.
 *
 * @address 0x0054e740
 */
void check_audibility_gate(datum_index definition_index, sound_location *location);

}  // namespace halo::sound::looping
