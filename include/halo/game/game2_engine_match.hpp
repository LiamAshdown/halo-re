#pragma once

#include "halo/game/game2_types.hpp"

namespace halo::game {

/**
 * Match flow of the multiplayer engine: tick, unload, death handling, lead tracking and end-of-game sequence.
 */
class EngineMatch {
public:
    static void on_player_death(datum_index killer, datum_index death_object, datum_index victim, char is_suicide);
    static void tick(void);
    static void __cdecl unload(void);
    static void send_end_game_notification(uint32_t reason);
    static void send_round_reset_message(void);
    static void send_team_allegiance_message(char broadcast);
    static uint8_t team_close_game_check(int32_t side, int32_t filter_value);
    static uint8_t team_is_leading(int32_t filter_value);
    static void update_end_game_sequence(float delta_time);
    static void update_lead_change_state(void **envelope, uint8_t *message);

private:
    static void send_message(datum_index target, uint32_t message_type, datum_index victim, wchar_t *buffer);
    static void message_players(datum_index killer, uint32_t message_type, datum_index victim, wchar_t *buffer);
};

}
