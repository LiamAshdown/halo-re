// game_engine_oddball_query_team_score  (not a Ghidra function; the oddball game engine definition's +0xa4 slot (unknown_a4); no C existed, so that
//   stored pointer trapped as unlisted_46d420)
// address 0x46d420, size 39 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x46d420..0x46d446: GameSpy query report: for key 0x1d writes the team score
//   (king_alt_team_score[team]) into the report (0x616640) and returns 1; other keys 0.
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
extern void qr2_buffer_add_int(void *buffer, int32_t value); // 0x616640, GameSpy query-report field writer (networking phase)

uint8_t game_engine_oddball_query_team_score(int32_t key, int32_t team, void *buffer)
{
    if (key != 0x1d) {
        return 0;
    }
    qr2_buffer_add_int(buffer, king_alt_team_score[team]);
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
