// game_engine_ctf_query_team_score  (not a Ghidra function; the ctf game engine definition's +0xa4 slot (unknown_a4); no C existed, so that
//   stored pointer trapped as unlisted_469fe0)
// address 0x469fe0, size 39 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x469fe0..0x46a006: GameSpy query report: for key 0x1d writes the team score
//   (ctf_team_flag_touch_count[team]) into the report (0x616640) and returns 1; other keys 0.
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
extern void qr2_buffer_add_int(void *buffer, int32_t value); // 0x616640, GameSpy query-report field writer (networking phase)

uint8_t game_engine_ctf_query_team_score(int32_t key, int32_t team, void *buffer)
{
    if (key != 0x1d) {
        return 0;
    }
    qr2_buffer_add_int(buffer, ctf_team_flag_touch_count[team]);
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
