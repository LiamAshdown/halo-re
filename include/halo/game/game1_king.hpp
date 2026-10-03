#pragma once

/* Include after the engine type headers (types/*.h carry no include guards). */

namespace halo::game::engine1 {

/**
 * King-of-the-hill game engine entry points: scores, round resets and score text. Stateless service class:
 * every function is a static member and the state it acts on lives in the engine globals.
 */
class King {
public:
    static uint8_t build_message_text(datum_index recipient, int32_t message_type, datum_index subject, wchar_t *text, uint32_t count);
    static wchar_t *build_player_text(datum_index player, wchar_t *buffer);
    static wchar_t *build_score_header_text(wchar_t *buffer);
    static wchar_t *build_team_score_text(int32_t team, wchar_t *buffer);
    static int32_t get_score(datum_index player, int32_t team_mode);
    static int32_t get_team_score(int32_t team);
    static uint8_t initialize_for_new_game(void);
    static void player_new_life(datum_index player_index);
    static void player_round_reset(datum_index player_index);
    static void profile_post_update(void **context);
    static uint8_t query_player_score(int32_t key, int32_t index, void *buffer);
    static uint8_t query_team_score(int32_t key, int32_t team, void *buffer);
    static void reset_objects(void);
    static void reset_round(void);
    static void unknown_48(void);
    static uint8_t waypoint_filter(datum_index player);

private:
    static const uint16_t *game_text(int16_t index);
    static const uint16_t *place_text(datum_index recipient);
    static uint16_t *multiplayer_text(int16_t index);
    static void skip_unchanged_message(message_delta_decode_state *state);
    static uint8_t read_changed(void **context, void *changed_base, void *destination);
};

}
