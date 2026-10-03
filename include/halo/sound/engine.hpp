/**
 * @file include/halo/sound/engine.hpp
 * The sound engine lifecycle: initialize, update, pause, resume and device reopening.
 */
#pragma once

#include "halo/sound/sound_types.hpp"

namespace halo::sound::engine {

/**
 * Stops every playing sound, marks the sound engine paused (notifying the driver once), then releases any
 * now-unused sound cache entries.
 *
 * @address 0x00548170
 */
void pause(void);

/**
 * Re-applies the EAX setting and, if the sound engine was paused, unpauses the driver and restarts the sound
 * clock. Does nothing while shell_window_proc_bypass is set.
 *
 * @address 0x005481a0
 */
void resume(void);

/**
 * Initializes the sound engine: resets state, allocates the sound cache, and (unless sound_disabled) resets the
 * four gain sliders and reverb environment to their defaults, opens the configured DirectSound driver,
 * allocates the "sounds"/"looping sounds" datum tables, and builds the logical sound_channels table from the
 * driver's per-type slot counts.
 *
 * @address 0x005492f0
 */
void initialize(void);

/**
 * Stops every currently playing sound, disposes the current DirectSound device, then reinitializes it with
 * `new_parameters`; on success, rebuilds sound_channels[] from the new per-type slot counts exactly as
 * src/sound/sound_initialize.c does, and re-detects EAX availability.
 *
 * @address 0x005494a0
 */
uint8_t reopen_device(sound_driver_parameters *new_parameters);

/**
 * Shuts down the sound device (if initialized) and frees the sound/looping-sound data_array headers it owns,
 * then unconditionally frees the sound cache's entry table and cache header.
 *
 * @address 0x00549760
 */
void dispose(void);

/**
 * Per-tick sound engine update: syncs sound_paused to the game-pause/focus state (telling the driver and, on
 * resume, reseeding sound_time), then -- when initialized/enabled and at least k_sound_update_interval_ms has
 * elapsed -- brackets a full update pass (gain fades, 3D listener, looping-sound maintenance, channel
 * assignment, active-instance gains, and the double-buffer toggle) between the driver's begin_frame/end_frame,
 * rolling back the clock advance if the engine ended up.
 *
 * @address 0x00549810
 */
void update(void);

/**
 * Reentrant-guarded lightweight tick: advances the sound clock and, once per k_sound_update_interval_ms while
 * not paused, refreshes every active instance's gain between the driver's begin_frame/end_frame (rolling the
 * clock back if paused by the time it matters, exactly as src/sound/sound_update.c does), then ages the sound
 * cache.
 *
 * @address 0x00549960
 */
void idle_update(void);

/**
 * Advances the sound engine's millisecond clock from the CPU performance counter and recomputes the per-tick
 * blend weight (3% of the elapsed milliseconds) used by the crossfade/ducking gain ramps.
 *
 * @address 0x0054ae60
 */
void update_clock(void);

}  // namespace halo::sound::engine
