/**
 * Scoreboard entries, ranking, winner queries and end-of-game result text shared by all game engines.
 */

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <stdint.h>

#include "halo/game/game1_scoreboard.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"

extern "C" {
extern game_engine_definition *current_game_engine;
extern uint8_t game_engine_teams_enabled_flag;
extern uint8_t network_server[];
}

namespace halo::game::engine1 {

/**
 * Gathers two paired per-side totals (score-like values via a callback, and matching counts from a status
 * table) used by the lead-change comparison helpers.
 *
 * Original register convention: EDI -> out_count, stack -> out_score, filter_value.
 *
 * @address 0x470690
 */
void Scoreboard::gather_team_score_totals(uint32_t out_count[2], uint32_t out_score[2], int32_t filter_value)
{
    out_score[0] = 0;
    out_score[1] = 0;
    out_count[0] = 0;
    out_count[1] = 0;

    if (current_game_engine != 0 && game_engine_teams_enabled_flag) {
        uint8_t *entry = network_server + 0x1c8;
        int32_t i;

        for (i = 0; i < 16; i++) {
            if (halo::networking::network_player_entry_validate((network_player_entry *)(entry - 0x1e)) != 0 && (int8_t)entry[1] != filter_value) {
                int8_t category = (int8_t)entry[0];

                if (category >= 0 && category < 2) {
                    out_count[category] = out_count[category] + 1;
                }
            }
            entry = entry + 0x20;
        }
        out_score[0] = ((uint32_t (*)(int32_t))current_game_engine->get_team_score)(0);
        out_score[1] = ((uint32_t (*)(int32_t))current_game_engine->get_team_score)(1);
    }
}

}
