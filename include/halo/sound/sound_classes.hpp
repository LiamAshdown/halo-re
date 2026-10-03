/**
 * @file include/halo/sound/sound_classes.hpp
 * Sound classes and the master, music and effects gain sliders.
 */
#pragma once

#include "halo/sound/sound_types.hpp"

namespace halo::sound::classes {

/**
 * Advances every sound class's gain fade by `ticks`: interpolates current_gain toward target_gain
 * proportionally while fade_ticks remains, or snaps directly to target_gain once the fade completes.
 *
 * @address 0x00545330
 */
void update_gain_fade(int32_t ticks);

/**
 * Sets target_gain (clamped [0, 1]) and fade_ticks (clamped >= 0) on every sound class whose name contains
 * `name` as a substring.
 *
 * @address 0x00545390
 */
void set_gain_by_name(char *name, float gain, int16_t ticks);

/**
 * Sets (muted = !enabled) on every sound class whose name contains `name` as a substring.
 *
 * @address 0x00545420
 */
void set_muted_by_name(uint8_t enabled, char *name);

/**
 * Sets the master gain slider. Crossing from audible to silent stops every sound and disables sound_enabled
 * first; crossing from silent to audible re-enables it and snaps to exactly 1.0 unless the requested gain
 * exceeds it (pop-avoidance hysteresis); otherwise the gain is set directly. Any actual change refreshes every
 * active playing instance's gain.
 *
 * @address 0x00548590
 */
void set_master_gain(float gain);

/**
 * Sets the music-class gain slider. Crossing from audible to silent mutes every "music"-named sound class and
 * zeroes the gain; crossing from silent to audible unmutes them and clamps the gain to at most 1.0; otherwise
 * the gain is set directly. Any actual change refreshes every active playing instance's gain.
 *
 * @address 0x00548680
 */
void set_music_gain(float gain);

/**
 * Sets the effects-class gain slider. Crossing from audible to silent mutes the 25 effects-related sound
 * classes and zeroes the gain; crossing from silent to audible unmutes them and caps the gain at 1.0; otherwise
 * the gain is set directly. Any actual change refreshes every active instance's gain.
 *
 * @address 0x005487b0
 */
void set_effects_gain(float gain);

/**
 * Combines a sound class's current fade gain with the appropriate gain sliders: the scripted dialog classes
 * scale by master gain only, music scales by music * ducking * master, unit dialog and scripted effects scale
 * by ducking * master (no category slider), and every other class scales by effects * ducking * master.
 *
 * @address 0x0054b100
 */
float compute_gain(SoundClass_t sound_class);

}  // namespace halo::sound::classes
