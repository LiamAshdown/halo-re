/* standalone/data/slice09.cpp -- engine globals 0x00721ebc..0x007c04a0 as C variables (all zero-initialised BSS in
   the original image). Generated once from standalone/globals.asm + the extern declarations in src/; the names are the
   symbols src/ links against. Names that share an address with another variable are /alternatename aliases.
   Left as absolute EQU symbols in globals.asm: the 0x00746280 block (unknown_00746280_block, ambient_noise,
   weather_particle_system_count, weather_wind_states, weather_frame_counter), see the slice 9 notes.

   All definitions sit in one extern "C" block: the ordered sections, the /alternatename pragmas and src/ reach these objects by their unmangled C names. */
#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "interface.h"
#include "rasterizer.h"
#include "shell.h"
#include "sound.h"
#include "networking.h"
#include "objects.h"
#include "effects.h"
#include "structures.h"
#include "scenario.h"

extern "C" {

/* the gamespy gcd game record (src/gamespy/gcd_authenticate_user.c, 0x10 bytes) */
typedef struct gcd_client_node_s09 {
    struct gcd_client_node_s09 *next, *prev;
} gcd_client_node_s09;
typedef struct gcd_game {
    int game_id;
    gcd_client_node_s09 sentinel;
} gcd_game;

/* the object must fit before the next fixed-address global of the original layout */
#define HALO_SZ_CHECK(var, limit) static_assert(sizeof(var) <= (limit))

keystone_update_fn keystone_update;  // 0x00721ebc
keystone_dispatch_message_fn keystone_dispatch_message;  // 0x00721ec0
keystone_unknown_fn keystone_set_focus_window;  // 0x00721ec4
chat_gui_release_fn chat_gui_release;  // 0x00721ec8
chat_gui_find_child_fn chat_gui_find_child;  // 0x00721ecc
chat_gui_finalize_fn chat_gui_finalize;  // 0x00721ed0
chat_gui_set_focus_fn chat_gui_set_focus;  // 0x00721ed4
keystone_unknown_fn keystone_window_add_dirty_control;  // 0x00721ed8
chat_gui_set_state_fn chat_gui_set_state;  // 0x00721edc
chat_gui_get_property_string_fn keystone_control_get_attribute;  // 0x00721ee0
chat_gui_set_property_string_fn keystone_control_set_attribute;  // 0x00721ee4
chat_gui_set_property_int_fn chat_gui_set_property_int;  // 0x00721ee8
uint8_t chat_gui_active;  // 0x00721eec
int32_t os_platform_value;  // 0x00721ef0
void *shell_instance_mutex;  // 0x00721f00
int32_t shell_instance_index;  // 0x00721f04
void *shell_stack_guard_page;  // 0x00721f08
uint32_t shell_stack_guard_old_protect;  // 0x00721f0c
report_fault_fn report_fault;  // 0x00721f10
int32_t sound_ogg_underrun_count;  // 0x00721f14
sound_effect_object *global_sound_effect_object;  // 0x00721f24
char *user_save_path_default;  // 0x00721f28
char user_save_paths[k_maximum_user_save_paths][k_user_save_path_slot_stride];  // 0x00721f30
uint32_t user_save_path_keys[k_maximum_user_save_paths];  // 0x00722758
detail_object_globals *detail_objects;  // 0x0072277c
uint8_t *runtime_decals_suppressed;  // 0x0072278c
int32_t unit_dialogue_variant_counter;  // 0x00722794
char network_session_start_host_name[8];  // 0x00722798
char network_session_start_map_name[8];  // 0x007227a0
char network_session_start_variant_name[16];  // 0x007227a8
int32_t network_session_start_game_type;  // 0x007227b8
uint8_t autopatch_download_pool_stop;  // 0x007227bc
network_mutex_record *autopatch_download_mutex;  // 0x007227c0
network_thread_record *autopatch_download_thread;  // 0x007227c4
uint8_t autopatch_download_active_count;  // 0x007227c8
char autopatch_proxy_server[0x100];  // 0x007227d0
uint8_t autopatch_proxy_ready;  // 0x007228d0
int32_t network_session_host_state;  // 0x00722a18
uint8_t network_session_host_closing;  // 0x00722a1c
void *network_session_host_object;  // 0x00722a20
int32_t network_session_host_last_tick;  // 0x00722a24
char network_qr2_text[0x100];  // 0x00722a28
int32_t config_linear_texture_addressing;  // 0x00722b28
int32_t config_linear_texture_addressing_zoom;  // 0x00722b2c
int32_t config_linear_texture_addressing_sun;  // 0x00722b30
int32_t config_use_fixed_function;  // 0x00722b34
int32_t config_disable_driver_management;  // 0x00722b38
int32_t config_unsupported_card;  // 0x00722b3c
int32_t config_prototype_card;  // 0x00722b40
int32_t config_old_driver;  // 0x00722b44
int32_t config_old_sound_driver;  // 0x00722b48
int32_t config_invalid_driver;  // 0x00722b4c
int32_t config_invalid_sound_driver;  // 0x00722b50
int32_t config_disable_buffering;  // 0x00722b54
int32_t config_enable_stop_start;  // 0x00722b58
int32_t config_head_relative_speech;  // 0x00722b5c
int32_t config_safe_mode;  // 0x00722b60
int32_t config_force_shader;  // 0x00722b64
int32_t config_use_anisotropic_filter;  // 0x00722b68
int32_t config_disable_specular;  // 0x00722b6c
int32_t config_disable_render_targets;  // 0x00722b70
int32_t config_disable_alpha_render_targets;  // 0x00722b74
int32_t config_use_alternate_convolve_mask;  // 0x00722b78
int32_t config_min_max_blend_op_is_broken;  // 0x00722b7c
float config_decal_z_bias;  // 0x00722b80
float config_transparent_decal_z_bias;  // 0x00722b84
float config_decal_slope_z_bias;  // 0x00722b88
float config_transparent_decal_slope_z_bias;  // 0x00722b8c
char *graphics_vendor_name;  // 0x00722b90
char *graphics_device_name;  // 0x00722b94
uint32_t graphics_device_id;  // 0x00722b98
uint32_t graphics_vendor_id;  // 0x00722b9c
large_integer graphics_driver_version;  // 0x00722ba0
uint32_t physical_memory;  // 0x00722ba8
uint32_t cpu_speed;  // 0x00722bac
uint32_t video_memory;  // 0x00722bb0
uint32_t display_adapter_count;  // 0x00722bb4
void *shell_module_handle;  // 0x00722bb8
char *rasterizer_shader_file_name;  // 0x00722bbc
int32_t fatal_error_remember_choice;  // 0x00722bc0
int32_t dialog_hyperlink_hovered;  // 0x00722bc8
uint32_t crypt_provider;  // 0x00722bcc
int32_t crash_in_progress;  // 0x00722bd0
char product_id_string[k_product_id_string_length];  // 0x00722bd8
char fatal_error_system_specs[0x100];  // 0x00722c58
char hwreq_quoted_string[k_hwreq_quoted_string_length];  // 0x00722d58
char config_error_text[k_shell_config_message_length];  // 0x00722e58
char config_unknown_property_text[k_shell_config_message_length];  // 0x00722f58
char hwreq_open_error_text[388];  // 0x00723058
unsigned short ghiProxyPort;  // 0x007231dc
char *ghiProxyAddress;  // 0x007231e0
gcd_game gcd_games[4];  // 0x00723200
unsigned char gt2_bignum_modulus[0x400];  // 0x00723240
char qr2_hostname[5124];  // 0x00723640
uint8_t is_dedicated_server_flag;  // 0x00724a44
uint8_t debug_render_cluster_pvs;  // 0x00724a45
uint8_t debug_count_all_leaf_portals;  // 0x00724a46
sound_decode_block_proc sound_decode_proc;  // 0x00724a48
uint8_t debug_sound_channels;  // 0x00724a4c
uint8_t debug_sound;  // 0x00724a4d
data_array *looping_sound_data;  // 0x00724a50
uint8_t sound_looping_audibility_check;  // 0x00724a54
sound_channel sound_channels[k_maximum_sound_channels];  // 0x00724a60
uint8_t debug_sound_channel_details;  // 0x007251f8
uint8_t sound_initialized;  // 0x00725200
uint8_t sound_enabled;  // 0x00725201
uint8_t sound_paused;  // 0x00725202
uint8_t sound_idle_update_active;  // 0x00725203
int32_t ai_communication_quiet_until_tick;  // 0x00725204
sound_driver *current_sound_driver;  // 0x00725208
int32_t sound_time;  // 0x0072520c
float sound_time_delta;  // 0x00725210
uint8_t sound_update_toggle;  // 0x00725214
sound_listener sound_listeners[1];  // 0x00725218
SoundEnvironment sound_environment;  // 0x0072525c
float sound_ducking_gain;  // 0x007252a4
float sound_music_gain;  // 0x007252a8
float sound_master_gain;  // 0x007252ac
float sound_effects_gain;  // 0x007252b0
int16_t sound_channel_count;  // 0x007252b4
uint8_t sound_disabled;  // 0x007252b6
uint8_t sound_stopping_all;  // 0x007252b7
int16_t sound_permutation_limit;  // 0x007252b8
uint8_t sound_dialog_unspatialized;  // 0x007252bc
data_array *sound_data;  // 0x007252c0
uint8_t directsound_initialized;  // 0x007252e0
int16_t directsound_binding_count;  // 0x007252e2
sound_channel_binding directsound_bindings[k_maximum_sound_channels];  // 0x007252e4
int16_t directsound_channel_count;  // 0x00725428
directsound_channel directsound_channels[k_maximum_sound_channels];  // 0x00725430
int16_t directsound_first_channel_of_type[4];  // 0x00746028
directsound_listener_cache directsound_listener_cached;  // 0x00746030
SoundEnvironment directsound_environment_cache;  // 0x00746064
uint8_t directsound_caps[0x60];  // 0x007460ac
void *directsound;  // 0x0074610c
void *directsound_primary_buffer;  // 0x00746110
void *directsound_listener;  // 0x00746114
uint8_t directsound_paused;  // 0x00746118
float directsound_fade;  // 0x0074611c
uint8_t directsound_eax_available;  // 0x00746120
uint8_t directsound_eax_enabled;  // 0x00746121
int16_t sound_supplementary_buffers_00746122;  // 0x00746122
int16_t directsound_hardware_3d_channel_count;  // 0x00746124
int32_t directsound_quality;  // 0x00746128
int32_t directsound_hardware_mode;  // 0x0074612c
int16_t sound_effect_object_state;  // 0x00746130
uint8_t directsound_deferred_dirty;  // 0x00746132
float sound_listener_rolloff_factor;  // 0x00746134
float sound_listener_doppler_factor;  // 0x00746138
sound_class_gain *sound_class_gains;  // 0x00746140
uint32_t sound_cluster_audible_bitmap[k_sound_cluster_bitmap_words];  // 0x00746160
data_array *game_looping_sound_data;  // 0x007461a0
game_sound_globals *game_sound_globals_ptr;  // 0x007461a4
char *shell_product_id;  // 0x007461a8
void *shell_instance;  // 0x007461c0
void *shell_window;  // 0x007461c4
void *rasterizer_window_handle;  // 0x007461c8
int32_t shell_show_command;  // 0x007461cc
void *shell_window_proc;  // 0x007461d0
char shell_window_class_name[k_shell_window_name_length];  // 0x007461d4
char shell_window_title[k_shell_window_name_length];  // 0x00746214
uint8_t shell_window_minimized;  // 0x00746254
uint8_t shell_window_maximized;  // 0x00746255
void *dinput8_module;  // 0x00746258
void *dsound_module;  // 0x0074625c
void *shfolder_module;  // 0x00746260
void *d3d9_module;  // 0x00746264
void *direct_input8_create;  // 0x00746268
void *sh_get_folder_path;  // 0x0074626c
void *direct_sound_create8;  // 0x00746270
void *direct3d_create9_procedure;  // 0x00746274
Scenario *global_scenario;  // 0x00746f8c
ModelCollisionGeometryBSP *global_collision_bsp;  // 0x00746f90
scenario_game_globals *global_scenario_game_globals;  // 0x00746f94
ModelCollisionGeometryBSP *global_structure_collision_bsp;  // 0x00746f98
ScenarioStructureBSP *global_structure_bsp;  // 0x00746f9c
Globals *global_globals;  // 0x00746fa0
uint8_t recover_saved_games_hack;  // 0x00746fa4
uint32_t lens_flare_batch_clock;  // 0x00746fa8
lens_flare_batch lens_flare_batches[k_lens_flare_batch_slots];  // 0x00746fc0
void *rasterizer_water_draw_procedure;  // 0x007bf050
rasterizer_vertex_buffer_slot rasterizer_vertex_buffer_slots[k_rasterizer_vertex_buffer_slots];  // 0x007bf060
d3dx_macro rasterizer_effect_defines[2];  // 0x007c0460
void *shader_environment_draw_simple;  // 0x007c0470
void *shader_environment_draw;  // 0x007c0474
uint8_t model_render_first_person;  // 0x007c0478
float planar_fog_attenuation;  // 0x007c047c
void *rasterizer_glass_draw_procedures[3];  // 0x007c0480
void *unknown_007c048c;  // 0x007c048c
void *unknown_007c0490;  // 0x007c0490
void *unknown_007c0494;  // 0x007c0494
d3d_present_parameters rasterizer_present_parameters;  // 0x007c04a0

/* names that share an address with one of the objects above */
#pragma comment(linker, "/alternatename:_unknown_00721ebc=_keystone_update")
#pragma comment(linker, "/alternatename:_unknown_00721ec8=_chat_gui_release")
#pragma comment(linker, "/alternatename:_unknown_00721edc=_chat_gui_set_state")
#pragma comment(linker, "/alternatename:_os_platform=_os_platform_value")
#pragma comment(linker, "/alternatename:_direct3d_create9=_direct3d_create9_procedure")

HALO_SZ_CHECK(keystone_update, 4);
HALO_SZ_CHECK(keystone_dispatch_message, 4);
HALO_SZ_CHECK(keystone_set_focus_window, 4);
HALO_SZ_CHECK(chat_gui_release, 4);
HALO_SZ_CHECK(chat_gui_find_child, 4);
HALO_SZ_CHECK(chat_gui_finalize, 4);
HALO_SZ_CHECK(chat_gui_set_focus, 4);
HALO_SZ_CHECK(keystone_window_add_dirty_control, 4);
HALO_SZ_CHECK(chat_gui_set_state, 4);
HALO_SZ_CHECK(keystone_control_get_attribute, 4);
HALO_SZ_CHECK(keystone_control_set_attribute, 4);
HALO_SZ_CHECK(chat_gui_set_property_int, 4);
HALO_SZ_CHECK(chat_gui_active, 4);
HALO_SZ_CHECK(os_platform_value, 16);
HALO_SZ_CHECK(shell_instance_mutex, 4);
HALO_SZ_CHECK(shell_instance_index, 4);
HALO_SZ_CHECK(shell_stack_guard_page, 4);
HALO_SZ_CHECK(shell_stack_guard_old_protect, 4);
HALO_SZ_CHECK(report_fault, 4);
HALO_SZ_CHECK(sound_ogg_underrun_count, 16);
HALO_SZ_CHECK(global_sound_effect_object, 4);
HALO_SZ_CHECK(user_save_path_default, 8);
HALO_SZ_CHECK(user_save_paths, 2088);
HALO_SZ_CHECK(user_save_path_keys, 36);
HALO_SZ_CHECK(detail_objects, 16);
HALO_SZ_CHECK(runtime_decals_suppressed, 8);
HALO_SZ_CHECK(unit_dialogue_variant_counter, 4);
HALO_SZ_CHECK(network_session_start_host_name, 8);
HALO_SZ_CHECK(network_session_start_map_name, 8);
HALO_SZ_CHECK(network_session_start_variant_name, 16);
HALO_SZ_CHECK(network_session_start_game_type, 4);
HALO_SZ_CHECK(autopatch_download_pool_stop, 4);
HALO_SZ_CHECK(autopatch_download_mutex, 4);
HALO_SZ_CHECK(autopatch_download_thread, 4);
HALO_SZ_CHECK(autopatch_download_active_count, 8);
HALO_SZ_CHECK(autopatch_proxy_server, 256);
HALO_SZ_CHECK(autopatch_proxy_ready, 4);
HALO_SZ_CHECK(network_session_host_state, 4);
HALO_SZ_CHECK(network_session_host_closing, 4);
HALO_SZ_CHECK(network_session_host_object, 4);
HALO_SZ_CHECK(network_session_host_last_tick, 4);
HALO_SZ_CHECK(network_qr2_text, 256);
HALO_SZ_CHECK(config_linear_texture_addressing, 4);
HALO_SZ_CHECK(config_linear_texture_addressing_zoom, 4);
HALO_SZ_CHECK(config_linear_texture_addressing_sun, 4);
HALO_SZ_CHECK(config_use_fixed_function, 4);
HALO_SZ_CHECK(config_disable_driver_management, 4);
HALO_SZ_CHECK(config_unsupported_card, 4);
HALO_SZ_CHECK(config_prototype_card, 4);
HALO_SZ_CHECK(config_old_driver, 4);
HALO_SZ_CHECK(config_old_sound_driver, 4);
HALO_SZ_CHECK(config_invalid_driver, 4);
HALO_SZ_CHECK(config_invalid_sound_driver, 4);
HALO_SZ_CHECK(config_disable_buffering, 4);
HALO_SZ_CHECK(config_enable_stop_start, 4);
HALO_SZ_CHECK(config_head_relative_speech, 4);
HALO_SZ_CHECK(config_safe_mode, 4);
HALO_SZ_CHECK(config_force_shader, 4);
HALO_SZ_CHECK(config_use_anisotropic_filter, 4);
HALO_SZ_CHECK(config_disable_specular, 4);
HALO_SZ_CHECK(config_disable_render_targets, 4);
HALO_SZ_CHECK(config_disable_alpha_render_targets, 4);
HALO_SZ_CHECK(config_use_alternate_convolve_mask, 4);
HALO_SZ_CHECK(config_min_max_blend_op_is_broken, 4);
HALO_SZ_CHECK(config_decal_z_bias, 4);
HALO_SZ_CHECK(config_transparent_decal_z_bias, 4);
HALO_SZ_CHECK(config_decal_slope_z_bias, 4);
HALO_SZ_CHECK(config_transparent_decal_slope_z_bias, 4);
HALO_SZ_CHECK(graphics_vendor_name, 4);
HALO_SZ_CHECK(graphics_device_name, 4);
HALO_SZ_CHECK(graphics_device_id, 4);
HALO_SZ_CHECK(graphics_vendor_id, 4);
HALO_SZ_CHECK(graphics_driver_version, 8);
HALO_SZ_CHECK(physical_memory, 4);
HALO_SZ_CHECK(cpu_speed, 4);
HALO_SZ_CHECK(video_memory, 4);
HALO_SZ_CHECK(display_adapter_count, 4);
HALO_SZ_CHECK(shell_module_handle, 4);
HALO_SZ_CHECK(rasterizer_shader_file_name, 4);
HALO_SZ_CHECK(fatal_error_remember_choice, 8);
HALO_SZ_CHECK(dialog_hyperlink_hovered, 4);
HALO_SZ_CHECK(crypt_provider, 4);
HALO_SZ_CHECK(crash_in_progress, 8);
HALO_SZ_CHECK(product_id_string, 128);
HALO_SZ_CHECK(fatal_error_system_specs, 256);
HALO_SZ_CHECK(hwreq_quoted_string, 256);
HALO_SZ_CHECK(config_error_text, 256);
HALO_SZ_CHECK(config_unknown_property_text, 256);
HALO_SZ_CHECK(hwreq_open_error_text, 388);
HALO_SZ_CHECK(ghiProxyPort, 4);
HALO_SZ_CHECK(ghiProxyAddress, 32);
HALO_SZ_CHECK(gcd_games, 64);
HALO_SZ_CHECK(gt2_bignum_modulus, 1024);
HALO_SZ_CHECK(qr2_hostname, 5124);
HALO_SZ_CHECK(is_dedicated_server_flag, 1);
HALO_SZ_CHECK(debug_render_cluster_pvs, 1);
HALO_SZ_CHECK(debug_count_all_leaf_portals, 2);
HALO_SZ_CHECK(sound_decode_proc, 4);
HALO_SZ_CHECK(debug_sound_channels, 1);
HALO_SZ_CHECK(debug_sound, 3);
HALO_SZ_CHECK(looping_sound_data, 4);
HALO_SZ_CHECK(sound_looping_audibility_check, 12);
HALO_SZ_CHECK(sound_channels, 1944);
HALO_SZ_CHECK(debug_sound_channel_details, 8);
HALO_SZ_CHECK(sound_initialized, 1);
HALO_SZ_CHECK(sound_enabled, 1);
HALO_SZ_CHECK(sound_paused, 1);
HALO_SZ_CHECK(sound_idle_update_active, 1);
HALO_SZ_CHECK(ai_communication_quiet_until_tick, 4);
HALO_SZ_CHECK(current_sound_driver, 4);
HALO_SZ_CHECK(sound_time, 4);
HALO_SZ_CHECK(sound_time_delta, 4);
HALO_SZ_CHECK(sound_update_toggle, 4);
HALO_SZ_CHECK(sound_listeners, 68);
HALO_SZ_CHECK(sound_environment, 72);
HALO_SZ_CHECK(sound_ducking_gain, 4);
HALO_SZ_CHECK(sound_music_gain, 4);
HALO_SZ_CHECK(sound_master_gain, 4);
HALO_SZ_CHECK(sound_effects_gain, 4);
HALO_SZ_CHECK(sound_channel_count, 2);
HALO_SZ_CHECK(sound_disabled, 1);
HALO_SZ_CHECK(sound_stopping_all, 1);
HALO_SZ_CHECK(sound_permutation_limit, 4);
HALO_SZ_CHECK(sound_dialog_unspatialized, 4);
HALO_SZ_CHECK(sound_data, 32);
HALO_SZ_CHECK(directsound_initialized, 2);
HALO_SZ_CHECK(directsound_binding_count, 2);
HALO_SZ_CHECK(directsound_bindings, 324);
HALO_SZ_CHECK(directsound_channel_count, 8);
HALO_SZ_CHECK(directsound_channels, 134136);
HALO_SZ_CHECK(directsound_first_channel_of_type, 8);
HALO_SZ_CHECK(directsound_listener_cached, 52);
HALO_SZ_CHECK(directsound_environment_cache, 72);
HALO_SZ_CHECK(directsound_caps, 96);
HALO_SZ_CHECK(directsound, 4);
HALO_SZ_CHECK(directsound_primary_buffer, 4);
HALO_SZ_CHECK(directsound_listener, 4);
HALO_SZ_CHECK(directsound_paused, 4);
HALO_SZ_CHECK(directsound_fade, 4);
HALO_SZ_CHECK(directsound_eax_available, 1);
HALO_SZ_CHECK(directsound_eax_enabled, 1);
HALO_SZ_CHECK(sound_supplementary_buffers_00746122, 2);
HALO_SZ_CHECK(directsound_hardware_3d_channel_count, 4);
HALO_SZ_CHECK(directsound_quality, 4);
HALO_SZ_CHECK(directsound_hardware_mode, 4);
HALO_SZ_CHECK(sound_effect_object_state, 2);
HALO_SZ_CHECK(directsound_deferred_dirty, 2);
HALO_SZ_CHECK(sound_listener_rolloff_factor, 4);
HALO_SZ_CHECK(sound_listener_doppler_factor, 8);
HALO_SZ_CHECK(sound_class_gains, 32);
HALO_SZ_CHECK(sound_cluster_audible_bitmap, 64);
HALO_SZ_CHECK(game_looping_sound_data, 4);
HALO_SZ_CHECK(game_sound_globals_ptr, 4);
HALO_SZ_CHECK(shell_product_id, 24);
HALO_SZ_CHECK(shell_instance, 4);
HALO_SZ_CHECK(shell_window, 4);
HALO_SZ_CHECK(rasterizer_window_handle, 4);
HALO_SZ_CHECK(shell_show_command, 4);
HALO_SZ_CHECK(shell_window_proc, 4);
HALO_SZ_CHECK(shell_window_class_name, 64);
HALO_SZ_CHECK(shell_window_title, 64);
HALO_SZ_CHECK(shell_window_minimized, 1);
HALO_SZ_CHECK(shell_window_maximized, 3);
HALO_SZ_CHECK(dinput8_module, 4);
HALO_SZ_CHECK(dsound_module, 4);
HALO_SZ_CHECK(shfolder_module, 4);
HALO_SZ_CHECK(d3d9_module, 4);
HALO_SZ_CHECK(direct_input8_create, 4);
HALO_SZ_CHECK(sh_get_folder_path, 4);
HALO_SZ_CHECK(direct_sound_create8, 4);
HALO_SZ_CHECK(direct3d_create9_procedure, 12);
HALO_SZ_CHECK(global_scenario, 4);
HALO_SZ_CHECK(global_collision_bsp, 4);
HALO_SZ_CHECK(global_scenario_game_globals, 4);
HALO_SZ_CHECK(global_structure_collision_bsp, 4);
HALO_SZ_CHECK(global_structure_bsp, 4);
HALO_SZ_CHECK(global_globals, 4);
HALO_SZ_CHECK(recover_saved_games_hack, 4);
HALO_SZ_CHECK(lens_flare_batch_clock, 8);
HALO_SZ_CHECK(lens_flare_batches, 491648);
HALO_SZ_CHECK(rasterizer_water_draw_procedure, 16);
HALO_SZ_CHECK(rasterizer_vertex_buffer_slots, 5120);
HALO_SZ_CHECK(rasterizer_effect_defines, 16);
HALO_SZ_CHECK(shader_environment_draw_simple, 4);
HALO_SZ_CHECK(shader_environment_draw, 4);
HALO_SZ_CHECK(model_render_first_person, 4);
HALO_SZ_CHECK(planar_fog_attenuation, 4);
HALO_SZ_CHECK(rasterizer_glass_draw_procedures, 12);
HALO_SZ_CHECK(unknown_007c048c, 4);
HALO_SZ_CHECK(unknown_007c0490, 4);
HALO_SZ_CHECK(unknown_007c0494, 12);
HALO_SZ_CHECK(rasterizer_present_parameters, 64);

}
