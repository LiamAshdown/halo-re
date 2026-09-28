// game_engine_slayer_reset_objects  (not a Ghidra function; the slayer game engine definition's +0xac slot (reset_objects); no C existed, so that
//   stored pointer trapped as unlisted_46fde0)
// address 0x46fde0, size 39 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x46fde0..0x46fe06: as the server, zeroes the slayer team and player scores.
// blam-cc: cdecl (called through the engine definition)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <wchar.h>
#include <string.h>

extern int16_t network_game_mode; // 0x00719720
extern int32_t slayer_team_score[16]; // 0x006b13d8
extern int32_t slayer_player_score[16]; // 0x006b1418
extern int32_t slayer_unknown_0087a4a0[16]; // 0x0087a4a0, UNSURE
extern int32_t slayer_unknown_0087a4e0[16]; // 0x0087a4e0, UNSURE

void game_engine_slayer_reset_objects(void)
{
    if (network_game_mode == 2) {
        memset(slayer_team_score, 0, sizeof(slayer_team_score));
        memset(slayer_player_score, 0, sizeof(slayer_player_score));
    }
}
