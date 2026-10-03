/**
 * @file include/halo/sound/effects_objects.hpp
 * Lifetime of the global EAX sound effects object.
 */
#pragma once

#include "halo/sound/sound_types.hpp"

namespace halo::sound::effects {

/**
 * Probes EAX3, then EAX2, then EAX1 support (constructing and initializing a fresh sound effect object for each
 * attempt, discarding it on failure) and keeps the first one that initializes successfully. Returns 2/1/0 for
 * EAX3/EAX2/EAX1, or -1 if none are available (or EAX is disabled/unavailable), logging the chosen mode either
 * way.
 *
 * @address 0x00551270
 */
int32_t detect_mode(int16_t channel_index, directsound_channel *channel);

/**
 * Shuts down the global effects object through its backend, frees it and marks the effects state as empty.
 *
 * @address 0x00551420
 */
void shutdown(void);

/**
 * pushed as the argument of the effects object's initialize_channel (vtable +8, __thiscall).
 *
 * @address 0x00551460
 */
int32_t initialize_channel(int16_t channel_index);

/**
 * Runs the effects object's per-channel initialization on every 3D hardware channel. Always returns 1.
 *
 * @address 0x00551480
 */
int apply_all_channels(void);

/**
 * Enables or disables EAX at run time: tears the effects object down, or probes EAX 3, then 2, then 1 and
 * applies the winner to every 3D channel. Does nothing before DirectSound is up.
 *
 * @address 0x005514d0
 */
void reinitialize(int enable);

}  // namespace halo::sound::effects
