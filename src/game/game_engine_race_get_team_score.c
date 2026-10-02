// game_engine_race_get_team_score  (not a Ghidra function; the race game engine definition's +0x50 slot (get_team_score); no C existed, so that
//   stored pointer trapped as unlisted_46eaa0)
// address 0x46eaa0, size 12 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x46eaa0..0x46eaab: the team's bucket score.
// blam-cc: cdecl (called through the engine definition)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <wchar.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int32_t game_engine_bucket_scores[16]; // 0x006b1318

int32_t game_engine_race_get_team_score(int32_t team)
{
    return game_engine_bucket_scores[team];
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
