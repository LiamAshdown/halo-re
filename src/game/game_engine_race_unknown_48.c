// game_engine_race_unknown_48  (not a Ghidra function; the race game engine definition's +0x48 slot (unknown_48); no C existed, so that
//   stored pointer trapped as unlisted_46e400)
// address 0x46e400, size 118 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x46e400..0x46e475: at game tick 2 queues sound 0x22 (teams) or 0x14; with teams,
//   a team without scoring capacity (0x46e250, team 0 then 1) begins the end game sequence; then the catch-up speed
//   boost (tail call).
// blam-cc: cdecl (called through the engine definition)
// FIXED 2026-09-28 (mp sound): 0x46be40 takes ESI sound, EDI player and a stack broadcast byte; the
//   call(s) here now pass all three as the binary loads them (they passed one value before).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "fn_game.h"
#include <wchar.h>

extern game_time_globals *game_time; // 0x006f1d6c
extern game_engine_definition *current_game_engine;
extern uint8_t game_engine_teams_enabled_flag; // 0x006f1cbc


void game_engine_race_unknown_48(void)
{
    uint8_t teams = current_game_engine != 0 ? game_engine_teams_enabled_flag : 0;

    if (game_time->game_time == 2) {
        game_engine_queue_multiplayer_sound(teams ? 0x22 : 0x14, 0xffffffff, 0);
    }
    if (current_game_engine != 0 && game_engine_teams_enabled_flag != 0) {
        if (game_engine_team_has_scoring_capacity(0) == 0) {
            game_engine_begin_end_game_sequence();
        }
        if (game_engine_team_has_scoring_capacity(1) == 0) {
            game_engine_begin_end_game_sequence();
        }
    }
    game_engine_apply_catchup_speed_boost();
}
