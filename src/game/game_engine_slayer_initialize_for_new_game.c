// game_engine_slayer_initialize_for_new_game  (not a Ghidra function; the slayer game engine definition's +0x0c slot (initialize_for_new_game); no C existed, so that
//   stored pointer trapped as unlisted_46f380)
// address 0x46f380, size 55 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x46f380..0x46f3b6: zeroes the slayer team and player scores and the two arrays
//   at 0x0087a4a0 / 0x0087a4e0; returns 1.
// blam-cc: cdecl (called through the engine definition)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <wchar.h>
#include <string.h>

extern int32_t slayer_team_score[16]; // 0x006b13d8
extern int32_t slayer_player_score[16]; // 0x006b1418
extern int32_t slayer_unknown_0087a4a0[16]; // 0x0087a4a0, UNSURE
extern int32_t slayer_unknown_0087a4e0[16]; // 0x0087a4e0, UNSURE

uint8_t game_engine_slayer_initialize_for_new_game(void)
{
    memset(slayer_team_score, 0, sizeof(slayer_team_score));
    memset(slayer_player_score, 0, sizeof(slayer_player_score));
    memset(slayer_unknown_0087a4a0, 0, sizeof(slayer_unknown_0087a4a0));
    memset(slayer_unknown_0087a4e0, 0, sizeof(slayer_unknown_0087a4e0));
    return 1;
}
