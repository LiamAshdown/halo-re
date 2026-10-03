#pragma once

/* Include after the engine type headers (types/*.h carry no include guards). */

namespace halo::game::engine1 {

/**
 * Scoreboard entries, ranking, winner queries and end-of-game result text shared by all game engines.
 * Stateless service class: every function is a static member and the state it acts on lives in the engine
 * globals.
 */
class Scoreboard {
public:
    static void build_end_game_result_text(datum_index player_handle, wchar_t *out);
    static uint8_t build_message_text(wchar_t *out, uint32_t buffer_size, datum_index subject, uint32_t param_1, uint32_t message_type);
    static uint32_t build_scoreboard_sort_key(uint32_t player_index, int32_t score);
    static int32_t build_sorted_player_list(uint8_t invert_low_stat, scoreboard_entry out_entries[16], int32_t mode);
    static void check_bucket_scores_and_end_round(void);
    static uint32_t compare_score_to_others(uint32_t subject, int32_t team_mode);
    static uint8_t find_first_eligible_player_on_team(int32_t team);
    static void find_player_by_name(char *source_name);
    static datum_index find_player_holding_object(datum_index target_object);
    static void gather_team_score_totals(uint32_t out_count[2], uint32_t out_score[2], int32_t filter_value);
    static wchar_t *get_default_multiplayer_string(const scoreboard_entry *entry);
    static wchar_t *get_multiplayer_text_list(uint32_t rank);
    static real *get_player_color(uint32_t player_index, real *out_rgb);
    static void get_player_scoreboard_entry(datum_index player, scoreboard_entry *out);
    static int32_t get_scoreboard_place(datum_index player, int32_t mode, uint8_t invert_low_stat);
    static uint32_t is_object_winning(uint32_t handle);
    static uint32_t is_tracked_object_winner(int32_t team);
    static uint8_t is_valid_team_player(uint32_t identifier);
    static uint8_t local_player_score_is_nonpositive(datum_index player_handle);

private:
    static wchar_t *multiplayer_game_text_string(int16_t index);
};

}
