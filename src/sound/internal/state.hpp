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

extern "C" {
extern data_array *game_looping_sound_data;
extern game_sound_globals *game_sound_globals_ptr;
extern data_array *object_data;
extern void object_get_root_object_velocities(uint32_t object_index, real_vector3d *out_velocity, real_vector3d *out_angular_velocity);
extern uint8_t *game_state_base;
extern int32_t game_state_cursor;
extern uint32_t game_state_crc;
extern data_array *game_state_new(char *name, int16_t maximum_count, int16_t element_size);
extern SoundEnvironment sound_environment;
extern uint32_t sound_cluster_audible_bitmap[k_sound_cluster_bitmap_words];
extern int64_t performance_frequency;
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask);
extern void object_get_root_location(int32_t *out, uint32_t object_index);
extern int32_t object_get_node_local_transform(datum_index object_index, char *marker_name, object_marker *marker, uint32_t flags);
extern uint8_t sound_looping_audibility_check;
extern sound_class_definition sound_class_definitions[k_maximum_sound_classes];
extern data_array *looping_sound_data;
extern data_array *sound_data;
extern int32_t sound_time;
extern uint8_t sound_initialized;
extern uint8_t sound_enabled;
extern uint8_t sound_disabled;
extern uint8_t sound_update_toggle;
extern const real_point3d *global_origin3d_pointer;
extern void player_effect_apply_at_object(uint32_t tag_reference, int16_t local_player_index, real_point3d *origin);
extern char k_empty_string[];
extern sound_channel sound_channels[k_maximum_sound_channels];
extern float sound_music_gain;
extern uint8_t sound_idle_update_active;
extern sound_channel_parameters_proc sound_channel_parameters_proc_ptr;
extern sound_driver *current_sound_driver;
extern game_time_globals *game_time;
extern int32_t ai_communication_quiet_until_tick;
extern uint8_t sound_dialog_unspatialized;
extern uint8_t *cinematic_globals_ptr;
extern const float sound_delay_per_world_unit;
extern char ai_marker_name_a[];
extern void object_type_definitions_notify_0x58(uint32_t object_index, datum_index definition_index, datum_index sound_index);
extern float sound_fade_duration_scale;
extern float sound_fade_curve_exponent;
extern double pow(double base, double exponent);
extern int16_t sound_channel_count;
extern sound_listener sound_listeners[1];
extern const real_point3d *global_zero_vector3d_pointer;
extern void unit_accumulate_clamped_offset(uint32_t object_index, float new_value);
extern double sqrt(double x);
extern uint8_t sound_paused;
extern int32_t time_query_performance_counter_ms(void);
extern uint8_t sound_stopping_all;
extern uint8_t debug_sound;
extern int16_t sound_permutation_limit;
extern double cos(double x);
extern double sin(double x);
extern player_globals *local_player_globals;
extern int16_t local_player_0_cluster_index;
extern real_point3d camera_point;
extern bsp_leaf_reference camera_leaf;
extern ScenarioStructureBSP *global_structure_bsp;
extern scenario_game_globals *global_scenario_game_globals;
extern SoundEnvironment k_default_sound_environment;
extern observer observers[1];
extern ModelCollisionGeometryBSP *global_collision_bsp;
extern Globals *global_globals;
extern float sound_dialog_ducking_gain;
extern float sound_ducking_gain;
extern float sound_time_delta;
extern uint8_t collision_test_movement_segment(uint32_t flags, real_point3d *origin, real_vector3d *delta, uint32_t exclude_object_index, collision_result *result);
extern int16_t directsound_first_channel_of_type[4];
extern directsound_channel directsound_channels[k_maximum_sound_channels];
extern float sound_master_gain;
extern float sound_effects_gain;
extern sound_driver *sound_drivers[2];
extern sound_driver_parameters driver_parameters;
extern uint16_t sound_channel_type_flag_table[4];
extern console_globals console_globals_data;
extern game_engine_definition *current_game_engine;
extern uint8_t shell_window_proc_bypass;
extern uint8_t directsound_eax_enabled;
extern uint8_t directsound_eax_available;
extern char *sound_class_names[k_maximum_sound_classes];
extern sound_class_gain *sound_class_gains;
extern double log10(double x);
extern int16_t adpcm_index_table[16];
extern int16_t adpcm_step_table[89];
extern sound_decode_block_proc k_sound_decode_procs[3];
extern sound_decode_block_proc sound_decode_proc;
extern int32_t sound_cache_size_megabytes;
extern char error_text_buffer[];
extern int32_t sound_ogg_underrun_count;
extern int32_t shell_nosound;
extern void *shell_window;
extern void *direct_sound_create8;
extern uint8_t directsound_initialized;
extern int16_t directsound_binding_count;
extern sound_channel_binding directsound_bindings[k_maximum_sound_channels];
extern int16_t directsound_channel_count;
extern uint8_t directsound_caps[0x60];
extern void *directsound;
extern void *directsound_primary_buffer;
extern void *directsound_listener;
extern uint8_t directsound_paused;
extern float directsound_fade;
extern int16_t directsound_hardware_3d_channel_count;
extern int32_t directsound_quality;
extern int32_t directsound_hardware_mode;
extern int16_t sound_effect_object_state;
extern const float directsound_rolloff_factor;
extern uint8_t iid_directsound_3d_listener[16];
extern uint8_t iid_directsound_3d_buffer[16];
extern uint32_t ds3dalg_hrtf_full[4];
extern sound_effect_object *global_sound_effect_object;
extern directsound_listener_cache directsound_listener_cached;
extern SoundEnvironment directsound_environment_cache;
extern uint8_t directsound_deferred_dirty;
extern uint8_t debug_sound_channels;
extern uint8_t debug_sound_channel_details;
extern int16_t hud_text_draw_background_mode;
extern int16_t text_tab_stops;
extern int32_t k_sound_sample_rates[2];
extern uint32_t config_enable_stop_start;
extern uint32_t config_head_relative_speech;
extern sound_effect_object_vtable sound_eax3_vtable;
extern sound_effect_object_vtable sound_eax2_vtable;
extern sound_effect_object_vtable sound_eax1_vtable;
extern const uint8_t sound_eax_listener_property_guid[16];
extern const uint8_t sound_eax_property_set_guid[16];
extern const uint8_t sound_eax20_buffer_property_guid[16];
extern float sound_eax20_underwater_direct_gain;
extern const uint8_t sound_eax20_listener_property_guid[16];
extern const uint8_t sound_eax30_buffer_property_guid[16];
extern float sound_underwater_direct_gain;
extern const uint8_t sound_eax30_listener_property_guid[16];
}
