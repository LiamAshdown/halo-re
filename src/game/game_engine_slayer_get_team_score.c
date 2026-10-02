// game_engine_slayer_get_team_score  (not a Ghidra function; the slayer game engine definition's +0x50 slot (get_team_score); no C existed, so that
//   stored pointer trapped as unlisted_46f9c0)
// address 0x46f9c0, size 12 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x46f9c0..0x46f9cb: the team score.
// blam-cc: cdecl (called through the engine definition)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <wchar.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int32_t slayer_team_score[16]; // 0x006b13d8
extern int32_t slayer_player_score[16]; // 0x006b1418
extern int32_t slayer_unknown_0087a4a0[16]; // 0x0087a4a0, UNSURE
extern int32_t slayer_unknown_0087a4e0[16]; // 0x0087a4e0, UNSURE

int32_t game_engine_slayer_get_team_score(int32_t team)
{
    return slayer_team_score[team];
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
