/**
 * @file include/halo/sound/directsound.hpp
 * Method table slots (byte offset / 4) of the DirectSound COM interfaces the sound code calls through raw method tables.
 */
#pragma once

#include <cstdint>

namespace halo::sound::dsound_slot {

/** IUnknown. */
inline constexpr uint32_t query_interface = 0;
inline constexpr uint32_t release = 2;

/** IDirectSound. */
inline constexpr uint32_t ds_create_sound_buffer = 0x0c / 4;
inline constexpr uint32_t ds_get_caps = 0x10 / 4;
inline constexpr uint32_t ds_set_cooperative_level = 0x18 / 4;

/** IDirectSoundBuffer. */
inline constexpr uint32_t sb_get_current_position = 0x10 / 4;
inline constexpr uint32_t sb_get_status = 0x24 / 4;
inline constexpr uint32_t sb_lock = 0x2c / 4;
inline constexpr uint32_t sb_play = 0x30 / 4;
inline constexpr uint32_t sb_set_current_position = 0x34 / 4;
inline constexpr uint32_t sb_set_format = 0x38 / 4;
inline constexpr uint32_t sb_set_volume = 0x3c / 4;
inline constexpr uint32_t sb_set_frequency = 0x44 / 4;
inline constexpr uint32_t sb_stop = 0x48 / 4;
inline constexpr uint32_t sb_unlock = 0x4c / 4;
inline constexpr uint32_t sb_restore = 0x50 / 4;

/** IDirectSound3DBuffer. */
inline constexpr uint32_t b3d_set_all_parameters = 0x30 / 4;
inline constexpr uint32_t b3d_set_cone_angles = 0x34 / 4;
inline constexpr uint32_t b3d_set_cone_orientation = 0x38 / 4;
inline constexpr uint32_t b3d_set_cone_outside_volume = 0x3c / 4;
inline constexpr uint32_t b3d_set_max_distance = 0x40 / 4;
inline constexpr uint32_t b3d_set_min_distance = 0x44 / 4;
inline constexpr uint32_t b3d_set_mode = 0x48 / 4;
inline constexpr uint32_t b3d_set_position = 0x4c / 4;
inline constexpr uint32_t b3d_set_velocity = 0x50 / 4;

/** IDirectSound3DListener. */
inline constexpr uint32_t lst_set_distance_factor = 0x2c / 4;
inline constexpr uint32_t lst_set_doppler_factor = 0x30 / 4;
inline constexpr uint32_t lst_set_orientation = 0x34 / 4;
inline constexpr uint32_t lst_set_position = 0x38 / 4;
inline constexpr uint32_t lst_set_rolloff_factor = 0x3c / 4;
inline constexpr uint32_t lst_set_velocity = 0x40 / 4;
inline constexpr uint32_t lst_commit_deferred_settings = 0x44 / 4;

/** IKsPropertySet. */
inline constexpr uint32_t ks_get = 0x0c / 4;
inline constexpr uint32_t ks_set = 0x10 / 4;
inline constexpr uint32_t ks_query_support = 0x14 / 4;

}  // namespace halo::sound::dsound_slot
