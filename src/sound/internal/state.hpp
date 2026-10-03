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

extern "C" {

extern object *object_try_and_get(datum_index object_index, uint32_t type_mask);
extern void object_get_root_location(int32_t *out, uint32_t object_index);
extern int32_t object_get_node_local_transform(datum_index object_index, char *marker_name, object_marker *marker, uint32_t flags);
extern void player_effect_apply_at_object(uint32_t tag_reference, int16_t local_player_index, real_point3d *origin);
extern int16_t scenario_location_fog_region(bsp_leaf_reference *leaf, real_point3d *point);
extern uint8_t collision_test_movement_segment(uint32_t flags, real_point3d *origin, real_vector3d *delta, uint32_t exclude_object_index, collision_result *result);
}
inline auto &game_looping_sound_data = halo::link::ref<data_array *>(halo::ui::vars().game_looping_sound_data);
inline auto &game_sound_globals_ptr = halo::link::ref<game_sound_globals *>(halo::sound::vars().game_sound_globals_ptr);
inline auto &object_data = halo::link::ref<data_array *>(halo::objects::vars().object_data);
#ifndef HALO_LINKED_game_state_base
#define HALO_LINKED_game_state_base
inline auto &game_state_base = halo::link::ref<uint8_t *>(halo::saved_games::vars().game_state_base);
#endif
#ifndef HALO_LINKED_game_state_cursor
#define HALO_LINKED_game_state_cursor
inline auto &game_state_cursor = halo::link::ref<int32_t>(halo::saved_games::vars().game_state_cursor);
#endif
#ifndef HALO_LINKED_game_state_crc
#define HALO_LINKED_game_state_crc
inline auto &game_state_crc = halo::link::ref<uint32_t>(halo::saved_games::vars().game_state_crc);
#endif
inline auto &sound_environment = halo::link::ref<SoundEnvironment>(halo::sound::vars().sound_environment);
inline auto &sound_cluster_audible_bitmap = halo::link::ref<uint32_t [k_sound_cluster_bitmap_words]>(halo::sound::vars().sound_cluster_audible_bitmap);
inline auto &sound_looping_audibility_check = halo::link::ref<uint8_t>(halo::sound::vars().sound_looping_audibility_check);
inline auto &sound_class_definitions = halo::link::ref<sound_class_definition [k_maximum_sound_classes]>(halo::sound::vars().sound_class_definitions);
inline auto &looping_sound_data = halo::link::ref<data_array *>(halo::sound::vars().looping_sound_data);
inline auto &sound_data = halo::link::ref<data_array *>(halo::sound::vars().sound_data);
inline auto &sound_time = halo::link::ref<int32_t>(halo::sound::vars().sound_time);
inline auto &sound_initialized = halo::link::ref<uint8_t>(halo::sound::vars().sound_initialized);
inline auto &sound_enabled = halo::link::ref<uint8_t>(halo::sound::vars().sound_enabled);
inline auto &sound_disabled = halo::link::ref<uint8_t>(halo::sound::vars().sound_disabled);
inline auto &sound_update_toggle = halo::link::ref<uint8_t>(halo::sound::vars().sound_update_toggle);
inline auto &global_origin3d_pointer = halo::link::ref<const real_point3d *>(halo::ai::vars().global_origin3d_pointer);
inline auto &k_empty_string = halo::link::ref<char []>(halo::networking::vars().k_empty_string);
inline auto &sound_channels = halo::link::ref<sound_channel [k_maximum_sound_channels]>(halo::sound::vars().sound_channels);
inline auto &sound_music_gain = halo::link::ref<float>(halo::sound::vars().sound_music_gain);
inline auto &sound_idle_update_active = halo::link::ref<uint8_t>(halo::sound::vars().sound_idle_update_active);
inline auto &sound_channel_parameters_proc_ptr = halo::link::ref<sound_channel_parameters_proc>(halo::sound::vars().sound_channel_parameters_proc_ptr);
inline auto &current_sound_driver = halo::link::ref<sound_driver *>(halo::sound::vars().current_sound_driver);
inline auto &game_time = halo::link::ref<game_time_globals *>(halo::ai::vars().game_time);
inline auto &ai_communication_quiet_until_tick = halo::link::ref<int32_t>(halo::ai::vars().ai_communication_quiet_until_tick);
inline auto &sound_dialog_unspatialized = halo::link::ref<uint8_t>(halo::sound::vars().sound_dialog_unspatialized);
inline auto &cinematic_globals_ptr = halo::link::ref<uint8_t *>(halo::game::vars().cinematic_globals_ptr);
inline auto &sound_delay_per_world_unit = halo::link::ref<const float>(halo::sound::vars().sound_delay_per_world_unit);
inline auto &ai_marker_name_a = halo::link::ref<char []>(halo::units::vars().ai_marker_name_a);
inline auto &sound_fade_duration_scale = halo::link::ref<float>(halo::sound::vars().sound_fade_duration_scale);
inline auto &sound_fade_curve_exponent = halo::link::ref<float>(halo::sound::vars().sound_fade_curve_exponent);
inline auto &sound_channel_count = halo::link::ref<int16_t>(halo::sound::vars().sound_channel_count);
inline auto &sound_listeners = halo::link::ref<sound_listener [1]>(halo::sound::vars().sound_listeners);
inline auto &global_zero_vector3d_pointer = halo::link::ref<const real_point3d *>(halo::units::vars().global_zero_vector3d_pointer);
inline auto &sound_paused = halo::link::ref<uint8_t>(halo::shell::vars().sound_paused);
inline auto &sound_stopping_all = halo::link::ref<uint8_t>(halo::sound::vars().sound_stopping_all);
inline auto &debug_sound = halo::link::ref<uint8_t>(halo::sound::vars().debug_sound);
inline auto &sound_permutation_limit = halo::link::ref<int16_t>(halo::ui::vars().sound_permutation_limit);
#ifndef HALO_LINKED_local_player_globals
#define HALO_LINKED_local_player_globals
inline auto &local_player_globals = halo::link::ref<player_globals *>(halo::game::vars().local_player_globals);
#endif
inline auto &local_player_0_cluster_index = halo::link::ref<int16_t>(halo::sound::vars().local_player_0_cluster_index);
inline auto &camera_point = halo::link::ref<real_point3d>(halo::game::vars().camera_point);
inline auto &camera_leaf = halo::link::ref<bsp_leaf_reference>(halo::sound::vars().camera_leaf);
#ifndef HALO_LINKED_global_structure_bsp
#define HALO_LINKED_global_structure_bsp
inline auto &global_structure_bsp = halo::link::ref<ScenarioStructureBSP *>(halo::ai::vars().global_structure_bsp);
#endif
inline auto &global_scenario_game_globals = halo::link::ref<scenario_game_globals *>(halo::shell::vars().global_scenario_game_globals);
inline auto &k_default_sound_environment = halo::link::ref<SoundEnvironment>(halo::game::vars().k_default_sound_environment);
inline auto &global_collision_bsp = halo::link::ref<ModelCollisionGeometryBSP *>(halo::physics::vars().global_collision_bsp);
#ifndef HALO_LINKED_global_globals
#define HALO_LINKED_global_globals
inline auto &global_globals = halo::link::ref<Globals *>(halo::game::vars().global_globals);
#endif
inline auto &sound_dialog_ducking_gain = halo::link::ref<float>(halo::sound::vars().sound_dialog_ducking_gain);
inline auto &sound_ducking_gain = halo::link::ref<float>(halo::sound::vars().sound_ducking_gain);
inline auto &sound_time_delta = halo::link::ref<float>(halo::sound::vars().sound_time_delta);
inline auto &directsound_first_channel_of_type = halo::link::ref<int16_t [4]>(halo::sound::vars().directsound_first_channel_of_type);
inline auto &directsound_channels = halo::link::ref<directsound_channel [k_maximum_sound_channels]>(halo::sound::vars().directsound_channels);
inline auto &sound_master_gain = halo::link::ref<float>(halo::ui::vars().sound_master_gain);
inline auto &sound_effects_gain = halo::link::ref<float>(halo::sound::vars().sound_effects_gain);
inline auto &sound_drivers = halo::link::ref<sound_driver *[2]>(halo::sound::vars().sound_drivers);
inline auto &driver_parameters = halo::link::ref<sound_driver_parameters>(halo::sound::vars().driver_parameters);
inline auto &sound_channel_type_flag_table = halo::link::ref<uint16_t [4]>(halo::sound::vars().sound_channel_type_flag_table);
inline auto &console_globals_data = halo::link::ref<console_globals>(halo::main::vars().console_globals_data);
inline auto &current_game_engine = halo::link::ref<game_engine_definition *>(halo::game::vars().current_game_engine);
inline auto &shell_window_proc_bypass = halo::link::ref<uint8_t>(halo::shell::vars().shell_window_proc_bypass);
inline auto &directsound_eax_enabled = halo::link::ref<uint8_t>(halo::hs::vars().directsound_eax_enabled);
inline auto &directsound_eax_available = halo::link::ref<uint8_t>(halo::ui::vars().directsound_eax_available);
inline auto &sound_class_names = halo::link::ref<char *[k_maximum_sound_classes]>(halo::sound::vars().sound_class_names);
inline auto &sound_class_gains = halo::link::ref<sound_class_gain *>(halo::game::vars().sound_class_gains);
inline auto &adpcm_index_table = halo::link::ref<int16_t [16]>(halo::sound::vars().adpcm_index_table);
inline auto &adpcm_step_table = halo::link::ref<int16_t [89]>(halo::sound::vars().adpcm_step_table);
inline auto &k_sound_decode_procs = halo::link::ref<sound_decode_block_proc [3]>(halo::sound::vars().k_sound_decode_procs);
inline auto &sound_decode_proc = halo::link::ref<sound_decode_block_proc>(halo::sound::vars().sound_decode_proc);
inline auto &sound_cache_size_megabytes = halo::link::ref<int32_t>(halo::shell::vars().sound_cache_size_megabytes);
inline auto &error_text_buffer = halo::link::ref<char []>(halo::sound::vars().error_text_buffer);
inline auto &sound_ogg_underrun_count = halo::link::ref<int32_t>(halo::sound::vars().sound_ogg_underrun_count);
inline auto &shell_nosound = halo::link::ref<int32_t>(halo::shell::vars().shell_nosound);
#ifndef HALO_LINKED_shell_window
#define HALO_LINKED_shell_window
inline auto &shell_window = halo::link::ref<void *>(halo::shell::vars().shell_window);
#endif
inline auto &direct_sound_create8 = halo::link::ref<void *>(halo::shell::vars().direct_sound_create8);
inline auto &directsound_initialized = halo::link::ref<uint8_t>(halo::ui::vars().directsound_initialized);
inline auto &directsound_binding_count = halo::link::ref<int16_t>(halo::sound::vars().directsound_binding_count);
inline auto &directsound_bindings = halo::link::ref<sound_channel_binding [k_maximum_sound_channels]>(halo::sound::vars().directsound_bindings);
inline auto &directsound_channel_count = halo::link::ref<int16_t>(halo::sound::vars().directsound_channel_count);
inline auto &directsound_caps = halo::link::ref<uint8_t [0x60]>(halo::sound::vars().directsound_caps);
inline auto &directsound = halo::link::ref<void *>(halo::sound::vars().directsound);
inline auto &directsound_primary_buffer = halo::link::ref<void *>(halo::sound::vars().directsound_primary_buffer);
inline auto &directsound_listener = halo::link::ref<void *>(halo::hs::vars().directsound_listener);
inline auto &directsound_paused = halo::link::ref<uint8_t>(halo::sound::vars().directsound_paused);
inline auto &directsound_fade = halo::link::ref<float>(halo::sound::vars().directsound_fade);
inline auto &directsound_hardware_3d_channel_count = halo::link::ref<int16_t>(halo::sound::vars().directsound_hardware_3d_channel_count);
inline auto &directsound_quality = halo::link::ref<int32_t>(halo::main::vars().directsound_quality);
inline auto &directsound_hardware_mode = halo::link::ref<int32_t>(halo::sound::vars().directsound_hardware_mode);
inline auto &sound_effect_object_state = halo::link::ref<int16_t>(halo::sound::vars().sound_effect_object_state);
inline auto &directsound_rolloff_factor = halo::link::ref<const float>(halo::sound::vars().directsound_rolloff_factor);
inline auto &iid_directsound_3d_listener = halo::link::ref<uint8_t [16]>(halo::sound::vars().iid_directsound_3d_listener);
inline auto &iid_directsound_3d_buffer = halo::link::ref<uint8_t [16]>(halo::sound::vars().iid_directsound_3d_buffer);
inline auto &ds3dalg_hrtf_full = halo::link::ref<uint32_t [4]>(halo::sound::vars().ds3dalg_hrtf_full);
inline auto &global_sound_effect_object = halo::link::ref<sound_effect_object *>(halo::game::vars().global_sound_effect_object);
inline auto &directsound_listener_cached = halo::link::ref<directsound_listener_cache>(halo::sound::vars().directsound_listener_cached);
inline auto &directsound_environment_cache = halo::link::ref<SoundEnvironment>(halo::sound::vars().directsound_environment_cache);
inline auto &directsound_deferred_dirty = halo::link::ref<uint8_t>(halo::sound::vars().directsound_deferred_dirty);
inline auto &debug_sound_channels = halo::link::ref<uint8_t>(halo::sound::vars().debug_sound_channels);
inline auto &debug_sound_channel_details = halo::link::ref<uint8_t>(halo::sound::vars().debug_sound_channel_details);
inline auto &hud_text_draw_background_mode = halo::link::ref<int16_t>(halo::networking::vars().hud_text_draw_background_mode);
inline auto &text_tab_stops = halo::link::ref<int16_t>(halo::game::vars().text_tab_stops);
inline auto &k_sound_sample_rates = halo::link::ref<int32_t [2]>(halo::sound::vars().k_sound_sample_rates);
inline auto &config_enable_stop_start = halo::link::ref<uint32_t>(halo::shell::vars().config_enable_stop_start);
inline auto &config_head_relative_speech = halo::link::ref<uint32_t>(halo::shell::vars().config_head_relative_speech);
inline auto &sound_eax3_vtable = halo::link::ref<sound_effect_object_vtable>(halo::sound::vars().sound_eax3_vtable);
inline auto &sound_eax2_vtable = halo::link::ref<sound_effect_object_vtable>(halo::sound::vars().sound_eax2_vtable);
inline auto &sound_eax1_vtable = halo::link::ref<sound_effect_object_vtable>(halo::sound::vars().sound_eax1_vtable);
inline auto &sound_eax_listener_property_guid = halo::link::ref<const uint8_t [16]>(halo::sound::vars().sound_eax_listener_property_guid);
inline auto &sound_eax_property_set_guid = halo::link::ref<const uint8_t [16]>(halo::sound::vars().sound_eax_property_set_guid);
inline auto &sound_eax20_buffer_property_guid = halo::link::ref<const uint8_t [16]>(halo::sound::vars().sound_eax20_buffer_property_guid);
inline auto &sound_eax20_underwater_direct_gain = halo::link::ref<float>(halo::sound::vars().sound_eax20_underwater_direct_gain);
inline auto &sound_eax20_listener_property_guid = halo::link::ref<const uint8_t [16]>(halo::sound::vars().sound_eax20_listener_property_guid);
inline auto &sound_eax30_buffer_property_guid = halo::link::ref<const uint8_t [16]>(halo::sound::vars().sound_eax30_buffer_property_guid);
inline auto &sound_underwater_direct_gain = halo::link::ref<float>(halo::sound::vars().sound_underwater_direct_gain);
inline auto &sound_eax30_listener_property_guid = halo::link::ref<const uint8_t [16]>(halo::sound::vars().sound_eax30_listener_property_guid);
