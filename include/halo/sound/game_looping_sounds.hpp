/**
 * @file include/halo/sound/game_looping_sounds.hpp
 * Game-state looping sounds bound to objects, scripts and the BSP background sound.
 */
#pragma once

#include "halo/sound/sound_types.hpp"

namespace halo::sound::game_looping {

/**
 * Allocates the object-looping-sounds datum array and registers the game_sound_globals block (update_count /
 * background_sound_index / last_update_time) at the current game state cursor.
 *
 * @address 0x00543a30
 */
void initialize(void);

/**
 * Walks every live object-looping-sound datum and, for each one, clears its definition tag's
 * SoundLooping.runtime_scripting_sound back-reference when it still points at this datum -- invalidating the
 * scripted-sound cache the way the HS "revert" path expects.
 *
 * @address 0x00543a90
 */
void revert_scripting_sounds(void);

/**
 * For every live object-looping-sound datum flagged as scripted, either re-establishes its definition tag's
 * SoundLooping.runtime_scripting_sound back-reference (when the tag still defines a loop) or deletes the
 * now-stale datum (when the tag's not_a_loop flag has been set). Afterwards, walks every sound tag and
 * invalidates its cached Sound.scripting_time.
 *
 * @address 0x00543b30
 */
void reconcile_scripting_state(void);

/**
 * Allocates a new object-looping-sound datum bound to `definition_index`. When `object_index` is valid, first
 * probes the object's node transform (failing the allocation if the object has none) and copies the resulting
 * node index, position, and forward direction into the new datum.
 *
 * @address 0x00543c20
 */
datum_index create(datum_index object_index, datum_index definition_index, char *marker_name, int16_t function_index);

/**
 * Re-stamps a game_looping_sound datum as still alive (via sound_looping_datum_touch) when either its bound
 * object function output is valid (or no function is bound / script gain is not being stopped), or the datum
 * has gone stale since the last full update pass while not already in the loop-stopping state.
 *
 * @address 0x00544290
 */
void touch_if_valid(datum_index looping_sound_index);

/**
 * Per-tick update of one game_looping_sound datum: reads its gain (object function value or script scale) and
 * liveness, places it on its object's node, and starts, continues or stops the underlying looping_sound through
 * sound_looping_set_state. Script sounds are deleted once their looping_sound reports it is gone.
 *
 * @address 0x00544330
 */
void update_sound(datum_index looping_sound_index, int32_t *root_location);

/**
 * Per-tick object-looping-sound update. Below the 33ms full-update threshold, only refreshes each live datum's
 * liveness stamp. At or above it, resolves the listener's current BSP cluster and its SoundEnvironment,
 * rebuilds the cluster-audibility bitmap, starts/stops the cluster's background ambient loop as needed, and
 * runs the full per-datum audibility/placement update, deleting datums whose owning object is gone.
 *
 * @address 0x005445c0
 */
void update(void);

/**
 * For every live, not-object-bound game_looping_sound datum whose definition has a music-class loop track,
 * detaches its definition's currently scripted instance and marks that instance stopped-by-music.
 *
 * @address 0x00544c70
 */
void stop_loops_conflicting_with_music(void);

}  // namespace halo::sound::game_looping
