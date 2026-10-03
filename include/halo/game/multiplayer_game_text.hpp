#pragma once

#include <stdint.h>

namespace halo::game::mp_text {

/** Indices into the "ui\\multiplayer_game_text" unicode string list used by the scoreboard and engine HUD. */
inline constexpr int16_t k_team_label_a = 0xc;
inline constexpr int16_t k_team_label_b = 0xd;
inline constexpr int16_t k_lives_none_left = 0x34;
inline constexpr int16_t k_lives_one_left = 0x35;
inline constexpr int16_t k_lives_left_format = 0x36;
inline constexpr int16_t k_result_win_teams = 0x38;
inline constexpr int16_t k_result_win_solo = 0x39;
inline constexpr int16_t k_result_loss_teams = 0x3a;
inline constexpr int16_t k_result_loss_solo = 0x3b;
inline constexpr int16_t k_team_leading_format = 0x3c;
inline constexpr int16_t k_team_trailing_format = 0x3d;
inline constexpr int16_t k_team_tied_format = 0x3e;
inline constexpr int16_t k_player_tied_format = 0x3f;
inline constexpr int16_t k_player_place_format = 0x40;
inline constexpr int16_t k_post_game_team_score_format_a = 0x41;
inline constexpr int16_t k_post_game_team_score_format_b = 0x42;
inline constexpr int16_t k_scoreboard_column_a = 0x43;
inline constexpr int16_t k_scoreboard_column_b = 0x44;
inline constexpr int16_t k_scoreboard_column_c = 0x45;
inline constexpr int16_t k_scoreboard_column_d = 0x46;
inline constexpr int16_t k_scoreboard_column_e = 0x47;
inline constexpr int16_t k_post_game_prompt_host = 0x48;
inline constexpr int16_t k_post_game_prompt_client = 0x49;
inline constexpr int16_t k_status_out_of_lives = 0x8a;
inline constexpr int16_t k_status_leaving = 0x8b;
inline constexpr int16_t k_server_address_label = 0xbe;

}  // namespace halo::game::mp_text
