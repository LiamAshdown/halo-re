// game_engine_ctf_get_team_score  (not a Ghidra function; the ctf game engine definition's +0x50 slot (get_team_score); no C existed, so that
//   stored pointer trapped as unlisted_4699d0)
// address 0x4699d0, size 12 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4699d0..0x4699db: the team's flag touch count.
// blam-cc: cdecl (called through the engine definition)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <wchar.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int32_t ctf_team_flag_touch_count[2]; // 0x006b0e98

int32_t game_engine_ctf_get_team_score(int32_t team)
{
    return ctf_team_flag_touch_count[team];
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
