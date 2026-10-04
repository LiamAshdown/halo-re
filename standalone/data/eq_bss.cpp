/**
 * standalone/data/eq_bss.cpp -- the engine globals of the zero-initialised part of the original image (past the
 * initialised .data piece) that the C code reaches through several names, as one larger object or with block clears
 * and copies that run across several of them. They were absolute EQU symbols in standalone/globals.asm.
 *
 * Each object is in its own section ".geq$<original address>v"; the linker sorts the group by name, so the objects
 * keep the original address order. A cluster (globals with no other global between them) is one gap-free run: every
 * object spans up to the next one and the last up to the next global of the original image, and a 16-aligned pad
 * ".geq$<cluster start>" in front keeps the run congruent with the original addresses, so every offset, overrun and
 * sweep is the original one. tools/globals_check_eq.py verifies the layout.
 *
 * All definitions sit in one extern "C" block: the ordered sections, the /alternatename pragmas and src/ reach these objects by their unmangled C names.
 */
#include <stdint.h>

extern "C" {

/** 0x006a8154..0x006a8954: cache_file_current_header, cache_file_current_header_crc32 */
#pragma section(".geq$006a8154", read, write)
__declspec(allocate(".geq$006a8154")) __declspec(align(16)) uint8_t eq_pad_006a8154[4] = {0};
#pragma section(".geq$006a8154v", read, write)
__declspec(allocate(".geq$006a8154v")) __declspec(align(4)) uint8_t cache_file_current_header[100] = {0};
#pragma section(".geq$006a81b8v", read, write)
__declspec(allocate(".geq$006a81b8v")) __declspec(align(8)) uint8_t cache_file_current_header_crc32[1948] = {0};

/** 0x006ac560..0x006ac658: directors, unknown_006ac568, local_player_look_frozen and 3 more */
#pragma section(".geq$006ac560v", read, write)
__declspec(allocate(".geq$006ac560v")) __declspec(align(16)) uint8_t directors[8] = {0};
#pragma section(".geq$006ac568v", read, write)
__declspec(allocate(".geq$006ac568v")) __declspec(align(8)) uint8_t unknown_006ac568[73] = {0};
#pragma section(".geq$006ac5b1v", read, write)
__declspec(allocate(".geq$006ac5b1v")) __declspec(align(1)) uint8_t local_player_look_frozen[1] = {0};
#pragma section(".geq$006ac5b2v", read, write)
__declspec(allocate(".geq$006ac5b2v")) __declspec(align(2)) uint8_t local_player_input_frozen[110] = {0};
#pragma section(".geq$006ac620v", read, write)
__declspec(allocate(".geq$006ac620v")) __declspec(align(16)) uint8_t unknown_006ac620[4] = {0};
#pragma section(".geq$006ac624v", read, write)
__declspec(allocate(".geq$006ac624v")) __declspec(align(4)) uint8_t unknown_006ac624[52] = {0};

/** 0x006ac65c..0x006ac8f8: observers, camera_point, camera_position_y_table and 3 more */
#pragma section(".geq$006ac65c", read, write)
__declspec(allocate(".geq$006ac65c")) __declspec(align(16)) uint8_t eq_pad_006ac65c[12] = {0};
#pragma section(".geq$006ac65cv", read, write)
__declspec(allocate(".geq$006ac65cv")) __declspec(align(4)) uint8_t observers[116] = {0};
#pragma section(".geq$006ac6d0v", read, write)
__declspec(allocate(".geq$006ac6d0v")) __declspec(align(16)) uint8_t camera_point[4] = {0};
#pragma section(".geq$006ac6d4v", read, write)
__declspec(allocate(".geq$006ac6d4v")) __declspec(align(4)) uint8_t camera_position_y_table[4] = {0};
#pragma section(".geq$006ac6d8v", read, write)
__declspec(allocate(".geq$006ac6d8v")) __declspec(align(8)) uint8_t camera_position_z_table[4] = {0};
#pragma section(".geq$006ac6dcv", read, write)
__declspec(allocate(".geq$006ac6dcv")) __declspec(align(4)) uint8_t camera_leaf[4] = {0};
#pragma section(".geq$006ac6e0v", read, write)
__declspec(allocate(".geq$006ac6e0v")) __declspec(align(16)) uint8_t local_player_0_cluster_index[536] = {0};

/** 0x006b0b80..0x006b0b84: main_game_globals */
#pragma section(".geq$006b0b80v", read, write)
__declspec(allocate(".geq$006b0b80v")) __declspec(align(16)) uint8_t main_game_globals[4] = {0};

/** 0x006b0e88..0x006b0ebc: ctf_team_flag_stand_position, ctf_team_flag_object, ctf_team_flag_touch_count and 6 more */
#pragma section(".geq$006b0e88", read, write)
__declspec(allocate(".geq$006b0e88")) __declspec(align(16)) uint8_t eq_pad_006b0e88[8] = {0};
#pragma section(".geq$006b0e88v", read, write)
__declspec(allocate(".geq$006b0e88v")) __declspec(align(8)) uint8_t ctf_team_flag_stand_position[8] = {0};
#pragma section(".geq$006b0e90v", read, write)
__declspec(allocate(".geq$006b0e90v")) __declspec(align(16)) uint8_t ctf_team_flag_object[8] = {0};
#pragma section(".geq$006b0e98v", read, write)
__declspec(allocate(".geq$006b0e98v")) __declspec(align(8)) uint8_t ctf_team_flag_touch_count[8] = {0};
#pragma section(".geq$006b0ea0v", read, write)
__declspec(allocate(".geq$006b0ea0v")) __declspec(align(16)) uint8_t ctf_flag_capture_limit_006b0ea0[4] = {0};
#pragma section(".geq$006b0ea4v", read, write)
__declspec(allocate(".geq$006b0ea4v")) __declspec(align(4)) uint8_t ctf_team_return_credit_active[4] = {0};
#pragma section(".geq$006b0ea8v", read, write)
__declspec(allocate(".geq$006b0ea8v")) __declspec(align(8)) uint8_t ctf_team_return_credit_ticks[8] = {0};
#pragma section(".geq$006b0eb0v", read, write)
__declspec(allocate(".geq$006b0eb0v")) __declspec(align(16)) uint8_t ctf_flag_auto_return_ticks[4] = {0};
#pragma section(".geq$006b0eb4v", read, write)
__declspec(allocate(".geq$006b0eb4v")) __declspec(align(4)) uint8_t ctf_notify_throttle_tick[4] = {0};
#pragma section(".geq$006b0eb8v", read, write)
__declspec(allocate(".geq$006b0eb8v")) __declspec(align(8)) uint8_t ctf_active_team[4] = {0};

/** 0x006b0ec0..0x006b106c: king_bucket_credit_ticks, king_bucket_last_credit_tick, king_hill_player_in_hill and 11 more */
#pragma section(".geq$006b0ec0v", read, write)
__declspec(allocate(".geq$006b0ec0v")) __declspec(align(16)) uint8_t king_bucket_credit_ticks[64] = {0};
#pragma section(".geq$006b0f00v", read, write)
__declspec(allocate(".geq$006b0f00v")) __declspec(align(16)) uint8_t king_bucket_last_credit_tick[64] = {0};
#pragma section(".geq$006b0f40v", read, write)
__declspec(allocate(".geq$006b0f40v")) __declspec(align(16)) uint8_t king_hill_player_in_hill[16] = {0};
#pragma section(".geq$006b0f50v", read, write)
__declspec(allocate(".geq$006b0f50v")) __declspec(align(16)) uint8_t king_starting_location_count[4] = {0};
#pragma section(".geq$006b0f54v", read, write)
__declspec(allocate(".geq$006b0f54v")) __declspec(align(4)) uint8_t king_hill_boundary_points[144] = {0};
#pragma section(".geq$006b0fe4v", read, write)
__declspec(allocate(".geq$006b0fe4v")) __declspec(align(4)) uint8_t king_hill_boundary_extra[96] = {0};
#pragma section(".geq$006b1044v", read, write)
__declspec(allocate(".geq$006b1044v")) __declspec(align(4)) uint8_t king_hill_boundary_center[12] = {0};
#pragma section(".geq$006b1050v", read, write)
__declspec(allocate(".geq$006b1050v")) __declspec(align(16)) uint8_t king_hill_state_globals[4] = {0};
#pragma section(".geq$006b1054v", read, write)
__declspec(allocate(".geq$006b1054v")) __declspec(align(4)) uint8_t king_hill_state_006b1054[4] = {0};
#pragma section(".geq$006b1058v", read, write)
__declspec(allocate(".geq$006b1058v")) __declspec(align(8)) uint8_t king_hill_index_006b1058[4] = {0};
#pragma section(".geq$006b105cv", read, write)
__declspec(allocate(".geq$006b105cv")) __declspec(align(4)) uint8_t king_hill_boundary_max_z[4] = {0};
#pragma section(".geq$006b1060v", read, write)
__declspec(allocate(".geq$006b1060v")) __declspec(align(16)) uint8_t king_hill_boundary_min_z[4] = {0};
#pragma section(".geq$006b1064v", read, write)
__declspec(allocate(".geq$006b1064v")) __declspec(align(4)) uint8_t king_starting_location_type[4] = {0};
#pragma section(".geq$006b1068v", read, write)
__declspec(allocate(".geq$006b1068v")) __declspec(align(8)) uint8_t king_hill_move_ticks_006b1068[4] = {0};

/** 0x006b1148..0x006b1458: the game-engine globals union (king / oddball / ctf / race views over one block) */
#pragma section(".geq$006b1148", read, write)
__declspec(allocate(".geq$006b1148")) __declspec(align(16)) uint8_t eq_pad_006b1148[8] = {0};
#pragma section(".geq$006b1148v", read, write)
__declspec(allocate(".geq$006b1148v")) __declspec(align(8)) uint8_t king_alt_score_target[4] = {0};
#pragma section(".geq$006b114cv", read, write)
__declspec(allocate(".geq$006b114cv")) __declspec(align(4)) uint8_t king_alt_team_score[64] = {0};
#pragma section(".geq$006b118cv", read, write)
__declspec(allocate(".geq$006b118cv")) __declspec(align(4)) uint8_t king_alt_player_score[64] = {0};
#pragma section(".geq$006b11ccv", read, write)
__declspec(allocate(".geq$006b11ccv")) __declspec(align(4)) uint8_t oddball_ball_timers_006b11cc[64] = {0};
#pragma section(".geq$006b120cv", read, write)
__declspec(allocate(".geq$006b120cv")) __declspec(align(4)) uint8_t king_hill_occupant_table[64] = {0};
#pragma section(".geq$006b124cv", read, write)
__declspec(allocate(".geq$006b124cv")) __declspec(align(4)) uint8_t king_hill_occupant_last_tick[68] = {0};
#pragma section(".geq$006b1290v", read, write)
__declspec(allocate(".geq$006b1290v")) __declspec(align(16)) uint8_t ctf_globals_live[68] = {0};
#pragma section(".geq$006b12d4v", read, write)
__declspec(allocate(".geq$006b12d4v")) __declspec(align(4)) uint8_t ctf_team_captured_flags_mask[64] = {0};
#pragma section(".geq$006b1314v", read, write)
__declspec(allocate(".geq$006b1314v")) __declspec(align(4)) uint8_t ctf_neutral_flag_id[4] = {0};
#pragma section(".geq$006b1318v", read, write)
__declspec(allocate(".geq$006b1318v")) __declspec(align(8)) uint8_t game_engine_bucket_scores[64] = {0};
#pragma section(".geq$006b1358v", read, write)
__declspec(allocate(".geq$006b1358v")) __declspec(align(8)) uint8_t game_engine_bucket_scores_extra[68] = {0};
#pragma section(".geq$006b139cv", read, write)
__declspec(allocate(".geq$006b139cv")) __declspec(align(4)) uint8_t race_used_locations[32] = {0};
#pragma section(".geq$006b13bcv", read, write)
__declspec(allocate(".geq$006b13bcv")) __declspec(align(4)) uint8_t race_used_location_count[4] = {0};
#pragma section(".geq$006b13c0v", read, write)
__declspec(allocate(".geq$006b13c0v")) __declspec(align(16)) uint8_t race_vehicle_counts[24] = {0};
#pragma section(".geq$006b13d8v", read, write)
__declspec(allocate(".geq$006b13d8v")) __declspec(align(8)) uint8_t slayer_team_score[64] = {0};
#pragma section(".geq$006b1418v", read, write)
__declspec(allocate(".geq$006b1418v")) __declspec(align(8)) uint8_t slayer_player_score[64] = {0};

/** 0x006b1868..0x006b2a68: input_devices, input_device_to_slot */
#pragma section(".geq$006b1868", read, write)
__declspec(allocate(".geq$006b1868")) __declspec(align(16)) uint8_t eq_pad_006b1868[8] = {0};
#pragma section(".geq$006b1868v", read, write)
__declspec(allocate(".geq$006b1868v")) __declspec(align(8)) uint8_t input_devices[560] = {0};
#pragma section(".geq$006b1a98v", read, write)
__declspec(allocate(".geq$006b1a98v")) __declspec(align(8)) uint8_t input_device_to_slot[4048] = {0};

/** 0x006b37f4..0x006b380c: new_profile_name_buffer_006b37f4, new_profile_name_terminator_006b380a */
#pragma section(".geq$006b37f4", read, write)
__declspec(allocate(".geq$006b37f4")) __declspec(align(16)) uint8_t eq_pad_006b37f4[4] = {0};
#pragma section(".geq$006b37f4v", read, write)
__declspec(allocate(".geq$006b37f4v")) __declspec(align(4)) uint8_t new_profile_name_buffer_006b37f4[22] = {0};
#pragma section(".geq$006b380av", read, write)
__declspec(allocate(".geq$006b380av")) __declspec(align(2)) uint8_t new_profile_name_terminator_006b380a[2] = {0};

/** 0x006b3a48..0x006b42d8: input_controls_live_006b3a48, controls_action_name_buffer */
#pragma section(".geq$006b3a48", read, write)
__declspec(allocate(".geq$006b3a48")) __declspec(align(16)) uint8_t eq_pad_006b3a48[8] = {0};
#pragma section(".geq$006b3a48v", read, write)
__declspec(allocate(".geq$006b3a48v")) __declspec(align(8)) uint8_t input_controls_live_006b3a48[768] = {0};
#pragma section(".geq$006b3d48v", read, write)
__declspec(allocate(".geq$006b3d48v")) __declspec(align(8)) uint8_t controls_action_name_buffer[1424] = {0};

/** 0x006b5e58..0x006b5e90: server_browser_player_ticker, DAT_006b5e60, DAT_006b5e64 and 7 more */
#pragma section(".geq$006b5e58", read, write)
__declspec(allocate(".geq$006b5e58")) __declspec(align(16)) uint8_t eq_pad_006b5e58[8] = {0};
#pragma section(".geq$006b5e58v", read, write)
__declspec(allocate(".geq$006b5e58v")) __declspec(align(8)) uint8_t server_browser_player_ticker[8] = {0};
#pragma section(".geq$006b5e60v", read, write)
__declspec(allocate(".geq$006b5e60v")) __declspec(align(16)) uint8_t DAT_006b5e60[4] = {0};
#pragma section(".geq$006b5e64v", read, write)
__declspec(allocate(".geq$006b5e64v")) __declspec(align(4)) uint8_t DAT_006b5e64[4] = {0};
#pragma section(".geq$006b5e68v", read, write)
__declspec(allocate(".geq$006b5e68v")) __declspec(align(8)) uint8_t DAT_006b5e68[4] = {0};
#pragma section(".geq$006b5e6cv", read, write)
__declspec(allocate(".geq$006b5e6cv")) __declspec(align(4)) uint8_t DAT_006b5e6c[8] = {0};
#pragma section(".geq$006b5e74v", read, write)
__declspec(allocate(".geq$006b5e74v")) __declspec(align(4)) uint8_t server_browser_variant_ticker[8] = {0};
#pragma section(".geq$006b5e7cv", read, write)
__declspec(allocate(".geq$006b5e7cv")) __declspec(align(4)) uint8_t DAT_006b5e7c[4] = {0};
#pragma section(".geq$006b5e80v", read, write)
__declspec(allocate(".geq$006b5e80v")) __declspec(align(16)) uint8_t DAT_006b5e80[4] = {0};
#pragma section(".geq$006b5e84v", read, write)
__declspec(allocate(".geq$006b5e84v")) __declspec(align(4)) uint8_t DAT_006b5e84[4] = {0};
#pragma section(".geq$006b5e88v", read, write)
__declspec(allocate(".geq$006b5e88v")) __declspec(align(8)) uint8_t DAT_006b5e88[8] = {0};

/** 0x006b85a8..0x006b85b8: network_buffer_pair_pool, network_buffer_pair_pool_count, network_buffer_pair_pool_data */
#pragma section(".geq$006b85a8", read, write)
__declspec(allocate(".geq$006b85a8")) __declspec(align(16)) uint8_t eq_pad_006b85a8[8] = {0};
#pragma section(".geq$006b85a8v", read, write)
__declspec(allocate(".geq$006b85a8v")) __declspec(align(8)) uint8_t network_buffer_pair_pool[4] = {0};
#pragma section(".geq$006b85acv", read, write)
__declspec(allocate(".geq$006b85acv")) __declspec(align(4)) uint8_t network_buffer_pair_pool_count[4] = {0};
#pragma section(".geq$006b85b0v", read, write)
__declspec(allocate(".geq$006b85b0v")) __declspec(align(16)) uint8_t network_buffer_pair_pool_data[8] = {0};

/** 0x006be810..0x006ce818: lens_flare_marker_visibility, king_hill_single_occupant_flag */
#pragma section(".geq$006be810v", read, write)
__declspec(allocate(".geq$006be810v")) __declspec(align(16)) uint8_t lens_flare_marker_visibility[10031] = {0};
#pragma section(".geq$006c0f3fv", read, write)
__declspec(allocate(".geq$006c0f3fv")) __declspec(align(1)) uint8_t king_hill_single_occupant_flag[55513] = {0};

/** 0x006d8828..0x006d9838: g_font_glyph_cache, font_glyph_cache_slots */
#pragma section(".geq$006d8828", read, write)
__declspec(allocate(".geq$006d8828")) __declspec(align(16)) uint8_t eq_pad_006d8828[8] = {0};
#pragma section(".geq$006d8828v", read, write)
__declspec(allocate(".geq$006d8828v")) __declspec(align(8)) uint8_t g_font_glyph_cache[16] = {0};
#pragma section(".geq$006d8838v", read, write)
__declspec(allocate(".geq$006d8838v")) __declspec(align(8)) uint8_t font_glyph_cache_slots[4096] = {0};

/** 0x006d98e8..0x006d99d8: rasterizer_dynamic_vertex_caches, render_unknown_d98f0 */
#pragma section(".geq$006d98e8", read, write)
__declspec(allocate(".geq$006d98e8")) __declspec(align(16)) uint8_t eq_pad_006d98e8[8] = {0};
#pragma section(".geq$006d98e8v", read, write)
__declspec(allocate(".geq$006d98e8v")) __declspec(align(8)) uint8_t rasterizer_dynamic_vertex_caches[8] = {0};
#pragma section(".geq$006d98f0v", read, write)
__declspec(allocate(".geq$006d98f0v")) __declspec(align(16)) uint8_t render_unknown_d98f0[232] = {0};

/** 0x006e17f4..0x006e1828: environment_techniques_no, environment_techniques_ps14 */
#pragma section(".geq$006e17f4", read, write)
__declspec(allocate(".geq$006e17f4")) __declspec(align(16)) uint8_t eq_pad_006e17f4[4] = {0};
#pragma section(".geq$006e17f4v", read, write)
__declspec(allocate(".geq$006e17f4v")) __declspec(align(4)) uint8_t environment_techniques_no[24] = {0};
#pragma section(".geq$006e180cv", read, write)
__declspec(allocate(".geq$006e180cv")) __declspec(align(4)) uint8_t environment_techniques_ps14[28] = {0};

/** 0x006e1a90..0x006e1b80: rasterizer_vertex_declarations[k_rasterizer_vertex_type_count] with the names inside it */
#pragma section(".geq$006e1a90v", read, write)
__declspec(allocate(".geq$006e1a90v")) __declspec(align(16)) uint8_t rasterizer_vertex_declarations[48] = {0};
#pragma section(".geq$006e1ac0v", read, write)
__declspec(allocate(".geq$006e1ac0v")) __declspec(align(16)) uint8_t rasterizer_model_vertex_declaration[48] = {0};
#pragma section(".geq$006e1af0v", read, write)
__declspec(allocate(".geq$006e1af0v")) __declspec(align(16)) uint8_t renderer_unknown_6e1af0[8] = {0};
#pragma section(".geq$006e1af8v", read, write)
__declspec(allocate(".geq$006e1af8v")) __declspec(align(8)) uint8_t renderer_unknown_6e1af8[96] = {0};
#pragma section(".geq$006e1b58v", read, write)
__declspec(allocate(".geq$006e1b58v")) __declspec(align(8)) uint8_t unknown_006e1b58[40] = {0};

/** 0x006e3208..0x006e357c: material_table_fallback, material_table_bad_index */
#pragma section(".geq$006e3208", read, write)
__declspec(allocate(".geq$006e3208")) __declspec(align(16)) uint8_t eq_pad_006e3208[8] = {0};
#pragma section(".geq$006e3208v", read, write)
__declspec(allocate(".geq$006e3208v")) __declspec(align(8)) uint8_t material_table_fallback[880] = {0};
#pragma section(".geq$006e3578v", read, write)
__declspec(allocate(".geq$006e3578v")) __declspec(align(8)) uint8_t material_table_bad_index[4] = {0};

/** 0x006e4734..0x006e476a: hud_text_draw_color_or_flags, hud_text_draw_column, hud_text_draw_color_a and 8 more */
#pragma section(".geq$006e4734", read, write)
__declspec(allocate(".geq$006e4734")) __declspec(align(16)) uint8_t eq_pad_006e4734[4] = {0};
#pragma section(".geq$006e4734v", read, write)
__declspec(allocate(".geq$006e4734v")) __declspec(align(4)) uint8_t hud_text_draw_color_or_flags[2] = {0};
#pragma section(".geq$006e4736v", read, write)
__declspec(allocate(".geq$006e4736v")) __declspec(align(2)) uint8_t hud_text_draw_column[2] = {0};
#pragma section(".geq$006e4738v", read, write)
__declspec(allocate(".geq$006e4738v")) __declspec(align(8)) uint8_t hud_text_draw_color_a[4] = {0};
#pragma section(".geq$006e473cv", read, write)
__declspec(allocate(".geq$006e473cv")) __declspec(align(4)) uint8_t hud_text_draw_color_r[4] = {0};
#pragma section(".geq$006e4740v", read, write)
__declspec(allocate(".geq$006e4740v")) __declspec(align(16)) uint8_t hud_text_draw_color_g[4] = {0};
#pragma section(".geq$006e4744v", read, write)
__declspec(allocate(".geq$006e4744v")) __declspec(align(4)) uint8_t hud_text_draw_color_b[4] = {0};
#pragma section(".geq$006e4748v", read, write)
__declspec(allocate(".geq$006e4748v")) __declspec(align(8)) uint8_t hud_text_draw_background_mode[2] = {0};
#pragma section(".geq$006e474av", read, write)
__declspec(allocate(".geq$006e474av")) __declspec(align(2)) uint8_t text_tab_stops[4] = {0};
#pragma section(".geq$006e474ev", read, write)
__declspec(allocate(".geq$006e474ev")) __declspec(align(2)) uint8_t hud_text_draw_box_field_474e[4] = {0};
#pragma section(".geq$006e4752v", read, write)
__declspec(allocate(".geq$006e4752v")) __declspec(align(2)) uint8_t hud_text_draw_tabstop_c[4] = {0};
#pragma section(".geq$006e4756v", read, write)
__declspec(allocate(".geq$006e4756v")) __declspec(align(2)) uint8_t hud_text_draw_box_field_4756[20] = {0};

/** 0x006f1c88..0x006f1d20: game_engine_variant, control_binding_device_type, game_engine_teams_enabled_flag and 9 more */
#pragma section(".geq$006f1c88", read, write)
__declspec(allocate(".geq$006f1c88")) __declspec(align(16)) uint8_t eq_pad_006f1c88[8] = {0};
#pragma section(".geq$006f1c88v", read, write)
__declspec(allocate(".geq$006f1c88v")) __declspec(align(8)) uint8_t game_engine_variant[48] = {0};
#pragma section(".geq$006f1cb8v", read, write)
__declspec(allocate(".geq$006f1cb8v")) __declspec(align(8)) uint8_t control_binding_device_type[4] = {0};
#pragma section(".geq$006f1cbcv", read, write)
__declspec(allocate(".geq$006f1cbcv")) __declspec(align(4)) uint8_t game_engine_teams_enabled_flag[4] = {0};
#pragma section(".geq$006f1cc0v", read, write)
__declspec(allocate(".geq$006f1cc0v")) __declspec(align(16)) uint8_t motion_sensor_override_value[32] = {0};
#pragma section(".geq$006f1ce0v", read, write)
__declspec(allocate(".geq$006f1ce0v")) __declspec(align(16)) uint8_t game_engine_variant_score_limit[8] = {0};
#pragma section(".geq$006f1ce8v", read, write)
__declspec(allocate(".geq$006f1ce8v")) __declspec(align(8)) uint8_t control_word_primary[4] = {0};
#pragma section(".geq$006f1cecv", read, write)
__declspec(allocate(".geq$006f1cecv")) __declspec(align(4)) uint8_t control_word_secondary[4] = {0};
#pragma section(".geq$006f1cf0v", read, write)
__declspec(allocate(".geq$006f1cf0v")) __declspec(align(16)) uint8_t vehicle_network_update_period[4] = {0};
#pragma section(".geq$006f1cf4v", read, write)
__declspec(allocate(".geq$006f1cf4v")) __declspec(align(4)) uint8_t g_006f1cf4[8] = {0};
#pragma section(".geq$006f1cfcv", read, write)
__declspec(allocate(".geq$006f1cfcv")) __declspec(align(4)) uint8_t game_engine_unknown_1cfc[8] = {0};
#pragma section(".geq$006f1d04v", read, write)
__declspec(allocate(".geq$006f1d04v")) __declspec(align(4)) uint8_t hill_pulse_grow_done[1] = {0};
#pragma section(".geq$006f1d05v", read, write)
__declspec(allocate(".geq$006f1d05v")) __declspec(align(1)) uint8_t hill_pulse_fade_done[27] = {0};

/** 0x006f1d24..0x006f1d28: game_engine_map_table_value, g_006f1d25 */
#pragma section(".geq$006f1d24", read, write)
__declspec(allocate(".geq$006f1d24")) __declspec(align(16)) uint8_t eq_pad_006f1d24[4] = {0};
#pragma section(".geq$006f1d24v", read, write)
__declspec(allocate(".geq$006f1d24v")) __declspec(align(4)) uint8_t game_engine_map_table_value[1] = {0};
#pragma section(".geq$006f1d25v", read, write)
__declspec(allocate(".geq$006f1d25v")) __declspec(align(1)) uint8_t g_006f1d25[3] = {0};

/** 0x006f1d48..0x006f1d6c: game_time_unknown_48, game_time_unknown_49 */
#pragma section(".geq$006f1d48", read, write)
__declspec(allocate(".geq$006f1d48")) __declspec(align(16)) uint8_t eq_pad_006f1d48[8] = {0};
#pragma section(".geq$006f1d48v", read, write)
__declspec(allocate(".geq$006f1d48v")) __declspec(align(8)) uint8_t game_time_unknown_48[1] = {0};
#pragma section(".geq$006f1d49v", read, write)
__declspec(allocate(".geq$006f1d49v")) __declspec(align(1)) uint8_t game_time_unknown_49[35] = {0};

/** 0x006f1d88..0x007102d4: update_server_initialized, update_server_tick, update_server_queues and 13 more */
#pragma section(".geq$006f1d88", read, write)
__declspec(allocate(".geq$006f1d88")) __declspec(align(16)) uint8_t eq_pad_006f1d88[8] = {0};
#pragma section(".geq$006f1d88v", read, write)
__declspec(allocate(".geq$006f1d88v")) __declspec(align(8)) uint8_t update_server_initialized[4] = {0};
#pragma section(".geq$006f1d8cv", read, write)
__declspec(allocate(".geq$006f1d8cv")) __declspec(align(4)) uint8_t update_server_tick[4] = {0};
#pragma section(".geq$006f1d90v", read, write)
__declspec(allocate(".geq$006f1d90v")) __declspec(align(16)) uint8_t update_server_queues[4] = {0};
#pragma section(".geq$006f1d94v", read, write)
__declspec(allocate(".geq$006f1d94v")) __declspec(align(4)) uint8_t update_server_history[24836] = {0};
#pragma section(".geq$006f7e98v", read, write)
__declspec(allocate(".geq$006f7e98v")) __declspec(align(8)) uint8_t update_client_initialized[4] = {0};
#pragma section(".geq$006f7e9cv", read, write)
__declspec(allocate(".geq$006f7e9cv")) __declspec(align(4)) uint8_t update_client_base_tick[4] = {0};
#pragma section(".geq$006f7ea0v", read, write)
__declspec(allocate(".geq$006f7ea0v")) __declspec(align(16)) uint8_t update_client_latest_tick[4] = {0};
#pragma section(".geq$006f7ea4v", read, write)
__declspec(allocate(".geq$006f7ea4v")) __declspec(align(4)) uint8_t update_client_staged[4] = {0};
#pragma section(".geq$006f7ea8v", read, write)
__declspec(allocate(".geq$006f7ea8v")) __declspec(align(8)) uint8_t update_client_unknown_ea8[4] = {0};
#pragma section(".geq$006f7eacv", read, write)
__declspec(allocate(".geq$006f7eacv")) __declspec(align(4)) uint8_t update_client_unknown_eac[24] = {0};
#pragma section(".geq$006f7ec4v", read, write)
__declspec(allocate(".geq$006f7ec4v")) __declspec(align(4)) uint8_t update_client_ticks_remaining[4] = {0};
#pragma section(".geq$006f7ec8v", read, write)
__declspec(allocate(".geq$006f7ec8v")) __declspec(align(8)) uint8_t update_client_held_control_flags[4] = {0};
#pragma section(".geq$006f7eccv", read, write)
__declspec(allocate(".geq$006f7eccv")) __declspec(align(4)) uint8_t update_client_staged_count[4] = {0};
#pragma section(".geq$006f7ed0v", read, write)
__declspec(allocate(".geq$006f7ed0v")) __declspec(align(16)) uint8_t update_client_queues[4] = {0};
#pragma section(".geq$006f7ed4v", read, write)
__declspec(allocate(".geq$006f7ed4v")) __declspec(align(4)) uint8_t update_client_history[97272] = {0};
#pragma section(".geq$0070faccv", read, write)
__declspec(allocate(".geq$0070faccv")) __declspec(align(4)) uint8_t player_control_look_rates_0070facc[2056] = {0};

/** 0x00710328..0x00712918: input_globals (0x97c dwords) with the binding tables it contains */
#pragma section(".geq$00710328", read, write)
__declspec(allocate(".geq$00710328")) __declspec(align(16)) uint8_t eq_pad_00710328[8] = {0};
#pragma section(".geq$00710328v", read, write)
__declspec(allocate(".geq$00710328v")) __declspec(align(8)) uint8_t input_globals[8] = {0};
#pragma section(".geq$00710330v", read, write)
__declspec(allocate(".geq$00710330v")) __declspec(align(16)) uint8_t keyboard_bindings[218] = {0};
#pragma section(".geq$0071040av", read, write)
__declspec(allocate(".geq$0071040av")) __declspec(align(2)) uint8_t mouse_button_bindings[16] = {0};
#pragma section(".geq$0071041av", read, write)
__declspec(allocate(".geq$0071041av")) __declspec(align(2)) uint8_t mouse_axis_bindings[12] = {0};
#pragma section(".geq$00710426v", read, write)
__declspec(allocate(".geq$00710426v")) __declspec(align(2)) uint8_t gamepad_button_bindings[256] = {0};
#pragma section(".geq$00710526v", read, write)
__declspec(allocate(".geq$00710526v")) __declspec(align(2)) uint8_t gamepad_action_buttons[16] = {0};
#pragma section(".geq$00710536v", read, write)
__declspec(allocate(".geq$00710536v")) __declspec(align(2)) uint8_t gamepad_axis_bindings[512] = {0};
#pragma section(".geq$00710736v", read, write)
__declspec(allocate(".geq$00710736v")) __declspec(align(2)) uint8_t gamepad_pov_bindings[7522] = {0};
#pragma section(".geq$00712498v", read, write)
__declspec(allocate(".geq$00712498v")) __declspec(align(8)) uint8_t local_player_input_states[8] = {0};
#pragma section(".geq$007124a0v", read, write)
__declspec(allocate(".geq$007124a0v")) __declspec(align(16)) uint8_t unknown_007124a0[1] = {0};
#pragma section(".geq$007124a1v", read, write)
__declspec(allocate(".geq$007124a1v")) __declspec(align(1)) uint8_t chimera_loading_screen_cleanup_gate[3] = {0};
#pragma section(".geq$007124a4v", read, write)
__declspec(allocate(".geq$007124a4v")) __declspec(align(4)) uint8_t local_player_hud_status_table[3] = {0};
#pragma section(".geq$007124a7v", read, write)
__declspec(allocate(".geq$007124a7v")) __declspec(align(1)) uint8_t chat_hotkey_all[1] = {0};
#pragma section(".geq$007124a8v", read, write)
__declspec(allocate(".geq$007124a8v")) __declspec(align(8)) uint8_t chat_hotkey_team[1] = {0};
#pragma section(".geq$007124a9v", read, write)
__declspec(allocate(".geq$007124a9v")) __declspec(align(1)) uint8_t chat_hotkey_vehicle[153] = {0};
#pragma section(".geq$00712542v", read, write)
__declspec(allocate(".geq$00712542v")) __declspec(align(2)) uint8_t controls_input_capture_flags[2] = {0};
#pragma section(".geq$00712544v", read, write)
__declspec(allocate(".geq$00712544v")) __declspec(align(4)) uint8_t controls_input_capture_buffer[640] = {0};
#pragma section(".geq$007127c4v", read, write)
__declspec(allocate(".geq$007127c4v")) __declspec(align(4)) uint8_t controls_captured_binding[13] = {0};
#pragma section(".geq$007127d1v", read, write)
__declspec(allocate(".geq$007127d1v")) __declspec(align(1)) uint8_t escape_key_state[3] = {0};
#pragma section(".geq$007127d4v", read, write)
__declspec(allocate(".geq$007127d4v")) __declspec(align(4)) uint8_t controls_current_binding_table[324] = {0};

/** 0x00712cc0..0x00712dcc: input_event_queue_active, unknown_00712ccc */
#pragma section(".geq$00712cc0v", read, write)
__declspec(allocate(".geq$00712cc0v")) __declspec(align(16)) uint8_t input_event_queue_active[12] = {0};
#pragma section(".geq$00712cccv", read, write)
__declspec(allocate(".geq$00712cccv")) __declspec(align(4)) uint8_t unknown_00712ccc[256] = {0};

/** 0x00712dd8..0x00718e80: the profile block: profile_globals_block's 0x1829-dword clear covers everything up to 0x00718e7c */
#pragma section(".geq$00712dd8", read, write)
__declspec(allocate(".geq$00712dd8")) __declspec(align(16)) uint8_t eq_pad_00712dd8[8] = {0};
#pragma section(".geq$00712dd8v", read, write)
__declspec(allocate(".geq$00712dd8v")) __declspec(align(8)) uint8_t profile_globals_block[296] = {0};
#pragma section(".geq$00712f00v", read, write)
__declspec(allocate(".geq$00712f00v")) __declspec(align(16)) uint8_t known_solo_level_index_00712f00[7] = {0};
#pragma section(".geq$00712f07v", read, write)
__declspec(allocate(".geq$00712f07v")) __declspec(align(1)) uint8_t profile_slot_flag[7885] = {0};
#pragma section(".geq$00714dd4v", read, write)
__declspec(allocate(".geq$00714dd4v")) __declspec(align(4)) uint8_t saved_player_profile_slots_handle[4] = {0};
#pragma section(".geq$00714dd8v", read, write)
__declspec(allocate(".geq$00714dd8v")) __declspec(align(8)) uint8_t local_team_00714dd8[4] = {0};
#pragma section(".geq$00714ddcv", read, write)
__declspec(allocate(".geq$00714ddcv")) __declspec(align(4)) uint8_t coop_profile_globals_block_00714ddc[2] = {0};
#pragma section(".geq$00714ddev", read, write)
__declspec(allocate(".geq$00714ddev")) __declspec(align(2)) uint8_t profile_slot_id[2] = {0};
#pragma section(".geq$00714de0v", read, write)
__declspec(allocate(".geq$00714de0v")) __declspec(align(16)) uint8_t game_variant_saved_default[152] = {0};
#pragma section(".geq$00714e78v", read, write)
__declspec(allocate(".geq$00714e78v")) __declspec(align(8)) uint8_t game_variant_saved_default_valid[4] = {0};
#pragma section(".geq$00714e7cv", read, write)
__declspec(allocate(".geq$00714e7cv")) __declspec(align(4)) uint8_t selected_saved_item[4] = {0};
#pragma section(".geq$00714e80v", read, write)
__declspec(allocate(".geq$00714e80v")) __declspec(align(16)) uint8_t saved_item_working_copy[56] = {0};
#pragma section(".geq$00714eb8v", read, write)
__declspec(allocate(".geq$00714eb8v")) __declspec(align(8)) uint8_t unknown_00714eb8[92] = {0};
#pragma section(".geq$00714f14v", read, write)
__declspec(allocate(".geq$00714f14v")) __declspec(align(4)) uint8_t unknown_00714f14[160] = {0};
#pragma section(".geq$00714fb4v", read, write)
__declspec(allocate(".geq$00714fb4v")) __declspec(align(4)) uint8_t control_keyboard_scan_table[218] = {0};
#pragma section(".geq$0071508ev", read, write)
__declspec(allocate(".geq$0071508ev")) __declspec(align(2)) uint8_t control_mouse_button_scan_table[16] = {0};
#pragma section(".geq$0071509ev", read, write)
__declspec(allocate(".geq$0071509ev")) __declspec(align(2)) uint8_t control_mouse_axis_scan_table[12] = {0};
#pragma section(".geq$007150aav", read, write)
__declspec(allocate(".geq$007150aav")) __declspec(align(2)) uint8_t control_gamepad_button_scan_table[256] = {0};
#pragma section(".geq$007151aav", read, write)
__declspec(allocate(".geq$007151aav")) __declspec(align(2)) uint8_t control_gamepad_action_scan_buttons[16] = {0};
#pragma section(".geq$007151bav", read, write)
__declspec(allocate(".geq$007151bav")) __declspec(align(2)) uint8_t control_gamepad_axis_scan_table[512] = {0};
#pragma section(".geq$007153bav", read, write)
__declspec(allocate(".geq$007153bav")) __declspec(align(2)) uint8_t control_gamepad_pov_scan_table[1050] = {0};
#pragma section(".geq$007157d4v", read, write)
__declspec(allocate(".geq$007157d4v")) __declspec(align(4)) uint8_t controls_device_sensitivity_a[4] = {0};
#pragma section(".geq$007157d8v", read, write)
__declspec(allocate(".geq$007157d8v")) __declspec(align(8)) uint8_t controls_device_sensitivity_b[5796] = {0};
#pragma section(".geq$00716e7cv", read, write)
__declspec(allocate(".geq$00716e7cv")) __declspec(align(4)) uint8_t saved_item_disk_copy[8188] = {0};
#pragma section(".geq$00718e78v", read, write)
__declspec(allocate(".geq$00718e78v")) __declspec(align(8)) uint8_t profile_load_complete[8] = {0};

/** 0x00718f94..0x00718fc8: ui_root_widget, ui_widget_history, ui_time_milliseconds and 17 more */
#pragma section(".geq$00718f94", read, write)
__declspec(allocate(".geq$00718f94")) __declspec(align(16)) uint8_t eq_pad_00718f94[4] = {0};
#pragma section(".geq$00718f94v", read, write)
__declspec(allocate(".geq$00718f94v")) __declspec(align(4)) uint8_t ui_root_widget[4] = {0};
#pragma section(".geq$00718f98v", read, write)
__declspec(allocate(".geq$00718f98v")) __declspec(align(8)) uint8_t ui_widget_history[4] = {0};
#pragma section(".geq$00718f9cv", read, write)
__declspec(allocate(".geq$00718f9cv")) __declspec(align(4)) uint8_t ui_time_milliseconds[4] = {0};
#pragma section(".geq$00718fa0v", read, write)
__declspec(allocate(".geq$00718fa0v")) __declspec(align(16)) uint8_t ui_pause_pending_count_00718fa0[4] = {0};
#pragma section(".geq$00718fa4v", read, write)
__declspec(allocate(".geq$00718fa4v")) __declspec(align(4)) uint8_t network_join_error_code[2] = {0};
#pragma section(".geq$00718fa6v", read, write)
__declspec(allocate(".geq$00718fa6v")) __declspec(align(2)) uint8_t ui_pause_depth[2] = {0};
#pragma section(".geq$00718fa8v", read, write)
__declspec(allocate(".geq$00718fa8v")) __declspec(align(8)) uint8_t screen_fade_progress[4] = {0};
#pragma section(".geq$00718facv", read, write)
__declspec(allocate(".geq$00718facv")) __declspec(align(4)) uint8_t quit_confirm_error_string_index[2] = {0};
#pragma section(".geq$00718faev", read, write)
__declspec(allocate(".geq$00718faev")) __declspec(align(2)) uint8_t quit_confirm_error_unknown_ae[2] = {0};
#pragma section(".geq$00718fb0v", read, write)
__declspec(allocate(".geq$00718fb0v")) __declspec(align(16)) uint8_t quit_confirm_error_modal[1] = {0};
#pragma section(".geq$00718fb1v", read, write)
__declspec(allocate(".geq$00718fb1v")) __declspec(align(1)) uint8_t quit_confirm_error_is_error[1] = {0};
#pragma section(".geq$00718fb2v", read, write)
__declspec(allocate(".geq$00718fb2v")) __declspec(align(2)) uint8_t ui_pending_error_alternate[4] = {0};
#pragma section(".geq$00718fb6v", read, write)
__declspec(allocate(".geq$00718fb6v")) __declspec(align(2)) uint8_t ui_pending_errors[6] = {0};
#pragma section(".geq$00718fbcv", read, write)
__declspec(allocate(".geq$00718fbcv")) __declspec(align(4)) uint8_t loading_thread[4] = {0};
#pragma section(".geq$00718fc0v", read, write)
__declspec(allocate(".geq$00718fc0v")) __declspec(align(16)) uint8_t loading_thread_result[2] = {0};
#pragma section(".geq$00718fc2v", read, write)
__declspec(allocate(".geq$00718fc2v")) __declspec(align(2)) uint8_t widget_memory_pool_valid[1] = {0};
#pragma section(".geq$00718fc3v", read, write)
__declspec(allocate(".geq$00718fc3v")) __declspec(align(1)) uint8_t widget_creating_children[1] = {0};
#pragma section(".geq$00718fc4v", read, write)
__declspec(allocate(".geq$00718fc4v")) __declspec(align(4)) uint8_t ui_widget_show_path_flag[1] = {0};
#pragma section(".geq$00718fc5v", read, write)
__declspec(allocate(".geq$00718fc5v")) __declspec(align(1)) uint8_t ui_input_batch_mode[1] = {0};
#pragma section(".geq$00718fc6v", read, write)
__declspec(allocate(".geq$00718fc6v")) __declspec(align(2)) uint8_t main_menu_music_pending[2] = {0};

/** 0x00719068..0x0071916e: level_select_current_path_00719068, level_select_frame_00719168, level_select_flags_0071916a and 2 more */
#pragma section(".geq$00719068", read, write)
__declspec(allocate(".geq$00719068")) __declspec(align(16)) uint8_t eq_pad_00719068[8] = {0};
#pragma section(".geq$00719068v", read, write)
__declspec(allocate(".geq$00719068v")) __declspec(align(8)) uint8_t level_select_current_path_00719068[256] = {0};
#pragma section(".geq$00719168v", read, write)
__declspec(allocate(".geq$00719168v")) __declspec(align(8)) uint8_t level_select_frame_00719168[2] = {0};
#pragma section(".geq$0071916av", read, write)
__declspec(allocate(".geq$0071916av")) __declspec(align(2)) uint8_t level_select_flags_0071916a[1] = {0};
#pragma section(".geq$0071916bv", read, write)
__declspec(allocate(".geq$0071916bv")) __declspec(align(1)) uint8_t level_select_flags_0071916b[1] = {0};
#pragma section(".geq$0071916cv", read, write)
__declspec(allocate(".geq$0071916cv")) __declspec(align(4)) uint8_t level_select_flags_0071916c[2] = {0};

/** 0x007193a8..0x0071941c: virtual_keyboard, DAT_007193be, network_host_edit_field_00719410 */
#pragma section(".geq$007193a8", read, write)
__declspec(allocate(".geq$007193a8")) __declspec(align(16)) uint8_t eq_pad_007193a8[8] = {0};
#pragma section(".geq$007193a8v", read, write)
__declspec(allocate(".geq$007193a8v")) __declspec(align(8)) uint8_t virtual_keyboard[22] = {0};
#pragma section(".geq$007193bev", read, write)
__declspec(allocate(".geq$007193bev")) __declspec(align(2)) uint8_t DAT_007193be[82] = {0};
#pragma section(".geq$00719410v", read, write)
__declspec(allocate(".geq$00719410v")) __declspec(align(16)) uint8_t network_host_edit_field_00719410[12] = {0};

/** 0x007196bc..0x007196cc: server_list, server_list_block_used, server_list_block_capacity and 1 more */
#pragma section(".geq$007196bc", read, write)
__declspec(allocate(".geq$007196bc")) __declspec(align(16)) uint8_t eq_pad_007196bc[12] = {0};
#pragma section(".geq$007196bcv", read, write)
__declspec(allocate(".geq$007196bcv")) __declspec(align(4)) uint8_t server_list[4] = {0};
#pragma section(".geq$007196c0v", read, write)
__declspec(allocate(".geq$007196c0v")) __declspec(align(16)) uint8_t server_list_block_used[4] = {0};
#pragma section(".geq$007196c4v", read, write)
__declspec(allocate(".geq$007196c4v")) __declspec(align(4)) uint8_t server_list_block_capacity[4] = {0};
#pragma section(".geq$007196c8v", read, write)
__declspec(allocate(".geq$007196c8v")) __declspec(align(8)) uint8_t server_browser_query_elapsed_ms[4] = {0};

/** 0x00719700..0x00719770: main_globals and the fields the code names; continues into slice08.c's .g08$ run at 0x00719770 */
#pragma section(".g08$0000_00719700v", read, write)
__declspec(allocate(".g08$0000_00719700v")) __declspec(align(16)) uint8_t main_globals_data[32] = {0};
#pragma section(".g08$0000_00719720v", read, write)
__declspec(allocate(".g08$0000_00719720v")) __declspec(align(16)) uint8_t network_game_mode[24] = {0};
#pragma section(".g08$0000_00719738v", read, write)
__declspec(allocate(".g08$0000_00719738v")) __declspec(align(8)) uint8_t unknown_00719738[1] = {0};
#pragma section(".g08$0000_00719739v", read, write)
__declspec(allocate(".g08$0000_00719739v")) __declspec(align(1)) uint8_t network_wait_flag_00719739[1] = {0};
#pragma section(".g08$0000_0071973av", read, write)
__declspec(allocate(".g08$0000_0071973av")) __declspec(align(2)) uint8_t revert_map[1] = {0};
#pragma section(".g08$0000_0071973bv", read, write)
__declspec(allocate(".g08$0000_0071973bv")) __declspec(align(1)) uint8_t revert_map_if_allowed[1] = {0};
#pragma section(".g08$0000_0071973cv", read, write)
__declspec(allocate(".g08$0000_0071973cv")) __declspec(align(4)) uint8_t network_join_error_reason[1] = {0};
#pragma section(".g08$0000_0071973dv", read, write)
__declspec(allocate(".g08$0000_0071973dv")) __declspec(align(1)) uint8_t main_globals_byte_0071973d[1] = {0};
#pragma section(".g08$0000_0071973ev", read, write)
__declspec(allocate(".g08$0000_0071973ev")) __declspec(align(2)) uint8_t main_globals_byte_0071973e[2] = {0};
#pragma section(".g08$0000_00719740v", read, write)
__declspec(allocate(".g08$0000_00719740v")) __declspec(align(16)) uint8_t main_globals_dword_00719740[4] = {0};
#pragma section(".g08$0000_00719744v", read, write)
__declspec(allocate(".g08$0000_00719744v")) __declspec(align(4)) uint8_t main_globals_dword_00719744[4] = {0};
#pragma section(".g08$0000_00719748v", read, write)
__declspec(allocate(".g08$0000_00719748v")) __declspec(align(8)) uint8_t main_menu_music_datum[4] = {0};
#pragma section(".g08$0000_0071974cv", read, write)
__declspec(allocate(".g08$0000_0071974cv")) __declspec(align(4)) uint8_t main_globals_word_0071974c[2] = {0};
#pragma section(".g08$0000_0071974ev", read, write)
__declspec(allocate(".g08$0000_0071974ev")) __declspec(align(2)) uint8_t main_globals_byte_0071974e[1] = {0};
#pragma section(".g08$0000_0071974fv", read, write)
__declspec(allocate(".g08$0000_0071974fv")) __declspec(align(1)) uint8_t lost_map[1] = {0};
#pragma section(".g08$0000_00719750v", read, write)
__declspec(allocate(".g08$0000_00719750v")) __declspec(align(16)) uint8_t global_00719750[1] = {0};
#pragma section(".g08$0000_00719751v", read, write)
__declspec(allocate(".g08$0000_00719751v")) __declspec(align(1)) uint8_t main_globals_byte_00719751[1] = {0};
#pragma section(".g08$0000_00719752v", read, write)
__declspec(allocate(".g08$0000_00719752v")) __declspec(align(2)) uint8_t main_globals_byte_00719752[1] = {0};
#pragma section(".g08$0000_00719753v", read, write)
__declspec(allocate(".g08$0000_00719753v")) __declspec(align(1)) uint8_t main_globals_byte_00719753[1] = {0};
#pragma section(".g08$0000_00719754v", read, write)
__declspec(allocate(".g08$0000_00719754v")) __declspec(align(4)) uint8_t split_screen_quit_prompt_string[3] = {0};
#pragma section(".g08$0000_00719757v", read, write)
__declspec(allocate(".g08$0000_00719757v")) __declspec(align(1)) uint8_t split_screen_quit_prompt_armed[4] = {0};
#pragma section(".g08$0000_0071975bv", read, write)
__declspec(allocate(".g08$0000_0071975bv")) __declspec(align(1)) uint8_t ui_event_byte_0071975b[13] = {0};
#pragma section(".g08$0000_00719768v", read, write)
__declspec(allocate(".g08$0000_00719768v")) __declspec(align(8)) uint8_t playback_requested_00719768[1] = {0};
#pragma section(".g08$0000_00719769v", read, write)
__declspec(allocate(".g08$0000_00719769v")) __declspec(align(1)) uint8_t time_is_running[1] = {0};
#pragma section(".g08$0000_0071976av", read, write)
__declspec(allocate(".g08$0000_0071976av")) __declspec(align(2)) uint8_t reset_frame_timers[2] = {0};
#pragma section(".g08$0000_0071976cv", read, write)
__declspec(allocate(".g08$0000_0071976cv")) __declspec(align(4)) uint8_t main_globals_byte_0071976c[2] = {0};
#pragma section(".g08$0000_0071976ev", read, write)
__declspec(allocate(".g08$0000_0071976ev")) __declspec(align(2)) uint8_t main_globals_word_0071976e[2] = {0};

/** 0x00746280..0x00746f8c: unknown_00746280_block, ambient_noise, weather_particle_system_count and 2 more */
#pragma section(".geq$00746280v", read, write)
__declspec(allocate(".geq$00746280v")) __declspec(align(16)) uint8_t unknown_00746280_block[4] = {0};
#pragma section(".geq$00746284v", read, write)
__declspec(allocate(".geq$00746284v")) __declspec(align(4)) uint8_t ambient_noise[2304] = {0};
#pragma section(".geq$00746b84v", read, write)
__declspec(allocate(".geq$00746b84v")) __declspec(align(4)) uint8_t weather_particle_system_count[4] = {0};
#pragma section(".geq$00746b88v", read, write)
__declspec(allocate(".geq$00746b88v")) __declspec(align(8)) uint8_t weather_wind_states[1024] = {0};
#pragma section(".geq$00746f88v", read, write)
__declspec(allocate(".geq$00746f88v")) __declspec(align(8)) uint8_t weather_frame_counter[4] = {0};

/** 0x00746fb0..0x00746fc0: lens_flare_current_key, lens_flare_batch_mode */
#pragma section(".geq$00746fb0v", read, write)
__declspec(allocate(".geq$00746fb0v")) __declspec(align(16)) uint8_t lens_flare_current_key[12] = {0};
#pragma section(".geq$00746fbcv", read, write)
__declspec(allocate(".geq$00746fbcv")) __declspec(align(4)) uint8_t lens_flare_batch_mode[4] = {0};

/** 0x007bf040..0x007bf050: lens_flare_applied_key, render_unknown_7bf04c */
#pragma section(".geq$007bf040v", read, write)
__declspec(allocate(".geq$007bf040v")) __declspec(align(16)) uint8_t lens_flare_applied_key[12] = {0};
#pragma section(".geq$007bf04cv", read, write)
__declspec(allocate(".geq$007bf04cv")) __declspec(align(4)) uint8_t render_unknown_7bf04c[4] = {0};

/** 0x007c10c0..0x007c1480: rasterizer_caps, rasterizer_capability_007c10e4, rasterizer_device_version and 16 more */
#pragma section(".geq$007c10c0v", read, write)
__declspec(allocate(".geq$007c10c0v")) __declspec(align(16)) uint8_t rasterizer_caps[36] = {0};
#pragma section(".geq$007c10e4v", read, write)
__declspec(allocate(".geq$007c10e4v")) __declspec(align(4)) uint8_t rasterizer_capability_007c10e4[168] = {0};
#pragma section(".geq$007c118cv", read, write)
__declspec(allocate(".geq$007c118cv")) __declspec(align(4)) uint8_t rasterizer_device_version[100] = {0};
#pragma section(".geq$007c11f0v", read, write)
__declspec(allocate(".geq$007c11f0v")) __declspec(align(16)) uint8_t rasterizer_desktop_display_mode[8] = {0};
#pragma section(".geq$007c11f8v", read, write)
__declspec(allocate(".geq$007c11f8v")) __declspec(align(8)) uint8_t os_platform_refresh_default[8] = {0};
#pragma section(".geq$007c1200v", read, write)
__declspec(allocate(".geq$007c1200v")) __declspec(align(16)) uint8_t rasterizer_time[8] = {0};
#pragma section(".geq$007c1208v", read, write)
__declspec(allocate(".geq$007c1208v")) __declspec(align(8)) uint8_t chimera_contrail_scale[24] = {0};
#pragma section(".geq$007c1220v", read, write)
__declspec(allocate(".geq$007c1220v")) __declspec(align(16)) uint8_t rasterizer_window[8] = {0};
#pragma section(".geq$007c1228v", read, write)
__declspec(allocate(".geq$007c1228v")) __declspec(align(8)) uint8_t rasterizer_camera_position[12] = {0};
#pragma section(".geq$007c1234v", read, write)
__declspec(allocate(".geq$007c1234v")) __declspec(align(4)) uint8_t rasterizer_camera_forward[32] = {0};
#pragma section(".geq$007c1254v", read, write)
__declspec(allocate(".geq$007c1254v")) __declspec(align(4)) uint8_t network_stats_overlay_text_rect_min[4] = {0};
#pragma section(".geq$007c1258v", read, write)
__declspec(allocate(".geq$007c1258v")) __declspec(align(8)) uint8_t network_stats_overlay_text_rect_max[432] = {0};
#pragma section(".geq$007c1408v", read, write)
__declspec(allocate(".geq$007c1408v")) __declspec(align(8)) uint8_t rasterizer_fog_flags[4] = {0};
#pragma section(".geq$007c140cv", read, write)
__declspec(allocate(".geq$007c140cv")) __declspec(align(4)) uint8_t rasterizer_fog_atmospheric_color[12] = {0};
#pragma section(".geq$007c1418v", read, write)
__declspec(allocate(".geq$007c1418v")) __declspec(align(8)) uint8_t rasterizer_fog_atmospheric_max_density[4] = {0};
#pragma section(".geq$007c141cv", read, write)
__declspec(allocate(".geq$007c141cv")) __declspec(align(4)) uint8_t rasterizer_fog_atmospheric_min_distance[4] = {0};
#pragma section(".geq$007c1420v", read, write)
__declspec(allocate(".geq$007c1420v")) __declspec(align(16)) uint8_t rasterizer_fog_atmospheric_max_distance[8] = {0};
#pragma section(".geq$007c1428v", read, write)
__declspec(allocate(".geq$007c1428v")) __declspec(align(8)) uint8_t rasterizer_fog_plane[16] = {0};
#pragma section(".geq$007c1438v", read, write)
__declspec(allocate(".geq$007c1438v")) __declspec(align(8)) uint8_t rasterizer_fog_planar_color[72] = {0};

/** 0x007c3114..0x007c32f4: camera_position, camera_position_y, camera_position_z and 17 more */
#pragma section(".geq$007c3114", read, write)
__declspec(allocate(".geq$007c3114")) __declspec(align(16)) uint8_t eq_pad_007c3114[4] = {0};
#pragma section(".geq$007c3114v", read, write)
__declspec(allocate(".geq$007c3114v")) __declspec(align(4)) uint8_t camera_position[4] = {0};
#pragma section(".geq$007c3118v", read, write)
__declspec(allocate(".geq$007c3118v")) __declspec(align(8)) uint8_t camera_position_y[4] = {0};
#pragma section(".geq$007c311cv", read, write)
__declspec(allocate(".geq$007c311cv")) __declspec(align(4)) uint8_t camera_position_z[4] = {0};
#pragma section(".geq$007c3120v", read, write)
__declspec(allocate(".geq$007c3120v")) __declspec(align(16)) uint8_t camera_forward_x[4] = {0};
#pragma section(".geq$007c3124v", read, write)
__declspec(allocate(".geq$007c3124v")) __declspec(align(4)) uint8_t camera_forward_y[4] = {0};
#pragma section(".geq$007c3128v", read, write)
__declspec(allocate(".geq$007c3128v")) __declspec(align(8)) uint8_t camera_forward_z[4] = {0};
#pragma section(".geq$007c312cv", read, write)
__declspec(allocate(".geq$007c312cv")) __declspec(align(4)) uint8_t camera_up[16] = {0};
#pragma section(".geq$007c313cv", read, write)
__declspec(allocate(".geq$007c313cv")) __declspec(align(4)) uint8_t camera_field_of_view[4] = {0};
#pragma section(".geq$007c3140v", read, write)
__declspec(allocate(".geq$007c3140v")) __declspec(align(16)) uint8_t render_viewport_top[2] = {0};
#pragma section(".geq$007c3142v", read, write)
__declspec(allocate(".geq$007c3142v")) __declspec(align(2)) uint8_t render_viewport_left[2] = {0};
#pragma section(".geq$007c3144v", read, write)
__declspec(allocate(".geq$007c3144v")) __declspec(align(4)) uint8_t render_viewport_bottom[2] = {0};
#pragma section(".geq$007c3146v", read, write)
__declspec(allocate(".geq$007c3146v")) __declspec(align(2)) uint8_t render_viewport_right[2] = {0};
#pragma section(".geq$007c3148v", read, write)
__declspec(allocate(".geq$007c3148v")) __declspec(align(8)) uint8_t screen_safe_area_right[4] = {0};
#pragma section(".geq$007c314cv", read, write)
__declspec(allocate(".geq$007c314cv")) __declspec(align(4)) uint8_t screen_safe_area_bottom[8] = {0};
#pragma section(".geq$007c3154v", read, write)
__declspec(allocate(".geq$007c3154v")) __declspec(align(4)) uint8_t portal_visibility_tolerance[20] = {0};
#pragma section(".geq$007c3168v", read, write)
__declspec(allocate(".geq$007c3168v")) __declspec(align(8)) uint8_t render_frustum_global[16] = {0};
#pragma section(".geq$007c3178v", read, write)
__declspec(allocate(".geq$007c3178v")) __declspec(align(8)) uint8_t render_camera_world_to_view[104] = {0};
#pragma section(".geq$007c31e0v", read, write)
__declspec(allocate(".geq$007c31e0v")) __declspec(align(16)) uint8_t render_camera_facing_basis[96] = {0};
#pragma section(".geq$007c3240v", read, write)
__declspec(allocate(".geq$007c3240v")) __declspec(align(16)) uint8_t waypoint_fade_near[4] = {0};
#pragma section(".geq$007c3244v", read, write)
__declspec(allocate(".geq$007c3244v")) __declspec(align(4)) uint8_t waypoint_fade_far[176] = {0};

/** 0x008603b0..0x008603cc: object_data, noncollideable_cluster_first, noncollideable_object_references and 1 more */
#pragma section(".geq$008603b0v", read, write)
__declspec(allocate(".geq$008603b0v")) __declspec(align(16)) uint8_t object_data[16] = {0};
#pragma section(".geq$008603c0v", read, write)
__declspec(allocate(".geq$008603c0v")) __declspec(align(16)) uint8_t noncollideable_cluster_first[4] = {0};
#pragma section(".geq$008603c4v", read, write)
__declspec(allocate(".geq$008603c4v")) __declspec(align(4)) uint8_t noncollideable_object_references[4] = {0};
#pragma section(".geq$008603c8v", read, write)
__declspec(allocate(".geq$008603c8v")) __declspec(align(8)) uint8_t noncollideable_cluster_partition[4] = {0};

/** 0x008603d0..0x008603dc: collideable_cluster_first, collideable_object_references, collideable_cluster_partition */
#pragma section(".geq$008603d0v", read, write)
__declspec(allocate(".geq$008603d0v")) __declspec(align(16)) uint8_t collideable_cluster_first[4] = {0};
#pragma section(".geq$008603d4v", read, write)
__declspec(allocate(".geq$008603d4v")) __declspec(align(4)) uint8_t collideable_object_references[4] = {0};
#pragma section(".geq$008603d8v", read, write)
__declspec(allocate(".geq$008603d8v")) __declspec(align(8)) uint8_t collideable_cluster_partition[4] = {0};

/** 0x008603e0..0x008607a0: the control binding region, strided through four names */
#pragma section(".geq$008603e0v", read, write)
__declspec(allocate(".geq$008603e0v")) __declspec(align(16)) uint8_t g_control_binding_region_e0[4] = {0};
#pragma section(".geq$008603e4v", read, write)
__declspec(allocate(".geq$008603e4v")) __declspec(align(4)) uint8_t g_control_binding_region_e4[8] = {0};
#pragma section(".geq$008603ecv", read, write)
__declspec(allocate(".geq$008603ecv")) __declspec(align(4)) uint8_t g_control_binding_region_ec[4] = {0};
#pragma section(".geq$008603f0v", read, write)
__declspec(allocate(".geq$008603f0v")) __declspec(align(16)) uint8_t g_control_binding_id[6] = {0};
#pragma section(".geq$008603f6v", read, write)
__declspec(allocate(".geq$008603f6v")) __declspec(align(2)) uint8_t g_control_binding_value[938] = {0};

/** 0x00860b20..0x00860b40: light_cluster_first, light_cluster_references, light_object_references */
#pragma section(".geq$00860b20v", read, write)
__declspec(allocate(".geq$00860b20v")) __declspec(align(16)) uint8_t light_cluster_first[4] = {0};
#pragma section(".geq$00860b24v", read, write)
__declspec(allocate(".geq$00860b24v")) __declspec(align(4)) uint8_t light_cluster_references[4] = {0};
#pragma section(".geq$00860b28v", read, write)
__declspec(allocate(".geq$00860b28v")) __declspec(align(8)) uint8_t light_object_references[24] = {0};

/** 0x00861340..0x00861de0: network_server_storage, unknown_00861d4e, unknown_00861d4f */
#pragma section(".geq$00861340v", read, write)
__declspec(allocate(".geq$00861340v")) __declspec(align(16)) uint8_t network_server_storage[2574] = {0};
#pragma section(".geq$00861d4ev", read, write)
__declspec(allocate(".geq$00861d4ev")) __declspec(align(2)) uint8_t unknown_00861d4e[1] = {0};
#pragma section(".geq$00861d4fv", read, write)
__declspec(allocate(".geq$00861d4fv")) __declspec(align(1)) uint8_t unknown_00861d4f[145] = {0};

/** 0x00871de0..0x00872de0: message_delta_definition_table */
#pragma section(".geq$00871de0v", read, write)
__declspec(allocate(".geq$00871de0v")) __declspec(align(16)) uint8_t message_delta_definition_table[4096] = {0};

/** 0x0087a478..0x0087a480: local_player_globals, team_data */
#pragma section(".geq$0087a478", read, write)
__declspec(allocate(".geq$0087a478")) __declspec(align(16)) uint8_t eq_pad_0087a478[8] = {0};
#pragma section(".geq$0087a478v", read, write)
__declspec(allocate(".geq$0087a478v")) __declspec(align(8)) uint8_t local_player_globals[4] = {0};
#pragma section(".geq$0087a47cv", read, write)
__declspec(allocate(".geq$0087a47cv")) __declspec(align(4)) uint8_t team_data[4] = {0};

/** 0x0087a680..0x0087a9a0: king_alt_team_scores_network, king_alt_team_scores_network2, king_alt_player_scores_network and 3 more */
#pragma section(".geq$0087a680v", read, write)
__declspec(allocate(".geq$0087a680v")) __declspec(align(16)) uint8_t king_alt_team_scores_network[4] = {0};
#pragma section(".geq$0087a684v", read, write)
__declspec(allocate(".geq$0087a684v")) __declspec(align(4)) uint8_t king_alt_team_scores_network2[64] = {0};
#pragma section(".geq$0087a6c4v", read, write)
__declspec(allocate(".geq$0087a6c4v")) __declspec(align(4)) uint8_t king_alt_player_scores_network[128] = {0};
#pragma section(".geq$0087a744v", read, write)
__declspec(allocate(".geq$0087a744v")) __declspec(align(4)) uint8_t king_alt_scores_network_tail[156] = {0};
#pragma section(".geq$0087a7e0v", read, write)
__declspec(allocate(".geq$0087a7e0v")) __declspec(align(16)) uint8_t king_team_hill_seconds_network[420] = {0};
#pragma section(".geq$0087a984v", read, write)
__declspec(allocate(".geq$0087a984v")) __declspec(align(4)) uint8_t king_hill_broadcast_overrun_value[28] = {0};

/** 0x0087aa14..0x0087aa24: game_engine_nameplate_fade_opacity_array, game_engine_dedicated_idle, game_engine_dedicated_idle_timer and 1 more */
#pragma section(".geq$0087aa14", read, write)
__declspec(allocate(".geq$0087aa14")) __declspec(align(16)) uint8_t eq_pad_0087aa14[4] = {0};
#pragma section(".geq$0087aa14v", read, write)
__declspec(allocate(".geq$0087aa14v")) __declspec(align(4)) uint8_t game_engine_nameplate_fade_opacity_array[4] = {0};
#pragma section(".geq$0087aa18v", read, write)
__declspec(allocate(".geq$0087aa18v")) __declspec(align(8)) uint8_t game_engine_dedicated_idle[4] = {0};
#pragma section(".geq$0087aa1cv", read, write)
__declspec(allocate(".geq$0087aa1cv")) __declspec(align(4)) uint8_t game_engine_dedicated_idle_timer[4] = {0};
#pragma section(".geq$0087aa20v", read, write)
__declspec(allocate(".geq$0087aa20v")) __declspec(align(16)) uint8_t game_engine_round_reset_tick[4] = {0};

/** 0x0087aa40..0x0087ab18: variant_defaults_source, unknown_0087aa7f, game_engine_pending_variant and 3 more */
#pragma section(".geq$0087aa40v", read, write)
__declspec(allocate(".geq$0087aa40v")) __declspec(align(16)) uint8_t variant_defaults_source[63] = {0};
#pragma section(".geq$0087aa7fv", read, write)
__declspec(allocate(".geq$0087aa7fv")) __declspec(align(1)) uint8_t unknown_0087aa7f[1] = {0};
#pragma section(".geq$0087aa80v", read, write)
__declspec(allocate(".geq$0087aa80v")) __declspec(align(16)) uint8_t game_engine_pending_variant[48] = {0};
#pragma section(".geq$0087aab0v", read, write)
__declspec(allocate(".geq$0087aab0v")) __declspec(align(16)) uint8_t cached_network_engine_index[60] = {0};
#pragma section(".geq$0087aaecv", read, write)
__declspec(allocate(".geq$0087aaecv")) __declspec(align(4)) uint8_t unknown_0087aaec[12] = {0};
#pragma section(".geq$0087aaf8v", read, write)
__declspec(allocate(".geq$0087aaf8v")) __declspec(align(8)) uint8_t unknown_0087aaf8[32] = {0};

/** names the C code uses for an object defined in eq_*.c or in a slice file (two names, one address) */
#pragma comment(linker, "/alternatename:_console_color_00686af8=_actor_mode_default_look_weights")
#pragma comment(linker, "/alternatename:_message_delta_parameters_protocol_broadcast_target=_message_delta_definition_table")
#pragma comment(linker, "/alternatename:_network_message_scratch=_message_delta_definition_table")
#pragma comment(linker, "/alternatename:_object_headers=_object_data")
#pragma comment(linker, "/alternatename:_object_update_gate_globals=_main_game_globals")
#pragma comment(linker, "/alternatename:_rcon_out_channel_key=_message_delta_definition_table")
#pragma comment(linker, "/alternatename:_render_camera_global=_camera_position")
#pragma comment(linker, "/alternatename:_scoreboard_server_address_raw=_network_resolved_local_address")
#pragma comment(linker, "/alternatename:_scoreboard_server_port=_network_game_socket_port")
#pragma comment(linker, "/alternatename:_shared_hud_text_draw_state=_message_delta_definition_table")
#pragma comment(linker, "/alternatename:_teleport_flash_alpha=_teleport_effect_const_00687af8")
#pragma comment(linker, "/alternatename:_teleport_flash_blue=_teleport_effect_const_00687b04")
#pragma comment(linker, "/alternatename:_teleport_flash_duration=_teleport_effect_const_00687b08")
#pragma comment(linker, "/alternatename:_teleport_flash_green=_teleport_effect_const_00687b00")
#pragma comment(linker, "/alternatename:_teleport_flash_maximum_intensity=_teleport_effect_const_00687af4")
#pragma comment(linker, "/alternatename:_teleport_flash_red=_teleport_effect_const_00687afc")

}
