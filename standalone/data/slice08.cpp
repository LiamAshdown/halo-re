/* standalone/data/slice08.cpp -- All definitions sit in one extern "C" block: the ordered sections, the /alternatename pragmas and src/ reach these objects by their unmangled C names. */
#include <stdint.h>
#include "crt.h"
#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "interface.h"
#include "rasterizer.h"
#include "structures.h"
#include "units.h"
#include "cutscene.h"
#include "main.h"
#include "render.h"
#include "networking.h"
#include "saved_games.h"
#include "hs.h"

extern "C" {

// standalone/data/slice08.cpp -- engine globals 0x00719772..0x00721eb8 as extern "C" definitions (was EQU symbols in
// standalone/globals.asm). All zero-initialised: this address range lies past the end of the initialised .data of the
// retail image (the BSS of the loader's reserve), so no initial bytes and no pointers to other globals.
//
// Layout: the engine zero-fills / scans some of these as one block (default_profile_data's 0x1001-dword clear runs on
// into player_profile_thread; savegame_index_file .. last_multiplayer_map_path is one 0x2c7-dword block;
// variant_write_request_state's 0x29-dword clear covers the three symbols after it; network_bandwidth_graph_globals is
// followed by its 0x200-byte label buffer). To keep every such overlap behaving exactly like the original, the whole
// range is emitted as ONE contiguous run in a dedicated section, in address order, each object followed by an explicit
// pad variable for the bytes the original had up to the next symbol, so every relative offset equals the original's.
// tools/globals_check_slice08.py verifies that against the link map.

// Each object has an explicit natural alignment (an array would otherwise get 4) and its own section ".g08$NNNN"; the linker sorts the "$" group by name and concatenates it into ".g08",
// which fixes the order (the compiler alone does not keep definition order inside one section)
//.

// 0x00719770: section base marker, keeps the run congruent with the original addresses modulo 16
#pragma section(".g08$0001", read, write)
__declspec(allocate(".g08$0001")) __declspec(align(16)) uint8_t g08_base_marker[2] = {0}; // 
#pragma section(".g08$0002", read, write)
__declspec(allocate(".g08$0002")) __declspec(align(2)) int16_t global_00719772 = {0}; // 0x00719772
#pragma section(".g08$0003", read, write)
__declspec(allocate(".g08$0003")) __declspec(align(4)) uint32_t unknown_00719774 = {0}; // 0x00719774
#pragma section(".g08$0004", read, write)
__declspec(allocate(".g08$0004")) __declspec(align(1)) uint8_t selected_level_pending_00719778 = {0}; // 0x00719778
#pragma section(".g08$0005", read, write)
__declspec(allocate(".g08$0005")) __declspec(align(1)) char unknown_00719779[255] = {0}; // 0x00719779
#pragma section(".g08$0006", read, write)
__declspec(allocate(".g08$0006")) __declspec(align(1)) uint8_t selected_level_active_00719878 = {0}; // 0x00719878
#pragma section(".g08$0007", read, write)
__declspec(allocate(".g08$0007")) __declspec(align(1)) char network_build_string[256] = {0}; // 0x00719879
#pragma section(".g08$0008", read, write)
__declspec(allocate(".g08$0008")) __declspec(align(1)) uint8_t unknown_00719979[256] = {0}; // 0x00719979
#pragma section(".g08$0009", read, write)
__declspec(allocate(".g08$0009")) __declspec(align(1)) uint8_t chat_state_00719a79 = {0}; // 0x00719a79
#pragma section(".g08$0010", read, write)
__declspec(allocate(".g08$0010")) __declspec(align(1)) uint8_t chat_state_00719a7a = {0}; // 0x00719a7a
#pragma section(".g08$0011", read, write)
__declspec(allocate(".g08$0011")) __declspec(align(1)) uint8_t g08_pad_00719a7a[31] = {0}; // 0x00719a7b pad to next symbol
#pragma section(".g08$0012", read, write)
__declspec(allocate(".g08$0012")) __declspec(align(1)) uint8_t chat_state_00719a9a = {0}; // 0x00719a9a
#pragma section(".g08$0013", read, write)
__declspec(allocate(".g08$0013")) __declspec(align(1)) uint8_t g08_pad_00719a9a[14] = {0}; // 0x00719a9b pad to next symbol
#pragma section(".g08$0014", read, write)
__declspec(allocate(".g08$0014")) __declspec(align(1)) uint8_t debug_print_safety_checks = {0}; // 0x00719aa9
#pragma section(".g08$0015", read, write)
__declspec(allocate(".g08$0015")) __declspec(align(1)) uint8_t g08_pad_00719aa9[2] = {0}; // 0x00719aaa pad to next symbol
#pragma section(".g08$0016", read, write)
__declspec(allocate(".g08$0016")) __declspec(align(2)) int16_t unknown_00719aac = {0}; // 0x00719aac
#pragma section(".g08$0017", read, write)
__declspec(allocate(".g08$0017")) __declspec(align(1)) uint8_t g08_pad_00719aac[2] = {0}; // 0x00719aae pad to next symbol
#pragma section(".g08$0018", read, write)
__declspec(allocate(".g08$0018")) main_frame_rate_average frame_rate_average_data = {0}; // 0x00719ab0
#pragma section(".g08$0019", read, write)
__declspec(allocate(".g08$0019")) __declspec(align(1)) uint8_t timedemo_globals_data[sizeof(timedemo_globals)] = {0}; // 0x00719afc, a timedemo_globals
#pragma section(".g08$0020", read, write)
__declspec(allocate(".g08$0020")) __declspec(align(4)) void * connect_thread = {0}; // 0x00719b64
#pragma section(".g08$0021", read, write)
__declspec(allocate(".g08$0021")) __declspec(align(4)) int32_t hostname_resolve_complete = {0}; // 0x00719b68
#pragma section(".g08$0022", read, write)
__declspec(allocate(".g08$0022")) __declspec(align(4)) void * hostname_resolve_result = {0}; // 0x00719b6c
#pragma section(".g08$0023", read, write)
__declspec(allocate(".g08$0023")) render_view render_views[2] = {0}; // 0x00719b70
#pragma section(".g08$0024", read, write)
__declspec(allocate(".g08$0024")) __declspec(align(1)) uint8_t g08_pad_00719b70[4] = {0}; // 0x00719cc8 pad to next symbol
#pragma section(".g08$0025", read, write)
__declspec(allocate(".g08$0025")) __declspec(align(4)) int32_t player_effect_reentry_count = {0}; // 0x00719ccc
#pragma section(".g08$0026", read, write)
__declspec(allocate(".g08$0026")) __declspec(align(4)) uint32_t random_seed_global = {0}; // 0x00719cd0
#pragma section(".g08$0027", read, write)
__declspec(allocate(".g08$0027")) __declspec(align(4)) uint32_t effect_random_seed = {0}; // 0x00719cd4
#pragma section(".g08$0028", read, write)
__declspec(allocate(".g08$0028")) __declspec(align(1)) uint8_t crc32_lookup_table_initialized = {0}; // 0x00719cd8
#pragma section(".g08$0029", read, write)
__declspec(allocate(".g08$0029")) __declspec(align(1)) uint8_t g08_pad_00719cd8[7] = {0}; // 0x00719cd9 pad to next symbol
#pragma section(".g08$0030", read, write)
__declspec(allocate(".g08$0030")) network_bandwidth_graph network_bandwidth_graph_globals = {0}; // 0x00719ce0
#pragma section(".g08$0031", read, write)
__declspec(allocate(".g08$0031")) __declspec(align(1)) uint8_t g08_pad_00719ce0[512] = {0}; // 0x0071c0c0 pad to next symbol
#pragma section(".g08$0032", read, write)
__declspec(allocate(".g08$0032")) __declspec(align(1)) uint8_t network_action_apply_active = {0}; // 0x0071c2c0
#pragma section(".g08$0033", read, write)
__declspec(allocate(".g08$0033")) __declspec(align(1)) uint8_t network_channel_table_default_flag = {0}; // 0x0071c2c1
#pragma section(".g08$0034", read, write)
__declspec(allocate(".g08$0034")) __declspec(align(1)) uint8_t network_session_active = {0}; // 0x0071c2c2
#pragma section(".g08$0035", read, write)
__declspec(allocate(".g08$0035")) __declspec(align(1)) uint8_t g08_pad_0071c2c2[1] = {0}; // 0x0071c2c3 pad to next symbol
#pragma section(".g08$0036", read, write)
__declspec(allocate(".g08$0036")) __declspec(align(4)) uint32_t network_ping_debug_last_sample = {0}; // 0x0071c2c4
#pragma section(".g08$0037", read, write)
__declspec(allocate(".g08$0037")) __declspec(align(1)) uint8_t network_channel_service_backoff_bypass = {0}; // 0x0071c2c8
#pragma section(".g08$0038", read, write)
__declspec(allocate(".g08$0038")) __declspec(align(1)) uint8_t g08_pad_0071c2c8[3] = {0}; // 0x0071c2c9 pad to next symbol
#pragma section(".g08$0039", read, write)
__declspec(allocate(".g08$0039")) __declspec(align(4)) int32_t network_bit_chunk_size = {0}; // 0x0071c2cc
#pragma section(".g08$0040", read, write)
__declspec(allocate(".g08$0040")) __declspec(align(1)) uint8_t port_overridden = {0}; // 0x0071c2d0
#pragma section(".g08$0041", read, write)
__declspec(allocate(".g08$0041")) __declspec(align(1)) uint8_t g08_pad_0071c2d0[3] = {0}; // 0x0071c2d1 pad to next symbol
#pragma section(".g08$0042", read, write)
__declspec(allocate(".g08$0042")) __declspec(align(4)) void * network_server = {0}; // 0x0071c2d4
#pragma section(".g08$0043", read, write)
__declspec(allocate(".g08$0043")) __declspec(align(4)) void * network_client = {0}; // 0x0071c2d8
#pragma section(".g08$0044", read, write)
__declspec(allocate(".g08$0044")) __declspec(align(1)) uint8_t network_disconnect_timeout_flag = {0}; // 0x0071c2dc
#pragma section(".g08$0045", read, write)
__declspec(allocate(".g08$0045")) __declspec(align(1)) uint8_t network_server_host_valid = {0}; // 0x0071c2dd
#pragma section(".g08$0046", read, write)
__declspec(allocate(".g08$0046")) __declspec(align(1)) uint8_t network_host_handoff_requested = {0}; // 0x0071c2de
#pragma section(".g08$0047", read, write)
__declspec(allocate(".g08$0047")) __declspec(align(1)) uint8_t g08_pad_0071c2de[1] = {0}; // 0x0071c2df pad to next symbol
#pragma section(".g08$0048", read, write)
__declspec(allocate(".g08$0048")) __declspec(align(4)) int32_t update_server_last_tick_ms = {0}; // 0x0071c2e0
#pragma section(".g08$0049", read, write)
__declspec(allocate(".g08$0049")) __declspec(align(1)) uint8_t update_server_pending_flush = {0}; // 0x0071c2e4
#pragma section(".g08$0050", read, write)
__declspec(allocate(".g08$0050")) __declspec(align(1)) uint8_t update_server_history_index = {0}; // 0x0071c2e5
#pragma section(".g08$0051", read, write)
__declspec(allocate(".g08$0051")) __declspec(align(1)) uint8_t g08_pad_0071c2e5[6] = {0}; // 0x0071c2e6 pad to next symbol
#pragma section(".g08$0052", read, write)
__declspec(allocate(".g08$0052")) __declspec(align(1)) uint8_t network_session_active2 = {0}; // 0x0071c2ec
#pragma section(".g08$0053", read, write)
__declspec(allocate(".g08$0053")) __declspec(align(1)) uint8_t g08_pad_0071c2ec[3] = {0}; // 0x0071c2ed pad to next symbol
#pragma section(".g08$0054", read, write)
__declspec(allocate(".g08$0054")) __declspec(align(4)) int32_t network_server_status_last_print_ms = {0}; // 0x0071c2f0
#pragma section(".g08$0055", read, write)
__declspec(allocate(".g08$0055")) __declspec(align(2)) uint16_t network_server_password[8] = {0}; // 0x0071c2f4
#pragma section(".g08$0056", read, write)
__declspec(allocate(".g08$0056")) __declspec(align(1)) uint8_t network_server_password_is_default = {0}; // 0x0071c304
#pragma section(".g08$0057", read, write)
__declspec(allocate(".g08$0057")) __declspec(align(1)) uint8_t g08_pad_0071c304[1] = {0}; // 0x0071c305 pad to next symbol
#pragma section(".g08$0058", read, write)
__declspec(allocate(".g08$0058")) __declspec(align(1)) uint8_t network_single_flag_force_reset_value = {0}; // 0x0071c306
#pragma section(".g08$0059", read, write)
__declspec(allocate(".g08$0059")) __declspec(align(1)) uint8_t g08_pad_0071c306[1] = {0}; // 0x0071c307 pad to next symbol
#pragma section(".g08$0060", read, write)
__declspec(allocate(".g08$0060")) __declspec(align(1)) char network_banlist_full_path[260] = {0}; // 0x0071c308
#pragma section(".g08$0061", read, write)
__declspec(allocate(".g08$0061")) __declspec(align(4)) int32_t sv_friendly_fire_mode = {0}; // 0x0071c40c
#pragma section(".g08$0062", read, write)
__declspec(allocate(".g08$0062")) __declspec(align(1)) char sv_rcon_password_value[9] = {0}; // 0x0071c410
#pragma section(".g08$0063", read, write)
__declspec(allocate(".g08$0063")) __declspec(align(1)) uint8_t unit_updates_suppressed = {0}; // 0x0071c419
#pragma section(".g08$0064", read, write)
__declspec(allocate(".g08$0064")) __declspec(align(1)) uint8_t g08_pad_0071c419[6] = {0}; // 0x0071c41a pad to next symbol
#pragma section(".g08$0065", read, write)
__declspec(allocate(".g08$0065")) __declspec(align(2)) uint16_t local_player_name_filter[1024] = {0}; // 0x0071c420
#pragma section(".g08$0066", read, write)
__declspec(allocate(".g08$0066")) __declspec(align(1)) uint8_t unknown_0071cc20[4] = {0}; // 0x0071cc20
#pragma section(".g08$0067", read, write)
__declspec(allocate(".g08$0067")) __declspec(align(4)) int32_t network_scenario_round_counter_b = {0}; // 0x0071cc24
#pragma section(".g08$0068", read, write)
__declspec(allocate(".g08$0068")) __declspec(align(1)) uint8_t g08_pad_0071cc24[896] = {0}; // 0x0071cc28 pad to next symbol
#pragma section(".g08$0069", read, write)
__declspec(allocate(".g08$0069")) __declspec(align(1)) uint8_t message_delta_parameters_enabled = {0}; // 0x0071cfa8
#pragma section(".g08$0070", read, write)
__declspec(allocate(".g08$0070")) __declspec(align(1)) uint8_t g08_pad_0071cfa8[3] = {0}; // 0x0071cfa9 pad to next symbol
#pragma section(".g08$0071", read, write)
__declspec(allocate(".g08$0071")) __declspec(align(4)) int32_t message_delta_parameters_protocol_sequence = {0}; // 0x0071cfac
#pragma section(".g08$0072", read, write)
__declspec(allocate(".g08$0072")) __declspec(align(4)) int32_t message_delta_parameter_count = {0}; // 0x0071cfb0
#pragma section(".g08$0073", read, write)
__declspec(allocate(".g08$0073")) __declspec(align(1)) uint8_t message_delta_parameters_sending = {0}; // 0x0071cfb4
#pragma section(".g08$0074", read, write)
__declspec(allocate(".g08$0074")) __declspec(align(1)) uint8_t g08_pad_0071cfb4[3] = {0}; // 0x0071cfb5 pad to next symbol
#pragma section(".g08$0075", read, write)
__declspec(allocate(".g08$0075")) __declspec(align(4)) void * lights_enabled = {0}; // 0x0071cfb8
#pragma section(".g08$0076", read, write)
__declspec(allocate(".g08$0076")) __declspec(align(1)) uint8_t physics_disable_integration = {0}; // 0x0071cfbc
#pragma section(".g08$0077", read, write)
__declspec(allocate(".g08$0077")) __declspec(align(1)) uint8_t render_clip_warning = {0}; // 0x0071cfbd
#pragma section(".g08$0078", read, write)
__declspec(allocate(".g08$0078")) __declspec(align(1)) uint8_t rendered_objects_full_warning = {0}; // 0x0071cfbe
#pragma section(".g08$0079", read, write)
__declspec(allocate(".g08$0079")) __declspec(align(1)) uint8_t build_sprite_group_warning = {0}; // 0x0071cfbf
#pragma section(".g08$0080", read, write)
__declspec(allocate(".g08$0080")) __declspec(align(4)) void * rasterizer_model_ambient_reflection_tint = {0}; // 0x0071cfc0
#pragma section(".g08$0081", read, write)
__declspec(allocate(".g08$0081")) __declspec(align(4)) void * cinematic_screen_effect_state = {0}; // 0x0071cfc4
#pragma section(".g08$0082", read, write)
__declspec(allocate(".g08$0082")) __declspec(align(1)) uint8_t g08_pad_0071cfc4[8] = {0}; // 0x0071cfc8 pad to next symbol
#pragma section(".g08$0083", read, write)
__declspec(allocate(".g08$0083")) __declspec(align(8)) int64_t frame_statistics_unknown_d0 = {0}; // 0x0071cfd0
#pragma section(".g08$0084", read, write)
__declspec(allocate(".g08$0084")) __declspec(align(8)) int64_t frame_statistics_unknown_d8 = {0}; // 0x0071cfd8
#pragma section(".g08$0085", read, write)
__declspec(allocate(".g08$0085")) __declspec(align(1)) uint8_t g08_pad_0071cfd8[8] = {0}; // 0x0071cfe0 pad to next symbol
#pragma section(".g08$0086", read, write)
__declspec(allocate(".g08$0086")) __declspec(align(4)) uint32_t frame_statistics_times[60] = {0}; // 0x0071cfe8
#pragma section(".g08$0087", read, write)
__declspec(allocate(".g08$0087")) __declspec(align(1)) uint8_t frame_statistics_dropped[60] = {0}; // 0x0071d0d8
#pragma section(".g08$0088", read, write)
__declspec(allocate(".g08$0088")) __declspec(align(2)) int16_t frame_statistics_count = {0}; // 0x0071d114
#pragma section(".g08$0089", read, write)
__declspec(allocate(".g08$0089")) __declspec(align(1)) uint8_t g08_pad_0071d114[2] = {0}; // 0x0071d116 pad to next symbol
#pragma section(".g08$0090", read, write)
__declspec(allocate(".g08$0090")) __declspec(align(4)) int32_t frame_graph_window_width = {0}; // 0x0071d118
#pragma section(".g08$0091", read, write)
__declspec(allocate(".g08$0091")) __declspec(align(4)) int32_t frame_graph_window_height = {0}; // 0x0071d11c
#pragma section(".g08$0092", read, write)
__declspec(allocate(".g08$0092")) __declspec(align(4)) int32_t frame_statistics_key_a_latch = {0}; // 0x0071d120
#pragma section(".g08$0093", read, write)
__declspec(allocate(".g08$0093")) __declspec(align(4)) int32_t frame_statistics_key_b_latch = {0}; // 0x0071d124
#pragma section(".g08$0094", read, write)
__declspec(allocate(".g08$0094")) __declspec(align(4)) int32_t frame_graph_render_graph = {0}; // 0x0071d128
#pragma section(".g08$0095", read, write)
__declspec(allocate(".g08$0095")) __declspec(align(4)) int32_t frame_graph_render_infos = {0}; // 0x0071d12c
#pragma section(".g08$0096", read, write)
__declspec(allocate(".g08$0096")) __declspec(align(4)) int32_t frame_statistics_last_time = {0}; // 0x0071d130
#pragma section(".g08$0097", read, write)
__declspec(allocate(".g08$0097")) __declspec(align(4)) int32_t lens_flare_instance_count = {0}; // 0x0071d134
#pragma section(".g08$0098", read, write)
__declspec(allocate(".g08$0098")) __declspec(align(1)) uint8_t lens_flare_instance_overflow = {0}; // 0x0071d138
#pragma section(".g08$0099", read, write)
__declspec(allocate(".g08$0099")) __declspec(align(1)) uint8_t g08_pad_0071d138[3] = {0}; // 0x0071d139 pad to next symbol
#pragma section(".g08$0100", read, write)
__declspec(allocate(".g08$0100")) __declspec(align(4)) void * rasterizer_scratch_memory = {0}; // 0x0071d13c
#pragma section(".g08$0101", read, write)
__declspec(allocate(".g08$0101")) __declspec(align(4)) uint32_t rasterizer_scratch_memory_used = {0}; // 0x0071d140
#pragma section(".g08$0102", read, write)
__declspec(allocate(".g08$0102")) __declspec(align(4)) uint32_t text_shadow_color_argb = {0}; // 0x0071d144
#pragma section(".g08$0103", read, write)
__declspec(allocate(".g08$0103")) __declspec(align(1)) uint8_t g08_pad_0071d144[4] = {0}; // 0x0071d148 pad to next symbol
#pragma section(".g08$0104", read, write)
__declspec(allocate(".g08$0104")) __declspec(align(4)) void * transparent_geometry_groups = {0}; // 0x0071d14c
#pragma section(".g08$0105", read, write)
__declspec(allocate(".g08$0105")) __declspec(align(4)) void * transparent_geometry_groups_secondary = {0}; // 0x0071d150
#pragma section(".g08$0106", read, write)
__declspec(allocate(".g08$0106")) __declspec(align(4)) int32_t transparent_geometry_group_count = {0}; // 0x0071d154
#pragma section(".g08$0107", read, write)
__declspec(allocate(".g08$0107")) __declspec(align(4)) int32_t transparent_geometry_group_secondary_count = {0}; // 0x0071d158
#pragma section(".g08$0108", read, write)
__declspec(allocate(".g08$0108")) __declspec(align(4)) void * transparent_geometry_group_sorted_indices = {0}; // 0x0071d15c
#pragma section(".g08$0109", read, write)
__declspec(allocate(".g08$0109")) __declspec(align(4)) uint32_t unknown_0071d160 = {0}; // 0x0071d160
#pragma section(".g08$0110", read, write)
__declspec(allocate(".g08$0110")) __declspec(align(4)) void * rasterizer_globals_data = {0}; // 0x0071d164
#pragma section(".g08$0111", read, write)
__declspec(allocate(".g08$0111")) __declspec(align(4)) int32_t rasterizer_ui_render_failed = {0}; // 0x0071d168
#pragma section(".g08$0112", read, write)
__declspec(allocate(".g08$0112")) __declspec(align(1)) uint8_t rasterizer_fullscreen = {0}; // 0x0071d16c
#pragma section(".g08$0113", read, write)
__declspec(allocate(".g08$0113")) __declspec(align(1)) uint8_t rasterizer_needs_reset = {0}; // 0x0071d16d
#pragma section(".g08$0114", read, write)
__declspec(allocate(".g08$0114")) __declspec(align(1)) uint8_t rasterizer_pending_clear = {0}; // 0x0071d16e
#pragma section(".g08$0115", read, write)
__declspec(allocate(".g08$0115")) __declspec(align(1)) uint8_t rasterizer_in_scene = {0}; // 0x0071d16f
#pragma section(".g08$0116", read, write)
__declspec(allocate(".g08$0116")) __declspec(align(1)) uint8_t video_force_mode_flag = {0}; // 0x0071d170
#pragma section(".g08$0117", read, write)
__declspec(allocate(".g08$0117")) __declspec(align(1)) uint8_t g08_pad_0071d170[3] = {0}; // 0x0071d171 pad to next symbol
#pragma section(".g08$0118", read, write)
__declspec(allocate(".g08$0118")) __declspec(align(4)) void * rasterizer_device = {0}; // 0x0071d174
#pragma section(".g08$0119", read, write)
__declspec(allocate(".g08$0119")) __declspec(align(4)) void * rasterizer_direct3d = {0}; // 0x0071d178
#pragma section(".g08$0120", read, write)
__declspec(allocate(".g08$0120")) __declspec(align(1)) uint8_t g08_pad_0071d178[4] = {0}; // 0x0071d17c pad to next symbol
#pragma section(".g08$0121", read, write)
__declspec(allocate(".g08$0121")) __declspec(align(4)) uint32_t d3d_adapter = {0}; // 0x0071d180
#pragma section(".g08$0122", read, write)
__declspec(allocate(".g08$0122")) __declspec(align(4)) void * rasterizer_window_icon_dc = {0}; // 0x0071d184
#pragma section(".g08$0123", read, write)
__declspec(allocate(".g08$0123")) __declspec(align(4)) void * rasterizer_window_icon_bitmap = {0}; // 0x0071d188
#pragma section(".g08$0124", read, write)
__declspec(allocate(".g08$0124")) __declspec(align(1)) uint8_t rasterizer_widescreen_camouflage_scale = {0}; // 0x0071d18c
#pragma section(".g08$0125", read, write)
__declspec(allocate(".g08$0125")) __declspec(align(1)) uint8_t unknown_0071d18d = {0}; // 0x0071d18d
#pragma section(".g08$0126", read, write)
__declspec(allocate(".g08$0126")) __declspec(align(1)) uint8_t rasterizer_use_fx_file = {0}; // 0x0071d18e
#pragma section(".g08$0127", read, write)
__declspec(allocate(".g08$0127")) __declspec(align(1)) uint8_t g08_pad_0071d18e[1] = {0}; // 0x0071d18f pad to next symbol
#pragma section(".g08$0128", read, write)
__declspec(allocate(".g08$0128")) __declspec(align(4)) float zoom_static_tint_r = {0}; // 0x0071d190
#pragma section(".g08$0129", read, write)
__declspec(allocate(".g08$0129")) __declspec(align(4)) float zoom_static_tint_g = {0}; // 0x0071d194
#pragma section(".g08$0130", read, write)
__declspec(allocate(".g08$0130")) __declspec(align(4)) float zoom_static_tint_b = {0}; // 0x0071d198
#pragma section(".g08$0131", read, write)
__declspec(allocate(".g08$0131")) __declspec(align(4)) void * rasterizer_node_part_indices = {0}; // 0x0071d19c
#pragma section(".g08$0132", read, write)
__declspec(allocate(".g08$0132")) __declspec(align(4)) int32_t rasterizer_node_part_count = {0}; // 0x0071d1a0
#pragma section(".g08$0133", read, write)
__declspec(allocate(".g08$0133")) __declspec(align(4)) int32_t checkfpu = {0}; // 0x0071d1a4
#pragma section(".g08$0134", read, write)
__declspec(allocate(".g08$0134")) __declspec(align(4)) int32_t rasterizer_window_requested = {0}; // 0x0071d1a8
#pragma section(".g08$0135", read, write)
__declspec(allocate(".g08$0135")) __declspec(align(4)) int32_t windowed = {0}; // 0x0071d1ac
#pragma section(".g08$0136", read, write)
__declspec(allocate(".g08$0136")) __declspec(align(1)) uint8_t unknown_0071d1b0 = {0}; // 0x0071d1b0
#pragma section(".g08$0137", read, write)
__declspec(allocate(".g08$0137")) __declspec(align(1)) uint8_t rasterizer_render_target_capture_requested = {0}; // 0x0071d1b1
#pragma section(".g08$0138", read, write)
__declspec(allocate(".g08$0138")) __declspec(align(1)) uint8_t rasterizer_render_target_capture_done = {0}; // 0x0071d1b2
#pragma section(".g08$0139", read, write)
__declspec(allocate(".g08$0139")) __declspec(align(1)) uint8_t g08_pad_0071d1b2[1] = {0}; // 0x0071d1b3 pad to next symbol
#pragma section(".g08$0140", read, write)
__declspec(allocate(".g08$0140")) __declspec(align(2)) uint16_t unknown_0071d1b4 = {0}; // 0x0071d1b4
#pragma section(".g08$0141", read, write)
__declspec(allocate(".g08$0141")) __declspec(align(1)) uint8_t g08_pad_0071d1b4[6] = {0}; // 0x0071d1b6 pad to next symbol
#pragma section(".g08$0142", read, write)
__declspec(allocate(".g08$0142")) __declspec(align(4)) void * rasterizer_decal_vertex_cache = {0}; // 0x0071d1bc
#pragma section(".g08$0143", read, write)
__declspec(allocate(".g08$0143")) __declspec(align(4)) void * rasterizer_decal_vertex_cache_handle = {0}; // 0x0071d1c0
#pragma section(".g08$0144", read, write)
__declspec(allocate(".g08$0144")) __declspec(align(1)) uint8_t unknown_0071d1c4 = {0}; // 0x0071d1c4
#pragma section(".g08$0145", read, write)
__declspec(allocate(".g08$0145")) __declspec(align(1)) uint8_t g08_pad_0071d1c4[3] = {0}; // 0x0071d1c5 pad to next symbol
#pragma section(".g08$0146", read, write)
__declspec(allocate(".g08$0146")) __declspec(align(4)) void * rasterizer_detail_object_vertex_buffer = {0}; // 0x0071d1c8
#pragma section(".g08$0147", read, write)
__declspec(allocate(".g08$0147")) __declspec(align(1)) uint8_t rasterizer_dynamic_index_overflow = {0}; // 0x0071d1cc
#pragma section(".g08$0148", read, write)
__declspec(allocate(".g08$0148")) __declspec(align(1)) uint8_t rasterizer_dynamic_vertex_overflow = {0}; // 0x0071d1cd
#pragma section(".g08$0149", read, write)
__declspec(allocate(".g08$0149")) __declspec(align(1)) uint8_t transparent_geometry_group_overflow_a = {0}; // 0x0071d1ce
#pragma section(".g08$0150", read, write)
__declspec(allocate(".g08$0150")) __declspec(align(1)) uint8_t g08_pad_0071d1ce[1] = {0}; // 0x0071d1cf pad to next symbol
#pragma section(".g08$0151", read, write)
__declspec(allocate(".g08$0151")) __declspec(align(4)) void * rasterizer_active_environment_effect = {0}; // 0x0071d1d0
#pragma section(".g08$0152", read, write)
__declspec(allocate(".g08$0152")) __declspec(align(1)) uint8_t g08_pad_0071d1d0[4] = {0}; // 0x0071d1d4 pad to next symbol
#pragma section(".g08$0153", read, write)
__declspec(allocate(".g08$0153")) __declspec(align(4)) float rasterizer_projected_light_luminance = {0}; // 0x0071d1d8
#pragma section(".g08$0154", read, write)
__declspec(allocate(".g08$0154")) __declspec(align(1)) uint8_t transparent_geometry_group_overflow_b = {0}; // 0x0071d1dc
#pragma section(".g08$0155", read, write)
__declspec(allocate(".g08$0155")) __declspec(align(1)) uint8_t g08_pad_0071d1dc[3] = {0}; // 0x0071d1dd pad to next symbol
#pragma section(".g08$0156", read, write)
__declspec(allocate(".g08$0156")) __declspec(align(4)) int32_t rasterizer_gamma_exponent = {0}; // 0x0071d1e0
#pragma section(".g08$0157", read, write)
__declspec(allocate(".g08$0157")) __declspec(align(4)) int32_t video_gamma_current = {0}; // 0x0071d1e4
#pragma section(".g08$0158", read, write)
__declspec(allocate(".g08$0158")) __declspec(align(1)) uint8_t rasterizer_gamma_disabled = {0}; // 0x0071d1e8
#pragma section(".g08$0159", read, write)
__declspec(allocate(".g08$0159")) __declspec(align(1)) uint8_t g08_pad_0071d1e8[3] = {0}; // 0x0071d1e9 pad to next symbol
#pragma section(".g08$0160", read, write)
__declspec(allocate(".g08$0160")) __declspec(align(4)) int32_t rasterizer_gamma_captured = {0}; // 0x0071d1ec
#pragma section(".g08$0161", read, write)
__declspec(allocate(".g08$0161")) __declspec(align(4)) void * rasterizer_active_model_context = {0}; // 0x0071d1f0
#pragma section(".g08$0162", read, write)
__declspec(allocate(".g08$0162")) __declspec(align(1)) uint8_t rasterizer_model_scratch_valid = {0}; // 0x0071d1f4
#pragma section(".g08$0163", read, write)
__declspec(allocate(".g08$0163")) __declspec(align(1)) uint8_t g08_pad_0071d1f4[3] = {0}; // 0x0071d1f5 pad to next symbol
#pragma section(".g08$0164", read, write)
__declspec(allocate(".g08$0164")) __declspec(align(2)) int16_t rasterizer_active_model_mode = {0}; // 0x0071d1f8
#pragma section(".g08$0165", read, write)
__declspec(allocate(".g08$0165")) __declspec(align(1)) uint8_t unknown_0071d1fa = {0}; // 0x0071d1fa
#pragma section(".g08$0166", read, write)
__declspec(allocate(".g08$0166")) __declspec(align(1)) uint8_t unknown_0071d1fb = {0}; // 0x0071d1fb
#pragma section(".g08$0167", read, write)
__declspec(allocate(".g08$0167")) __declspec(align(1)) uint8_t unknown_0071d1fc = {0}; // 0x0071d1fc
#pragma section(".g08$0168", read, write)
__declspec(allocate(".g08$0168")) __declspec(align(1)) uint8_t unknown_0071d1fd = {0}; // 0x0071d1fd
#pragma section(".g08$0169", read, write)
__declspec(allocate(".g08$0169")) __declspec(align(1)) uint8_t rasterizer_camouflage_fade_active = {0}; // 0x0071d1fe
#pragma section(".g08$0170", read, write)
__declspec(allocate(".g08$0170")) __declspec(align(1)) uint8_t g08_pad_0071d1fe[1] = {0}; // 0x0071d1ff pad to next symbol
#pragma section(".g08$0171", read, write)
__declspec(allocate(".g08$0171")) __declspec(align(4)) float rasterizer_camouflage_fade = {0}; // 0x0071d200
#pragma section(".g08$0172", read, write)
__declspec(allocate(".g08$0172")) __declspec(align(1)) uint8_t transparent_geometry_group_overflow_c = {0}; // 0x0071d204
#pragma section(".g08$0173", read, write)
__declspec(allocate(".g08$0173")) __declspec(align(1)) uint8_t rasterizer_motion_sensor_ready = {0}; // 0x0071d205
#pragma section(".g08$0174", read, write)
__declspec(allocate(".g08$0174")) __declspec(align(1)) uint8_t g08_pad_0071d205[2] = {0}; // 0x0071d206 pad to next symbol
#pragma section(".g08$0175", read, write)
__declspec(allocate(".g08$0175")) __declspec(align(4)) void * rasterizer_render_target_index_buffer = {0}; // 0x0071d208
#pragma section(".g08$0176", read, write)
__declspec(allocate(".g08$0176")) __declspec(align(4)) void * rasterizer_render_target_vertex_buffer = {0}; // 0x0071d20c
#pragma section(".g08$0177", read, write)
__declspec(allocate(".g08$0177")) __declspec(align(4)) uint32_t screen_effect_techniques[11] = {0}; // 0x0071d210
#pragma section(".g08$0178", read, write)
__declspec(allocate(".g08$0178")) __declspec(align(4)) void * screen_flash_techniques = {0}; // 0x0071d23c
#pragma section(".g08$0179", read, write)
__declspec(allocate(".g08$0179")) __declspec(align(1)) uint8_t g08_pad_0071d23c[20] = {0}; // 0x0071d240 pad to next symbol
#pragma section(".g08$0180", read, write)
__declspec(allocate(".g08$0180")) __declspec(align(4)) void * rasterizer_effect_pool = {0}; // 0x0071d254
#pragma section(".g08$0181", read, write)
__declspec(allocate(".g08$0181")) __declspec(align(4)) int32_t rasterizer_vertex_buffer_slot_high_water = {0}; // 0x0071d258
#pragma section(".g08$0182", read, write)
__declspec(allocate(".g08$0182")) __declspec(align(4)) int32_t rasterizer_vertex_buffer_slot_count = {0}; // 0x0071d25c
#pragma section(".g08$0183", read, write)
__declspec(allocate(".g08$0183")) __declspec(align(4)) void * rasterizer_object_shadow_model_context = {0}; // 0x0071d260
#pragma section(".g08$0184", read, write)
__declspec(allocate(".g08$0184")) __declspec(align(1)) uint8_t rasterizer_object_shadow_prepared = {0}; // 0x0071d264
#pragma section(".g08$0185", read, write)
__declspec(allocate(".g08$0185")) __declspec(align(1)) uint8_t rasterizer_object_shadow_model_active = {0}; // 0x0071d265
#pragma section(".g08$0186", read, write)
__declspec(allocate(".g08$0186")) __declspec(align(1)) uint8_t g08_pad_0071d265[10] = {0}; // 0x0071d266 pad to next symbol
#pragma section(".g08$0187", read, write)
__declspec(allocate(".g08$0187")) __declspec(align(4)) void * rasterizer_misc_vertex_buffer = {0}; // 0x0071d270
#pragma section(".g08$0188", read, write)
__declspec(allocate(".g08$0188")) __declspec(align(1)) uint8_t rasterizer_secondary_groups_drawn = {0}; // 0x0071d274
#pragma section(".g08$0189", read, write)
__declspec(allocate(".g08$0189")) __declspec(align(1)) uint8_t unknown_0071d275 = {0}; // 0x0071d275
#pragma section(".g08$0190", read, write)
__declspec(allocate(".g08$0190")) __declspec(align(1)) uint8_t unknown_0071d276 = {0}; // 0x0071d276
#pragma section(".g08$0191", read, write)
__declspec(allocate(".g08$0191")) __declspec(align(1)) uint8_t g08_pad_0071d276[1] = {0}; // 0x0071d277 pad to next symbol
#pragma section(".g08$0192", read, write)
__declspec(allocate(".g08$0192")) __declspec(align(4)) void * rasterizer_effect_pool_scratch = {0}; // 0x0071d278
#pragma section(".g08$0193", read, write)
__declspec(allocate(".g08$0193")) __declspec(align(1)) uint8_t transparent_geometry_group_overflow_d = {0}; // 0x0071d27c
#pragma section(".g08$0194", read, write)
__declspec(allocate(".g08$0194")) __declspec(align(1)) uint8_t g08_pad_0071d27c[3] = {0}; // 0x0071d27d pad to next symbol
#pragma section(".g08$0195", read, write)
__declspec(allocate(".g08$0195")) saved_player_profile default_profile_data = {0}; // 0x0071d280
#pragma section(".g08$0196", read, write)
__declspec(allocate(".g08$0196")) __declspec(align(1)) uint8_t g08_pad_0071d280[8192] = {0}; // 0x0071f27c pad to next symbol
#pragma section(".g08$0197", read, write)
__declspec(allocate(".g08$0197")) __declspec(align(4)) void * player_profile_thread = {0}; // 0x0072127c
#pragma section(".g08$0198", read, write)
__declspec(allocate(".g08$0198")) __declspec(align(1)) uint8_t default_player_profile_initialized = {0}; // 0x00721280
#pragma section(".g08$0199", read, write)
__declspec(allocate(".g08$0199")) __declspec(align(1)) uint8_t g08_pad_00721280[7] = {0}; // 0x00721281 pad to next symbol
#pragma section(".g08$0200", read, write)
__declspec(allocate(".g08$0200")) variant_write_request variant_write_request_state = {0}; // 0x00721288
#pragma section(".g08$0201", read, write)
__declspec(allocate(".g08$0201")) __declspec(align(4)) void * variant_write_thread = {0}; // 0x00721324
#pragma section(".g08$0202", read, write)
__declspec(allocate(".g08$0202")) __declspec(align(2)) int16_t default_game_variant_count = {0}; // 0x00721328
#pragma section(".g08$0203", read, write)
__declspec(allocate(".g08$0203")) __declspec(align(1)) uint8_t unknown_0072132a = {0}; // 0x0072132a
#pragma section(".g08$0204", read, write)
__declspec(allocate(".g08$0204")) __declspec(align(1)) uint8_t g08_pad_0072132a[5] = {0}; // 0x0072132b pad to next symbol
#pragma section(".g08$0205", read, write)
__declspec(allocate(".g08$0205")) file_reference savegame_index_file = {0}; // 0x00721330
#pragma section(".g08$0206", read, write)
__declspec(allocate(".g08$0206")) __declspec(align(4)) void * saved_game_files_mutex = {0}; // 0x0072143c
#pragma section(".g08$0207", read, write)
__declspec(allocate(".g08$0207")) __declspec(align(4)) void * savegame_index_mutex = {0}; // 0x00721440
#pragma section(".g08$0208", read, write)
__declspec(allocate(".g08$0208")) __declspec(align(2)) int16_t savegame_index_write_count = {0}; // 0x00721444
#pragma section(".g08$0209", read, write)
__declspec(allocate(".g08$0209")) __declspec(align(1)) uint8_t saved_game_files_initialized = {0}; // 0x00721446
#pragma section(".g08$0210", read, write)
__declspec(allocate(".g08$0210")) __declspec(align(1)) uint8_t savegame_index_dirty = {0}; // 0x00721447
#pragma section(".g08$0211", read, write)
__declspec(allocate(".g08$0211")) __declspec(align(1)) uint8_t saved_game_index_file_open = {0}; // 0x00721448
#pragma section(".g08$0212", read, write)
__declspec(allocate(".g08$0212")) __declspec(align(1)) char saved_game_root_directory[256] = {0}; // 0x00721449
#pragma section(".g08$0213", read, write)
__declspec(allocate(".g08$0213")) __declspec(align(1)) char savegames_directory[256] = {0}; // 0x00721549
#pragma section(".g08$0214", read, write)
__declspec(allocate(".g08$0214")) __declspec(align(1)) char saved_directory[256] = {0}; // 0x00721649
#pragma section(".g08$0215", read, write)
__declspec(allocate(".g08$0215")) __declspec(align(1)) char player_profiles_directory[256] = {0}; // 0x00721749
#pragma section(".g08$0216", read, write)
__declspec(allocate(".g08$0216")) __declspec(align(1)) char default_player_profiles_directory[256] = {0}; // 0x00721849
#pragma section(".g08$0217", read, write)
__declspec(allocate(".g08$0217")) __declspec(align(1)) char playlists_directory[256] = {0}; // 0x00721949
#pragma section(".g08$0218", read, write)
__declspec(allocate(".g08$0218")) __declspec(align(1)) char default_playlists_directory[256] = {0}; // 0x00721a49
#pragma section(".g08$0219", read, write)
__declspec(allocate(".g08$0219")) __declspec(align(1)) char last_profile_path[256] = {0}; // 0x00721b49
#pragma section(".g08$0220", read, write)
__declspec(allocate(".g08$0220")) __declspec(align(1)) char last_game_variant_path[256] = {0}; // 0x00721c49
#pragma section(".g08$0221", read, write)
__declspec(allocate(".g08$0221")) __declspec(align(1)) char last_multiplayer_map_path[256] = {0}; // 0x00721d49
#pragma section(".g08$0222", read, write)
__declspec(allocate(".g08$0222")) __declspec(align(1)) uint8_t g08_pad_00721d49[3] = {0}; // 0x00721e49 pad to next symbol
#pragma section(".g08$0223", read, write)
__declspec(allocate(".g08$0223")) __declspec(align(1)) uint8_t material_table_warning_issued = {0}; // 0x00721e4c
#pragma section(".g08$0224", read, write)
__declspec(allocate(".g08$0224")) __declspec(align(1)) uint8_t g08_pad_00721e4c[3] = {0}; // 0x00721e4d pad to next symbol
#pragma section(".g08$0225", read, write)
__declspec(allocate(".g08$0225")) __declspec(align(4)) int32_t numeric_countdown_timer_remaining_ms = {0}; // 0x00721e50
#pragma section(".g08$0226", read, write)
__declspec(allocate(".g08$0226")) __declspec(align(1)) uint8_t numeric_countdown_timer_running = {0}; // 0x00721e54
#pragma section(".g08$0227", read, write)
__declspec(allocate(".g08$0227")) __declspec(align(1)) uint8_t g08_pad_00721e54[3] = {0}; // 0x00721e55 pad to next symbol
#pragma section(".g08$0228", read, write)
__declspec(allocate(".g08$0228")) __declspec(align(4)) int32_t numeric_countdown_timer_last_update_ms = {0}; // 0x00721e58
#pragma section(".g08$0229", read, write)
__declspec(allocate(".g08$0229")) __declspec(align(4)) uint32_t cpu_features = {0}; // 0x00721e5c
#pragma section(".g08$0230", read, write)
__declspec(allocate(".g08$0230")) __declspec(align(4)) uint32_t cpu_extended_features = {0}; // 0x00721e60
#pragma section(".g08$0231", read, write)
__declspec(allocate(".g08$0231")) __declspec(align(4)) uint32_t cpu_signature = {0}; // 0x00721e64
#pragma section(".g08$0232", read, write)
__declspec(allocate(".g08$0232")) __declspec(align(4)) uint32_t cpu_l1_tlb_large = {0}; // 0x00721e68
#pragma section(".g08$0233", read, write)
__declspec(allocate(".g08$0233")) __declspec(align(4)) uint32_t cpu_l1_tlb_4k = {0}; // 0x00721e6c
#pragma section(".g08$0234", read, write)
__declspec(allocate(".g08$0234")) __declspec(align(4)) uint32_t cpu_l1_data_cache = {0}; // 0x00721e70
#pragma section(".g08$0235", read, write)
__declspec(allocate(".g08$0235")) __declspec(align(4)) uint32_t cpu_l1_code_cache = {0}; // 0x00721e74
#pragma section(".g08$0236", read, write)
__declspec(allocate(".g08$0236")) __declspec(align(4)) uint32_t cpu_l2_tlb_large = {0}; // 0x00721e78
#pragma section(".g08$0237", read, write)
__declspec(allocate(".g08$0237")) __declspec(align(4)) uint32_t cpu_l2_tlb_4k = {0}; // 0x00721e7c
#pragma section(".g08$0238", read, write)
__declspec(allocate(".g08$0238")) __declspec(align(4)) uint32_t cpu_l2_cache = {0}; // 0x00721e80
#pragma section(".g08$0239", read, write)
__declspec(allocate(".g08$0239")) __declspec(align(4)) uint32_t cpu_l2_unknown = {0}; // 0x00721e84
#pragma section(".g08$0240", read, write)
__declspec(allocate(".g08$0240")) __declspec(align(4)) int32_t cpu_identification_state = {0}; // 0x00721e88
#pragma section(".g08$0241", read, write)
__declspec(allocate(".g08$0241")) __declspec(align(1)) uint8_t shell_application_inactive = {0}; // 0x00721e8c
#pragma section(".g08$0242", read, write)
__declspec(allocate(".g08$0242")) __declspec(align(1)) uint8_t shell_window_proc_bypass = {0}; // 0x00721e8d
#pragma section(".g08$0243", read, write)
__declspec(allocate(".g08$0243")) __declspec(align(1)) uint8_t g08_pad_00721e8d[2] = {0}; // 0x00721e8e pad to next symbol
#pragma section(".g08$0244", read, write)
__declspec(allocate(".g08$0244")) __declspec(align(4)) void * shell_argv = {0}; // 0x00721e90
#pragma section(".g08$0245", read, write)
__declspec(allocate(".g08$0245")) __declspec(align(4)) int32_t shell_argc = {0}; // 0x00721e94
#pragma section(".g08$0246", read, write)
__declspec(allocate(".g08$0246")) __declspec(align(4)) void * shell_direct3d = {0}; // 0x00721e98
#pragma section(".g08$0247", read, write)
__declspec(allocate(".g08$0247")) __declspec(align(4)) void * keystone_module = {0}; // 0x00721e9c
#pragma section(".g08$0248", read, write)
__declspec(allocate(".g08$0248")) __declspec(align(4)) void * unknown_00721ea0 = {0}; // 0x00721ea0
#pragma section(".g08$0249", read, write)
__declspec(allocate(".g08$0249")) __declspec(align(4)) void * chat_gui_root_handle = {0}; // 0x00721ea4
#pragma section(".g08$0250", read, write)
__declspec(allocate(".g08$0250")) __declspec(align(4)) void * keystone_current_directory = {0}; // 0x00721ea8
#pragma section(".g08$0251", read, write)
__declspec(allocate(".g08$0251")) __declspec(align(4)) void * unknown_00721eac = {0}; // 0x00721eac
#pragma section(".g08$0252", read, write)
__declspec(allocate(".g08$0252")) __declspec(align(4)) void * keystone_translate_accelerator = {0}; // 0x00721eb0
#pragma section(".g08$0253", read, write)
__declspec(allocate(".g08$0253")) __declspec(align(4)) void * unknown_00721eb4 = {0}; // 0x00721eb4
#pragma section(".g08$0254", read, write)
__declspec(allocate(".g08$0254")) __declspec(align(4)) void * unknown_00721eb8 = {0}; // 0x00721eb8

// two names at one address in the original: one object, the other symbol resolves to it
#pragma comment(linker, "/alternatename:_keystone_create=_unknown_00721ea0")
#pragma comment(linker, "/alternatename:_keystone_release=_unknown_00721eac")
#pragma comment(linker, "/alternatename:_keystone_create_window=_unknown_00721eb4")
#pragma comment(linker, "/alternatename:_chat_gui_find_object=_unknown_00721eb8")

}
