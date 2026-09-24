#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

// The header targets a 32-bit image, so every size below is stated for 4-byte pointers and
// corrected by the host pointer width times the number of pointers the struct contains.
#define PW ((int)sizeof(void *) - 4)

typedef char check_game_time_globals[(sizeof(game_time_globals) == 0x20) ? 1 : -1];
typedef char check_game_variant[(sizeof(game_variant) == 0x98) ? 1 : -1];
typedef char check_game_engine_definition[(sizeof(game_engine_definition) == 0xb0 + 43*PW) ? 1 : -1];
typedef char check_game_variant_history_entry[(sizeof(game_variant_history_entry) == 0xa4 + 2*PW) ? 1 : -1];
typedef char check_player[(sizeof(player) == 0x200 + 6*PW) ? 1 : -1];
typedef char check_team[(sizeof(team) == 0x40) ? 1 : -1];
typedef char check_player_globals[(sizeof(player_globals) == 0x98) ? 1 : -1];
typedef char check_local_player_control[(sizeof(local_player_control) == 0x40) ? 1 : -1];
typedef char check_player_control_globals[(sizeof(player_control_globals) == 0x50) ? 1 : -1];
typedef char check_player_profile[(sizeof(player_profile) == 0x30) ? 1 : -1];
typedef char check_team_pair_override[(sizeof(team_pair_override) == 0x12) ? 1 : -1];
typedef char check_team_pair_globals[(sizeof(team_pair_globals) == 0xb4) ? 1 : -1];
typedef char check_scoreboard_entry[(sizeof(scoreboard_entry) == 0x1c) ? 1 : -1];
typedef char check_custom_waypoint[(sizeof(custom_waypoint) == 0x20) ? 1 : -1];
typedef char check_multiplayer_sound_request[(sizeof(multiplayer_sound_request) == 0x10) ? 1 : -1];
typedef char check_circular_queue[(sizeof(circular_queue) == 0x18 + 2*PW) ? 1 : -1];
typedef char check_player_update_queue[(sizeof(player_update_queue) == 0x3c + 2*PW) ? 1 : -1];
typedef char check_update_record[(sizeof(update_record) == 0x308) ? 1 : -1];
typedef char check_update_server_queue[(sizeof(update_server_queue) == 0x64 + 2*PW) ? 1 : -1];
typedef char check_observer_target_candidate[(sizeof(observer_target_candidate) == 0x38) ? 1 : -1];
typedef char check_observer_target_cone[(sizeof(observer_target_cone) == 0x10) ? 1 : -1];
typedef char check_ctf_globals[(sizeof(ctf_globals) == 0x148) ? 1 : -1];
typedef char check_king_globals[(sizeof(king_globals) == 0x0c) ? 1 : -1];
typedef char check_king_hill_marker_history[(sizeof(king_hill_marker_history) == 0x40) ? 1 : -1];
typedef char check_savegame_index_record[(sizeof(savegame_index_record) == 0x206) ? 1 : -1];
typedef char check_user_save_path_table[(sizeof(user_save_path_table) == 0x848) ? 1 : -1];

// individual field offsets the decompilation pins directly
typedef char chk_p_local[(__builtin_offsetof(player, local_player_index) == 0x02) ? 1 : -1];
typedef char chk_p_team[(__builtin_offsetof(player, team) == 0x20) ? 1 : -1];
typedef char chk_p_interact[(__builtin_offsetof(player, interaction_object) == 0x24) ? 1 : -1];
typedef char chk_p_respawn[(__builtin_offsetof(player, respawn_timer) == 0x2c) ? 1 : -1];
typedef char chk_p_unit[(__builtin_offsetof(player, unit) == 0x34) ? 1 : -1];
typedef char chk_p_prev[(__builtin_offsetof(player, previous_unit) == 0x38) ? 1 : -1];
typedef char chk_p_name2[(__builtin_offsetof(player, identifier_name) == 0x48) ? 1 : -1];
typedef char chk_p_streak[(__builtin_offsetof(player, kill_streak) == 0x68) ? 1 : -1];
typedef char chk_p_speed[(__builtin_offsetof(player, speed) == 0x6c) ? 1 : -1];
typedef char chk_p_death[(__builtin_offsetof(player, last_death_tick) == 0x84) ? 1 : -1];
typedef char chk_p_odd[(__builtin_offsetof(player, odd_man_out) == 0x8c) ? 1 : -1];
typedef char chk_p_kills[(__builtin_offsetof(player, kills) == 0x9c) ? 1 : -1];
typedef char chk_p_deaths[(__builtin_offsetof(player, deaths) == 0xae) ? 1 : -1];
typedef char chk_p_assists[(__builtin_offsetof(player, assists) == 0xa4) ? 1 : -1];
typedef char chk_prof_deaths[(__builtin_offsetof(player_profile, deaths) == 0x1a) ? 1 : -1];
typedef char chk_p_obj[(__builtin_offsetof(player, objective_time) == 0xc4) ? 1 : -1];
typedef char chk_p_del[(__builtin_offsetof(player, marked_for_deletion) == 0xd5) ? 1 : -1];
typedef char chk_p_hist[(__builtin_offsetof(player, update_history) == 0x120) ? 1 : -1];
typedef char chk_p_pos[(__builtin_offsetof(player, position_updates) == 0x170 + 2*PW) ? 1 : -1];
typedef char chk_p_veh[(__builtin_offsetof(player, vehicle_updates) == 0x1d0 + 4*PW) ? 1 : -1];

typedef char chk_v_engine[(__builtin_offsetof(game_variant, game_engine_index) == 0x30) ? 1 : -1];
typedef char chk_v_teams[(__builtin_offsetof(game_variant, teams) == 0x34) ? 1 : -1];
typedef char chk_v_lives[(__builtin_offsetof(game_variant, lives_per_round) == 0x50) ? 1 : -1];
typedef char chk_v_speed[(__builtin_offsetof(game_variant, speed_scale) == 0x54) ? 1 : -1];
typedef char chk_v_ctf[(__builtin_offsetof(game_variant, ctf_option_7c) == 0x7c) ? 1 : -1];

typedef char chk_e_reset[(__builtin_offsetof(game_engine_definition, reset_objects) == 0xac + 42*PW) ? 1 : -1];
typedef char chk_e_score[(__builtin_offsetof(game_engine_definition, get_score) == 0x4c + 18*PW) ? 1 : -1];

typedef char chk_g_tick[(__builtin_offsetof(game_time_globals, game_time) == 0x0c) ? 1 : -1];
typedef char chk_g_speed[(__builtin_offsetof(game_time_globals, speed) == 0x18) ? 1 : -1];

typedef char chk_pc_local[(__builtin_offsetof(player_control_globals, local_players) == 0x10) ? 1 : -1];
typedef char chk_lpc_pitchmin[(__builtin_offsetof(local_player_control, pitch_minimum) == 0x38) ? 1 : -1];
typedef char chk_prof_obj[(__builtin_offsetof(player_profile, objective_time) == 0x1e) ? 1 : -1];
typedef char chk_tp_enemy[(__builtin_offsetof(team_pair_globals, enemy_bits) == 0xa4) ? 1 : -1];
typedef char chk_usq[(__builtin_offsetof(update_server_queue, queue) == 0x28) ? 1 : -1];

int main(void) { return 0; }
