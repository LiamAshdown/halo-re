/* Engine globals of slice 10 (0x007c04e0..0x008805a0) as extern "C" definitions.

   All of them lie past the initialised part of the original .data (zero-initialised BSS), so every definition is
   zero. The sizes are the declared sizes in src/ and types/; the ones that cannot be separated from their
   neighbours (alias names at one address, fields of a bigger object that other files name individually) are
   still absolute symbols in standalone/globals.asm. Generated once by hand-checked script; do not add globals
   here without the matching extern declaration in src/.

   All definitions sit in one extern "C" block: the ordered sections, the /alternatename pragmas and src/ reach these objects by their unmangled C names. */
#include "code_refs.hpp"
#include <stdint.h>
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "objects.h"
#include "cache.h"
#include "networking.h"
#include "interface.h"
#include "units.h"
#include "hs.h"
#include "ai.h"
#include "rasterizer.h"
#include "win32.h"
#include "saved_games.h"
#include "sound.h"
#include "shell.h"
#include "effects.h"
#include "items.h"
#include "physics.h"
#include "input.h"
#include "camera.h"
#include "structures.h"
#include "render.h"
#include "cutscene.h"
#include "main.h"
#include "projectiles.h"
#include "bitmaps.h"
#include "models.h"
#include "text.h"
#include "scenario.h"
#include "devices.h"
#include "shaders.h"

extern "C" {

rasterizer_skinning_matrix rasterizer_skinning_palette[63] = {0}; /* 0x007c04e0 */
uint8_t rasterizer_device_lost = {0}; /* 0x007c10b0 */
int32_t rasterizer_light_count = {0}; /* 0x007c1480 */
rasterizer_light rasterizer_lights[k_rasterizer_maximum_lights] = {0}; /* 0x007c1484 */
int32_t rasterizer_fixed_function_light_count = {0}; /* 0x007c3084 */
rasterizer_frame_statistics rasterizer_frame_statistics_state = {0}; /* 0x007c30a0 */
float build_sprite_screen_coverage = {0}; /* 0x007c30c4 */
int16_t build_sprite_large_quad_count = {0}; /* 0x007c30c8 */
real_vector3d build_sprite_view_up = {0}; /* 0x007c30d0 */
real_vector3d build_sprite_view_left = {0}; /* 0x007c30dc */
uint8_t render_debug_objects = {0}; /* 0x007c30e8 */
data_array *object_render_state_cache = {0}; /* 0x007c30ec */
int32_t render_frame_index = {0}; /* 0x007c3100 */
int32_t render_window_count = {0}; /* 0x007c3104 */
int16_t current_local_player_index = {0}; /* 0x007c3108 */
int16_t render_window_index = {0}; /* 0x007c310a */
float render_time_since_tick = {0}; /* 0x007c310c */
float render_time_since_frame = {0}; /* 0x007c3110 */
render_fog render_fog_state = {0}; /* 0x007c32f4 */
int32_t render_leaf_index = {0}; /* 0x007c3344 */
int32_t render_cluster_index = {0}; /* 0x007c3348 */
uint8_t render_cluster_has_sky = {0}; /* 0x007c334d */
int16_t render_cluster_sky_index = {0}; /* 0x007c334e */
uint32_t cluster_visible_bits[0x10] = {0}; /* 0x007c3350 */
structure_bsp_visible_cluster visible_clusters[k_maximum_visible_clusters] = {0}; /* 0x007c3390 */
int16_t visible_cluster_count = {0}; /* 0x007d0390 */
uint32_t surface_visible_bits[k_maximum_visible_surface_bits] = {0}; /* 0x007d0394 */
int16_t visible_surface_count = {0}; /* 0x00850394 */
int32_t visible_surface_indices[k_maximum_visible_surfaces] = {0}; /* 0x00850398 */
data_array *widget_data = {0}; /* 0x00860398 */
data_array *glow_data = {0}; /* 0x008603a0 */
data_array *glow_particle_data = {0}; /* 0x008603a4 */
data_array *flag_data = {0}; /* 0x008603a8 */
data_array *antenna_data = {0}; /* 0x008603ac */
int32_t object_cluster_stamp = {0}; /* 0x008603cc */
object_type_definition *object_type_definition_list = {0}; /* 0x008603dc */
uint8_t g_control_binding_state = {0}; /* 0x008607a0 */
uint8_t g_control_binding_secondary_active = {0}; /* 0x008607a1 */
uint8_t light_render_unknown_7c0 = {0}; /* 0x008607c0 */
int32_t light_frame_counter = {0}; /* 0x008607c4 */
int16_t light_active_list_count = {0}; /* 0x008607c8 */
datum_index light_active_list[0x80] = {0}; /* 0x008607cc */
light_transient light_transient_table[k_maximum_transient_lights] = {0}; /* 0x008609cc */
int16_t light_transient_count = {0}; /* 0x00860b0c */
int16_t light_transient_count_or_queue = {0}; /* 0x00860b10 */
data_array *light_data = {0}; /* 0x00860b14 */
char message_delta_config_text_buffer[0x800] = {0}; /* 0x00860b40 */
uint8_t network_incoming_message_scratch[0x510] = {0}; /* 0x00861de0 */
network_client_globals network_client_storage = {0}; /* 0x00872de0 */
uint8_t unknown_00873d30 = {0}; /* 0x00873d30 */
int16_t motion_sensor_render_local_player = {0}; /* 0x00873d32 */
float motion_sensor_render_center[2] = {0}; /* 0x00873d38 */
HUDGlobals *hud_messaging_parameters = {0}; /* 0x00873d40 */
profile_carousel_slot profile_carousel_slots[3] = {0}; /* 0x00873d60 */
variant_carousel_slot variant_carousel_slots[3] = {0}; /* 0x00879d60 */
uint32_t unknown_00879f34 = {0}; /* 0x00879f34 */
uint32_t unknown_00879f38 = {0}; /* 0x00879f38 */
float override_color_00879f40 = {0}; /* 0x00879f40 */
float override_color_00879f44 = {0}; /* 0x00879f44 */
float override_color_00879f48 = {0}; /* 0x00879f48 */
float override_color_00879f4c = {0}; /* 0x00879f4c */
int32_t last_controller_index_00879f50 = {0}; /* 0x00879f50 */
di_object_data_format joystick_objects[k_input_joystick_object_count] = {0}; /* 0x00879f60 */
int32_t last_input_device = {0}; /* 0x0087a460 */
data_array *object_list_header_data = {0}; /* 0x0087a464 */
data_array *object_list_reference_data = {0}; /* 0x0087a468 */
data_array *hs_globals_data = {0}; /* 0x0087a46c */
data_array *hs_thread_data = {0}; /* 0x0087a470 */
data_array *hs_syntax_data = {0}; /* 0x0087a474 */
data_array *player_data = {0}; /* 0x0087a480 */
int32_t slayer_unknown_0087a4a0[16] = {0}; /* 0x0087a4a0 */
int32_t slayer_unknown_0087a4e0[16] = {0}; /* 0x0087a4e0 */
ctf_globals ctf_globals_network = {0}; /* 0x0087a520 */
king_hill_marker_history king_hill_markers = {0}; /* 0x0087a9a0 */
int32_t ctf_touch_counts_network[3] = {0}; /* 0x0087a9e0 */
int32_t game_engine_unknown_aa00 = {0}; /* 0x0087aa00 */
int32_t game_engine_auto_team_counter = {0}; /* 0x0087aa04 */
float game_engine_end_game_timer = {0}; /* 0x0087aa08 */
float game_engine_post_game_fade = {0}; /* 0x0087aa0c */
game_engine_state game_engine_state_value = {}; /* 0x0087aa10 */
int32_t game_engine_ctf_reset_ticks = {0}; /* 0x0087aa24 */
uint8_t DAT_0087ab18 = {0}; /* 0x0087ab18 */
game_variant game_engine_active_variant = {0}; /* 0x0087ab20 */
uint8_t g_0087abc0 = {0}; /* 0x0087abc0 */
uint8_t DAT_0087abc1 = {0}; /* 0x0087abc1 */
uint8_t weapon_bottomless_clip = {0}; /* 0x0087abc2 */
uint8_t DAT_0087abc3 = {0}; /* 0x0087abc3 */
uint8_t cheat_super_jump = {0}; /* 0x0087abc4 */
uint8_t g_0087abc5 = {0}; /* 0x0087abc5 */
uint8_t ai_debug_gate_87abc6 = {0}; /* 0x0087abc6 */
uint8_t g_0087abc7 = {0}; /* 0x0087abc7 */
uint8_t weapon_infinite_ammo = {0}; /* 0x0087abc9 */
data_array *weather_particle_data = {0}; /* 0x0087abcc */
data_array *particle_data = {0}; /* 0x0087abd0 */
data_array *particle_system_data = {0}; /* 0x0087abd4 */
data_array *particle_system_particle_data = {0}; /* 0x0087abd8 */
data_array *effect_data = {0}; /* 0x0087abdc */
data_array *effect_location_data = {0}; /* 0x0087abe0 */
data_array *decal_data = {0}; /* 0x0087abe4 */
data_array *contrail_point_data = {0}; /* 0x0087abe8 */
data_array *contrail_data = {0}; /* 0x0087abec */
data_array *device_groups = {0}; /* 0x0087abf0 */
uint8_t console_debug_flag_0 = {0}; /* 0x0087ac00 */
uint8_t error_file_enabled = {0}; /* 0x0087ac01 */
uint8_t console_debug_flag_4 = {0}; /* 0x0087ac04 */
uint8_t console_debug_flag_5 = {0}; /* 0x0087ac05 */
uint8_t debug_log_level = {0}; /* 0x0087ac06 */
int16_t console_debug_word_8 = {0}; /* 0x0087ac08 */
uint8_t *hs_camera_control_pointer = {0}; /* 0x0087bc0c */
tag_instance *tag_instances = {0}; /* 0x0087bc14 */
network_pending_connection network_pending_connections[k_network_pending_connection_count] = {0}; /* 0x0087bc20 */
network_summary_statistics network_summary_stats = {0}; /* 0x0087bea0 */
network_connection_statistics network_connection_stats[k_network_connection_stats_count] = {0}; /* 0x0087bec0 */
data_array *prop_data = {0}; /* 0x008802c0 */
encounter_platoon_state *encounter_platoon_states = {0}; /* 0x008802c4 */
data_array *encounter_data = {0}; /* 0x008802c8 */
encounter_squad_state *encounter_squad_states = {0}; /* 0x008802cc */
data_array *ai_pursuit_data = {0}; /* 0x008802d0 */
data_array *ai_conversation_data = {0}; /* 0x008802d4 */
int16_t conversation_index_lookup[58] = {0}; /* 0x008802e0 */
ai_globals *ai_globals_ptr = {0}; /* 0x00880354 */
data_array *swarm_component_data = {0}; /* 0x00880358 */
data_array *swarm_data = {0}; /* 0x0088035c */
data_array *actor_data = {0}; /* 0x00880360 */
float actor_avoidance_samples_a[16][7] = {0}; /* 0x00880380 */
float actor_avoidance_circle[8][3] = {0}; /* 0x00880540 */
float actor_avoidance_samples_b[9][7] = {0}; /* 0x008805a0 */

}
