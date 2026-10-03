/* standalone/data/slice01.cpp -- engine globals of address range 0x000050..0x66e660, as extern "C" definitions.
   (globals->C slice 1.) Replaces the absolute EQU symbols formerly in standalone/globals.asm.
   Initial values are the bytes the data image held at the original address; each pointer is expressed in C
   (function name, &global, string literal). The tables of the AI region (0x655254..0x6571f4) are strided across
   their symbols by the original code, so they sit in one section in address order with no gaps: the layout
   is the original one (tools/globals_check_slice01.py verifies it from the link map).

   All definitions sit in one extern "C" block: the ordered sections, the /alternatename pragmas and src/ reach these objects by their unmangled C names. */
#include "halo/hs/api.hpp"
#include "halo/ai/api.hpp"
#include "halo/shell/api.hpp"
#include "halo/cseries/api.hpp"
#include "halo/effects/api.hpp"
#include "halo/sound/api.hpp"
#include <stdint.h>
#include <stddef.h>

extern "C" {

/* globals of other slices that these tables point at */
extern char actor_mode_guard_look_weights_ambush[];
extern char actor_mode_guard_look_weights_idle[];
extern char actor_mode_uncover_look_weights_active[];
extern char console_color_00685214[];
extern char console_message_default_color[];
extern char global_white_argb[];
extern char hud_text_message_hold_color[];
extern char hud_text_message_normal_color[];
#pragma section(".gdat01", read, write)

/* forward declarations of the definitions below */
extern uint32_t ds3dalg_hrtf_full[4];
extern uint8_t sound_eax_property_set_guid[16];
extern uint8_t iid_directsound_3d_buffer[16];
extern uint8_t iid_directsound_3d_listener[16];
extern uint32_t dsdevid_default_playback[4];
extern uint32_t guid_sys_keyboard[4];
extern uint32_t guid_sys_mouse[4];
extern uint32_t iid_directinput8a[4];
extern uint32_t iid_direct_draw7[4];
extern uint8_t sound_eax_listener_property_guid[16];
extern uint8_t sound_eax20_listener_property_guid[16];
extern uint8_t sound_eax20_buffer_property_guid[16];
extern uint8_t sound_eax30_listener_property_guid[16];
extern uint8_t sound_eax30_buffer_property_guid[16];
extern char md5_hex_byte_format[8];
extern void * std_exception_vtable[2];
extern void * logic_error_vtable[2];
extern void * length_error_vtable[2];
extern void * out_of_range_vtable[2];
extern char string_invalid_string_position[24];
extern char string_string_too_long[16];
extern char k_empty_string[4];
extern uint32_t actor_mode_definitions[9];
extern uint32_t ai_actor_mode_dispatch_table[184];
extern uint32_t actor_dodge_table[14];
extern int16_t order_code_mode_data_expect[12];
extern int16_t actor_lookup_table_006555a8[12];
extern uint32_t actor_firing_position_score_rules[14];
extern uint32_t actor_firing_position_reject_rules[12];
extern int16_t actor_vocalization_variant[2];
extern int16_t actor_dialogue_variant_table_g[4];
extern int16_t actor_dialogue_variant_table_d[2];
extern int16_t actor_dialogue_variant_table_a[2];
extern int16_t actor_dialogue_variant_table_c[2];
extern int16_t actor_dialogue_variant_table_e[2];
extern int16_t actor_dialogue_variant_table_f[8];
extern int16_t actor_dialogue_variant_table_b[6];
extern float actor_vocalization_duration[17];
extern float actor_avoidance_b_radius[9];
extern float actor_avoidance_b_elevation[9];
extern float actor_avoidance_b_bearing[10];
extern float actor_avoidance_a_bearing[8];
extern float actor_avoidance_a_radius[2];
extern float actor_avoidance_a_elevation[3];
extern float actor_avoidance_near_weights[72];
extern uint32_t actor_avoidance_ray_weights[6];
extern int16_t actor_combat_status_min_grade[28];
extern uint8_t actor_control_animation_state_table[12];
extern int16_t ai_communication_class_priority[8];
extern float ai_communication_class_tail_seconds[8];
extern int16_t ai_communication_class_follow_up[8];
extern int16_t ai_communication_class_look_marker[8];
extern int16_t ai_communication_class_no_actor_class[14];
extern float ai_communication_class_repeat_delay[8];
extern float ai_communication_direction_table[70];
extern uint32_t ai_communication_selector_delay_seconds[14];
extern uint8_t ai_communication_lines[20];
extern uint32_t DAT_00655ab4[1045];
extern uint32_t ai_communication_event_definitions[7];
extern uint32_t DAT_00656b24[412];
extern int16_t ai_vocalization_line_table[48];
extern int8_t bitmap_format_bits_per_pixel[18];
extern uint8_t natneg_magic[6];
extern float observer_channel_acceleration_limit[5];
extern uint32_t k_decal_type_parameters[16];
extern void * particle_system_update_physics_table[2];
extern void * particle_creation_physics_table[3];
extern void * particle_update_physics_table[1];
extern int16_t weapon_zoom_index_substitutions[36];
extern float player_placement_ring[27];
extern int16_t hs_object_type_masks[6];
extern uint32_t hs_tag_group_for_type[8];
extern int16_t hs_type_sizes[50];
extern char hs_space_characters[4];
extern char hs_newline_characters[4];
extern void * hs_parse_primitive_procedures[49];
extern char input_action_names[27][16];
extern uint32_t input_default_profile_guid[4];
extern char joystick_button_prefix[24];
extern char joystick_axis_prefix[24];
extern char joystick_pov_prefix[24];
extern char pov_direction_names[8][10];
extern char decimal_suffixes[32][3];
extern int16_t virtual_key_to_key[256];
extern int16_t character_to_key[128];
extern int16_t scan_code_to_key[256];
extern int32_t resolution_index_table_0065bf74[16];
extern int32_t resolution_row_count_table_0065bfb4[5];
extern float motion_sensor_blip_colors[18];
extern int32_t controls_reserved_action_table[9];
extern float k_octahedron_vertices[18];
extern int16_t k_octahedron_faces[24];
extern float global_origin3d[3];
extern int16_t k_projection_axes[12];
extern uint8_t bit_mask_clear[8];
extern uint8_t bit_mask_keep[9];
extern void * network_bandwidth_units_label_table[2];
extern void * network_bandwidth_direction_label_table[2];
extern uint32_t message_delta_unary_ones[2];
extern uint8_t message_delta_item_count_bits[2049];
extern uint32_t object_lighting_default[29];
extern float object_lightmap_probe_direction[3];
extern float object_lighting_probe_sideways[12];
extern void * ai_gc_callback_table[6];
extern int16_t rasterizer_vertex_sizes[20];
extern uint32_t rasterizer_blend_src_table[9];
extern uint32_t rasterizer_blend_dest_table[9];
extern uint32_t rasterizer_blend_op_table[12];
extern uint32_t rasterizer_triangle_buffer_primitive_types[2];
extern int32_t rasterizer_bitmap_format_to_d3dformat[18];
extern int16_t rasterizer_cube_face_to_d3d_face[6];
extern float rasterizer_sun_glow_blur_offsets[32];
extern float rasterizer_identity_vertex_constants[20];
extern uint32_t vertex_elements_debug[6];
extern uint32_t vertex_elements_decal[6];
extern uint32_t vertex_elements_detail_object[8];
extern uint32_t vertex_elements_dynamic_screen[8];
extern uint32_t vertex_elements_dynamic[8];
extern uint32_t vertex_elements_unlit_zsprite[10];
extern uint32_t vertex_elements_model[16];
extern uint32_t vertex_elements_model_ff[8];
extern uint32_t vertex_elements_model_processed[8];
extern uint32_t vertex_elements_environment_lightmap[16];
extern uint32_t vertex_elements_environment_lightmap_ff[10];
extern uint32_t vertex_elements_environment_uncompressed[12];
extern uint32_t vertex_elements_environment_uncompressed_ff[8];
extern uint32_t vertex_elements_environment_single_stream_ff[10];
extern uint32_t vertex_elements_screen_transformed_lit[8];
extern uint32_t vertex_elements_screen_transformed_lit_specular[10];
extern int16_t rasterizer_first_map_bitmap_types[4];
extern uint32_t rasterizer_first_map_address_modes[4];
extern int16_t rasterizer_extended_first_map_bitmap_types[4];
extern uint32_t rasterizer_extended_first_map_address_modes[4];
extern int32_t k_sound_sample_rates[2];
extern uint32_t k_default_sound_environment[18];
extern float sound_delay_per_world_unit[1];
extern int16_t adpcm_index_table[16];
extern int16_t adpcm_step_table[89];
extern void * k_sound_decode_procs[3];
extern float near_clip_plane[4];
extern float placement_offset_table[81];
extern int16_t unit_speech_fallback_index[210];
extern int16_t unit_speech_priority_table[12];
extern float unit_speech_repeat_seconds[12];
extern char network_log_path_format[4];
extern char prop_array_name[8];
extern char joystick_set_separator_0065f010[4];
extern char DAT_0065fb14[4];
extern char DAT_0065fb2c[4];
extern char decimal_format_string[4];
extern char network_summary_log_mode_string[4];
extern char file_open_mode_w[4];
extern char s_primary_trigger_marker[16];
extern char hwreq_version_root_block[4];
extern char error_file_no_timestamp[24];
extern char error_file_timestamp_format[32];
extern char error_file_name[12];
extern char error_file_open_mode[4];
extern char error_file_address_format[24];
extern char error_file_function_format[28];
extern char error_file_function_name[24];
extern char error_file_banner[80];
extern char error_file_spacer[8];
extern wchar_t message_delta_config_value_delimiters[4];
extern wchar_t PTR_s_parameter_handles_0063fff0_0x35_006607a0[4];
extern wchar_t chat_local_prompt_string[4];
extern wchar_t empty_string[2];
extern char console_echo_prefix[4];
extern char campaign_level_short_names[40];
extern char player_help_name_d40[4];
extern char player_help_name_d20[4];
extern char player_help_name_c40[4];
extern char player_help_name_c20[4];
extern char player_help_name_c10[4];
extern char player_help_name_b40[4];
extern char player_help_name_b30[4];
extern char player_help_name_a50[4];
extern char player_help_name_a30[4];
extern char player_help_name_a10[4];
extern char DAT_00669ae0[4];
extern wchar_t ui_out_of_memory_text[16];
extern wchar_t hud_text_unbound[4];
extern wchar_t hud_text_quote[2];
extern wchar_t prompt_percent_text[2];
extern wchar_t ip_port_format_string_0066a564[4];
extern wchar_t hud_text_unknown[10];
extern wchar_t ui_format_narrow_string[4];
extern char ui_version_string[16];
extern wchar_t ui_invalid_replacement_text[10];
extern wchar_t fortune_easter_egg_text[10];
extern wchar_t hud_text_no_button_icon[18];
extern wchar_t ticker_field_separator[4];
extern char network_ban_indefinite_marker[4];
extern char DAT_0066b090[8];
extern char s_ground_point_marker[16];
extern char s_secondary_trigger_marker[20];
extern char player_update_log_file_mode_string[4];
extern char ai_marker_name_a[8];
extern char sv_tk_grace_arg_buffer[4];
extern char sv_ban_penalty_arg_buffer[8];
extern char network_ban_file_read_mode_string[4];
extern char network_team_color_name_blue[8];
extern char network_team_color_name_red[4];
extern char network_team_color_names[8];
extern char message_delta_config_mode_string[4];

#define SLICE01_SIZE_CHECK(name, bytes) static_assert(sizeof(name) == (bytes))

/* 0x0064e1fc, 0x10 bytes */
__declspec(align(4)) uint32_t ds3dalg_hrtf_full[4] = {
    0xc241333f, 0x11d21c1b, 0xc000f594, 0xca8ac24f
};
SLICE01_SIZE_CHECK(ds3dalg_hrtf_full, 16);

/* 0x0064e20c, 0x10 bytes */
__declspec(align(4)) uint8_t sound_eax_property_set_guid[16] = {
    0x30, 0xac, 0xef, 0x31, 0x5c, 0x51, 0xd0, 0x11, 0xa9, 0xaa, 0x00, 0xaa, 0x00, 0x61, 0xbe, 0x93
};
SLICE01_SIZE_CHECK(sound_eax_property_set_guid, 16);

/* 0x0064e21c, 0x10 bytes */
__declspec(align(4)) uint8_t iid_directsound_3d_buffer[16] = {
    0x86, 0xfa, 0x9a, 0x27, 0x81, 0x49, 0xce, 0x11, 0xa5, 0x21, 0x00, 0x20, 0xaf, 0x0b, 0xe5, 0x60
};
SLICE01_SIZE_CHECK(iid_directsound_3d_buffer, 16);

/* 0x0064e22c, 0x10 bytes */
__declspec(align(4)) uint8_t iid_directsound_3d_listener[16] = {
    0x84, 0xfa, 0x9a, 0x27, 0x81, 0x49, 0xce, 0x11, 0xa5, 0x21, 0x00, 0x20, 0xaf, 0x0b, 0xe5, 0x60
};
SLICE01_SIZE_CHECK(iid_directsound_3d_listener, 16);

/* 0x0064e23c, 0x10 bytes */
__declspec(align(4)) uint32_t dsdevid_default_playback[4] = {
    0xdef00000, 0x47ed9c6d, 0xda4df1aa, 0x035c2b8f
};
SLICE01_SIZE_CHECK(dsdevid_default_playback, 16);

/* 0x0064e24c, 0x10 bytes */
__declspec(align(4)) uint32_t guid_sys_keyboard[4] = {
    0x6f1d2b61, 0x11cfd5a0, 0x4544c7bf, 0x00005453
};
SLICE01_SIZE_CHECK(guid_sys_keyboard, 16);

/* 0x0064e25c, 0x10 bytes */
__declspec(align(4)) uint32_t guid_sys_mouse[4] = {
    0x6f1d2b60, 0x11cfd5a0, 0x4544c7bf, 0x00005453
};
SLICE01_SIZE_CHECK(guid_sys_mouse, 16);

/* 0x0064e2ac, 0x10 bytes */
__declspec(align(4)) uint32_t iid_directinput8a[4] = {
    0xbf798030, 0x4da2483a, 0x645d99aa, 0x009736ed
};
SLICE01_SIZE_CHECK(iid_directinput8a, 16);

/* 0x0064e2bc, 0x10 bytes */
__declspec(align(4)) uint32_t iid_direct_draw7[4] = {
    0x15e65ec0, 0x11d23b9c, 0x60002fb9, 0x5bea9797
};
SLICE01_SIZE_CHECK(iid_direct_draw7, 16);

/* 0x0064e2d0, 0x10 bytes */
__declspec(align(4)) uint8_t sound_eax_listener_property_guid[16] = {
    0xc1, 0x6f, 0x4e, 0x4a, 0x41, 0xc3, 0xd1, 0x11, 0xb7, 0x3a, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00
};
SLICE01_SIZE_CHECK(sound_eax_listener_property_guid, 16);

/* 0x0064e2f0, 0x10 bytes */
__declspec(align(4)) uint8_t sound_eax20_listener_property_guid[16] = {
    0xa8, 0xa6, 0x06, 0x03, 0x24, 0xb2, 0xd2, 0x11, 0x99, 0xe5, 0x00, 0x00, 0xe8, 0xd8, 0xc7, 0x22
};
SLICE01_SIZE_CHECK(sound_eax20_listener_property_guid, 16);

/* 0x0064e300, 0x10 bytes */
__declspec(align(4)) uint8_t sound_eax20_buffer_property_guid[16] = {
    0xa7, 0xa6, 0x06, 0x03, 0x24, 0xb2, 0xd2, 0x11, 0x99, 0xe5, 0x00, 0x00, 0xe8, 0xd8, 0xc7, 0x22
};
SLICE01_SIZE_CHECK(sound_eax20_buffer_property_guid, 16);

/* 0x0064e310, 0x10 bytes */
__declspec(align(4)) uint8_t sound_eax30_listener_property_guid[16] = {
    0x82, 0x68, 0xfa, 0xa8, 0x76, 0xb4, 0xd3, 0x11, 0xbd, 0xb9, 0x00, 0xc0, 0xf0, 0x2d, 0xdf, 0x87
};
SLICE01_SIZE_CHECK(sound_eax30_listener_property_guid, 16);

/* 0x0064e320, 0x10 bytes */
__declspec(align(4)) uint8_t sound_eax30_buffer_property_guid[16] = {
    0x81, 0x68, 0xfa, 0xa8, 0x76, 0xb4, 0xd3, 0x11, 0xbd, 0xb9, 0x00, 0xc0, 0xf0, 0x2d, 0xdf, 0x87
};
SLICE01_SIZE_CHECK(sound_eax30_buffer_property_guid, 16);

/* 0x0064e4dc, 0x8 bytes */
char md5_hex_byte_format[8] = "%02x";
SLICE01_SIZE_CHECK(md5_hex_byte_format, 8);

/* 0x0064ef90, 0x8 bytes */
__declspec(align(4)) void * std_exception_vtable[2] = {
    nullptr, (void *)0x627e32
};
SLICE01_SIZE_CHECK(std_exception_vtable, 8);

/* 0x00655080, 0x8 bytes */
__declspec(align(4)) void * logic_error_vtable[2] = {
    (void *)&halo::shell::hwreq_parse_exception_scalar_deleting_destruct, (void *)&halo::shell::std_exception_what
};
SLICE01_SIZE_CHECK(logic_error_vtable, 8);

/* 0x0065508c, 0x8 bytes */
__declspec(align(4)) void * length_error_vtable[2] = {
    (void *)&halo::shell::hwreq_length_error_scalar_deleting_destruct, (void *)&halo::shell::std_exception_what
};
SLICE01_SIZE_CHECK(length_error_vtable, 8);

/* 0x00655098, 0x8 bytes */
__declspec(align(4)) void * out_of_range_vtable[2] = {
    (void *)&halo::shell::hwreq_out_of_range_scalar_deleting_destruct, (void *)&halo::shell::std_exception_what
};
SLICE01_SIZE_CHECK(out_of_range_vtable, 8);

/* 0x006550a0, 0x18 bytes */
char string_invalid_string_position[24] = "invalid string position";
SLICE01_SIZE_CHECK(string_invalid_string_position, 24);

/* 0x006550b8, 0x10 bytes */
char string_string_too_long[16] = "string too long";
SLICE01_SIZE_CHECK(string_string_too_long, 16);

/* 0x0065512c, 0x4 bytes */
char k_empty_string[4] = "";
SLICE01_SIZE_CHECK(k_empty_string, 4);

/* ---- ai_tables: 0x655254.. in address order, contiguous ---- */
/* 0x00655254, cluster item, 0x24 bytes */
__declspec(allocate(".gdat01")) __declspec(align(4)) uint32_t actor_mode_definitions[9] = {
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000
};
SLICE01_SIZE_CHECK(actor_mode_definitions, 36);

/* 0x00655278, cluster item, 0x2e0 bytes */
__declspec(allocate(".gdat01")) __declspec(align(4)) uint32_t ai_actor_mode_dispatch_table[184] = {
    0, 0, 1, (uint32_t)"sleep",
    (uint32_t)&hud_text_message_hold_color, 0, 0, 0,
    0, 0, (uint32_t)&halo::ai::actor_mode_sleep_update, 0,
    0, 0, 0, 0,
    2, (uint32_t)"alert", (uint32_t)(hud_text_message_hold_color + 0x4), 0x0000005c,
    0, (uint32_t)&halo::cseries::function_do_nothing, (uint32_t)&halo::ai::actor_mode_alert_process, (uint32_t)&halo::ai::actor_mode_alert_tick,
    (uint32_t)&halo::ai::actor_mode_alert_update, 0, 0, 0,
    (uint32_t)&halo::ai::actor_mode_alert_movement_cancelled, (uint32_t)&halo::ai::actor_mode_alert_target_cleared, 3, (uint32_t)"fight",
    (uint32_t)&global_white_argb, 4, 4, (uint32_t)&halo::cseries::function_do_nothing,
    (uint32_t)&halo::ai::actor_update_movement_destination, (uint32_t)&halo::ai::actor_mode_fight_tick, (uint32_t)&halo::ai::actor_mode_fight_update, (uint32_t)&halo::cseries::function_do_nothing,
    0, 0, 0, 0,
    4, (uint32_t)"flee", (uint32_t)&hud_text_message_normal_color, 0x00000030,
    2, (uint32_t)&halo::ai::actor_mode_flee_enter, (uint32_t)&halo::ai::actor_mode_flee_process, (uint32_t)&halo::ai::actor_mode_flee_tick,
    (uint32_t)&halo::ai::actor_mode_flee_update, (uint32_t)&halo::ai::actor_mode_flee_exit, (uint32_t)&halo::ai::actor_mode_flee_get_look_weights, (uint32_t)&halo::ai::actor_mode_flee_replace_reference,
    (uint32_t)&halo::ai::actor_mode_flee_movement_cancelled, 0, 5, (uint32_t)"uncover",
    (uint32_t)&actor_mode_uncover_look_weights_active, 0x00000034, 3, (uint32_t)&halo::ai::actor_mode_uncover_enter,
    (uint32_t)&halo::ai::actor_request_path_with_grenade_arc, (uint32_t)&halo::ai::actor_mode_uncover_tick, (uint32_t)&halo::ai::actor_mode_uncover_update, 0,
    (uint32_t)&halo::ai::actor_mode_uncover_get_look_weights, 0, (uint32_t)&halo::ai::actor_mode_uncover_movement_cancelled, 0,
    6, (uint32_t)"guard", (uint32_t)&actor_mode_guard_look_weights_idle, 0x00000044,
    1, (uint32_t)&halo::ai::actor_mode_guard_enter, (uint32_t)&halo::ai::actor_request_move_and_face, (uint32_t)&halo::ai::actor_mode_guard_tick,
    (uint32_t)&halo::ai::actor_mode_guard_update, (uint32_t)&halo::ai::actor_mode_guard_exit, (uint32_t)&halo::ai::actor_mode_guard_get_look_weights, (uint32_t)&halo::ai::actor_mode_guard_replace_reference,
    (uint32_t)&halo::ai::actor_mode_guard_movement_cancelled, (uint32_t)&halo::ai::actor_mode_guard_target_cleared, 7, (uint32_t)"search",
    (uint32_t)(actor_mode_guard_look_weights_idle + 0x4), 0x0000002c, 3, (uint32_t)&halo::ai::actor_mode_search_enter,
    (uint32_t)&halo::ai::actor_mode_search_process, (uint32_t)&halo::ai::actor_mode_search_tick, (uint32_t)&halo::ai::actor_mode_search_update, 0,
    0, 0, (uint32_t)&halo::ai::actor_mode_search_movement_cancelled, 0,
    8, (uint32_t)"wait", (uint32_t)(actor_mode_guard_look_weights_idle + 0x8), 0x00000018,
    3, (uint32_t)&halo::cseries::function_do_nothing, (uint32_t)&halo::ai::actor_mode_wait_process, (uint32_t)&halo::ai::actor_mode_wait_tick,
    (uint32_t)&halo::ai::actor_mode_wait_update, 0, 0, 0,
    0, 0, 9, (uint32_t)"vehicle",
    (uint32_t)&console_color_00685214, 0x0000004c, 2, (uint32_t)&halo::ai::actor_mode_vehicle_enter,
    (uint32_t)&halo::ai::actor_investigate_disturbance_update, (uint32_t)&halo::cseries::function_do_nothing, (uint32_t)&halo::ai::actor_mode_vehicle_update, 0,
    0, 0, 0, 0,
    0x0000000a, (uint32_t)"charge", (uint32_t)&console_message_default_color, 0x00000038,
    4, (uint32_t)&halo::ai::actor_mode_charge_enter, (uint32_t)&halo::ai::actor_mode_charge_process, (uint32_t)&halo::ai::actor_mode_charge_tick,
    (uint32_t)&halo::ai::actor_mode_charge_update, 0, 0, 0,
    0, 0, 0x0000000b, (uint32_t)"obey",
    (uint32_t)&actor_mode_guard_look_weights_ambush, 0x00000084, 2, (uint32_t)&halo::ai::actor_mode_obey_enter,
    (uint32_t)&halo::ai::actor_mode_obey_process, (uint32_t)&halo::ai::actor_mode_obey_tick_members, (uint32_t)&halo::ai::actor_mode_obey_update, (uint32_t)&halo::ai::actor_mode_obey_exit,
    0, 0, 0, 0,
    0x0000000c, (uint32_t)"converse", (uint32_t)(actor_mode_guard_look_weights_ambush + 0x4), 0x00000014,
    2, (uint32_t)&halo::cseries::function_do_nothing, (uint32_t)&halo::ai::actor_mode_converse_process, (uint32_t)&halo::cseries::function_do_nothing,
    (uint32_t)&halo::ai::actor_mode_converse_update, (uint32_t)&halo::ai::actor_mode_converse_exit, 0, (uint32_t)&halo::ai::actor_mode_converse_replace_reference,
    0, 0, 0x0000000d, (uint32_t)"avoid",
    (uint32_t)(actor_mode_guard_look_weights_ambush + 0x8), 4, 2, (uint32_t)&halo::cseries::function_do_nothing,
    (uint32_t)&halo::ai::actor_update_path_if_needed, (uint32_t)&halo::cseries::function_do_nothing, (uint32_t)&halo::ai::actor_mode_avoid_update, (uint32_t)&halo::cseries::function_do_nothing,
    0, 0, 0, 0
};
SLICE01_SIZE_CHECK(ai_actor_mode_dispatch_table, 736);

/* 0x00655558, cluster item, 0x38 bytes */
__declspec(allocate(".gdat01")) __declspec(align(4)) uint32_t actor_dodge_table[14] = {
    0x0000000a, 0x3fc00000, 0x00000006, 0x00000000, 0x0001000b, 0x3fc00000,
    0x00010007, 0x00000000, 0x00020008, 0x3fc00000, 0x00030009, 0x3fc00000,
    0xffffffff, 0x00000000
};
SLICE01_SIZE_CHECK(actor_dodge_table, 56);

/* 0x00655590, cluster item, 0x18 bytes */
__declspec(allocate(".gdat01")) int16_t order_code_mode_data_expect[12] = {
    0, 0, 0, 1, 2, 3, 4, 5, 0, 0, 0, 0
};
SLICE01_SIZE_CHECK(order_code_mode_data_expect, 24);

/* 0x006555a8, cluster item, 0x18 bytes */
__declspec(allocate(".gdat01")) int16_t actor_lookup_table_006555a8[12] = {
    0, 2, 2, 3, 4, 5, 6, 7, 8, 9, 9, 8
};
SLICE01_SIZE_CHECK(actor_lookup_table_006555a8, 24);

/* 0x006555c0, cluster item, 0x38 bytes */
__declspec(allocate(".gdat01")) __declspec(align(4)) uint32_t actor_firing_position_score_rules[14] = {
    0x0000ffff, (uint32_t)&halo::ai::actor_score_firing_positions_by_threat, 9, (uint32_t)&halo::ai::actor_score_firing_positions_by_range,
    0x0000004d, (uint32_t)&halo::ai::actor_score_firing_positions_by_history, 0x00000010, (uint32_t)&halo::ai::actor_score_firing_positions_close_range,
    2, (uint32_t)&halo::ai::actor_score_firing_positions_by_standoff, 0x00000020, (uint32_t)&halo::ai::actor_score_firing_positions_near_target,
    0, 0
};
SLICE01_SIZE_CHECK(actor_firing_position_score_rules, 56);

/* 0x006555f8, cluster item, 0x30 bytes */
__declspec(allocate(".gdat01")) __declspec(align(4)) uint32_t actor_firing_position_reject_rules[12] = {
    0x0000ffff, (uint32_t)&halo::ai::actor_reject_firing_position_unreachable, 0x00000051, (uint32_t)&halo::ai::actor_reject_firing_position_by_request_result,
    8, (uint32_t)&halo::ai::actor_reject_firing_position_by_target_approach, 6, (uint32_t)&halo::ai::actor_reject_firing_position_by_perception,
    0x00000020, (uint32_t)&halo::ai::actor_reject_firing_position_by_pursuit, 0, 0
};
SLICE01_SIZE_CHECK(actor_firing_position_reject_rules, 48);

/* 0x00655628, cluster item, 0x4 bytes */
__declspec(allocate(".gdat01")) int16_t actor_vocalization_variant[2] = {
    1, 1
};
SLICE01_SIZE_CHECK(actor_vocalization_variant, 4);

/* 0x0065562c, cluster item, 0x8 bytes */
__declspec(allocate(".gdat01")) int16_t actor_dialogue_variant_table_g[4] = {
    2, 2, 2, 2
};
SLICE01_SIZE_CHECK(actor_dialogue_variant_table_g, 8);

/* 0x00655634, cluster item, 0x4 bytes */
__declspec(allocate(".gdat01")) int16_t actor_dialogue_variant_table_d[2] = {
    3, 3
};
SLICE01_SIZE_CHECK(actor_dialogue_variant_table_d, 4);

/* 0x00655638, cluster item, 0x4 bytes */
__declspec(allocate(".gdat01")) int16_t actor_dialogue_variant_table_a[2] = {
    5, 3
};
SLICE01_SIZE_CHECK(actor_dialogue_variant_table_a, 4);

/* 0x0065563c, cluster item, 0x4 bytes */
__declspec(allocate(".gdat01")) int16_t actor_dialogue_variant_table_c[2] = {
    4, 4
};
SLICE01_SIZE_CHECK(actor_dialogue_variant_table_c, 4);

/* 0x00655640, cluster item, 0x4 bytes */
__declspec(allocate(".gdat01")) int16_t actor_dialogue_variant_table_e[2] = {
    5, 4
};
SLICE01_SIZE_CHECK(actor_dialogue_variant_table_e, 4);

/* 0x00655644, cluster item, 0x10 bytes */
__declspec(allocate(".gdat01")) int16_t actor_dialogue_variant_table_f[8] = {
    5, 4, 4, 3, 5, 5, 6, 3
};
SLICE01_SIZE_CHECK(actor_dialogue_variant_table_f, 16);

/* 0x00655654, cluster item, 0xc bytes */
__declspec(allocate(".gdat01")) int16_t actor_dialogue_variant_table_b[6] = {
    6, 3, 7, 5, 7, 2
};
SLICE01_SIZE_CHECK(actor_dialogue_variant_table_b, 12);

/* 0x00655660, cluster item, 0x44 bytes */
__declspec(allocate(".gdat01")) __declspec(align(4)) float actor_vocalization_duration[17] = {
    0.0f, 1.29999995f, 0.899999976f, 0.899999976f, 0.899999976f, 0.699999988f,
    0.899999976f, 0.899999976f, 1.20000005f, 2.0f, 2.0f, 2.5f,
    1.5f, 1000.0f, 1.0f, 0.699999988f, 0.052359879f
};
SLICE01_SIZE_CHECK(actor_vocalization_duration, 68);

/* 0x006556a4, cluster item, 0x24 bytes */
__declspec(allocate(".gdat01")) __declspec(align(4)) float actor_avoidance_b_radius[9] = {
    0.0f, 0.5f, 0.5f, 0.5f, 0.5f, 1.0f,
    1.0f, 1.0f, 1.0f
};
SLICE01_SIZE_CHECK(actor_avoidance_b_radius, 36);

/* 0x006556c8, cluster item, 0x24 bytes */
__declspec(allocate(".gdat01")) __declspec(align(4)) float actor_avoidance_b_elevation[9] = {
    0.0f, 0.300000012f, 0.300000012f, 0.300000012f, 0.300000012f, 1.0f,
    1.0f, 1.0f, 1.0f
};
SLICE01_SIZE_CHECK(actor_avoidance_b_elevation, 36);

/* 0x006556ec, cluster item, 0x28 bytes */
__declspec(allocate(".gdat01")) __declspec(align(4)) float actor_avoidance_b_bearing[10] = {
    0.0f, 0.0f, 1.57079637f, 3.14159274f, 4.71238899f, 0.0f,
    1.57079637f, 3.14159274f, 4.71238899f, 0.699999988f
};
SLICE01_SIZE_CHECK(actor_avoidance_b_bearing, 40);

/* 0x00655714, cluster item, 0x20 bytes */
__declspec(allocate(".gdat01")) __declspec(align(4)) float actor_avoidance_a_bearing[8] = {
    0.0f, 0.785398185f, 1.57079637f, 2.3561945f, 3.14159274f, 3.92699099f,
    4.71238899f, 5.49778748f
};
SLICE01_SIZE_CHECK(actor_avoidance_a_bearing, 32);

/* 0x00655734, cluster item, 0x8 bytes */
__declspec(allocate(".gdat01")) __declspec(align(4)) float actor_avoidance_a_radius[2] = {
    0.699999988f, 1.0f
};
SLICE01_SIZE_CHECK(actor_avoidance_a_radius, 8);

/* 0x0065573c, cluster item, 0xc bytes */
__declspec(allocate(".gdat01")) __declspec(align(4)) float actor_avoidance_a_elevation[3] = {
    0.52359879f, 0.959931076f, 0.0f
};
SLICE01_SIZE_CHECK(actor_avoidance_a_elevation, 12);

/* 0x00655748, cluster item, 0x120 bytes */
__declspec(allocate(".gdat01")) __declspec(align(4)) float actor_avoidance_near_weights[72] = {
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f, -0.5f, -0.5f,
    0.0f, 0.699999988f, 1.0f, 0.699999988f, 0.0f, -0.5f,
    0.0f, -0.5f, -0.5f, -0.5f, 0.0f, 0.699999988f,
    1.0f, 0.699999988f, 1.0f, 0.699999988f, 0.0f, -0.5f,
    -0.5f, -0.5f, 0.0f, 0.699999988f, 0.0f, 0.699999988f,
    1.0f, 0.699999988f, 0.0f, -0.5f, -0.5f, -0.5f
};
SLICE01_SIZE_CHECK(actor_avoidance_near_weights, 288);

/* 0x00655868, cluster item, 0x18 bytes */
__declspec(allocate(".gdat01")) __declspec(align(4)) uint32_t actor_avoidance_ray_weights[6] = {
    0x3f4ccccd, 0x3f99999a, 0x0000004b, 0x3f4ccccd, 0x3f000000, 0x3f000000
};
SLICE01_SIZE_CHECK(actor_avoidance_ray_weights, 24);

/* 0x00655880, cluster item, 0x38 bytes */
__declspec(allocate(".gdat01")) int16_t actor_combat_status_min_grade[28] = {
    0, 0, 0, 1, 2, 2, 3, 4, 5, 5, 7, 7,
    0, 0, 1, 3, 0, 1, 2, 3, 0, 2, 3, 4,
    0, 3, 4, 4
};
SLICE01_SIZE_CHECK(actor_combat_status_min_grade, 56);

/* 0x006558b8, cluster item, 0xc bytes */
__declspec(allocate(".gdat01")) __declspec(align(4)) uint8_t actor_control_animation_state_table[12] = {
    0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x05, 0x00, 0x06, 0x00, 0x00, 0x00
};
SLICE01_SIZE_CHECK(actor_control_animation_state_table, 12);

/* 0x006558c4, cluster item, 0x10 bytes */
__declspec(allocate(".gdat01")) int16_t ai_communication_class_priority[8] = {
    0, 3, 3, 3, 4, 5, 5, 8
};
SLICE01_SIZE_CHECK(ai_communication_class_priority, 16);

/* 0x006558d4, cluster item, 0x20 bytes */
__declspec(allocate(".gdat01")) __declspec(align(4)) float ai_communication_class_tail_seconds[8] = {
    0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f,
    0.5f, 0.300000012f
};
SLICE01_SIZE_CHECK(ai_communication_class_tail_seconds, 32);

/* 0x006558f4, cluster item, 0x10 bytes */
__declspec(allocate(".gdat01")) int16_t ai_communication_class_follow_up[8] = {
    0, 4, 4, 5, 6, 6, 6, 4
};
SLICE01_SIZE_CHECK(ai_communication_class_follow_up, 16);

/* 0x00655904, cluster item, 0x10 bytes */
__declspec(allocate(".gdat01")) int16_t ai_communication_class_look_marker[8] = {
    0, 3, 3, 4, 5, 5, 6, 4
};
SLICE01_SIZE_CHECK(ai_communication_class_look_marker, 16);

/* 0x00655914, cluster item, 0x1c bytes */
__declspec(allocate(".gdat01")) int16_t ai_communication_class_no_actor_class[14] = {
    0, 2, 3, 4, 4, 6, 6, 7, 60, 0, 0, 0,
    0, 0
};
SLICE01_SIZE_CHECK(ai_communication_class_no_actor_class, 28);

/* 0x00655930, cluster item, 0x20 bytes */
__declspec(allocate(".gdat01")) __declspec(align(4)) float ai_communication_class_repeat_delay[8] = {
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f
};
SLICE01_SIZE_CHECK(ai_communication_class_repeat_delay, 32);

/* 0x00655950, cluster item, 0x118 bytes */
__declspec(allocate(".gdat01")) __declspec(align(4)) float ai_communication_direction_table[70] = {
    4.0f, 4.5f, 2.5f, 2.0f, 0.0f, 5.0f,
    5.0f, 3.0f, 2.5f, 0.0f, 3.0f, 3.5f,
    2.0f, 1.5f, 0.0f, 4.5f, 4.5f, 2.5f,
    2.0f, 0.0f, 0.300000012f, 1.0f, 1.0f, 1.0f,
    0.800000012f, 1.0f, 2.0f, 1.5f, 1.5f, 0.300000012f,
    0.0f, 0.5f, 0.5f, 0.5f, 1.29999995f, 0.200000003f,
    1.0f, 1.0f, 1.0f, 0.800000012f, 0.0f, 0.0f,
    1.0f, 1.0f, 1.5f, 0.0f, 0.0f, 1.5f,
    1.5f, 1.0f, 0.0f, 0.0f, 0.5f, 0.0f,
    1.5f, 0.0f, 0.0f, 0.5f, 0.0f, 1.5f,
    0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.5f, 0.0f, 0.0f
};
SLICE01_SIZE_CHECK(ai_communication_direction_table, 280);

/* 0x00655a68, cluster item, 0x38 bytes */
__declspec(allocate(".gdat01")) __declspec(align(4)) uint32_t ai_communication_selector_delay_seconds[14] = {
    0x00000000, 0x3f000000, 0x3f4ccccd, 0x3f000000, 0x3f4ccccd, 0x0000001e,
    0x0000002d, 0x0000001e, 0x00000384, 0x41f00000, 0x40400000, 0x41700000,
    0x3f3504f3, 0x40000000
};
SLICE01_SIZE_CHECK(ai_communication_selector_delay_seconds, 56);

/* 0x00655aa0, cluster item, 0x14 bytes */
__declspec(allocate(".gdat01")) __declspec(align(4)) uint8_t ai_communication_lines[20] = {
    0x00, 0x00, 0x03, 0x00, 0x31, 0x00, 0xff, 0xff, 0x01, 0x00, 0x06, 0x00, 0x02, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x20, 0x41
};
SLICE01_SIZE_CHECK(ai_communication_lines, 20);

/* 0x00655ab4, cluster item, 0x1054 bytes */
__declspec(allocate(".gdat01")) __declspec(align(4)) uint32_t DAT_00655ab4[1045] = {
    0x00000000, 0xffff0000, 0xffff0002, 0xffffffff, 0x0000ffff, 0x00060000,
    0xffff0033, 0x00060001, 0x00060003, 0x41a00000, 0x00000000, 0xffff0042,
    0xffff0002, 0xffff0001, 0x0000ffff, 0x00020000, 0xffff0035, 0x00010001,
    0x00010002, 0x41200000, 0x00000000, 0xffff0008, 0xffff0003, 0xffffffff,
    0x0000ffff, 0x00060000, 0xffff0037, 0x00060001, 0x00060003, 0x41a00000,
    0x00000000, 0xffff004a, 0xffff0003, 0xffff0001, 0x0000ffff, 0x00020000,
    0xffff0039, 0x00010001, 0x00010002, 0x41200000, 0x00000000, 0xffff0008,
    0xffff0003, 0xffff0004, 0x0000ffff, 0x00020000, 0xffff003b, 0x00010001,
    0x00010002, 0x42200000, 0x00000000, 0xffff0008, 0xffff0003, 0xffff0008,
    0x0000ffff, 0x00020000, 0xffff003d, 0x00010001, 0x00010002, 0x42200000,
    0x00000000, 0xffff0008, 0xffff0003, 0xffff0010, 0x0000ffff, 0x00020000,
    0xffff003f, 0x00010001, 0x00010002, 0x42200000, 0x00000000, 0xffff0008,
    0xffff0003, 0xffff0040, 0x0000ffff, 0x00020000, 0xffff0041, 0x00010001,
    0x00010002, 0x41200000, 0x41200000, 0xffff0000, 0xffff0003, 0xffffffff,
    0x00000002, 0x00020000, 0xffff0043, 0x00010001, 0x00010002, 0x41200000,
    0x41200000, 0xffff0000, 0xffff0003, 0xffffffff, 0x0000000a, 0x00020000,
    0xffff0042, 0x00010001, 0x00010002, 0x41200000, 0x00000000, 0xffff0000,
    0xffff0003, 0xffffffff, 0x0000000b, 0x00030000, 0xffff0044, 0x00010001,
    0x00010002, 0x41a00000, 0x00000000, 0xffff0000, 0xffff0003, 0xffffffff,
    0x00000005, 0x00030000, 0xffff0045, 0x00010001, 0x00010002, 0x41a00000,
    0x00000000, 0xffff0000, 0xffff0003, 0xffffffff, 0x00000003, 0x00030000,
    0xffff0046, 0x00010001, 0x00010002, 0x41a00000, 0x00000000, 0xffff0000,
    0xffff0003, 0xffffffff, 0x00000004, 0x00030000, 0xffff0047, 0x00010001,
    0x00010002, 0x41a00000, 0x00000000, 0xffff0000, 0xffff0003, 0xffffffff,
    0x00000006, 0x00020000, 0xffff0048, 0x00010001, 0x00010002, 0x41a00000,
    0x00000000, 0xffff0000, 0xffff0003, 0xffffffff, 0x00000007, 0x00020000,
    0xffff0049, 0x00010001, 0x00010002, 0x41a00000, 0x00000000, 0xffff0000,
    0xffff0003, 0xffffffff, 0x0000000c, 0x00030000, 0xffff004a, 0x00010001,
    0x00010002, 0x41f00000, 0x00000000, 0xffff0000, 0xffff0003, 0xffffffff,
    0x00000009, 0x00020000, 0xffff004b, 0x00010001, 0x00010002, 0x41f00000,
    0x41200000, 0xffff0000, 0xffff0003, 0xffffffff, 0x00000008, 0x00020000,
    0xffff0060, 0x00010002, 0x00010003, 0x41200000, 0x00000000, 0xffff0000,
    0xffffffff, 0xffffffff, 0x0000ffff, 0x00060000, 0xffff0061, 0x00060002,
    0x00060003, 0x41700000, 0x00000000, 0xffff0042, 0xffffffff, 0xffff0001,
    0x0000ffff, 0x00030000, 0xffff0062, 0x00050002, 0x00010001, 0x41200000,
    0x00000000, 0xffff0000, 0xffff0002, 0xffffffff, 0x0000ffff, 0x00040000,
    0xffff0063, 0x00060002, 0x00050003, 0x41a00000, 0x00000000, 0xffff0042,
    0xffff0002, 0x0001ffff, 0x0000ffff, 0x00020000, 0xffff0064, 0x00010002,
    0x00010003, 0x41200000, 0x00000000, 0xffff0000, 0xffff0003, 0xffffffff,
    0x0000ffff, 0x00020000, 0xffff0065, 0x00010002, 0x00010003, 0x41200000,
    0x00000000, 0xffff0000, 0xffff0003, 0x0001ffff, 0x0000ffff, 0x00020000,
    0xffff0066, 0x00010002, 0x00010003, 0x41200000, 0x00000000, 0xffff0000,
    0xffff0003, 0x0004ffff, 0x0000ffff, 0x00020000, 0xffff0067, 0x00010002,
    0x00010003, 0x42200000, 0x00000000, 0xffff0000, 0xffff0003, 0x0038ffff,
    0x0000ffff, 0x00020000, 0xffff0068, 0x00010002, 0x00010003, 0x42200000,
    0x00000000, 0xffff0000, 0xffff0003, 0x0040ffff, 0x0000ffff, 0x00050000,
    0xffff0069, 0x00060002, 0x00060003, 0x41f00000, 0x00000000, 0xffff0002,
    0xffff0004, 0xffffffff, 0x0000ffff, 0x00040001, 0xffff004c, 0x00010000,
    0x00010002, 0x41f00000, 0x00000000, 0xffff0008, 0xffffffff, 0xffffffff,
    0x0000ffff, 0x00030003, 0xffff0015, 0x00010001, 0x00010002, 0x41200000,
    0x00000000, 0xffff0000, 0xffff0002, 0xffffffff, 0x0000ffff, 0x00030003,
    0xffff0016, 0x00010001, 0x00010002, 0x41200000, 0x00000000, 0xffff0002,
    0xffff0002, 0xffff0001, 0x0000ffff, 0x00030003, 0xffff001d, 0x00050000,
    0x00010003, 0x41200000, 0x00000000, 0xffff0008, 0xffff0002, 0xffffffff,
    0x0000ffff, 0x00030003, 0xffff001f, 0x00060000, 0x00010003, 0x41200000,
    0x40a00000, 0xffff0000, 0xffff0002, 0x0001ffff, 0x0000ffff, 0x00010002,
    0xffff0017, 0x00010001, 0x00000000, 0x41200000, 0x41200000, 0xffff0000,
    0xffff0003, 0xffffffff, 0x0000ffff, 0x00020002, 0xffff0020, 0x00050000,
    0x00000000, 0x41200000, 0x41200000, 0xffff0000, 0xffff0003, 0xffffffff,
    0x0000ffff, 0x00020002, 0xffff0023, 0x00050000, 0x00000000, 0x41200000,
    0x41200000, 0xffff0000, 0xffff0003, 0xffffffff, 0x00000002, 0x00020002,
    0xffff0024, 0x00050000, 0x00000000, 0x41200000, 0x41200000, 0xffff0000,
    0xffff0003, 0xffffffff, 0x0000000b, 0x00020002, 0xffff0025, 0x00050000,
    0x00000000, 0x41200000, 0x41200000, 0xffff0000, 0xffff0003, 0xffffffff,
    0x0000000a, 0x00030002, 0xffff0026, 0x00050000, 0x00000000, 0x41a00000,
    0x00000000, 0xffff0000, 0xffff0003, 0xffffffff, 0x00000005, 0x00020002,
    0xffff0028, 0x00050000, 0x00000000, 0x41a00000, 0x00000000, 0xffff0000,
    0xffff0003, 0xffffffff, 0x00000004, 0x00030002, 0xffff0029, 0x00050000,
    0x00000000, 0x41a00000, 0x00000000, 0xffff0000, 0xffff0003, 0xffffffff,
    0x00000006, 0x00020002, 0xffff002a, 0x00050000, 0x00000000, 0x41a00000,
    0x00000000, 0xffff0000, 0xffff0003, 0xffffffff, 0x00000007, 0x00020002,
    0xffff002b, 0x00050000, 0x00000000, 0x41a00000, 0x00000000, 0xffff0000,
    0xffff0003, 0xffffffff, 0x0000000c, 0x00030002, 0xffff002c, 0x00050000,
    0x00000000, 0x41f00000, 0x00000000, 0xffff0000, 0xffff0003, 0xffffffff,
    0x00000009, 0x00020002, 0xffff002d, 0x00050000, 0x00000000, 0x41f00000,
    0x00000000, 0xffff0000, 0xffff0003, 0xffffffff, 0x00000008, 0x00050004,
    0x0003006c, 0x00060000, 0x00060003, 0x41200000, 0x41200000, 0xffff0000,
    0x0000ffff, 0xffffffff, 0x0000ffff, 0x00050004, 0x0003006d, 0x00060000,
    0x00060003, 0x41200000, 0x41c80000, 0xffff0000, 0x0002ffff, 0xffffffff,
    0x0000ffff, 0x00050005, 0x0003006e, 0x00060000, 0x00060003, 0x41200000,
    0x41c80000, 0xffff0000, 0x0002ffff, 0xffffffff, 0x0000ffff, 0x00030006,
    0xffff006f, 0x00060000, 0x00040003, 0x41200000, 0x41000000, 0xffff0001,
    0xffffffff, 0xffffffff, 0x0000ffff, 0x00050007, 0xffff0070, 0x00060000,
    0x00060003, 0x41200000, 0x41a00000, 0xffff0000, 0x0004ffff, 0xffffffff,
    0x0000ffff, 0x00060008, 0xffff0071, 0x00060000, 0x00060003, 0x41200000,
    0x00000000, 0xffff0044, 0xffff0004, 0xffffffff, 0x0000ffff, 0x00040008,
    0xffff0072, 0x00060000, 0x00040003, 0x41200000, 0x00000000, 0xffff0040,
    0xffff0002, 0xffffffff, 0x0000ffff, 0x00040019, 0x00030094, 0x00060000,
    0x00050003, 0x41200000, 0x41700000, 0xffff0000, 0x0004ffff, 0xffffffff,
    0x0000ffff, 0x0004000d, 0xffff00a1, 0x00000000, 0x00010002, 0x41200000,
    0x41a00000, 0xffff0000, 0x0003ffff, 0xffffffff, 0x0000ffff, 0x0004000d,
    0xffff007f, 0x00000000, 0x00010002, 0x41700000, 0x41a00000, 0x00000000,
    0x0003ffff, 0xffffffff, 0x0000ffff, 0x0004000f, 0xffff007d, 0x00040000,
    0x00050002, 0x41200000, 0x41a00000, 0xffff0000, 0x0002ffff, 0xffffffff,
    0x0000ffff, 0x0002000e, 0x00010081, 0x00040000, 0x00010002, 0x41200000,
    0x00000000, 0x00010000, 0xffffffff, 0xffffffff, 0x00000000, 0x00030010,
    0xffff0083, 0x00000000, 0x00010002, 0x41200000, 0x41000000, 0x00000000,
    0x0003ffff, 0xffffffff, 0x0000ffff, 0x00030011, 0xffff0084, 0x00050000,
    0x00010002, 0x41200000, 0x41c80000, 0x00000000, 0x0003ffff, 0xffffffff,
    0x0000ffff, 0x00030012, 0xffff0086, 0x00000000, 0x00010002, 0x41200000,
    0x41c80000, 0x00000000, 0x0003ffff, 0xffffffff, 0x0000ffff, 0x00030013,
    0xffff0087, 0x00000000, 0x00010002, 0x41200000, 0x41000000, 0x00000000,
    0x0003ffff, 0xffffffff, 0x0000ffff, 0x00040014, 0xffff0088, 0x00000000,
    0x00010002, 0x41200000, 0x00000000, 0x00000000, 0x0003ffff, 0xffffffff,
    0x0000ffff, 0x00030015, 0x00020089, 0x00000000, 0x00010002, 0x41200000,
    0x41f00000, 0x00010000, 0x0003ffff, 0xffffffff, 0x0000ffff, 0x00020018,
    0xffff008f, 0x00000000, 0x00010002, 0x41200000, 0x41f00000, 0x00010000,
    0xffffffff, 0xffffffff, 0x0000ffff, 0x00040016, 0x0002008b, 0x00000000,
    0x00010002, 0x41200000, 0x00000000, 0xffff0000, 0xffffffff, 0xffffffff,
    0x0000ffff, 0x00040017, 0xffff008d, 0x00000000, 0x00010002, 0x41200000,
    0x00000000, 0xffff0000, 0xffffffff, 0xffffffff, 0x0000ffff, 0x0001001a,
    0xffff0095, 0x00000000, 0x00000000, 0x41200000, 0x41200000, 0xffff0000,
    0xffffffff, 0xffffffff, 0x0000ffff, 0x0001001b, 0xffff0096, 0x00000000,
    0x00000000, 0x41a00000, 0x41200000, 0xffff0000, 0xffffffff, 0xffffffff,
    0x0000ffff, 0x0002001c, 0xffff0097, 0x00000000, 0x00000000, 0x41a00000,
    0x00000000, 0xffff0000, 0xffffffff, 0xffffffff, 0x0000ffff, 0x0002001d,
    0xffff0098, 0x00000000, 0x00000000, 0x41200000, 0x00000000, 0xffff0000,
    0xffffffff, 0xffffffff, 0x0000ffff, 0x0001001e, 0xffff0099, 0x00000000,
    0x00000000, 0x41f00000, 0x41200000, 0xffff0000, 0xffffffff, 0xffffffff,
    0x0000ffff, 0x0003001f, 0xffff009c, 0x00000000, 0x00010002, 0x41200000,
    0x00000000, 0xffff0000, 0xffffffff, 0xffffffff, 0x0000ffff, 0x00040020,
    0xffff009e, 0x00000000, 0x00010002, 0x41200000, 0x00000000, 0xffff0000,
    0xffffffff, 0xffffffff, 0x0000ffff, 0x00030021, 0xffff0002, 0x00000000,
    0x00000000, 0x41200000, 0x00000000, 0xffff0000, 0xffffffff, 0xffffffff,
    0x0000ffff, 0x00020022, 0xffff009f, 0x00040000, 0x00050002, 0x41200000,
    0x41200000, 0x00010000, 0xffffffff, 0xffffffff, 0x0000ffff, 0x00030023,
    0xffff00a2, 0x00000000, 0x00010002, 0x41200000, 0x41f00000, 0x00010000,
    0x0003ffff, 0xffffffff, 0x0000ffff, 0x00030024, 0xffff00a3, 0x00000000,
    0x00000000, 0x41200000, 0x41200000, 0xffff0000, 0xffffffff, 0xffffffff,
    0x0000ffff, 0x00030025, 0xffff00a4, 0x00000000, 0x00000000, 0x41200000,
    0x41200000, 0xffff0000, 0xffffffff, 0xffffffff, 0x0000ffff, 0x00020026,
    0x000c00a5, 0x00000002, 0x00000000, 0x41200000, 0x41c80000, 0xffff0030,
    0xffffffff, 0xffffffff, 0x0000ffff, 0x00020027, 0x000d00a6, 0x00000002,
    0x00000000, 0x41200000, 0x41c80000, 0xffff0030, 0xffffffff, 0xffffffff,
    0x0000ffff, 0x00070028, 0x000d000a, 0x00000002, 0x00000000, 0x41200000,
    0x00000000, 0xffff0030, 0xffffffff, 0xffffffff, 0x0000ffff, 0x0003000b,
    0xffff0075, 0x00060000, 0x00060004, 0x41200000, 0x40800000, 0x00000000,
    0xffff0003, 0xffffffff, 0x0000ffff, 0x0003000a, 0xffff0074, 0x00060000,
    0x00060004, 0x41200000, 0x40800000, 0xffff0000, 0x0004ffff, 0xffffffff,
    0x0000ffff, 0x0005000c, 0xffff0076, 0x00050000, 0x00050004, 0x41200000,
    0x40800000, 0x00000000, 0xffff0003, 0xffffffff, 0x0000ffff, 0x0005000c,
    0xffff0078, 0x00050000, 0x00050004, 0x41200000, 0x00000000, 0x00000000,
    0xffff0002, 0xffffffff, 0x0000ffff, 0x0006000c, 0xffff0077, 0x00060000,
    0x00050004, 0x41200000, 0x00000000, 0xffff0000, 0xffff0001, 0xffffffff,
    0x0000ffff, 0x00070029, 0xffff00b1, 0x00060000, 0x00040002, 0x41200000,
    0x00000000, 0xffff0000, 0xffffffff, 0xffffffff, 0x0000ffff, 0x0007002a,
    0xffff00b2, 0x00040000, 0x00010002, 0x41200000, 0x00000000, 0xffff0000,
    0xffffffff, 0xffffffff, 0x0000ffff, 0x0007002b, 0xffff00b3, 0x00040000,
    0x00010002, 0x41200000, 0x00000000, 0xffff0000, 0xffffffff, 0xffffffff,
    0x0000ffff, 0x00070009, 0xffff0073, 0x00040000, 0x00010002, 0x41200000,
    0x00000000, 0xffff0000, 0xffffffff, 0xffffffff, 0x0000ffff, 0x0007002c,
    0xffff00b4, 0x00040000, 0x00010002, 0x41200000, 0x00000000, 0xffff0000,
    0xffffffff, 0xffffffff, 0x0000ffff, 0x0007002f, 0xffff00b6, 0x00040000,
    0x00010002, 0x41200000, 0x00000000, 0xffff0000, 0xffffffff, 0xffffffff,
    0x0000ffff, 0x0007002e, 0xffff000a, 0x00040000, 0x00010002, 0x41200000,
    0x00000000, 0xffff0000, 0xffffffff, 0xffffffff, 0x0000ffff, 0x00030030,
    0xffff00c5, 0x00050000, 0x00000000, 0x41200000, 0x00000000, 0xffff0000,
    0xffffffff, 0xffffffff, 0x0000ffff, 0x00030031, 0xffff00c6, 0x00000000,
    0x00000000, 0x41200000, 0x00000000, 0xffff0000, 0xffffffff, 0xffffffff,
    0x0000ffff, 0x00030032, 0xffff00c7, 0x00000000, 0x00000000, 0x41200000,
    0x00000000, 0xffff0000, 0xffffffff, 0xffffffff, 0x0000ffff, 0x00030033,
    0xffff00c9, 0x00000000, 0x00000000, 0x41200000, 0x00000000, 0xffff0000,
    0xffffffff, 0xffffffff, 0x0000ffff, 0x00030034, 0xffff00cb, 0x00000000,
    0x00000000, 0x41200000, 0x00000000, 0xffff0000, 0xffffffff, 0xffffffff,
    0x0000ffff, 0x00030035, 0xffff00bd, 0x00040000, 0x00010002, 0x41200000,
    0x00000000, 0xffff0000, 0xffff0003, 0xffffffff, 0x0000ffff, 0x00030036,
    0xffff00be, 0x00040000, 0x00010002, 0x41200000, 0x00000000, 0xffff0000,
    0xffff0002, 0xffffffff, 0x0000ffff, 0x00030037, 0xffff00bf, 0x00040000,
    0x00010002, 0x41200000, 0x00000000, 0xffff0000, 0xffffffff, 0xffffffff,
    0x0000ffff, 0x00060037, 0xffff00c0, 0x00040000, 0x00010002, 0x41200000,
    0x00000000, 0xffff0002, 0xffffffff, 0x0001ffff, 0x0000ffff, 0x00030038,
    0xffff00bc, 0x00040000, 0x00010002, 0x41200000, 0x00000000, 0xffff0000,
    0xffffffff, 0xffffffff, 0x0000ffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0x00000000, 0x00000000, 0xffff0000, 0xffffffff, 0xffffffff,
    0x0000ffff
};
SLICE01_SIZE_CHECK(DAT_00655ab4, 4180);

/* 0x00656b08, cluster item, 0x1c bytes */
__declspec(allocate(".gdat01")) __declspec(align(4)) uint32_t ai_communication_event_definitions[7] = {
    0xffff004c, 0x005c0002, 0x0002ffff, 0x00000000, 0x00000000, 0x3f800000,
    0x3f333333
};
SLICE01_SIZE_CHECK(ai_communication_event_definitions, 28);

/* 0x00656b24, cluster item, 0x670 bytes */
__declspec(allocate(".gdat01")) __declspec(align(4)) uint32_t DAT_00656b24[412] = {
    0x41f00000, (uint32_t)&halo::ai::actor_target_is_close_and_recognized, 0x00020035, 0x00510002,
    0x0002ffff, 0, 0, 0x3f000000,
    0x3f333333, 0x42700000, (uint32_t)&halo::ai::actor_target_is_close_and_recognized, 0x000b0035,
    0x00520002, 0x0002ffff, 0, 0,
    0x3f19999a, 0x3f333333, 0x42700000, (uint32_t)&halo::ai::actor_target_is_close_and_recognized,
    0x000a0035, 0x00530002, 0x0002ffff, 0,
    0, 0x3f19999a, 0x3f333333, 0x42700000,
    (uint32_t)&halo::ai::actor_target_is_close_and_recognized, 0x00050035, 0x00540002, 0x0002ffff,
    0, 0, 0x3f4ccccd, 0x3f333333,
    0x42700000, (uint32_t)&halo::ai::actor_target_is_close_and_recognized, 0x00030035, 0x00550002,
    0x0002ffff, 0, 0x3f000000, 0x3f666666,
    0x3f333333, 0x42700000, (uint32_t)&halo::ai::actor_target_is_close_and_recognized, 0x00040035,
    0x00560002, 0x0002ffff, 0, 0,
    0x3f800000, 0x3f333333, 0x42700000, (uint32_t)&halo::ai::actor_target_is_close_and_recognized,
    0x00060035, 0x00570002, 0x0002ffff, 0,
    0, 0x3f800000, 0x3f333333, 0x42700000,
    (uint32_t)&halo::ai::actor_target_is_close_and_recognized, 0x00070035, 0x00580002, 0x0002ffff,
    0, 0, 0x3f666666, 0x3f333333,
    0x42700000, (uint32_t)&halo::ai::actor_target_is_close_and_recognized, 0x000c0035, 0x00590002,
    0x0002ffff, 0, 0, 0x3f19999a,
    0x3f333333, 0x42700000, (uint32_t)&halo::ai::actor_target_is_close_and_recognized, 0x00090035,
    0x005a0002, 0x0002ffff, 0, 0,
    0x3f4ccccd, 0x3f333333, 0x42700000, (uint32_t)&halo::ai::actor_target_is_close_and_recognized,
    0x00080035, 0x005b0002, 0x0002ffff, 0,
    0, 0x3f800000, 0x3f333333, 0x42700000,
    (uint32_t)&halo::ai::actor_target_is_close_and_recognized, 0xffff0037, 0x00380002, 0x0004ffff,
    1, 0x3f800000, 0x3f800000, 0x3e99999a,
    0, (uint32_t)&halo::ai::actor_target_is_close_and_recognized, 0xffff0039, 0x003a0002,
    0x0002ffff, 0, 0x3f4ccccd, 0x3f19999a,
    0x3f000000, 0x41f00000, (uint32_t)&halo::ai::actor_target_is_close_and_recognized, 0xffff003b,
    0x003c0002, 0x0002ffff, 0, 0x3f4ccccd,
    0x3f19999a, 0x3f000000, 0x41a00000, (uint32_t)&halo::ai::actor_target_is_close_and_recognized,
    0xffff003d, 0x003e0002, 0x0002ffff, 0,
    0x3f4ccccd, 0x3f19999a, 0x3f000000, 0x41a00000,
    (uint32_t)&halo::ai::actor_target_is_close_and_recognized, 0xffff003f, 0x00400002, 0x0002ffff,
    0, 0x3f4ccccd, 0x3f19999a, 0x3f000000,
    0x41a00000, (uint32_t)&halo::ai::actor_target_is_close_and_recognized, 0xffff0035, 0x00360002,
    0x0002ffff, 0, 0x3f19999a, 0x3ecccccd,
    0x3f000000, 0x41f00000, (uint32_t)&halo::ai::actor_target_is_close_and_recognized, 0xffff0035,
    0x00500002, 0x0002ffff, 0, 0,
    0x3ecccccd, 0x3f333333, 0x42200000, (uint32_t)&halo::ai::actor_target_is_close_and_recognized,
    0xffff0017, 0x00180002, 0x0001ffff, 0,
    0x3f4ccccd, 0, 0x3f333333, 0x41a00000,
    0, 0xffff0020, 0x00210004, 0x0001ffff,
    0, 0x3f4ccccd, 0, 0x3f333333,
    0x41a00000, 0, 0xffff0020, 0x00220002,
    0x0001ffff, 0, 0x3f4ccccd, 0,
    0x3f333333, 0x41a00000, 0, 0xffff0033,
    0x00340002, 0x0004ffff, 1, 0x3f800000,
    0, 0x3e99999a, 0, (uint32_t)&halo::ai::actor_target_is_close_and_recognized,
    0xffff0031, 0x00320002, 0x0003ffff, 0,
    0x3f333333, 0, 0x3e99999a, 0x41a00000,
    (uint32_t)&halo::ai::actor_target_is_close_and_recognized, 0xffff001d, 0x001e0003, 0x0002ffff,
    0, 0x3f333333, 0x3ecccccd, 0x3f000000,
    0x41a00000, 0, 0xffff006c, 0x007b0002,
    0x0003ffff, 0, 0x3f4ccccd, 0,
    0x3f333333, 0x41a00000, (uint32_t)&halo::ai::actor_target_is_close_and_recognized, 0xffff006c,
    0x007c0002, 0x0003ffff, 0, 0x3f4ccccd,
    0, 0x3f333333, 0x41a00000, (uint32_t)&halo::ai::ai_dialogue_condition_42f4f0,
    0xffff006d, 0x007b0002, 0x0003ffff, 0,
    0x3f4ccccd, 0, 0x3f333333, 0x41a00000,
    (uint32_t)&halo::ai::actor_target_is_close_and_recognized, 0xffff006d, 0x007c0002, 0x0003ffff,
    0, 0x3f4ccccd, 0, 0x3f333333,
    0x41a00000, (uint32_t)&halo::ai::ai_dialogue_condition_42f4f0, 0xffff006e, 0x007b0002,
    0x0003ffff, 0, 0x3f4ccccd, 0,
    0x3f333333, 0x41a00000, (uint32_t)&halo::ai::actor_target_is_close_and_recognized, 0xffff006e,
    0x007c0002, 0x0003ffff, 0, 0x3f4ccccd,
    0, 0x3f333333, 0x41a00000, (uint32_t)&halo::ai::ai_dialogue_condition_42f4f0,
    0xffff007d, 0x007e0003, 0x0003ffff, 0,
    0x3f333333, 0, 0x3f000000, 0x41700000,
    0, 0xffff007f, 0x00800002, 0x0003ffff,
    0, 0x3f333333, 0, 0x3f000000,
    0x41f00000, (uint32_t)&halo::ai::ai_dialogue_condition_42f7b0, 0xffff0081, 0x00820003,
    0x0003ffff, 0, 0x3f000000, 0,
    0x3f000000, 0x41a00000, 0, 0xffff0084,
    0x00850002, 0x0003ffff, 0, 0x3f800000,
    0, 0x3e99999a, 0x41a00000, (uint32_t)&halo::ai::ai_dialogue_condition_42f560,
    0xffff0089, 0x008a0002, 0x0003ffff, 0,
    0x3f800000, 0, 0x3e99999a, 0x41a00000,
    (uint32_t)&halo::ai::ai_dialogue_condition_42f6f0, 0xffff008b, 0x008c0002, 0x0004ffff,
    0, 0x3f333333, 0, 0x3f333333,
    0x41a00000, (uint32_t)&halo::ai::ai_dialogue_condition_42f5b0, 0xffff008d, 0x008e0002,
    0x0004ffff, 0, 0x3f333333, 0,
    0x3f333333, 0x41a00000, (uint32_t)&halo::ai::ai_dialogue_condition_42f5b0, 0xffff009c,
    0x009d0002, 0x0003ffff, 0, 0x3f000000,
    0, 0x3f333333, 0x41f00000, (uint32_t)&halo::ai::ai_dialogue_condition_42f690,
    0xffff009c, 0x009a0004, 0x0003ffff, 0,
    0x3f000000, 0, 0x3f333333, 0x41f00000,
    (uint32_t)&halo::ai::ai_dialogue_condition_42f650, 0xffff009e, 0x009a0004, 0x0003ffff,
    0, 0x3f000000, 0, 0x3f333333,
    0x41f00000, (uint32_t)&halo::ai::ai_dialogue_condition_42f650, 0xffff009f, 0x00a00002,
    0x0003ffff, 0, 0x3f000000, 0,
    0x3f333333, 0x41a00000, (uint32_t)&halo::ai::ai_dialogue_condition_42f7f0, 0xffff00c7,
    0x00c80002, 0x0003ffff, 0, 0x3f4ccccd,
    0, 0x3f000000, 0, 0,
    0xffff00c9, 0x00ca0002, 0x0003ffff, 0,
    0x3f4ccccd, 0, 0x3f000000, 0,
    0, 0xffff00cb, 0x00cc0002, 0x0003ffff,
    0, 0x3f4ccccd, 0, 0x3f000000,
    0, 0, 0xffffffff, 0xffffffff,
    0xffffffff, 0, 0, 0,
    0, 0, 0, 0x0000002f,
    0xffffffff, 0, 0xffffffff, 0xffffffff
};
SLICE01_SIZE_CHECK(DAT_00656b24, 1648);

/* 0x00657194, cluster item, 0x60 bytes */
__declspec(allocate(".gdat01")) int16_t ai_vocalization_line_table[48] = {
    9, 7, 8, 10, -13107, 15948, -13107, 16076, -26214, 16153, -13107, 16204,
    0, 16256, 0, 16320, 0, 16384, 0, 16512, 0, 16000, 0, 16128,
    0, 16192, 0, 16256, 0, 16320, 0, 16384, 0, 16512, 0, 16640,
    0, 1, 1, 1, 1, 2, 2, 2, 2, 0, 128, 14208
};
SLICE01_SIZE_CHECK(ai_vocalization_line_table, 96);

/* ---- end of cluster ---- */
/* 0x006571f4, 0x12 bytes */
__declspec(align(4)) int8_t bitmap_format_bits_per_pixel[18] = {
    8, 8, 8, 16, 0, 0, 16, 0, 16, 16, 32, 32, 0, 0, 4, 8,
    8, 8
};
SLICE01_SIZE_CHECK(bitmap_format_bits_per_pixel, 18);

/* 0x00657208, 0x6 bytes */
__declspec(align(4)) uint8_t natneg_magic[6] = {
    0xfd, 0xfc, 0x1e, 0x66, 0x6a, 0xb2
};
SLICE01_SIZE_CHECK(natneg_magic, 6);

/* 0x006572c4, 0x14 bytes */
__declspec(align(4)) float observer_channel_acceleration_limit[5] = {
    1500.0f, 1500.0f, 100000.0f, 100000.0f, 100000.0f
};
SLICE01_SIZE_CHECK(observer_channel_acceleration_limit, 20);

/* 0x006573f8, 0x40 bytes */
__declspec(align(4)) uint32_t k_decal_type_parameters[16] = {
    0x42200000, 0x42dc0000, 0x3fc00000, 1,
    0x42200000, 0x42dc0000, 0x3fc00000, 1,
    0x42200000, 0x42dc0000, 0x3fc00000, 1,
    0x41200000, 0x41200000, 0x3fc00000, 0
};
SLICE01_SIZE_CHECK(k_decal_type_parameters, 64);

/* 0x0065743c, 0x8 bytes */
__declspec(align(4)) void * particle_system_update_physics_table[2] = {
    (void *)&halo::effects::particle_system_update_physics_default, (void *)&halo::effects::particle_system_update_physics_explosion
};
SLICE01_SIZE_CHECK(particle_system_update_physics_table, 8);

/* 0x00657444, 0xc bytes */
__declspec(align(4)) void * particle_creation_physics_table[3] = {
    (void *)&halo::effects::particle_creation_physics_default, (void *)&halo::effects::particle_creation_physics_explosion, (void *)&halo::effects::particle_creation_physics_jet
};
SLICE01_SIZE_CHECK(particle_creation_physics_table, 12);

/* 0x00657450, 0x4 bytes */
__declspec(align(4)) void * particle_update_physics_table[1] = {
    (void *)&halo::effects::particle_update_physics_default
};
SLICE01_SIZE_CHECK(particle_update_physics_table, 4);

/* 0x00657470, 0x48 bytes */
int16_t weapon_zoom_index_substitutions[36] = {
    4, 5, 6, 7, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0
};
SLICE01_SIZE_CHECK(weapon_zoom_index_substitutions, 72);

/* 0x006574c0, 0x6c bytes */
__declspec(align(4)) float player_placement_ring[27] = {
    1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, -1.0f, 0.0f, 0.707106769f, -0.707106769f, 0.0f,
    0.707106769f, 0.707106769f, 0.0f, 0.577350259f, 0.577350259f, 0.577350259f,
    0.577350259f, 0.577350259f, -0.577350259f, 0.577350259f, -0.577350259f, 0.577350259f,
    0.577350259f, -0.577350259f, -0.577350259f
};
SLICE01_SIZE_CHECK(player_placement_ring, 108);

/* 0x00657538, 0xc bytes */
int16_t hs_object_type_masks[6] = {
    -1, 3, 2, 4, 896, 64
};
SLICE01_SIZE_CHECK(hs_object_type_masks, 12);

/* 0x00657544, 0x20 bytes */
__declspec(align(4)) uint32_t hs_tag_group_for_type[8] = {
    0x736e6421, 0x65666665, 0x6a707421, 0x6c736e64, 0x616e7472, 0x61637476,
    0x6a707421, 0x6f626a65
};
SLICE01_SIZE_CHECK(hs_tag_group_for_type, 32);

/* 0x00657568, 0x64 bytes */
int16_t hs_type_sizes[50] = {
    0, 0, 0, 0, 0, 1, 4, 2, 4, 4, 4, 2,
    2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 2, 2, 2, 2,
    2, 4, 4, 4, 4, 4, 4, 2, 2, 2, 2, 2,
    2, 0
};
SLICE01_SIZE_CHECK(hs_type_sizes, 100);

/* 0x0065b660, 0x4 bytes */
char hs_space_characters[4] = " \t";
SLICE01_SIZE_CHECK(hs_space_characters, 4);

/* 0x0065b664, 0x4 bytes */
char hs_newline_characters[4] = "\n\r";
SLICE01_SIZE_CHECK(hs_newline_characters, 4);

/* 0x0065b668, 0xc4 bytes */
__declspec(align(4)) void * hs_parse_primitive_procedures[49] = {
    0, 0, 0, 0,
    0, (void *)&halo::hs::hs_parse_boolean, (void *)&halo::hs::hs_parse_real, (void *)&halo::hs::hs_parse_integer,
    (void *)&halo::hs::hs_parse_integer, (void *)&halo::hs::hs_parse_string, (void *)&halo::hs::hs_parse_script, (void *)&halo::hs::hs_parse_trigger_volume,
    (void *)&halo::hs::hs_parse_cutscene_flag, (void *)&halo::hs::hs_parse_cutscene_camera_point, (void *)&halo::hs::hs_parse_cutscene_title, (void *)&halo::hs::hs_parse_cutscene_recording,
    (void *)&halo::hs::hs_parse_device_group, (void *)&halo::hs::hs_parse_ai, (void *)&halo::hs::hs_parse_ai_command_list, (void *)&halo::hs::hs_parse_starting_profile,
    (void *)&halo::hs::hs_parse_conversation, (void *)&halo::hs::hs_parse_navpoint, (void *)&halo::hs::hs_parse_hud_message, (void *)&halo::hs::hs_parse_object_list,
    (void *)&halo::hs::hs_parse_tag_reference, (void *)&halo::hs::hs_parse_tag_reference, (void *)&halo::hs::hs_parse_tag_reference, (void *)&halo::hs::hs_parse_tag_reference,
    (void *)&halo::hs::hs_parse_tag_reference, (void *)&halo::hs::hs_parse_tag_reference, (void *)&halo::hs::hs_parse_tag_reference, (void *)&halo::hs::hs_parse_tag_reference,
    (void *)&halo::hs::hs_report_expected_enum_values, (void *)&halo::hs::hs_report_expected_enum_values, (void *)&halo::hs::hs_report_expected_enum_values, (void *)&halo::hs::hs_report_expected_enum_values,
    (void *)&halo::hs::hs_report_expected_enum_values, (void *)&halo::hs::hs_parse_object, (void *)&halo::hs::hs_parse_object, (void *)&halo::hs::hs_parse_object,
    (void *)&halo::hs::hs_parse_object, (void *)&halo::hs::hs_parse_object, (void *)&halo::hs::hs_parse_object, (void *)&halo::hs::hs_parse_object_name,
    (void *)&halo::hs::hs_parse_object_name, (void *)&halo::hs::hs_parse_object_name, (void *)&halo::hs::hs_parse_object_name, (void *)&halo::hs::hs_parse_object_name,
    (void *)&halo::hs::hs_parse_object_name
};
SLICE01_SIZE_CHECK(hs_parse_primitive_procedures, 196);

/* 0x0065b730, 0x1b0 bytes */
char input_action_names[27][16] = {
    "jump", "switch_grenade", "action", "switch_weapon",
    "melee", "flashlight", "throw_grenade", "fire",
    "accept", "back", "crouch", "zoom",
    "showscores", "reload", "exchange_weapon", "say",
    "sayteam", "sayvehicle", "screenshot", "forward",
    "backward", "left", "right", "look_up",
    "look_down", "look_left", "look_right"
};
SLICE01_SIZE_CHECK(input_action_names, 432);

/* 0x0065b8e0, 0x10 bytes */
__declspec(align(4)) uint32_t input_default_profile_guid[4] = {
    0x23a8e6bc, 0x4ef8ce3b, 0x6b656b81, 0x73d56786
};
SLICE01_SIZE_CHECK(input_default_profile_guid, 16);

/* 0x0065b8f0, 0x18 bytes */
char joystick_button_prefix[24] = "button";
SLICE01_SIZE_CHECK(joystick_button_prefix, 24);

/* 0x0065b908, 0x18 bytes */
char joystick_axis_prefix[24] = "axis";
SLICE01_SIZE_CHECK(joystick_axis_prefix, 24);

/* 0x0065b920, 0x18 bytes */
char joystick_pov_prefix[24] = "pov";
SLICE01_SIZE_CHECK(joystick_pov_prefix, 24);

/* 0x0065b938, 0x50 bytes */
char pov_direction_names[8][10] = {
    "north", "northeast", "east", "southeast",
    "south", "southwest", "west", "northwest"
};
SLICE01_SIZE_CHECK(pov_direction_names, 80);

/* 0x0065b988, 0x60 bytes */
char decimal_suffixes[32][3] = {
    "0", "1", "2", "3",
    "4", "5", "6", "7",
    "8", "9", "10", "11",
    "12", "13", "14", "15",
    "16", "17", "18", "19",
    "20", "21", "22", "23",
    "24", "25", "26", "27",
    "28", "29", "30", "31"
};
SLICE01_SIZE_CHECK(decimal_suffixes, 96);

/* 0x0065ba58, 0x200 bytes */
int16_t virtual_key_to_key[256] = {
    -1, -1, -1, -1, -1, -1, -1, -1, 29, 30, -1, -1,
    -1, 56, -1, -1, 110, 111, 71, 15, 44, -1, -1, -1,
    -1, -1, -1, 0, -1, -1, -1, -1, -1, 83, 86, 85,
    82, 79, 77, 80, 78, -1, -1, -1, -1, 81, 84, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, 70, 74, 75, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, 1, 2, 3, 4, 5, 6, 7, 8,
    9, 10, 11, 12, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    87, 14, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1
};
SLICE01_SIZE_CHECK(virtual_key_to_key, 512);

/* 0x0065bc58, 0x100 bytes */
int16_t character_to_key[128] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, 72, 17, 55, 19,
    20, 21, 23, 55, 25, 26, 24, 28, 65, 27, 66, 67,
    26, 17, 18, 19, 20, 21, 22, 23, 24, 25, 54, 54,
    65, 28, 66, 67, 18, 45, 62, 60, 47, 33, 48, 49,
    50, 38, 51, 52, 53, 64, 63, 39, 40, 31, 34, 46,
    35, 37, 61, 32, 59, 36, 58, 41, 43, 42, 22, 27,
    16, 45, 62, 60, 47, 33, 48, 49, 50, 38, 51, 52,
    53, 64, 63, 39, 40, 31, 34, 46, 35, 37, 61, 32,
    59, 36, 58, 41, 43, 42, 16, 84
};
SLICE01_SIZE_CHECK(character_to_key, 256);

/* 0x0065bd58, 0x200 bytes */
int16_t scan_code_to_key[256] = {
    -1, 0, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26,
    27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38,
    39, 40, 41, 42, 56, 69, 45, 46, 47, 48, 49, 50,
    51, 52, 53, 54, 55, 16, 57, 43, 58, 59, 60, 61,
    62, 63, 64, 65, 66, 67, 68, 89, 71, 72, 44, 1,
    2, 3, 4, 5, 6, 7, 8, 9, 10, 87, 14, 97,
    98, 99, 100, 94, 95, 96, 101, 91, 92, 93, 90, 103,
    -1, -1, 108, 11, 12, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, 104, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    107, 105, 106, -1, 16, -1, -1, -1, -1, -1, -1, -1,
    102, 76, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, 88, -1, 13, 73, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, 15, -1, 82, 77, 83, -1, 79,
    -1, 80, -1, 85, 78, 86, 81, 84, -1, -1, -1, -1,
    -1, -1, -1, 70, 74, 75, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, 0
};
SLICE01_SIZE_CHECK(scan_code_to_key, 512);

/* 0x0065bf74, 0x40 bytes */
__declspec(align(4)) int32_t resolution_index_table_0065bf74[16] = {
    2, 3, 4, 5, 6, 7,
    8, 9, 10, 11, 12, 13,
    14, 15, 16, 15
};
SLICE01_SIZE_CHECK(resolution_index_table_0065bf74, 64);

/* 0x0065bfb4, 0x14 bytes */
__declspec(align(4)) int32_t resolution_row_count_table_0065bfb4[5] = {
    1, 3, 7, 9, 15
};
SLICE01_SIZE_CHECK(resolution_row_count_table_0065bfb4, 20);

/* 0x0065c108, 0x48 bytes */
__declspec(align(4)) float motion_sensor_blip_colors[18] = {
    1.0f, 0.5f, 0.0f, 1.0f, 1.0f, 0.0f,
    1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,
    1.0f, 0.0f, 0.0f, 0.5f, 0.5f, 1.0f
};
SLICE01_SIZE_CHECK(motion_sensor_blip_colors, 72);

/* 0x0065c15c, 0x24 bytes */
__declspec(align(4)) int32_t controls_reserved_action_table[9] = {
    0, 56, 87, 70, 74, 16,
    13, 75, 102
};
SLICE01_SIZE_CHECK(controls_reserved_action_table, 36);

/* 0x0065c190, 0x48 bytes */
__declspec(align(4)) float k_octahedron_vertices[18] = {
    0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
    1.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f,
    -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f
};
SLICE01_SIZE_CHECK(k_octahedron_vertices, 72);

/* 0x0065c1d8, 0x30 bytes */
int16_t k_octahedron_faces[24] = {
    0, 1, 2, 0, 2, 3, 0, 3, 4, 0, 4, 1,
    5, 1, 4, 5, 4, 3, 5, 3, 2, 5, 2, 1
};
SLICE01_SIZE_CHECK(k_octahedron_faces, 48);

/* 0x0065c230, 0xc bytes */
__declspec(align(4)) float global_origin3d[3] = {
    0.0f, 0.0f, 0.0f
};
SLICE01_SIZE_CHECK(global_origin3d, 12);

/* 0x0065c29c, 0x18 bytes */
int16_t k_projection_axes[12] = {
    2, 1, 1, 2, 0, 2, 2, 0, 1, 0, 0, 1
};
SLICE01_SIZE_CHECK(k_projection_axes, 24);

/* 0x0065c2b4, 0x8 bytes */
__declspec(align(4)) uint8_t bit_mask_clear[8] = {
    0xff, 0xfe, 0xfc, 0xf8, 0xf0, 0xe0, 0xc0, 0x80
};
SLICE01_SIZE_CHECK(bit_mask_clear, 8);

/* 0x0065c2c0, 0x9 bytes */
__declspec(align(4)) uint8_t bit_mask_keep[9] = {
    0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff
};
SLICE01_SIZE_CHECK(bit_mask_keep, 9);

/* 0x0065d428, 0x8 bytes */
__declspec(align(4)) void * network_bandwidth_units_label_table[2] = {
    (void *)"bytes", (void *)"packets"
};
SLICE01_SIZE_CHECK(network_bandwidth_units_label_table, 8);

/* 0x0065d430, 0x8 bytes */
__declspec(align(4)) void * network_bandwidth_direction_label_table[2] = {
    (void *)"sent", (void *)"recv"
};
SLICE01_SIZE_CHECK(network_bandwidth_direction_label_table, 8);

/* 0x0065d438, 0x8 bytes */
__declspec(align(4)) uint32_t message_delta_unary_ones[2] = {
    0xffffffff, 0x00000000
};
SLICE01_SIZE_CHECK(message_delta_unary_ones, 8);

/* 0x0065d51f, 0x801 bytes */
uint8_t message_delta_item_count_bits[2049] = {
    0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
    0x04, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
    0x05, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
    0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
    0x06, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
    0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
    0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
    0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
    0x07, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
    0x08, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,
    0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,
    0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,
    0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,
    0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,
    0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,
    0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,
    0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,
    0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,
    0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,
    0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,
    0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,
    0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,
    0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,
    0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,
    0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,
    0x09, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a,
    0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a,
    0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a,
    0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a,
    0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a,
    0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a,
    0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a,
    0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a,
    0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a,
    0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a,
    0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a,
    0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a,
    0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a,
    0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a,
    0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a,
    0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a,
    0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a,
    0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a,
    0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a,
    0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a,
    0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a,
    0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a,
    0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a,
    0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a,
    0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a,
    0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a,
    0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a,
    0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a,
    0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a,
    0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a,
    0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a,
    0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a,
    0x0a, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0b
};
SLICE01_SIZE_CHECK(message_delta_item_count_bits, 2049);

/* 0x0065dd20, 0x74 bytes */
__declspec(align(4)) uint32_t object_lighting_default[29] = {
    0x3e4ccccd, 0x3e4ccccd, 0x3e4ccccd, 2,
    0x3f800000, 0x3f800000, 0x3f800000, 0xbf13b646,
    0xbf13b646, 0xbf13b646, 0x3ecccccd, 0x3ecccccd,
    0x3f000000, 0, 0, 0x3f800000,
    0, 0, 0, 0x3f000000,
    0x3f800000, 0x3f800000, 0x3f800000, 0,
    0, 0xbf800000, 0, 0,
    0
};
SLICE01_SIZE_CHECK(object_lighting_default, 116);

/* 0x0065dd94, 0xc bytes */
__declspec(align(4)) float object_lightmap_probe_direction[3] = {
    0.0f, 0.0f, -10.0f
};
SLICE01_SIZE_CHECK(object_lightmap_probe_direction, 12);

/* 0x0065dda0, 0x30 bytes */
__declspec(align(4)) float object_lighting_probe_sideways[12] = {
    -10.0f, 0.0f, 0.0f, 10.0f, 0.0f, 0.0f,
    0.0f, -10.0f, 0.0f, 0.0f, 10.0f, 0.0f
};
SLICE01_SIZE_CHECK(object_lighting_probe_sideways, 48);

/* 0x0065ddd0, 0x18 bytes */
__declspec(align(4)) void * ai_gc_callback_table[6] = {
    0, (void *)&halo::ai::ai_release_inactive_swarms, (void *)&halo::ai::ai_build_priority_target_list, (void *)&halo::ai::ai_release_inactive_encounters,
    0, 0
};
SLICE01_SIZE_CHECK(ai_gc_callback_table, 24);

/* 0x0065de00, 0x28 bytes */
int16_t rasterizer_vertex_sizes[20] = {
    56, 32, 20, 8, 68, 32, 24, 36, 24, 16, 16, 20,
    32, 8, 32, 32, 36, 28, 32, 40
};
SLICE01_SIZE_CHECK(rasterizer_vertex_sizes, 40);

/* 0x0065dfbc, 0x24 bytes */
__declspec(align(4)) uint32_t rasterizer_blend_src_table[9] = {
    0x00000005, 0x00000009, 0x00000009, 0x00000002, 0x00000002, 0x00000002,
    0x00000002, 0x00000002, 0xffffffff
};
SLICE01_SIZE_CHECK(rasterizer_blend_src_table, 36);

/* 0x0065dfe0, 0x24 bytes */
__declspec(align(4)) uint32_t rasterizer_blend_dest_table[9] = {
    0x00000006, 0x00000001, 0x00000003, 0x00000002, 0x00000002, 0x00000002,
    0x00000002, 0x00000006, 0xffffffff
};
SLICE01_SIZE_CHECK(rasterizer_blend_dest_table, 36);

/* 0x0065e004, 0x30 bytes */
__declspec(align(4)) uint32_t rasterizer_blend_op_table[12] = {
    0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000003, 0x00000004,
    0x00000005, 0x00000001, 0xffffffff, 0x000a0009, 0x00070006, 0x00000014
};
SLICE01_SIZE_CHECK(rasterizer_blend_op_table, 48);

/* 0x0065e034, 0x8 bytes */
__declspec(align(4)) uint32_t rasterizer_triangle_buffer_primitive_types[2] = {
    0x00000004, 0x00000005
};
SLICE01_SIZE_CHECK(rasterizer_triangle_buffer_primitive_types, 8);

/* 0x0065e040, 0x48 bytes */
__declspec(align(4)) int32_t rasterizer_bitmap_format_to_d3dformat[18] = {
    -1, -1, -1, -1, -1, -1,
    23, -1, 25, 26, 22, 21,
    -1, -1, 827611204, 844388420, 877942852, -1
};
SLICE01_SIZE_CHECK(rasterizer_bitmap_format_to_d3dformat, 72);

/* 0x0065e088, 0xc bytes */
int16_t rasterizer_cube_face_to_d3d_face[6] = {
    0, 2, 1, 3, 4, 5
};
SLICE01_SIZE_CHECK(rasterizer_cube_face_to_d3d_face, 12);

/* 0x0065e098, 0x80 bytes */
__declspec(align(4)) float rasterizer_sun_glow_blur_offsets[32] = {
    1.0f, 0.0f, 0.0f, -0.0078125f, 0.0f, 1.0f,
    0.0f, -0.0078125f, 1.0f, 0.0f, 0.0f, 0.0078125f,
    0.0f, 1.0f, 0.0f, 0.0078125f, 1.0f, 0.0f,
    0.0f, -0.0078125f, 0.0f, 1.0f, 0.0f, 0.0078125f,
    1.0f, 0.0f, 0.0f, 0.0078125f, 0.0f, 1.0f,
    0.0f, -0.0078125f
};
SLICE01_SIZE_CHECK(rasterizer_sun_glow_blur_offsets, 128);

/* 0x0065e118, 0x50 bytes */
__declspec(align(4)) float rasterizer_identity_vertex_constants[20] = {
    1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
    0.0f, 1.0f
};
SLICE01_SIZE_CHECK(rasterizer_identity_vertex_constants, 80);

/* 0x0065e168, 0x18 bytes */
__declspec(align(4)) uint32_t vertex_elements_debug[6] = {
    0, 2, 0x000c0000, 0x000a0004,
    0x000000ff, 0x00000011
};
SLICE01_SIZE_CHECK(vertex_elements_debug, 24);

/* 0x0065e180, 0x18 bytes */
__declspec(align(4)) uint32_t vertex_elements_decal[6] = {
    0, 2, 0x000c0000, 0x000a0004,
    0x000000ff, 0x00000011
};
SLICE01_SIZE_CHECK(vertex_elements_decal, 24);

/* 0x0065e198, 0x20 bytes */
__declspec(align(4)) uint32_t vertex_elements_detail_object[8] = {
    0, 2, 0x000c0000, 0x000a0004,
    0x00100000, 0x010a0004, 0x000000ff, 0x00000011
};
SLICE01_SIZE_CHECK(vertex_elements_detail_object, 32);

/* 0x0065e1b8, 0x20 bytes */
__declspec(align(4)) uint32_t vertex_elements_dynamic_screen[8] = {
    0, 2, 0x000c0000, 0x000a0004,
    0x00100000, 0x00050001, 0x000000ff, 0x00000011
};
SLICE01_SIZE_CHECK(vertex_elements_dynamic_screen, 32);

/* 0x0065e1d8, 0x20 bytes */
__declspec(align(4)) uint32_t vertex_elements_dynamic[8] = {
    0, 2, 0x000c0000, 0x000a0004,
    0x00100000, 0x00050001, 0x000000ff, 0x00000011
};
SLICE01_SIZE_CHECK(vertex_elements_dynamic, 32);

/* 0x0065e1f8, 0x28 bytes */
__declspec(align(4)) uint32_t vertex_elements_unlit_zsprite[10] = {
    0, 2, 0x000c0000, 0x000a0004,
    0x00100000, 0x00050001, 1, 0x01050001,
    0x000000ff, 0x00000011
};
SLICE01_SIZE_CHECK(vertex_elements_unlit_zsprite, 40);

/* 0x0065e220, 0x40 bytes */
__declspec(align(4)) uint32_t vertex_elements_model[16] = {
    0, 2, 0x000c0000, 0x00030002,
    0x00180000, 0x00070002, 0x00240000, 0x00060002,
    0x00300000, 0x00050001, 0x00380000, 0x00020006,
    0x003c0000, 0x00010001, 0x000000ff, 0x00000011
};
SLICE01_SIZE_CHECK(vertex_elements_model, 64);

/* 0x0065e260, 0x20 bytes */
__declspec(align(4)) uint32_t vertex_elements_model_ff[8] = {
    0, 2, 0x000c0000, 0x00030002,
    0x00180000, 0x00050001, 0x000000ff, 0x00000011
};
SLICE01_SIZE_CHECK(vertex_elements_model_ff, 32);

/* 0x0065e280, 0x20 bytes */
__declspec(align(4)) uint32_t vertex_elements_model_processed[8] = {
    0, 2, 0x000c0000, 0x00030002,
    0x00180000, 0x00050001, 0x000000ff, 0x00000011
};
SLICE01_SIZE_CHECK(vertex_elements_model_processed, 32);

/* 0x0065e2a0, 0x40 bytes */
__declspec(align(4)) uint32_t vertex_elements_environment_lightmap[16] = {
    0, 2, 0x000c0000, 0x00030002,
    0x00180000, 0x00070002, 0x00240000, 0x00060002,
    0x00300000, 0x00050001, 1, 0x01030002,
    0x000c0001, 0x01050001, 0x000000ff, 0x00000011
};
SLICE01_SIZE_CHECK(vertex_elements_environment_lightmap, 64);

/* 0x0065e2e0, 0x28 bytes */
__declspec(align(4)) uint32_t vertex_elements_environment_lightmap_ff[10] = {
    0, 2, 0x000c0000, 0x00030002,
    0x00180000, 0x00050001, 1, 0x01050001,
    0x000000ff, 0x00000011
};
SLICE01_SIZE_CHECK(vertex_elements_environment_lightmap_ff, 40);

/* 0x0065e308, 0x30 bytes */
__declspec(align(4)) uint32_t vertex_elements_environment_uncompressed[12] = {
    0, 2, 0x000c0000, 0x00030002,
    0x00180000, 0x00070002, 0x00240000, 0x00060002,
    0x00300000, 0x00050001, 0x000000ff, 0x00000011
};
SLICE01_SIZE_CHECK(vertex_elements_environment_uncompressed, 48);

/* 0x0065e338, 0x20 bytes */
__declspec(align(4)) uint32_t vertex_elements_environment_uncompressed_ff[8] = {
    0, 2, 0x000c0000, 0x00030002,
    0x00180000, 0x00050001, 0x000000ff, 0x00000011
};
SLICE01_SIZE_CHECK(vertex_elements_environment_uncompressed_ff, 32);

/* 0x0065e358, 0x28 bytes */
__declspec(align(4)) uint32_t vertex_elements_environment_single_stream_ff[10] = {
    0, 2, 0x000c0000, 0x00030002,
    0x00180000, 0x00050001, 0x00200000, 0x01050001,
    0x000000ff, 0x00000011
};
SLICE01_SIZE_CHECK(vertex_elements_environment_single_stream_ff, 40);

/* 0x0065e380, 0x20 bytes */
__declspec(align(4)) uint32_t vertex_elements_screen_transformed_lit[8] = {
    0, 0x00090003, 0x00100000, 0x000a0004,
    0x00140000, 0x00050001, 0x000000ff, 0x00000011
};
SLICE01_SIZE_CHECK(vertex_elements_screen_transformed_lit, 32);

/* 0x0065e3a0, 0x28 bytes */
__declspec(align(4)) uint32_t vertex_elements_screen_transformed_lit_specular[10] = {
    0, 0x00090003, 0x00100000, 0x000a0004,
    0x00140000, 0x010a0004, 0x00180000, 0x00050001,
    0x000000ff, 0x00000011
};
SLICE01_SIZE_CHECK(vertex_elements_screen_transformed_lit_specular, 40);

/* 0x0065e3c8, 0x8 bytes */
int16_t rasterizer_first_map_bitmap_types[4] = {
    0, 2, 2, 2
};
SLICE01_SIZE_CHECK(rasterizer_first_map_bitmap_types, 8);

/* 0x0065e3d0, 0x10 bytes */
__declspec(align(4)) uint32_t rasterizer_first_map_address_modes[4] = {
    0x00000001, 0x00000003, 0x00000003, 0x00000003
};
SLICE01_SIZE_CHECK(rasterizer_first_map_address_modes, 16);

/* 0x0065e3e0, 0x8 bytes */
int16_t rasterizer_extended_first_map_bitmap_types[4] = {
    0, 2, 2, 2
};
SLICE01_SIZE_CHECK(rasterizer_extended_first_map_bitmap_types, 8);

/* 0x0065e3e8, 0x10 bytes */
__declspec(align(4)) uint32_t rasterizer_extended_first_map_address_modes[4] = {
    0x00000001, 0x00000003, 0x00000003, 0x00000003
};
SLICE01_SIZE_CHECK(rasterizer_extended_first_map_address_modes, 16);

/* 0x0065e4f8, 0x8 bytes */
__declspec(align(4)) int32_t k_sound_sample_rates[2] = {
    22050, 44100
};
SLICE01_SIZE_CHECK(k_sound_sample_rates, 8);

/* 0x0065e508, 0x48 bytes */
__declspec(align(4)) uint32_t k_default_sound_environment[18] = {
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x3f800000,
    0x3f000000, 0x00000000, 0x3ca3d70a, 0x00000000, 0x3d23d70a, 0x3f800000,
    0x3f800000, 0x459c4000, 0x00000000, 0x00000000, 0x00000000, 0x00000000
};
SLICE01_SIZE_CHECK(k_default_sound_environment, 72);

/* 0x0065e560, 0x4 bytes */
__declspec(align(4)) float sound_delay_per_world_unit[1] = {
    8.96470642f
};
SLICE01_SIZE_CHECK(sound_delay_per_world_unit, 4);

/* 0x0065e56c, 0x20 bytes */
int16_t adpcm_index_table[16] = {
    -1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1,
    2, 4, 6, 8
};
SLICE01_SIZE_CHECK(adpcm_index_table, 32);

/* 0x0065e590, 0xb2 bytes */
int16_t adpcm_step_table[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21,
    23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66,
    73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209,
    230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658,
    724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
    2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484,
    7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350,
    22385, 24623, 27086, 29794, 32767
};
SLICE01_SIZE_CHECK(adpcm_step_table, 178);

/* 0x0065e640, 0xc bytes */
__declspec(align(4)) void * k_sound_decode_procs[3] = {
    (void *)0x7fff, (void *)&halo::sound::sound_adpcm_decode_mono, (void *)&halo::sound::sound_adpcm_decode_stereo
};
SLICE01_SIZE_CHECK(k_sound_decode_procs, 12);

/* 0x0065e64c, 0x10 bytes */
__declspec(align(4)) float near_clip_plane[4] = {
    0.0f, 0.0f, -1.0f, 0.00999999978f
};
SLICE01_SIZE_CHECK(near_clip_plane, 16);

/* 0x0065e660, 0x144 bytes */
__declspec(align(4)) float placement_offset_table[81] = {
    0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.707106769f, 0.0f, 0.707106769f,
    0.577350259f, 0.577350259f, 0.577350259f, 0.577350259f, -0.577350259f, 0.577350259f,
    0.707106769f, 0.707106769f, 0.0f, 0.707106769f, -0.707106769f, 0.0f,
    0.0f, 0.707106769f, 0.707106769f, 0.0f, -0.707106769f, 0.707106769f,
    -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, -1.0f, 0.0f, -0.707106769f, -0.707106769f, 0.0f,
    -0.707106769f, 0.707106769f, 0.0f, -0.707106769f, 0.0f, 0.707106769f,
    -0.577350259f, -0.577350259f, 0.577350259f, -0.577350259f, 0.577350259f, 0.577350259f,
    0.0f, 0.0f, -1.0f, 0.707106769f, 0.0f, -0.707106769f,
    -0.707106769f, 0.0f, -0.707106769f, 0.0f, 0.707106769f, -0.707106769f,
    0.0f, -0.707106769f, -0.707106769f, 0.577350259f, 0.577350259f, -0.577350259f,
    0.577350259f, -0.577350259f, -0.577350259f, -0.577350259f, -0.577350259f, -0.577350259f,
    -0.577350259f, 0.577350259f, -0.577350259f
};
SLICE01_SIZE_CHECK(placement_offset_table, 324);

/* 0x0065e7a8, 0x1a4 bytes */
int16_t unit_speech_fallback_index[210] = {
    1, -1, -1, -1, -1, -1, -1, 6, 6, 6, -1, 10,
    -1, 12, 15, -1, 14, 15, 14, 15, -1, -1, 21, -1,
    -1, -1, -1, -1, -1, -1, -1, 29, -1, -1, -1, 32,
    32, 32, 32, 32, 32, 32, 32, 32, 32, 32, -1, -1,
    -1, -1, -1, 49, -1, -1, -1, 53, 54, 53, 54, 53,
    54, 53, 54, 53, 54, 53, 53, 53, 53, 53, 53, 53,
    53, 53, 53, 53, -1, -1, -1, -1, -1, 80, 80, 80,
    80, 80, 80, 80, 80, 80, 80, 80, 80, 93, 94, 95,
    -1, 96, 96, 98, 96, 100, 100, 100, 100, -1, -1, -1,
    108, 109, 108, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, 53, 112, 154, 191,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, 0, 0
};
SLICE01_SIZE_CHECK(unit_speech_fallback_index, 420);

/* 0x0065e94c, 0x18 bytes */
int16_t unit_speech_priority_table[12] = {
    0, 0, 1, 1, 2, 2, 5, 5, 7, 7, 10, 0
};
SLICE01_SIZE_CHECK(unit_speech_priority_table, 24);

/* 0x0065e964, 0x30 bytes */
__declspec(align(4)) float unit_speech_repeat_seconds[12] = {
    0.0f, 0.0f, 0.0f, 1.5f, 3.0f, 4.0f,
    3.40282347e+38f, 3.0f, 3.0f, 3.0f, 3.0f, 1.5f
};
SLICE01_SIZE_CHECK(unit_speech_repeat_seconds, 48);

/* 0x0065efec, 0x4 bytes */
char network_log_path_format[4] = "%s";
SLICE01_SIZE_CHECK(network_log_path_format, 4);

/* 0x0065f008, 0x8 bytes */
char prop_array_name[8] = "prop";
SLICE01_SIZE_CHECK(prop_array_name, 8);

/* 0x0065f010, 0x4 bytes */
char joystick_set_separator_0065f010[4] = "\r\n";
SLICE01_SIZE_CHECK(joystick_set_separator_0065f010, 4);

/* 0x0065fb14, 0x4 bytes */
char DAT_0065fb14[4] = "\n";
SLICE01_SIZE_CHECK(DAT_0065fb14, 4);

/* 0x0065fb2c, 0x4 bytes */
char DAT_0065fb2c[4] = "\t";
SLICE01_SIZE_CHECK(DAT_0065fb2c, 4);

/* 0x0065fb30, 0x4 bytes */
char decimal_format_string[4] = "%d";
SLICE01_SIZE_CHECK(decimal_format_string, 4);

/* 0x0065fd30, 0x4 bytes */
char network_summary_log_mode_string[4] = "wt";
SLICE01_SIZE_CHECK(network_summary_log_mode_string, 4);

/* 0x0065ff44, 0x4 bytes */
char file_open_mode_w[4] = "w+";
SLICE01_SIZE_CHECK(file_open_mode_w, 4);

/* 0x006600a0, 0x10 bytes */
char s_primary_trigger_marker[16] = "primary trigger";
SLICE01_SIZE_CHECK(s_primary_trigger_marker, 16);

/* 0x006600e8, 0x4 bytes */
char hwreq_version_root_block[4] = "\\";
SLICE01_SIZE_CHECK(hwreq_version_root_block, 4);

/* 0x00660100, 0x18 bytes */
char error_file_no_timestamp[24] = "<TIME UNAVAILABLE>  ";
SLICE01_SIZE_CHECK(error_file_no_timestamp, 24);

/* 0x00660118, 0x20 bytes */
char error_file_timestamp_format[32] = "%02d.%02d.%02d %02d:%02d:%02d  ";
SLICE01_SIZE_CHECK(error_file_timestamp_format, 32);

/* 0x00660138, 0xc bytes */
char error_file_name[12] = "debug.txt";
SLICE01_SIZE_CHECK(error_file_name, 12);

/* 0x00660144, 0x4 bytes */
char error_file_open_mode[4] = "a+b";
SLICE01_SIZE_CHECK(error_file_open_mode, 4);

/* 0x00660148, 0x18 bytes */
char error_file_address_format[24] = "reference address: %x\r\n";
SLICE01_SIZE_CHECK(error_file_address_format, 24);

/* 0x00660160, 0x1c bytes */
char error_file_function_format[28] = "reference function: %s\r\n";
SLICE01_SIZE_CHECK(error_file_function_format, 28);

/* 0x0066017c, 0x18 bytes */
char error_file_function_name[24] = "_write_to_error_file";
SLICE01_SIZE_CHECK(error_file_function_name, 24);

/* 0x00660198, 0x50 bytes */
char error_file_banner[80] = "halo pc 01.00.10.0621(CACHE) ----------------------------------------------\r\n";
SLICE01_SIZE_CHECK(error_file_banner, 80);

/* 0x006601e8, 0x8 bytes */
char error_file_spacer[8] = "\r\n\r\n";
SLICE01_SIZE_CHECK(error_file_spacer, 8);

/* 0x00660788, 0x8 bytes */
wchar_t message_delta_config_value_delimiters[4] = { 0x0020, 0x0000, 0x0025, 0x0073 };
SLICE01_SIZE_CHECK(message_delta_config_value_delimiters, 8);

/* 0x006607a0, 8 bytes: L"%d". globals.asm had this symbol at 0x0063fff0 (the start of an unrelated D3DX string); every use passes it as the "%d" format */
wchar_t PTR_s_parameter_handles_0063fff0_0x35_006607a0[4] = L"%d";
SLICE01_SIZE_CHECK(PTR_s_parameter_handles_0063fff0_0x35_006607a0, 8);

/* 0x006607a0, 0x8 bytes */
wchar_t chat_local_prompt_string[4] = L"%d";
SLICE01_SIZE_CHECK(chat_local_prompt_string, 8);

/* 0x00660c34, 0x4 bytes */
wchar_t empty_string[2] = L"";
SLICE01_SIZE_CHECK(empty_string, 4);

/* 0x00669140, 0x4 bytes */
char console_echo_prefix[4] = "|t";
SLICE01_SIZE_CHECK(console_echo_prefix, 4);

/* 0x00669a38, 0x28 bytes */
char campaign_level_short_names[40] = { 100, 52, 48, 0, 100, 50, 48, 0, 99, 52, 48, 0, 99, 50, 48, 0, 99, 49, 48, 0, 98, 52, 48, 0, 98, 51, 48, 0, 97, 53, 48, 0, 97, 51, 48, 0, 97, 49, 48, 0 };
SLICE01_SIZE_CHECK(campaign_level_short_names, 40);

/* 0x00669a38, 0x4 bytes */
char player_help_name_d40[4] = "d40";
SLICE01_SIZE_CHECK(player_help_name_d40, 4);

/* 0x00669a3c, 0x4 bytes */
char player_help_name_d20[4] = "d20";
SLICE01_SIZE_CHECK(player_help_name_d20, 4);

/* 0x00669a40, 0x4 bytes */
char player_help_name_c40[4] = "c40";
SLICE01_SIZE_CHECK(player_help_name_c40, 4);

/* 0x00669a44, 0x4 bytes */
char player_help_name_c20[4] = "c20";
SLICE01_SIZE_CHECK(player_help_name_c20, 4);

/* 0x00669a48, 0x4 bytes */
char player_help_name_c10[4] = "c10";
SLICE01_SIZE_CHECK(player_help_name_c10, 4);

/* 0x00669a4c, 0x4 bytes */
char player_help_name_b40[4] = "b40";
SLICE01_SIZE_CHECK(player_help_name_b40, 4);

/* 0x00669a50, 0x4 bytes */
char player_help_name_b30[4] = "b30";
SLICE01_SIZE_CHECK(player_help_name_b30, 4);

/* 0x00669a54, 0x4 bytes */
char player_help_name_a50[4] = "a50";
SLICE01_SIZE_CHECK(player_help_name_a50, 4);

/* 0x00669a58, 0x4 bytes */
char player_help_name_a30[4] = "a30";
SLICE01_SIZE_CHECK(player_help_name_a30, 4);

/* 0x00669a5c, 0x4 bytes */
char player_help_name_a10[4] = "a10";
SLICE01_SIZE_CHECK(player_help_name_a10, 4);

/* 0x00669ae0, 0x4 bytes */
char DAT_00669ae0[4] = "|n";
SLICE01_SIZE_CHECK(DAT_00669ae0, 4);

/* 0x00669ca8, 0x20 bytes */
wchar_t ui_out_of_memory_text[16] = L"<out of memory>";
SLICE01_SIZE_CHECK(ui_out_of_memory_text, 32);

/* 0x00669cc8, 0x8 bytes */
wchar_t hud_text_unbound[4] = L"\?\?\?";
SLICE01_SIZE_CHECK(hud_text_unbound, 8);

/* 0x00669cd0, 0x4 bytes */
wchar_t hud_text_quote[2] = L"\"";
SLICE01_SIZE_CHECK(hud_text_quote, 4);

/* 0x00669cd4, 0x4 bytes */
wchar_t prompt_percent_text[2] = L"%";
SLICE01_SIZE_CHECK(prompt_percent_text, 4);

/* 0x0066a564, 0x8 bytes */
wchar_t ip_port_format_string_0066a564[4] = L":%d";
SLICE01_SIZE_CHECK(ip_port_format_string_0066a564, 8);

/* 0x0066a750, 0x14 bytes */
wchar_t hud_text_unknown[10] = L"<unknown>";
SLICE01_SIZE_CHECK(hud_text_unknown, 20);

/* 0x0066a888, 0x8 bytes */
wchar_t ui_format_narrow_string[4] = L"%S";
SLICE01_SIZE_CHECK(ui_format_narrow_string, 8);

/* 0x0066a890, 0x10 bytes */
char ui_version_string[16] = "01.00.10.0621";
SLICE01_SIZE_CHECK(ui_version_string, 16);

/* 0x0066a8a0, 0x14 bytes */
wchar_t ui_invalid_replacement_text[10] = L"<invalid>";
SLICE01_SIZE_CHECK(ui_invalid_replacement_text, 20);

/* 0x0066a8b4, 0x14 bytes */
wchar_t fortune_easter_egg_text[10] = L".fortune";
SLICE01_SIZE_CHECK(fortune_easter_egg_text, 20);

/* 0x0066a94c, 0x24 bytes */
wchar_t hud_text_no_button_icon[18] = L"<no button icon>";
SLICE01_SIZE_CHECK(hud_text_no_button_icon, 36);

/* 0x0066af68, 0x8 bytes */
wchar_t ticker_field_separator[4] = L" | ";
SLICE01_SIZE_CHECK(ticker_field_separator, 8);

/* 0x0066b038, 0x4 bytes */
char network_ban_indefinite_marker[4] = "--";
SLICE01_SIZE_CHECK(network_ban_indefinite_marker, 4);

/* 0x0066b090, 0x8 bytes */
char DAT_0066b090[8] = "ping";
SLICE01_SIZE_CHECK(DAT_0066b090, 8);

/* 0x0066b180, 0x10 bytes */
char s_ground_point_marker[16] = "ground point";
SLICE01_SIZE_CHECK(s_ground_point_marker, 16);

/* 0x0066b1bc, 0x14 bytes */
char s_secondary_trigger_marker[20] = "secondary trigger";
SLICE01_SIZE_CHECK(s_secondary_trigger_marker, 20);

/* 0x0066b87c, 0x4 bytes */
char player_update_log_file_mode_string[4] = "a";
SLICE01_SIZE_CHECK(player_update_log_file_mode_string, 4);

/* 0x0066bfa0, 0x8 bytes */
char ai_marker_name_a[8] = "head";
SLICE01_SIZE_CHECK(ai_marker_name_a, 8);

/* 0x0066d568, 0x4 bytes */
char sv_tk_grace_arg_buffer[4] = "ms";
SLICE01_SIZE_CHECK(sv_tk_grace_arg_buffer, 4);

/* 0x0066d6a4, 0x8 bytes */
char sv_ban_penalty_arg_buffer[8] = "dhms";
SLICE01_SIZE_CHECK(sv_ban_penalty_arg_buffer, 8);

/* 0x0066d81c, 0x4 bytes */
char network_ban_file_read_mode_string[4] = "rt";
SLICE01_SIZE_CHECK(network_ban_file_read_mode_string, 4);

/* 0x0066db40, 0x8 bytes */
char network_team_color_name_blue[8] = "Blue";
SLICE01_SIZE_CHECK(network_team_color_name_blue, 8);

/* 0x0066db48, 0x4 bytes */
char network_team_color_name_red[4] = "Red";
SLICE01_SIZE_CHECK(network_team_color_name_red, 4);

/* 0x0066db78, 0x8 bytes */
char network_team_color_names[8] = "Name";
SLICE01_SIZE_CHECK(network_team_color_names, 8);

/* 0x0066e660, 0x4 bytes */
char message_delta_config_mode_string[4] = "rb";
SLICE01_SIZE_CHECK(message_delta_config_mode_string, 4);

}
