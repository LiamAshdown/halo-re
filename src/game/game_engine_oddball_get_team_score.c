// game_engine_oddball_get_team_score  (not a Ghidra function; the oddball game engine definition's +0x50 slot (get_team_score); no C existed, so that
//   stored pointer trapped as unlisted_46cee0)
// address 0x46cee0, size 12 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x46cee0..0x46ceeb: the team score.
// blam-cc: cdecl (called through the engine definition)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <wchar.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int32_t king_alt_team_score[16]; // 0x006b114c (oddball team score)

int32_t game_engine_oddball_get_team_score(int32_t team)
{
    return king_alt_team_score[team];
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
