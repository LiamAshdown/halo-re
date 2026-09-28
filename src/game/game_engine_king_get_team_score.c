// game_engine_king_get_team_score  (not a Ghidra function; the king game engine definition's +0x50 slot (get_team_score); no C existed, so that
//   stored pointer trapped as unlisted_46b240)
// address 0x46b240, size 12 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x46b240..0x46b24b: the team's hill ticks.
// blam-cc: cdecl (called through the engine definition)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <wchar.h>

extern int32_t king_bucket_credit_ticks[16]; // 0x006b0ec0

int32_t game_engine_king_get_team_score(int32_t team)
{
    return king_bucket_credit_ticks[team];
}
