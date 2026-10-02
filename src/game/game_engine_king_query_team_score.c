// game_engine_king_query_team_score  (not a Ghidra function; the king game engine definition's +0xa4 slot (unknown_a4); no C existed, so that
//   stored pointer trapped as unlisted_46bb40)
// address 0x46bb40, size 39 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x46bb40..0x46bb66: GameSpy query report: for key 0x1d writes the team score
//   (king_bucket_credit_ticks[team]) into the report (0x616640) and returns 1; other keys 0.
// blam-cc: cdecl (called through the engine definition)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <wchar.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int32_t king_bucket_credit_ticks[16]; // 0x006b0ec0
extern void qr2_buffer_add_int(void *buffer, int32_t value); // 0x616640, GameSpy query-report field writer (networking phase)

uint8_t game_engine_king_query_team_score(int32_t key, int32_t team, void *buffer)
{
    if (key != 0x1d) {
        return 0;
    }
    qr2_buffer_add_int(buffer, king_bucket_credit_ticks[team]);
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
