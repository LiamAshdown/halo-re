/**
 * @file include/halo/sound/spatial.hpp
 * Listener, environment and range handling for spatialized sounds.
 */
#pragma once

#include "halo/sound/sound_types.hpp"

namespace halo::sound::spatial {

/**
 * Finds the local player's sound environment from the fog region and the cluster's environment palette entry,
 * eases the scenario's live environment toward it in clamped steps (or copies it outright when the water flag
 * changes), and reports the background sound tag, the environment and a changed flag.
 *
 * @address 0x0053f150
 */
void environment_update(uint32_t *out_environment_ptr, void **out_environment_slot, uint8_t *out_changed);

/**
 * Rebuilds sound_cluster_audible_bitmap: bit `cluster` is set when the BSP's per-cluster distance table places
 * `cluster` within sound-audible range (scaled distance < 256.0) of the listener's current cluster
 * (observers[0].camera.cluster_index), gated on a local player existing.
 *
 * @address 0x00544980
 */
void build_cluster_range_bitmap(void);

/**
 * Recomputes the BSP leaf and cluster of every absolute-position sound after the structure BSP changed.
 *
 * @address 0x00549a00
 */
void refresh_structure_locations(void);

/**
 * Recomputes the local player's listener orientation, position and velocity from the observer camera once per
 * update (skipped while the game clock is stopped, or with no local player), triggers the enter/exit-water
 * ambient sound on an underwater transition, and pushes the fixed (always-origin) listener parameters to the
 * sound driver.
 *
 * @address 0x0054b970
 */
void update_listener(void);

/**
 * Squared distance from `location` to `sound_listeners[listener_index]`'s position (type
 * _sound_location_absolute), or the squared length of `location`'s own position (type
 * _sound_location_listener_relative, already listener-space), or 0 for _sound_location_none.
 *
 * @address 0x0054bbd0
 */
float location_distance_squared(int16_t listener_index, sound_location *location);

/**
 * Real distance from `location` to `sound_listeners[listener_index]`'s position (type
 * _sound_location_absolute), or the length of `location`'s own position (type
 * _sound_location_listener_relative), or 0 for _sound_location_none.
 *
 * @address 0x0054bc50
 */
float location_distance(int16_t listener_index, sound_location *location);

/**
 * Per-update pass over every live sound: stops any whose channel/location proc has failed, range-checks it
 * against a listener and fades it in or out of audible range, and (for scripted dialog sounds, when no player
 * currently has a unit) fades it out once its track has finished. Also ramps the shared dialog-ducking gain
 * toward its target (0.7 while a scripted dialog sound -- other than scripted_effect -- was seen this pass,
 * otherwise back toward 1.0).
 *
 * @address 0x0054bd60
 */
void update_range_and_ducking(void);

}  // namespace halo::sound::spatial
