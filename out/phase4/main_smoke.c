#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "main.h"

// terminal_console (types/interface.h) holds text_edit_state, which holds a char *, so the
// console_globals checks past +0x004 only fire with -m32. Run both:
//   gcc -fsyntax-only -I types out/phase4/main_smoke.c
//   gcc -m32 -fsyntax-only -I types out/phase4/main_smoke.c
#define PTRS32 (sizeof(void *) == 4)
#define CHECK(name, cond) typedef char check_##name[(cond) ? 1 : -1]
#define CHECK32(name, cond) typedef char check_##name[(!PTRS32 || (cond)) ? 1 : -1]
#define OFF(t, f) __builtin_offsetof(t, f)

// sizes
CHECK(main_globals, sizeof(main_globals) == 0x3b0);
CHECK(main_frame_rate_average, sizeof(main_frame_rate_average) == 0x4c);
CHECK(timedemo_bucket, sizeof(timedemo_bucket) == 0x08);
CHECK(timedemo_globals, sizeof(timedemo_globals) == 0x68);
CHECK(multiplayer_map_table_entry, sizeof(multiplayer_map_table_entry) == 0x0c);
CHECK(bink_movie_prefix, sizeof(bink_movie_prefix) == 0x100);
CHECK32(console_globals, sizeof(console_globals) == 0x9c4);
CHECK32(terminal_console, sizeof(terminal_console) == 0x1be);

// console_globals against the absolute addresses (base 0x006b7020)
CHECK(cg_enabled, OFF(console_globals, enabled) == 0x6b7021 - 0x6b7020);
CHECK(cg_terminal, OFF(console_globals, terminal) == 0x6b7024 - 0x6b7020);
CHECK(cg_key_code0, OFF(console_globals, terminal.key_events[0].key_code) == 0x6b7028 - 0x6b7020);
CHECK(cg_color, OFF(console_globals, terminal.color) == 0x6b70a8 - 0x6b7020);
CHECK(cg_prompt, OFF(console_globals, terminal.prompt) == 0x6b70b8 - 0x6b7020);
CHECK(cg_input, OFF(console_globals, terminal.input) == 0x6b70d8 - 0x6b7020);
CHECK32(cg_edit, OFF(console_globals, terminal.edit) == 0x6b71d8 - 0x6b7020);
CHECK32(cg_cursor, OFF(console_globals, terminal.edit.cursor) == 0x6b71de - 0x6b7020);
CHECK32(cg_anchor, OFF(console_globals, terminal.edit.selection_anchor) == 0x6b71e0 - 0x6b7020);
CHECK32(cg_history, OFF(console_globals, history) == 0x6b71e4 - 0x6b7020);
CHECK32(cg_count, OFF(console_globals, history_count) == 0x6b79dc - 0x6b7020);
CHECK32(cg_newest, OFF(console_globals, history_newest_index) == 0x6b79de - 0x6b7020);
CHECK32(cg_browse, OFF(console_globals, history_browse_index) == 0x6b79e0 - 0x6b7020);
CHECK32(cg_end, 0x6b7020 + sizeof(console_globals) <= 0x6b79e8);

// main_globals against the absolute addresses (base 0x00719700)
#define MG(field, addr) CHECK(mg_##field, OFF(main_globals, field) == (addr) - 0x719700)
MG(frame_time_ms, 0x719708);
MG(render_counter_low, 0x719710);
MG(frame_time_overflow, 0x719718);
MG(frame_delta_time, 0x71971c);
MG(game_connection, 0x719720);
MG(screenshot_index, 0x719722);
MG(movie_frame_bitmap, 0x719724);
MG(movie_frame_index, 0x719730);
MG(movie_frame_delta_time, 0x719734);
MG(reset_map, 0x719738);
MG(level_transition, 0x719739);
MG(revert_map, 0x71973a);
MG(revert_map_if_allowed, 0x71973b);
MG(save_map, 0x71973c);
MG(save_map_require_safe, 0x71973d);
MG(save_map_with_timeout, 0x71973e);
MG(save_map_write_pending, 0x71973f);
MG(save_map_retry_countdown, 0x719740);
MG(save_map_attempt_count, 0x719744);
MG(level_transition_fade_end_ms, 0x719748);
MG(save_map_safe_streak, 0x71974c);
MG(won_map, 0x71974e);
MG(lost_map, 0x71974f);
MG(respawn_coop_players, 0x719750);
MG(save_core, 0x719751);
MG(load_core, 0x719752);
MG(load_core_next_session, 0x719753);
MG(switch_structure_bsp_index, 0x719754);
MG(main_menu_scenario_loaded, 0x719756);
MG(return_to_main_menu, 0x719757);
MG(unknown_058, 0x719758);
MG(idle_timeout_reached, 0x719759);
MG(unknown_05a, 0x71975a);
MG(quit, 0x71975b);
MG(idle_timeout_ms, 0x71975c);
MG(last_gameplay_time_ms, 0x719760);
MG(last_activity_time_ms, 0x719764);
MG(start_film_playback, 0x719768);
MG(time_is_running, 0x719769);
MG(reset_frame_timers, 0x71976a);
MG(unknown_06b, 0x71976b);
MG(skip_ticks, 0x71976c);
MG(skip_tick_count, 0x71976e);
MG(lost_map_frames, 0x719770);
MG(respawn_coop_frames, 0x719772);
MG(cache_file_open_pending, 0x719774);
MG(restore_checkpoint_on_load, 0x719778);
MG(scenario_path, 0x719779);
MG(multiplayer_map_name, 0x719879);
MG(pending_cache_file_name, 0x719979);
MG(connect_pending, 0x719a79);
MG(connect_address, 0x719a7a);
MG(connect_password, 0x719a9a);
MG(disable_frame_output, 0x719aa8);
MG(debug_game_save, 0x719aa9);
MG(screenshot_tile_count, 0x719aac);
CHECK(mg_end, 0x719700 + sizeof(main_globals) == 0x719ab0);
// the terminators the code writes land on the last byte of each buffer
CHECK(mg_path_nul, OFF(main_globals, scenario_path) + 0xff == 0x719878 - 0x719700);
CHECK(mg_mp_nul, OFF(main_globals, multiplayer_map_name) + 0xff == 0x719978 - 0x719700);
CHECK(mg_address_nul, OFF(main_globals, connect_address) + 0x1f == 0x719a99 - 0x719700);
CHECK(mg_password_nul, OFF(main_globals, connect_password) + 8 == 0x719aa2 - 0x719700);

// frame rate average (0x00719ab0) and timedemo (0x00719afc)
CHECK(fra_history, 0x719ab0 + OFF(main_frame_rate_average, history) == 0x719ab8);
CHECK(fra_count, 0x719ab0 + OFF(main_frame_rate_average, count) == 0x719af8);
CHECK(fra_end, 0x719ab0 + sizeof(main_frame_rate_average) == 0x719afc);
CHECK(td_total, 0x719afc + OFF(timedemo_globals, total_time_ms) == 0x719b00);
CHECK(td_b60_frames, 0x719afc + OFF(timedemo_globals, buckets[0].frames) == 0x719b08);
CHECK(td_b5_time, 0x719afc + OFF(timedemo_globals, buckets[8].time_ms) == 0x719b44);
CHECK(td_b5_frames, 0x719afc + OFF(timedemo_globals, buckets[8].frames) == 0x719b48);
CHECK(td_current, 0x719afc + OFF(timedemo_globals, current_time_ms) == 0x719b54);
CHECK(td_previous, 0x719afc + OFF(timedemo_globals, previous_time_ms) == 0x719b58);
CHECK(td_frame, 0x719afc + OFF(timedemo_globals, frame_time_ms) == 0x719b5c);
CHECK(td_last_tick, 0x719afc + OFF(timedemo_globals, last_game_time) == 0x719b60);
CHECK(td_end, 0x719afc + sizeof(timedemo_globals) == 0x719b64);

// the static tables
CHECK(mp_table_span, 0x68e588 + k_main_multiplayer_map_count * sizeof(multiplayer_map_table_entry) == 0x68e66c);
CHECK(bink_paused, OFF(bink_movie_prefix, paused) == 0xfc);

// the foreign records main relies on
CHECK(load_request, sizeof(network_scenario_load_request) == 0x10c);
CHECK(time_globals, sizeof(game_time_globals) == 0x20);

int main(void) { return 0; }
