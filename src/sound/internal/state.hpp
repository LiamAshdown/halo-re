/**
 * @file src/sound/internal/state.hpp
 * Includes shared by the sound implementation files and the engine globals they reference. The globals are
 * defined in C by standalone/data/*.c at fixed addresses, so they are declared here with C linkage.
 */
#pragma once

#include "halo/sound/sound.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/cseries/api.hpp"
#include "halo/core/link.hpp"
#include "halo/ai/vars.hpp"
#include "halo/game/vars.hpp"
#include "halo/hs/vars.hpp"
#include "halo/interface/vars.hpp"
#include "halo/main/vars.hpp"
#include "halo/networking/vars.hpp"
#include "halo/objects/vars.hpp"
#include "halo/physics/vars.hpp"
#include "halo/saved_games/vars.hpp"
#include "halo/shell/vars.hpp"
#include "halo/sound/vars.hpp"
#include "halo/units/vars.hpp"
#include "halo/core/libm.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/core/shared_state_links.hpp"


static auto &game_looping_sound_data = halo::link::ref<data_array *>(halo::ui::vars().game_looping_sound_data);
static auto &game_sound_globals_ptr = halo::link::ref<game_sound_globals *>(halo::sound::vars().game_sound_globals_ptr);
static auto &object_data = halo::link::ref<data_array *>(halo::objects::vars().object_data);
static auto &sound_environment = halo::link::ref<SoundEnvironment>(halo::sound::vars().sound_environment);
static auto &sound_cluster_audible_bitmap = halo::link::ref<uint32_t [k_sound_cluster_bitmap_words]>(halo::sound::vars().sound_cluster_audible_bitmap);
static auto &sound_looping_audibility_check = halo::link::ref<uint8_t>(halo::sound::vars().sound_looping_audibility_check);
static auto &sound_class_definitions = halo::link::ref<sound_class_definition [k_maximum_sound_classes]>(halo::sound::vars().sound_class_definitions);
static auto &looping_sound_data = halo::link::ref<data_array *>(halo::sound::vars().looping_sound_data);
static auto &sound_data = halo::link::ref<data_array *>(halo::sound::vars().sound_data);
static auto &sound_time = halo::link::ref<int32_t>(halo::sound::vars().sound_time);

/** The sound instance record of a handle in the sounds data array. */
inline sound *sound_instance(datum_index handle)
{
    return (sound *)((uint8_t *)sound_data->data + (handle & halo::k_slot_mask) * sizeof(sound));
}

/** The looping sound record of a handle in the looping sounds data array. */
inline looping_sound *looping_sound_state(datum_index handle)
{
    return (looping_sound *)((uint8_t *)looping_sound_data->data + (handle & halo::k_slot_mask) * sizeof(looping_sound));
}

/** The sound cache entry a permutation's samples pointer refers to. */
inline sound_cache_entry *sound_cache_entry_at(uint32_t samples_pointer)
{
    return (sound_cache_entry *)((uint8_t *)halo::cache::globals().sound_cache_entries->data +
                                 (samples_pointer & halo::k_slot_mask) * sizeof(sound_cache_entry));
}
static auto &sound_initialized = halo::link::ref<uint8_t>(halo::sound::vars().sound_initialized);
static auto &sound_enabled = halo::link::ref<uint8_t>(halo::sound::vars().sound_enabled);
static auto &sound_disabled = halo::link::ref<uint8_t>(halo::sound::vars().sound_disabled);
static auto &sound_update_toggle = halo::link::ref<uint8_t>(halo::sound::vars().sound_update_toggle);
static auto &global_origin3d_pointer = halo::link::ref<const real_point3d *>(halo::ai::vars().global_origin3d_pointer);
static auto &k_empty_string = halo::link::ref<char []>(halo::networking::vars().k_empty_string);
static auto &sound_channels = halo::link::ref<sound_channel [k_maximum_sound_channels]>(halo::sound::vars().sound_channels);
static auto &sound_music_gain = halo::link::ref<float>(halo::sound::vars().sound_music_gain);
static auto &sound_idle_update_active = halo::link::ref<uint8_t>(halo::sound::vars().sound_idle_update_active);
static auto &sound_channel_parameters_proc_ptr = halo::link::ref<sound_channel_parameters_proc>(halo::sound::vars().sound_channel_parameters_proc_ptr);
static auto &current_sound_driver = halo::link::ref<sound_driver *>(halo::sound::vars().current_sound_driver);
static auto &game_time = halo::link::ref<game_time_globals *>(halo::ai::vars().game_time);
static auto &ai_communication_quiet_until_tick = halo::link::ref<int32_t>(halo::ai::vars().ai_communication_quiet_until_tick);
static auto &sound_dialog_unspatialized = halo::link::ref<uint8_t>(halo::sound::vars().sound_dialog_unspatialized);
static auto &cinematic_globals_ptr = halo::link::ref<uint8_t *>(halo::game::vars().cinematic_globals_ptr);
static auto &sound_delay_per_world_unit = halo::link::ref<const float>(halo::sound::vars().sound_delay_per_world_unit);
static auto &ai_marker_name_a = halo::link::ref<char []>(halo::units::vars().ai_marker_name_a);
static auto &sound_fade_duration_scale = halo::link::ref<float>(halo::sound::vars().sound_fade_duration_scale);
static auto &sound_fade_curve_exponent = halo::link::ref<float>(halo::sound::vars().sound_fade_curve_exponent);
static auto &sound_channel_count = halo::link::ref<int16_t>(halo::sound::vars().sound_channel_count);
static auto &sound_listeners = halo::link::ref<sound_listener [1]>(halo::sound::vars().sound_listeners);
static auto &global_zero_vector3d_pointer = halo::link::ref<const real_point3d *>(halo::units::vars().global_zero_vector3d_pointer);
static auto &sound_paused = halo::link::ref<uint8_t>(halo::shell::vars().sound_paused);
static auto &sound_stopping_all = halo::link::ref<uint8_t>(halo::sound::vars().sound_stopping_all);
static auto &debug_sound = halo::link::ref<uint8_t>(halo::sound::vars().debug_sound);
static auto &sound_permutation_limit = halo::link::ref<int16_t>(halo::ui::vars().sound_permutation_limit);
static auto &local_player_0_cluster_index = halo::link::ref<int16_t>(halo::sound::vars().local_player_0_cluster_index);
static auto &camera_point = halo::link::ref<real_point3d>(halo::game::vars().camera_point);
static auto &camera_leaf = halo::link::ref<bsp_leaf_reference>(halo::sound::vars().camera_leaf);
static auto &global_scenario_game_globals = halo::link::ref<scenario_game_globals *>(halo::shell::vars().global_scenario_game_globals);
static auto &k_default_sound_environment = halo::link::ref<SoundEnvironment>(halo::game::vars().k_default_sound_environment);
static auto &global_collision_bsp = halo::link::ref<ModelCollisionGeometryBSP *>(halo::physics::vars().global_collision_bsp);
static auto &sound_dialog_ducking_gain = halo::link::ref<float>(halo::sound::vars().sound_dialog_ducking_gain);
static auto &sound_ducking_gain = halo::link::ref<float>(halo::sound::vars().sound_ducking_gain);
static auto &sound_time_delta = halo::link::ref<float>(halo::sound::vars().sound_time_delta);
static auto &directsound_first_channel_of_type = halo::link::ref<int16_t [4]>(halo::sound::vars().directsound_first_channel_of_type);
static auto &directsound_channels = halo::link::ref<directsound_channel [k_maximum_sound_channels]>(halo::sound::vars().directsound_channels);
static auto &sound_master_gain = halo::link::ref<float>(halo::ui::vars().sound_master_gain);
static auto &sound_effects_gain = halo::link::ref<float>(halo::sound::vars().sound_effects_gain);
static auto &sound_drivers = halo::link::ref<sound_driver *[2]>(halo::sound::vars().sound_drivers);
static auto &driver_parameters = halo::link::ref<sound_driver_parameters>(halo::sound::vars().driver_parameters);
static auto &sound_channel_type_flag_table = halo::link::ref<uint16_t [4]>(halo::sound::vars().sound_channel_type_flag_table);
static auto &console_globals_data = halo::link::ref<console_globals>(halo::main::vars().console_globals_data);
static auto &current_game_engine = halo::link::ref<game_engine_definition *>(halo::game::vars().current_game_engine);
static auto &shell_window_proc_bypass = halo::link::ref<uint8_t>(halo::shell::vars().shell_window_proc_bypass);
static auto &directsound_eax_enabled = halo::link::ref<uint8_t>(halo::hs::vars().directsound_eax_enabled);
static auto &directsound_eax_available = halo::link::ref<uint8_t>(halo::ui::vars().directsound_eax_available);
static auto &sound_class_names = halo::link::ref<char *[k_maximum_sound_classes]>(halo::sound::vars().sound_class_names);
static auto &sound_class_gains = halo::link::ref<sound_class_gain *>(halo::game::vars().sound_class_gains);
static auto &adpcm_index_table = halo::link::ref<int16_t [16]>(halo::sound::vars().adpcm_index_table);
static auto &adpcm_step_table = halo::link::ref<int16_t [89]>(halo::sound::vars().adpcm_step_table);
static auto &k_sound_decode_procs = halo::link::ref<sound_decode_block_proc [3]>(halo::sound::vars().k_sound_decode_procs);
static auto &sound_decode_proc = halo::link::ref<sound_decode_block_proc>(halo::sound::vars().sound_decode_proc);
static auto &sound_cache_size_megabytes = halo::link::ref<int32_t>(halo::shell::vars().sound_cache_size_megabytes);
static auto &error_text_buffer = halo::link::ref<char []>(halo::sound::vars().error_text_buffer);
static auto &sound_ogg_underrun_count = halo::link::ref<int32_t>(halo::sound::vars().sound_ogg_underrun_count);
static auto &shell_nosound = halo::link::ref<int32_t>(halo::shell::vars().shell_nosound);
static auto &direct_sound_create8 = halo::link::ref<void *>(halo::shell::vars().direct_sound_create8);
static auto &directsound_initialized = halo::link::ref<uint8_t>(halo::ui::vars().directsound_initialized);
static auto &directsound_binding_count = halo::link::ref<int16_t>(halo::sound::vars().directsound_binding_count);
static auto &directsound_bindings = halo::link::ref<sound_channel_binding [k_maximum_sound_channels]>(halo::sound::vars().directsound_bindings);
static auto &directsound_channel_count = halo::link::ref<int16_t>(halo::sound::vars().directsound_channel_count);
static auto &directsound_caps = halo::link::ref<uint8_t [0x60]>(halo::sound::vars().directsound_caps);
static auto &directsound = halo::link::ref<void *>(halo::sound::vars().directsound);
static auto &directsound_primary_buffer = halo::link::ref<void *>(halo::sound::vars().directsound_primary_buffer);
static auto &directsound_listener = halo::link::ref<void *>(halo::hs::vars().directsound_listener);
static auto &directsound_paused = halo::link::ref<uint8_t>(halo::sound::vars().directsound_paused);
static auto &directsound_fade = halo::link::ref<float>(halo::sound::vars().directsound_fade);
static auto &directsound_hardware_3d_channel_count = halo::link::ref<int16_t>(halo::sound::vars().directsound_hardware_3d_channel_count);
static auto &directsound_quality = halo::link::ref<int32_t>(halo::main::vars().directsound_quality);
static auto &directsound_hardware_mode = halo::link::ref<int32_t>(halo::sound::vars().directsound_hardware_mode);
static auto &sound_effect_object_state = halo::link::ref<int16_t>(halo::sound::vars().sound_effect_object_state);
static auto &directsound_rolloff_factor = halo::link::ref<const float>(halo::sound::vars().directsound_rolloff_factor);
static auto &iid_directsound_3d_listener = halo::link::ref<uint8_t [16]>(halo::sound::vars().iid_directsound_3d_listener);
static auto &iid_directsound_3d_buffer = halo::link::ref<uint8_t [16]>(halo::sound::vars().iid_directsound_3d_buffer);
static auto &ds3dalg_hrtf_full = halo::link::ref<uint32_t [4]>(halo::sound::vars().ds3dalg_hrtf_full);
static auto &global_sound_effect_object = halo::link::ref<sound_effect_object *>(halo::game::vars().global_sound_effect_object);
static auto &directsound_listener_cached = halo::link::ref<directsound_listener_cache>(halo::sound::vars().directsound_listener_cached);
static auto &directsound_environment_cache = halo::link::ref<SoundEnvironment>(halo::sound::vars().directsound_environment_cache);
static auto &directsound_deferred_dirty = halo::link::ref<uint8_t>(halo::sound::vars().directsound_deferred_dirty);
static auto &debug_sound_channels = halo::link::ref<uint8_t>(halo::sound::vars().debug_sound_channels);
static auto &debug_sound_channel_details = halo::link::ref<uint8_t>(halo::sound::vars().debug_sound_channel_details);
static auto &hud_text_draw_background_mode = halo::link::ref<int16_t>(halo::networking::vars().hud_text_draw_background_mode);
static auto &text_tab_stops = halo::link::ref<int16_t>(halo::game::vars().text_tab_stops);
static auto &k_sound_sample_rates = halo::link::ref<int32_t [2]>(halo::sound::vars().k_sound_sample_rates);
static auto &config_enable_stop_start = halo::link::ref<uint32_t>(halo::shell::vars().config_enable_stop_start);
static auto &config_head_relative_speech = halo::link::ref<uint32_t>(halo::shell::vars().config_head_relative_speech);
static auto &sound_eax3_vtable = halo::link::ref<sound_effect_object_vtable>(halo::sound::vars().sound_eax3_vtable);
static auto &sound_eax2_vtable = halo::link::ref<sound_effect_object_vtable>(halo::sound::vars().sound_eax2_vtable);
static auto &sound_eax1_vtable = halo::link::ref<sound_effect_object_vtable>(halo::sound::vars().sound_eax1_vtable);
static auto &sound_eax_listener_property_guid = halo::link::ref<const uint8_t [16]>(halo::sound::vars().sound_eax_listener_property_guid);
static auto &sound_eax_property_set_guid = halo::link::ref<const uint8_t [16]>(halo::sound::vars().sound_eax_property_set_guid);
static auto &sound_eax20_buffer_property_guid = halo::link::ref<const uint8_t [16]>(halo::sound::vars().sound_eax20_buffer_property_guid);
static auto &sound_eax20_underwater_direct_gain = halo::link::ref<float>(halo::sound::vars().sound_eax20_underwater_direct_gain);
static auto &sound_eax20_listener_property_guid = halo::link::ref<const uint8_t [16]>(halo::sound::vars().sound_eax20_listener_property_guid);
static auto &sound_eax30_buffer_property_guid = halo::link::ref<const uint8_t [16]>(halo::sound::vars().sound_eax30_buffer_property_guid);
static auto &sound_underwater_direct_gain = halo::link::ref<float>(halo::sound::vars().sound_underwater_direct_gain);
static auto &sound_eax30_listener_property_guid = halo::link::ref<const uint8_t [16]>(halo::sound::vars().sound_eax30_listener_property_guid);
