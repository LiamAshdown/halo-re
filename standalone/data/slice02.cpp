/* standalone/data/slice02.cpp -- engine globals 0x0066e674..0x0068941d as extern "C" definitions (slice 2 of the
   globals->C conversion). The initial values are the retail bytes as decoded from the committed data image
   (standalone/image/*.asm), pointers expressed in C (function names, &other_global, string literals).
   The names are unchanged (_name), so no src/ edit is needed. Objects that could not be converted safely stay
   EQU in standalone/globals.asm; tools/globals_check_slice02.py lists them with the reason.

   All definitions sit in one extern "C" block: the ordered sections, the /alternatename pragmas and src/ reach these objects by their unmangled C names. */
#include "tables.h"
#include "halo/hs/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/shell/api.hpp"
#include "halo/camera/api.hpp"
#include "halo/cutscene/api.hpp"
#include "halo/sound/api.hpp"
#include <stdint.h>

extern "C" {

/* ---- code addresses stored in the tables below */

/* ---- other engine globals whose address is stored (defined elsewhere or still absolute) */
extern unsigned char k_empty_string;
extern unsigned char render_frame_index;

/* ---- definitions (address in the original executable) */
/* 0x0066e674 */
char message_delta_config_write_mode_string[3] = "wb";
/* 0x00670f8c */
char locale_codepage_format[4] = ".%d";
/* 0x00670f90 */
uint8_t default_locale_name[2] = { 0x43, 0x0 };
/* 0x00671d04 */
void *sound_eax2_vtable[9] = {
    (void *)&halo::sound::sound_eax20_effect_shutdown,
    (void *)&halo::sound::sound_eax20_effect_initialize,
    (void *)&halo::sound::sound_eax_effect_initialize_channel,
    (void *)&halo::sound::sound_effect_object_listener_supported,
    (void *)&halo::sound::sound_effect_object_channel_supported,
    (void *)&halo::sound::sound_eax20_effect_apply_channel,
    (void *)&halo::sound::sound_eax20_effect_set_environment_index,
    (void *)&halo::sound::sound_eax20_effect_apply_listener,
    (void *)&halo::sound::sound_eax20_effect_set_room_gain
};
/* 0x00671d28 */
void *sound_eax3_vtable[9] = {
    (void *)&halo::sound::sound_eax30_effect_shutdown,
    (void *)&halo::sound::sound_eax30_effect_initialize,
    (void *)&halo::sound::sound_eax_effect_initialize_channel,
    (void *)&halo::sound::sound_effect_object_listener_supported,
    (void *)&halo::sound::sound_effect_object_channel_supported,
    (void *)&halo::sound::sound_eax30_effect_apply_channel,
    (void *)&halo::sound::sound_eax30_effect_set_environment_index,
    (void *)&halo::sound::sound_eax30_effect_apply_listener,
    (void *)&halo::sound::sound_eax30_effect_set_room_gain
};
/* 0x00671d4c */
void *sound_eax1_vtable[9] = {
    (void *)&halo::sound::sound_eax1_effect_shutdown,
    (void *)&halo::sound::sound_eax1_effect_initialize,
    (void *)&halo::sound::sound_eax1_effect_initialize_channel,
    (void *)&halo::sound::sound_effect_object_listener_supported,
    (void *)&halo::sound::sound_eax1_effect_channel_supported,
    (void *)&halo::sound::sound_eax1_effect_apply_channel,
    (void *)&halo::sound::sound_eax1_effect_set_environment_index,
    (void *)&halo::sound::sound_eax1_effect_apply_listener,
    (void *)&halo::sound::sound_eax1_effect_set_room_gain
};
/* 0x00671fa0 */
char text_markup_codes[11] = "ibukprlctn";
/* 0x00671fac */
uint16_t missing_string_text[17] = { 0x3c, 0x6d, 0x69, 0x73, 0x73, 0x69, 0x6e, 0x67, 0x20, 0x73, 0x74, 0x72, 0x69, 0x6e, 0x67, 0x3e, 0x0 };
/* 0x00671fd0 */
char missing_string[17] = "<missing string>";
/* 0x00671ffc */
char s_left_hand_marker[10] = "left hand";
/* 0x00672034 */
char ai_marker_name_b[5] = "body";
/* 0x00672080 */
char s_blur_permutation[6] = "~blur";
/* 0x006721e8 */
void *hwreq_parser_vtable_instance[16] = {
    (void *)&halo::shell::hwreq_parser_parse,
    (void *)&halo::shell::hwreq_parser_scalar_deleting_destructor,
    (void *)&halo::shell::hwreq_parser_get_flags,
    (void *)&halo::shell::hwreq_parser_find_property_set,
    (void *)&halo::shell::hwreq_parser_get_flag_count,
    (void *)&halo::shell::hwreq_parser_get_flag_name,
    (void *)&halo::shell::hwreq_parser_get_flag_value,
    (void *)&halo::shell::hwreq_parser_get_requirement_count,
    (void *)&halo::shell::hwreq_parser_get_requirement_name,
    (void *)&halo::shell::hwreq_parser_get_requirement_value,
    (void *)&halo::shell::hwreq_parser_get_graphics_device_name,
    (void *)&halo::shell::hwreq_parser_get_graphics_vendor_name,
    (void *)&halo::shell::hwreq_parser_get_sound_device_name,
    (void *)&halo::shell::hwreq_parser_get_sound_vendor_name,
    (void *)&halo::shell::hwreq_parser_has_error,
    (void *)&halo::shell::hwreq_parser_get_error_message
};
/* 0x00672258 */
char string_vector_too_long[19] = "vector<T> too long";
/* 0x0067226c */
char string_invalid_vector_subscript[28] = "invalid vector<T> subscript";
/* 0x00672378 */
char hwreq_cannot_find_format[17] = "Cannot find '%s'";
/* 0x0067238c */
char hwreq_config_file_suffix[12] = "\\config.txt";
/* 0x00672578 */
uint16_t dxdiag_sound_device_child_name[4] = { 0x30, 0x0, 0x0, 0x0 };
/* 0x006728c4 */
uint32_t iid_dxdiag_provider[4] = { 0x9c6b4cb0u, 0x49cc23f8u, 0xa545eda3u, 0xd2a60050u };
/* 0x006728d4 */
uint32_t clsid_dxdiag_provider[4] = { 0xa65b8071u, 0x42133bfeu, 0x1d495b9au, 0xa71c46a4u };
/* 0x00672ac0 */
float k_real_zero = 0.0f;
/* 0x00672ac4 */
float k_real_one = 1.0f;
/* 0x00672ac8 */
float ticks_per_second = 30.0f;
/* 0x00672ae8 */
float sound_fade_duration_scale = 1000.0f;
/* 0x00672af8 */
double cursor_sensitivity_curve_bias = 1.0;
/* 0x00672b60 */
float text_color_scale = 255.0f;
/* 0x00672b84 */
float k_random_scale_65536 = 1.52590219e-05f;
/* 0x00672ba8 */
float k_projection_numerator = -1.0f;
/* 0x00672bc8 */
float actor_dialogue_variant_offset_3a = 0.400000006f;
/* 0x00672be4 */
float actor_dialogue_variant_offset_2a = 2.0f;
/* 0x00672c00 */
double k_plane_side_epsilon = 0.10000000149011612;
/* 0x00672c08 */
double response_curve_scale_limit = 0.0;
/* 0x00672c28 */
float actor_dialogue_variant_offset_1a = 4.0f;
/* 0x00672ca8 */
float k_real_point_six = 0.600000024f;
/* 0x00672cac */
float actor_dialogue_variant_scale_1b = 1.5f;
/* 0x00672cf0 */
double sqrt_pow_exponent = 0.5;
/* 0x00672da8 */
double cursor_sensitivity_curve_scale = 0.050000000000000003;
/* 0x00672db0 */
float k_surface_resolve_step = 0.000244140625f;
/* 0x00672dd8 */
float recorded_animation_angle_scale = 0.00314159272f;
/* 0x00672dec */
float actor_dialogue_variant_scale_23b = 1.19999993f;
/* 0x00672df0 */
float actor_dialogue_variant_scale_2a = 0.799999952f;
/* 0x00672ea0 */
float k_weapon_zoom_fov_maximum = 3.1101768f;
/* 0x00672ea4 */
float k_weapon_zoom_fov_minimum = 0.0314159282f;
/* 0x0067321c */
float hud_damage_indicator_screen_center_x = 320.0f;
/* 0x00683824 */
uint8_t NNMagicData[6] = { 0xfd, 0xfc, 0x1e, 0x66, 0x6a, 0xb2 };
/* 0x0068382c */
void *Matchup1Hostname = (void *)"natneg1.hosthpc.com";
/* 0x00683830 */
void *Matchup2Hostname = (void *)"natneg2.hosthpc.com";
/* 0x00683838 */
uint32_t static_rec[66] = { 0xffffffffu, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u, 0x0u };
/* 0x00683940 */
void *current_rec = (void *)&static_rec;
/* 0x00683944 */
uint32_t gt2_bignum_length = 0x10u;
/* 0x00683948 */
uint8_t md5_padding[64] = { 0x80, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };
/* 0x00683988 */
uint32_t gcd_socket = 0xffffffffu;
/* 0x00683990 */
void *qr2_registered_key_list[256] = {
    (void *)&k_empty_string,
    (void *)"hostname",
    (void *)"gamename",
    (void *)"gamever",
    (void *)"hostport",
    (void *)"mapname",
    (void *)"gametype",
    (void *)"gamevariant",
    (void *)"numplayers",
    (void *)"numteams",
    (void *)"maxplayers",
    (void *)"gamemode",
    (void *)"teamplay",
    (void *)"fraglimit",
    (void *)"teamfraglimit",
    (void *)"timeelapsed",
    (void *)"timelimit",
    (void *)"roundtime",
    (void *)"roundelapsed",
    (void *)"password",
    (void *)"groupid",
    (void *)"player_",
    (void *)"score_",
    (void *)"skill_",
    (void *)"ping_",
    (void *)"team_",
    (void *)"deaths_",
    (void *)"pid_",
    (void *)"team_t",
    (void *)"score_t",
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    (void *)"GTI2AwaitingServerChallenge",
    (void *)"GTI2AwaitingAcceptance"
};
/* 0x00683dd4 */
uint32_t ghiThrottleBufferSize = 0x7du;
/* 0x00683dd8 */
uint32_t ghiThrottleTimeDelay = 0xfau;
/* 0x006869a4 */
uint32_t connect_address = 0x0u;
/* 0x006869b0 */
uint32_t network_local_address = 0x0u;
/* 0x006869b4 */
uint32_t network_resolved_local_address = 0x0u;
/* 0x006869b8 */
uint8_t network_winsock_initialized[4] = { 0x0, 0x0, 0x0, 0x0 };
/* 0x006869bc */
uint8_t network_summary_log_needs_open = 0x1;
/* 0x006869bd */
uint8_t network_connection_log_needs_open = 0x1;
/* 0x006869be */
uint8_t network_channels_open_ok = 0x1;
/* 0x006869bf */
uint8_t network_update_unknown_869bf = 0x1;
/* 0x006869c4 */
uint32_t sound_cache_size_megabytes = 0x8u;
/* 0x006869c8 */
uint32_t sound_cache_unknown_c8 = 0x30u;
/* 0x00686a28 */
uint32_t camera_input_axes[28] = { 0x40005u, 0xffffu, 0x3e19999au, 0x0u, 0xff7fffffu, 0x7f7fffffu, 0x1u, 0x70006u, 0xffffu, 0x3d99999au, 0x0u, 0xff7fffffu, 0x7f7fffffu, 0x0u, 0x1u, 0xffffu, 0x3d99999au, 0x0u, 0xff7fffffu, 0x7f7fffffu, 0x1u, 0x20003u, 0xffffu, 0x3d99999au, 0x0u, 0xff7fffffu, 0x7f7fffffu, 0x1u };
/* 0x00686a98 */
uint32_t director_camera_switching = 0x1u;
/* 0x00686a9c */
float flying_camera_speed = 1.0f;
/* 0x00686aa0 */
uint32_t flying_camera_attached_object = 0xffffffffu;
/* 0x00686aa4 */
void *flying_camera_render_frame = (void *)&render_frame_index;
/* 0x00686aa8 */
void *flying_camera_update_procs[2] = {
    (void *)&halo::camera::flying_camera_update,
    (void *)&halo::camera::orbiting_camera_update
};
/* 0x00686ab0 */
void *flying_camera_transition_procs[4] = {
    0,
    0,
    (void *)&halo::camera::flying_camera_enter_flying,
    (void *)&halo::camera::flying_camera_enter_orbiting
};
/* 0x00686ae0 */
int16_t observer_parameter_float_counts[5] = { 3, 3, 1, 1, 6 };
/* 0x00686aec */
int16_t observer_derivative_float_counts[5] = { 3, 3, 1, 1, 3 };
/* 0x00686b48 */
uint8_t error_file_needs_header[4] = { 0x1, 0x0, 0x0, 0x0 };
/* 0x00686b4c */
uint32_t external_00686b4c = 0xffffffffu;
/* 0x00686b50 */
uint8_t external_00686b50[4] = { 0x0, 0x0, 0x0, 0x0 };
/* 0x00686b54 */
uint32_t external_00686b54 = 0x0u;
/* 0x00686b58 */
void *external_00686b58 = 0;
/* 0x00686b5c */
void *external_00686b5c = 0;
/* 0x00686b60 */
float cinematic_saved_music_gain = -1.0f;
/* 0x00686d98 */
void *recorded_animation_compressed_event_handlers[23] = {
    0,
    0,
    (void *)&halo::cutscene::recorded_animation_decode_animation_state_event,
    (void *)&halo::cutscene::recorded_animation_decode_aiming_speed_event,
    (void *)&halo::cutscene::recorded_animation_decode_control_flags_event,
    (void *)&halo::cutscene::recorded_animation_decode_weapon_index_event,
    (void *)&halo::cutscene::recorded_animation_decode_throttle_event,
    (void *)&halo::cutscene::recorded_animation_decode_char_difference_event,
    (void *)&halo::cutscene::recorded_animation_decode_char_difference_event,
    (void *)&halo::cutscene::recorded_animation_decode_char_difference_event,
    (void *)&halo::cutscene::recorded_animation_decode_char_difference_event,
    (void *)&halo::cutscene::recorded_animation_decode_char_difference_event,
    (void *)&halo::cutscene::recorded_animation_decode_char_difference_event,
    (void *)&halo::cutscene::recorded_animation_decode_char_difference_event,
    (void *)&halo::cutscene::recorded_animation_decode_char_difference_event,
    (void *)&halo::cutscene::recorded_animation_decode_short_difference_event,
    (void *)&halo::cutscene::recorded_animation_decode_short_difference_event,
    (void *)&halo::cutscene::recorded_animation_decode_short_difference_event,
    (void *)&halo::cutscene::recorded_animation_decode_short_difference_event,
    (void *)&halo::cutscene::recorded_animation_decode_short_difference_event,
    (void *)&halo::cutscene::recorded_animation_decode_short_difference_event,
    (void *)&halo::cutscene::recorded_animation_decode_short_difference_event,
    (void *)&halo::cutscene::recorded_animation_decode_short_difference_event
};
/* 0x00686ea8 */
void *recorded_animation_v1_event_handlers[23] = {
    0,
    0,
    (void *)&halo::cutscene::recorded_animation_decode_animation_state_event_v1,
    (void *)&halo::cutscene::recorded_animation_decode_aiming_speed_event_v1,
    (void *)&halo::cutscene::recorded_animation_decode_control_flags_event_v1,
    (void *)&halo::cutscene::recorded_animation_decode_weapon_index_event_v1,
    (void *)&halo::cutscene::recorded_animation_decode_throttle_event_v1,
    0,
    0,
    (void *)&halo::cutscene::recorded_animation_decode_facing_vector_event_v1,
    (void *)&halo::cutscene::recorded_animation_decode_aiming_vector_event_v1,
    (void *)&halo::cutscene::recorded_animation_decode_looking_vector_event_v1,
    (void *)&halo::cutscene::recorded_animation_decode_multi_vector_event_v1,
    (void *)&halo::cutscene::recorded_animation_decode_multi_vector_event_v1,
    (void *)&halo::cutscene::recorded_animation_decode_multi_vector_event_v1,
    (void *)&halo::cutscene::recorded_animation_decode_multi_vector_event_v1,
    (void *)&halo::cutscene::recorded_animation_decode_angle_vector_event_v1,
    (void *)&halo::cutscene::recorded_animation_decode_angle_vector_event_v1,
    (void *)&halo::cutscene::recorded_animation_decode_angle_vector_event_v1,
    (void *)&halo::cutscene::recorded_animation_decode_angle_vector_event_v1,
    (void *)&halo::cutscene::recorded_animation_decode_angle_vector_event_v1,
    (void *)&halo::cutscene::recorded_animation_decode_angle_vector_event_v1,
    (void *)&halo::cutscene::recorded_animation_decode_angle_vector_event_v1
};
/* 0x00687004 */
uint8_t decals_enabled = 0x1;
/* 0x00687014 */
uint8_t first_person_effects_enabled[4] = { 0x1, 0x0, 0x0, 0x0 };
/* 0x00687018 */
void *particle_impact_vector_names[2] = {
    (void *)"velocity",
    (void *)"gravity"
};
/* 0x00687218 */
int16_t screen_flash_pass[8] = { 0, 1, 2, 3, 4, 5, 6, 0 };
/* 0x00687350 */
uint8_t weather_enabled = 0x1;
/* 0x00687af4 */
float teleport_effect_const_00687af4 = 1.0f;
/* 0x00687af8 */
float teleport_effect_const_00687af8 = 0.5f;
/* 0x00687afc */
float teleport_effect_const_00687afc = 0.349999994f;
/* 0x00687b00 */
float teleport_effect_const_00687b00 = 1.0f;
/* 0x00687b04 */
float teleport_effect_const_00687b04 = 0.349999994f;
/* 0x00687b08 */
float teleport_effect_const_00687b08 = 1.0f;
/* 0x00687b0c */
void *game_variant_history = 0;
/* 0x00687b10 */
uint32_t game_variant_history_count = 0x0u;
/* 0x00687b14 */
uint32_t game_variant_history_capacity = 0x0u;
/* 0x00687b18 */
uint32_t game_variant_history_current = 0xffffffffu;
/* 0x00688328 */
uint32_t multiplayer_sound_enabled[30] = {
    0x01010101u, 0x01010101u, 0x01010101u, 0x01010101u,
    0x01010101u, 0x01010101u, 0x00000101u, 0,
    0x01010101u, 0x01010101u, 0x00000101u, 0,
    0x00000019u, 0xffffffffu, 0xffffffffu, 0xffffffffu,
    0xffffffffu, 0x00000001u, 0, (uint32_t)&multiplayer_sound_enabled_00687020[0],
    0x00000001u, 0xffffffffu, (uint32_t)&multiplayer_sound_enabled_00687020[8], 0,
    0, 0, 0, 0,
    0, 0,
};
/* 0x006883a0 */
uint32_t king_hill_idle_timeout = 0xd2u;
/* 0x006887a8 */
uint8_t game_engine_input_source_flag = 0x1;
/* 0x006887b0 */
uint32_t update_client_write_cursor = 0x1u;
/* 0x006887b4 */
uint32_t server_maximum_queued_client_updates = 0x2u;
/* 0x006887b8 */
uint32_t server_maximum_pending_client_update_ticks = 0x6u;
/* 0x006887bc */
uint32_t catchup_backlog_threshold = 0x2u;
/* 0x006887c0 */
uint32_t catchup_time_threshold = 0x6u;
/* 0x006889d0 */
uint32_t global_006889d0 = 0x5u;
/* 0x006889d4 */
float global_006889d4 = 1.0f;
/* 0x006889d8 */
float global_006889d8 = 0.800000012f;
/* 0x006889dc */
float global_006889dc = 0.800000012f;
/* 0x006889e0 */
float global_006889e0 = 2.0f;
/* 0x006889e4 */
uint32_t global_006889e4 = 0x2u;
/* 0x006889e8 */
float global_006889e8 = 1.0f;
/* 0x006889ec */
float global_006889ec = 0.349999994f;
/* 0x006889f0 */
float global_006889f0 = 0.349999994f;
/* 0x006889f4 */
float global_006889f4 = 2.0f;
/* 0x00688a78 */
void *hs_type_names[49] = {
    (void *)"unparsed",
    (void *)"special form",
    (void *)"function name",
    (void *)"passthrough",
    (void *)"void",
    (void *)"boolean",
    (void *)"real",
    (void *)"short",
    (void *)"long",
    (void *)"string",
    (void *)"script",
    (void *)"trigger_volume",
    (void *)"cutscene_flag",
    (void *)"cutscene_camera_point",
    (void *)"cutscene_title",
    (void *)"cutscene_recording",
    (void *)"device_group",
    (void *)"ai",
    (void *)"ai_command_list",
    (void *)"starting_profile",
    (void *)"conversation",
    (void *)"navpoint",
    (void *)"hud_message",
    (void *)"object_list",
    (void *)"sound",
    (void *)"effect",
    (void *)"damage",
    (void *)"looping_sound",
    (void *)"animation_graph",
    (void *)"actor_variant",
    (void *)"damage_effect",
    (void *)"object_definition",
    (void *)"game_difficulty",
    (void *)"team",
    (void *)"ai_default_state",
    (void *)"actor_type",
    (void *)"hud_corner",
    (void *)"object",
    (void *)"unit",
    (void *)"vehicle",
    (void *)"weapon",
    (void *)"device",
    (void *)"scenery",
    (void *)"object_name",
    (void *)"unit_name",
    (void *)"vehicle_name",
    (void *)"weapon_name",
    (void *)"device_name",
    (void *)"scenery_name"
};
/* 0x00688b3c */
void *hs_script_type_names[5] = {
    (void *)"startup",
    (void *)"dormant",
    (void *)"continuous",
    (void *)"static",
    (void *)"stub"
};
/* 0x00688b50 */
void *hs_empty_string = (void *)&k_empty_string;
/* 0x00689380 */
void *hs_autocomplete_procedures[18] = {
    (void *)&halo::hs::hs_enumerate_special_form_names,
    (void *)&halo::hs::hs_autocomplete_add_startup,
    (void *)&halo::hs::hs_autocomplete_add_type_names,
    (void *)&halo::hs::hs_autocomplete_add_function_names,
    (void *)&halo::hs::hs_autocomplete_add_script_names,
    (void *)&halo::hs::hs_autocomplete_add_global_names,
    (void *)&halo::hs::hs_autocomplete_add_encounter_names,
    (void *)&halo::hs::hs_autocomplete_add_command_list_names,
    (void *)&halo::hs::hs_autocomplete_add_starting_profile_names,
    (void *)&halo::hs::hs_autocomplete_add_conversation_names,
    (void *)&halo::hs::hs_autocomplete_add_object_names,
    (void *)&halo::hs::hs_autocomplete_add_trigger_volume_names,
    (void *)&halo::hs::hs_autocomplete_add_cutscene_flag_names,
    (void *)&halo::hs::hs_autocomplete_add_cutscene_camera_point_names,
    (void *)&halo::hs::hs_autocomplete_add_cutscene_title_names,
    (void *)&halo::hs::hs_autocomplete_add_recorded_animation_names,
    (void *)&halo::hs::hs_autocomplete_add_navpoint_names,
    (void *)&halo::hs::hs_autocomplete_add_hud_message_names
};
/* 0x006893c8 */
uint32_t k_vehicle_minimum_age_ticks = 0x4u;
/* 0x006893cc */
uint8_t biped_detach_from_flipped_vehicle[4] = { 0x1, 0x0, 0x0, 0x0 };
/* 0x006893d0 */
uint32_t k_biped_minimum_age_ticks = 0x7u;
/* 0x006893d4 */
float sound_dialog_ducking_gain = 0.699999988f;
/* 0x006893e0 */
uint8_t console_debug_toggle_6893e0[2] = { 0x0, 0x0 };
/* 0x006893e2 */
uint16_t frame_statistics_level = 0x0;
/* 0x006893e4 */
uint16_t console_debug_toggle_6893e4 = 0x0;
/* 0x006893e6 */
uint8_t console_debug_toggle_6893e6[2] = { 0x0, 0x0 };
/* 0x006893e8 */
int16_t console_model_lod_override = -1;
/* 0x006893eb */
uint8_t console_debug_toggle_6893eb = 0x0;
/* 0x006893ec */
uint8_t console_debug_toggle_6893ec = 0x1;
/* 0x006893ed */
uint8_t console_debug_toggle_6893ed = 0x1;
/* 0x006893ee */
uint8_t console_debug_toggle_6893ee = 0x1;
/* 0x006893ef */
uint8_t shader_stage_config_enabled[2] = { 0x1, 0x2 };
/* 0x006893f1 */
uint8_t console_debug_toggle_6893f1 = 0x1;
/* 0x006893f2 */
uint8_t console_debug_toggle_6893f2 = 0x1;
/* 0x006893f3 */
uint8_t console_debug_toggle_6893f3 = 0x1;
/* 0x006893f4 */
uint8_t console_debug_toggle_6893f4 = 0x1;
/* 0x006893f5 */
uint8_t decals_for_all_responses = 0x1;
/* 0x006893f6 */
uint8_t console_debug_toggle_6893f6 = 0x1;
/* 0x006893f7 */
uint8_t console_debug_toggle_6893f7 = 0x1;
/* 0x006893f8 */
uint8_t console_debug_toggle_6893f8 = 0x1;
/* 0x006893f9 */
uint8_t console_debug_toggle_6893f9 = 0x1;
/* 0x006893fa */
uint8_t console_debug_toggle_6893fa = 0x1;
/* 0x006893fb */
uint8_t console_debug_toggle_6893fb = 0x1;
/* 0x006893fc */
uint8_t console_debug_toggle_6893fc = 0x1;
/* 0x006893fd */
uint8_t device_reset_cleared_flag = 0x1;
/* 0x006893fe */
uint8_t rasterizer_water_enabled = 0x1;
/* 0x006893ff */
uint8_t decals_and_lens_flares_enabled = 0x1;
/* 0x00689400 */
uint8_t console_debug_toggle_689400[2] = { 0x1, 0x1 };
/* 0x00689402 */
uint8_t text_rendering_enabled = 0x1;
/* 0x00689403 */
uint8_t console_debug_toggle_689403 = 0x1;
/* 0x00689404 */
uint8_t console_debug_toggle_689404[3] = { 0x1, 0x1, 0x0 };
/* 0x00689407 */
uint8_t console_debug_toggle_689407 = 0x1;
/* 0x00689408 */
uint8_t console_debug_toggle_689408 = 0x1;
/* 0x00689409 */
uint8_t console_debug_toggle_689409[3] = { 0x1, 0x0, 0x0 };
/* 0x0068940c */
float underwater_tint_jitter_forced_value = 1.0f;
/* 0x00689412 */
uint16_t debug_print_enabled_flag = 0x0;
/* 0x00689418 */
float model_lighting_ambient_override = 0.0f;
/* 0x0068941c */
uint8_t console_debug_toggle_68941c = 0x1;
/* 0x0068941d */
uint8_t console_debug_toggle_68941d = 0x1;

/* ---- symbols that share the address of a definition above */
#pragma comment(linker, "/alternatename:_specular_projected_light_enabled=_console_debug_toggle_6893f6")
#pragma comment(linker, "/alternatename:_specular_lightmap_enabled=_console_debug_toggle_6893f7")
#pragma comment(linker, "/alternatename:_environment_multipurpose_enabled=_console_debug_toggle_6893f8")
#pragma comment(linker, "/alternatename:_fog_screen_overlay_enabled=_console_debug_toggle_68941d")

}
