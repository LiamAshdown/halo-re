/**
 * @file include/halo/sound/sound_instances.hpp
 * Playing sound instances: starting, stopping, fading and per-update gain.
 */
#pragma once

#include "halo/sound/sound_types.hpp"

namespace halo::sound::instances {

/**
 * Starts `definition_index` attached to a point on one of `object_index`'s nodes; the sound follows the node
 * because the marker data rides along as the sound's callback data.
 *
 * @address 0x00543ce0
 */
datum_index start_at_object_marker(datum_index object_index, Point3D *position, Vector3D *forward, datum_index definition_index, int16_t node_index, float scale, uint32_t first_person_hint);

/**
 * Starts a one-shot, ownerless sound at a fixed world placement.
 *
 * @address 0x00543d80
 */
datum_index start_at_location(datum_index definition_index, sound_placement *placement, float scale);

/**
 * Starts a one-shot, ownerless sound with no position, such as a music or interface sound.
 *
 * @address 0x00543dd0
 */
datum_index start_unspatialized(datum_index definition_index, float scale);

/**
 * hs sound_impulse_start: replaces the tag's scripted one-shot with a new instance, on the object's head marker
 * (or its origin) when an object is given, unspatialized otherwise.
 *
 * @address 0x00543e10
 */
void impulse_start(datum_index object_index, datum_index definition_index, float scale);

/**
 * Ticks remaining until `sound_tag_handle`'s scripting_time deadline (0 if it has none set, or if the deadline
 * has already passed).
 *
 * @address 0x00543fc0
 */
int32_t impulse_time(datum_index sound_tag_handle);

/**
 * sound_location_proc for sounds attached to an object marker: places `location` at the callback's node-space
 * point/direction on `owner`'s node, with the root object's cluster and velocity. Fails when the object is gone
 * or outside every cluster.
 *
 * @address 0x005448c0
 */
uint8_t object_marker_location_proc(datum_index owner, void *callback_data, sound_location *location);

/**
 * Fades every playing sound out over 0.3 s (letting the mixer run for 300 ms), makes sure the device is
 * unpaused, then stops all sounds and deletes all looping sounds.
 *
 * @address 0x005495f0
 */
void fade_out_and_stop_all(void);

/**
 * Creates a new playing-sound datum for `definition_index`, applying the scripted-dialog suppression window,
 * format/channel validation, a skip-fraction probability roll, pitch-range and permutation selection, and a
 * distance-based start-time delay. Recurses through the definition's promotion_sound when the per-object
 * retrigger throttle asks for a substitute.
 *
 * @address 0x00549af0
 */
datum_index play_new(datum_index definition_index, sound_location *location, datum_index owner_index, sound_location_proc location_proc, void *callback_data, int32_t callback_data_size, uint32_t first_person_hint);

/**
 * Stops a one-shot sound with a 0.3 s fade-out, if the handle is still live.
 *
 * @address 0x00549ee0
 */
void impulse_fade_out(datum_index sound_index);

/**
 * Stops every playing sound and empties the looping sound table: used when a gain slider crosses zero (mute),
 * on pause and on reset.
 *
 * @address 0x0054adb0
 */
void stop_all(void);

/**
 * Starts a `duration_seconds`-long gain fade on up to two sound instances: `fade_in_handle` ramps from wherever
 * its current fade left off (or from silence if it was not already fading) up to full gain, and
 * `fade_out_handle` ramps from wherever its current fade left off down to silence. Either handle may be
 * k_datum_index_none to skip that side.
 *
 * @address 0x0054af60
 */
void schedule_gain_fade(datum_index fade_in_handle, int16_t fade_curve, float duration_seconds, datum_index fade_out_handle);

/**
 * Stops a playing-sound datum: releases its own cached permutation reference (or its driver channel and the
 * reference held by whichever channel was playing it), clears it from its owning looping_sound's track_sounds
 * slot if it is a track sound, and deletes its datum entry. If this was the last sound of a finished music
 * loop, also sweeps every track of the owning SoundLooping definition for now-unused cached pages.
 *
 * @address 0x0054b180
 */
void stop(datum_index sound_handle);

/**
 * If `sound_handle`'s start time has arrived and it has not been flagged as delayed-start, runs its
 * location_proc once. On failure: non-impulse sounds and dialog-class sounds are left pending (returns 0, tried
 * again next update); other classes give up and clear location_proc (returns 1, as if the call had never been
 * due). Returns 1 whenever nothing was due to run.
 *
 * @address 0x0054bcd0
 */
uint32_t invoke_location_proc(datum_index sound_handle);

/**
 * Computes and applies the current gain (and, the first time this channel is assigned, the full parameter set
 * and driver voice start) for the one-shot sound playing on `channel_index`.
 *
 * @address 0x0054c750
 */
void update_gain(int16_t channel_index, float external_gain_multiplier);

/**
 * Per-update pass over every active playback channel: retires ones whose fade has reached silence,
 * computes/applies 3D spatialization (or listener-relative distance attenuation for non-3D channels), applies
 * gain through the one-shot or looping path depending on play_state, and drives mouth-data lip sync for
 * dialog-class channels using the object marker location proc.
 *
 * @address 0x0054c900
 */
void update_active(void);

/**
 * Queues a Sound tag switch for `sound_handle` if `new_definition_index` differs from its current definition (a
 * later pass picks this up via pending_definition_index).
 *
 * @address 0x0054dd90
 */
void queue_definition_switch(datum_index sound_handle, datum_index new_definition_index);

/**
 * Applies a queued definition switch (sound.pending_definition_index): re-resolves the sound's pitch range and
 * permutation against the new tag. If it already has a channel, re-checks the per-tag/per-owner channel budgets
 * under the new tag and stops either a conflicting channel or, failing that, this sound itself.
 *
 * @address 0x0054ddc0
 */
void apply_pending_definition_switch(datum_index sound_handle);

/**
 * Evaluates a sound's current fade progress: linear time fraction between fade_start_time and fade_end_time,
 * clamped to [0,1], optionally reshaped by the power fade curve (inverted when fading out, i.e. fade_end_gain
 * <= fade_start_gain), then lerped between fade_start_gain and fade_end_gain. Returns 1.0 (full gain,
 * unmodified) if there is no fade in progress. Clears the fade's start/end time once progress reaches 1.0.
 *
 * @address 0x0054e3c0
 */
float evaluate_fade_gain(datum_index sound_handle);

/**
 * When sound debug display is enabled, formats "<tag path>|n<obstruction> <occlusion>" for `sound_handle` into
 * a scratch buffer.
 *
 * @address 0x0054e6d0
 */
void render_debug(datum_index sound_handle);

/**
 * Sets bit 0x40000 in the object's flags word and returns 1.
 *
 * @address 0x0054e800
 */
uint8_t scenery_create(datum_index object_index);

}  // namespace halo::sound::instances
