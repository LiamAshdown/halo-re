// game_engine_oddball_player_round_reset  (not a Ghidra function; the oddball game engine definition's +0x98 slot (player_round_reset); no C existed, so that
//   stored pointer trapped as unlisted_46d310)
// address 0x46d310, size 84 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x46d310..0x46d363: for a valid player handle: king_alt_player_score[player_index
//   & 0xffff] = 0;
// blam-cc: cdecl (called through the engine definition)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <wchar.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *player_data; // 0x0087a480
extern void *datum_get(datum_index handle, data_array *array); // 0x4d0680, blam-cc: EDX handle, ESI array
extern int32_t king_alt_player_score[]; // 0x006b118c (oddball player score)

void game_engine_oddball_player_round_reset(datum_index player_index)
{
    uint8_t *player = (uint8_t *)datum_get(player_index, player_data);

    if (player != 0) {
        king_alt_player_score[player_index & 0xffff] = 0;
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
