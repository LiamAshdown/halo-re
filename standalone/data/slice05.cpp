/* standalone/data/slice05.cpp -- All definitions sit in one extern "C" block: the ordered sections, the /alternatename pragmas and src/ reach these objects by their unmangled C names. */
#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"
#include "rasterizer.h"
#include "render.h"
#include "objects.h"
#include "main.h"
#include "units.h"
#include "cutscene.h"
#include "effects.h"
#include "physics.h"
#include "models.h"
#include "structures.h"
#include "text.h"

extern "C" {

// Engine globals at 0x006b1358..0x006e1af0 (slice 5), zero-initialised BSS in the original image.
// Definitions replace the absolute EQU symbols of standalone/globals.asm; types follow the extern declarations in src/.


uint32_t game_engine_attribute_enabled;                                // 0x006b1458
player_control_globals * player_control_globals_ptr;                   // 0x006b145c
datum_index machine_to_player[16];                                     // 0x006b1460
int16_t hs_autocomplete_maximum_count;                                 // 0x006b14a0
char * hs_autocomplete_prefix;                                         // 0x006b14a4
uint8_t hs_reload_pending;                                             // 0x006b14a8
uint16_t hs_autocomplete_gametype_mask;                                // 0x006b14ac
int16_t hs_autocomplete_count;                                         // 0x006b14b0
char ** hs_autocomplete_results;                                       // 0x006b14b4
uint8_t hs_compiling;                                                  // 0x006b14b8
int32_t hs_compiled_source_length;                                     // 0x006b14bc
char * hs_compiled_source;                                             // 0x006b14c0
uint8_t hs_syntax_data_dirty;                                          // 0x006b14d0
char * hs_compile_error;                                               // 0x006b14d4
int32_t hs_compile_error_offset;                                       // 0x006b14d8
char hs_compile_error_buffer[0x100];                                   // 0x006b14dc
uint8_t hs_compiled_source_owned;                                      // 0x006b15dc
uint8_t hs_compile_release_source;                                     // 0x006b15dd
uint8_t hs_blocking_forbidden;                                         // 0x006b15de
uint8_t hs_set_forbidden;                                              // 0x006b15df
uint8_t hs_postprocessing;                                             // 0x006b15e0
int16_t hs_comparison_types[2];                                        // 0x006b15e4
uint8_t hs_runtime_active;                                             // 0x006b15e8
int16_t hs_current_thread_index;                                       // 0x006b15ea
uint8_t input_acquired;                                                // 0x006b15f8
uint8_t input_suppressed;                                              // 0x006b15f9
void * direct_input;                                                   // 0x006b15fc
key_block_timer key_block_timers[k_input_key_block_timer_count];       // 0x006b1600
uint8_t key_frames[0x6d];                                              // 0x006b1620
uint8_t key_release_pending[0x6d];                                     // 0x006b168d
int16_t key_event_read_index;                                          // 0x006b16fa
int16_t key_event_count;                                               // 0x006b16fc
ui_key_event key_events[k_input_key_event_capacity];                   // 0x006b16fe
void ** keyboard_device;                                               // 0x006b1800
void * mouse_device;                                                   // 0x006b1804
int32_t mouse_wheel_granularity;                                       // 0x006b1808
mouse_state live_mouse_state;                                          // 0x006b180c
mouse_state mouse_neutral_state;                                       // 0x006b1828
int32_t input_device_count;                                            // 0x006b1844
void * joystick_devices[8];                                            // 0x006b1848
joystick_state joystick_states[4];                                     // 0x006b2a68
int32_t joystick_slot_devices[4];                                      // 0x006b2ce8
joystick_state joystick_neutral_state;                                 // 0x006b2cf8
first_person_weapon_interface * first_person_weapon_interfaces;        // 0x006b2d98
void * console_input_handle;                                           // 0x006b2dcc
void * console_output_handle;                                          // 0x006b2dd0
char console_window_title[0x20];                                       // 0x006b2dd8
char console_last_line[0x100];                                         // 0x006b2df8
int32_t console_last_cursor_column;                                    // 0x006b2ef8
uint8_t terminal_initialized;                                          // 0x006b2efc
data_array * terminal_messages;                                        // 0x006b2f00
datum_index console_message_head;                                      // 0x006b2f04
datum_index console_message_tail;                                      // 0x006b2f08
terminal_console * console_active;                                     // 0x006b2f0c
uint8_t console_caret_visible;                                         // 0x006b2f10
int32_t console_caret_blink_time;                                      // 0x006b2f14
uint8_t console_win32_attached;                                        // 0x006b2f18
int32_t console_rcon_handle;                                           // 0x006b2f1c
int32_t previous_mouse_y;                                              // 0x006b2f20
int32_t previous_mouse_x;                                              // 0x006b2f24
uint16_t progress_screen_text[0x20];                                   // 0x006b2f28
uint16_t progress_screen_subtext[0x40];                                // 0x006b2f68
uint16_t formatted_prompt_scratch[0x400];                              // 0x006b2fe8
growable_array hud_text_message_queue;                                 // 0x006b37e8
void * server_list_entries_006b380c[9];                                // 0x006b380c
growable_array ui_lists[3];                                            // 0x006b3830
uint16_t ui_player_number_text[2];                                     // 0x006b3854
uint32_t chat_dialog_open;                                             // 0x006b3858
int32_t chat_scope_active;                                             // 0x006b385c
int32_t chat_listbox_x;                                                // 0x006b38e4
int32_t chat_listbox_y;                                                // 0x006b38e8
int32_t chat_listbox_width;                                            // 0x006b38ec
int32_t chat_listbox_height;                                           // 0x006b38f0
int32_t chat_window_unused_6b38f4;                                              // 0x006b38f4
int32_t chat_window_unused_6b3914;                                              // 0x006b3914
int32_t hud_chat_message_expiry[8];                                    // 0x006b3a20
hud_messaging_globals * hud_messaging;                                 // 0x006b3a40
hud_waypoint_state * hud_waypoints;                                    // 0x006b3a44
controls_gamepad_record controls_available_gamepads[8];                // 0x006b42d8
controls_gamepad_record controls_assigned_gamepads[4];                 // 0x006b53d8
wchar_t unicode_string_list_scratch_buffer[256];                       // 0x006b5c58
wchar_t string_widen_scratch[0x400];                                   // 0x006b5e90
video_resolution video_resolutions[0x20];                              // 0x006b6690
rasterizer_display_mode ui_video_requested_display_mode_006b7010;      // 0x006b7010
console_globals console_globals_data;                                  // 0x006b7020
render_view pregame_render_view;                                       // 0x006b79e8
char timedemo_pixel_shader_version[0x14];                              // 0x006b7a94
periodic_function_table * periodic_function_tables[12];                // 0x006b7aa8
periodic_function_table * transition_function_tables[6];               // 0x006b7ad8
uint8_t periodic_functions_initialized;                                // 0x006b7af0
real_point3d * sphere_point_table;                                     // 0x006b7af4
int16_t sphere_point_table_count;                                      // 0x006b7af8
crc32_table crc32_lookup_table;                                        // 0x006b7b00
const char * data_packet_group_error;                                        // 0x006b7f00
float model_render_default_function_values[4];                         // 0x006b7f08
render_model_effect model_render_default_effect;                       // 0x006b7f18
uint8_t model_render_default_region_permutations[32];                  // 0x006b7f40
ColorRGB model_render_default_change_colors[4];                        // 0x006b7f60
int32_t update_server_last_log_ms;                                     // 0x006b7f90
/* The packet block header and its body are one contiguous object in the original (0x006b7f98, body at +2); the packet queueing code writes a header word and then reads the whole block as a packet. */
uint8_t network_challenge_packet_storage[2 + 1536];                    // 0x006b7f98
growable_array ban_list;                                               // 0x006b859c
uint8_t network_log_path_buffer[0x104];                                // 0x006b85b8
message_delta_parameter message_delta_parameters[0x300 / sizeof(message_delta_parameter)]; // 0x006b86c0
uint8_t message_delta_field_changed_flags[0x40];                       // 0x006b89c0
int32_t object_sound_event_last_tick;                                  // 0x006b8a00
uint32_t object_unknown_006b8c60;                                      // 0x006b8c60
ModelCollisionGeometryMaterial default_collision_material;             // 0x006b8c68
uint16_t object_visibility_computed_mask;                              // 0x006b8cb0
memory_pool * object_memory_pool;                                      // 0x006b8cb4
datum_index * object_name_list;                                        // 0x006b8cb8
object_globals * object_globals_pointer;                               // 0x006b8cbc
uint8_t object_marker_scratch[0xb0];                                   // 0x006b8cc0
data_array * light_volume_instances;                                   // 0x006b8d70
data_array * lightning_instances;                                      // 0x006b8d74
breakable_surface_globals * breakable_surface_state;                   // 0x006b8d78
float k_water_density;                                                 // 0x006b8d7c
float k_air_density;                                                   // 0x006b8d80
float render_saved_projection_z[15];                                   // 0x006b8d84
int16_t rendered_object_count;                                         // 0x006b8dc0
datum_index rendered_objects[0x100];                                   // 0x006b8dc4
render_lighting render_uncached_object_lighting;                       // 0x006b91c8
float sky_animation_times[9];                                          // 0x006b923c
frame_graph frame_graphs[1];                                           // 0x006b9260
uint32_t lens_flare_object_visibility_table[0x8c0];                    // 0x006bc510
lens_flare_instance lens_flare_instances[0x400];                       // 0x006ce818
int16_t transparent_geometry_group_draw_cursor;                        // 0x006d9838
uint32_t transparent_geometry_group_drawn_bits[12];                    // 0x006d983c
int16_t rasterizer_bound_bitmap_size_a[2];                             // 0x006d986c
int16_t rasterizer_bound_bitmap_size_b[2];                             // 0x006d9870
int16_t rasterizer_bound_bitmap_size_c[2];                             // 0x006d9874
float rasterizer_screen_quad_vertices[4][6];                           // 0x006d9878
int16_t rasterizer_decal_blend_mode;                                   // 0x006d98d8
int16_t rasterizer_decal_layer;                                        // 0x006d98dc
uint32_t rasterizer_decal_bitmap_tag;                                  // 0x006d98e0
int16_t rasterizer_decal_bitmap_frame;                                 // 0x006d98e4
rasterizer_dynamic_vertex_slot rasterizer_dynamic_vertex_slots[k_rasterizer_dynamic_vertex_slots]; // 0x006d99d8
int32_t rasterizer_dynamic_vertex_slot_count;                          // 0x006dd9d8
rasterizer_dynamic_index_slot rasterizer_dynamic_index_slots[k_rasterizer_dynamic_vertex_slots]; // 0x006dd9e0
int32_t rasterizer_dynamic_index_slot_count;                           // 0x006e09e0
int32_t rasterizer_dynamic_index_count;                                // 0x006e09e4
void ** rasterizer_dynamic_index_buffer;                               // 0x006e09e8
float water_fade_plane_distance;                                       // 0x006e09ec
float water_fade_factor_a;                                             // 0x006e09f0
float water_fade_factor_b;                                             // 0x006e09f4
float rasterizer_underwater_tint_jitter_r;                             // 0x006e09f8
float rasterizer_underwater_tint_jitter_g;                             // 0x006e09fc
float rasterizer_underwater_tint_jitter_b;                             // 0x006e0a00
uint8_t environment_effect_variant;                                              // 0x006e0a04
BitmapData * rasterizer_environment_lightmap;                          // 0x006e0a08
uint8_t rasterizer_environment_lightmap_missing;                       // 0x006e0a0c
rasterizer_projected_light_constants rasterizer_projected_light;       // 0x006e0a10
uint8_t rasterizer_projected_light_has_cube_map;                       // 0x006e0a60
uint32_t rasterizer_projected_light_cube_map;                          // 0x006e0a64
uint8_t rasterizer_lightmap_bitmap_missing;                            // 0x006e0a68
BitmapData * rasterizer_lightmap_bitmap;                               // 0x006e0a6c
transparent_geometry_group transparent_geometry_group_immediate;       // 0x006e0a70
d3d_gamma_ramp rasterizer_desktop_gamma_ramp;                          // 0x006e0b18
d3d_gamma_ramp rasterizer_game_gamma_ramp;                             // 0x006e1118
uint8_t rasterizer_gamma_high_bit_17;                                  // 0x006e1718
rasterizer_dynamic_screen_vertex rasterizer_shadow_screen_quad[4];     // 0x006e1720
int32_t environment_techniques_multipurpose[24];                       // 0x006e1780
void * rasterizer_model_scratch_lighting;                              // 0x006e17e0
float rasterizer_model_effect_vector[4];                               // 0x006e17e4
transparent_geometry_group transparent_geometry_group_environment_immediate; // 0x006e1828
void * rasterizer_model_scratch_function_source;                       // 0x006e18d0
void * rasterizer_model_scratch_node_matrices;                         // 0x006e18d4
int32_t environment_techniques_self_illumination[24];                  // 0x006e18d8
int32_t environment_techniques_plain[12];                              // 0x006e1938
int32_t environment_techniques_reflection[24];                         // 0x006e1968
int16_t rasterizer_model_scratch_node_count;                           // 0x006e19c8
int32_t environment_techniques_change_color[24];                       // 0x006e19d0
rasterizer_dynamic_screen_vertex rasterizer_screen_effect_quad[4];     // 0x006e1a30

}
