// game_engine_slayer_player_round_reset  (not a Ghidra function; the slayer game engine definition's +0x98 slot (player_round_reset); no C existed, so that
//   stored pointer trapped as unlisted_46fc70)
// address 0x46fc70, size 178 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x46fc70..0x46fd21: for a valid player: its +0x88 becomes -1 and its score 0, and
//   every player whose +0x88 names it is reset to -1 too (a player data iterator).
// blam-cc: cdecl (called through the engine definition)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include <wchar.h>
#include "objects.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *player_data; // 0x0087a480
extern void *datum_get(datum_index handle, data_array *array); // 0x4d0680, blam-cc: EDX handle, ESI array
extern int32_t slayer_team_score[16]; // 0x006b13d8
extern int32_t slayer_player_score[16]; // 0x006b1418
extern int32_t slayer_unknown_0087a4a0[16]; // 0x0087a4a0, UNSURE
extern int32_t slayer_unknown_0087a4e0[16]; // 0x0087a4e0, UNSURE
extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, blam-cc: EDI

void game_engine_slayer_player_round_reset(datum_index player_index)
{
    uint8_t *player = (uint8_t *)datum_get(player_index, player_data);
    data_iterator iterator;
    uint8_t *other;

    if (player == 0) {
        return;
    }
    ((struct player *)player)->slayer_target = -1;
    slayer_player_score[player_index & 0xffff] = 0;
    iterator.data = player_data;
    iterator.next_index = 0;
    iterator.index = 0xffffffff;
    iterator.signature = (uint32_t)player_data ^ 0x69746572; // 'reti'
    for (other = (uint8_t *)data_iterator_next(&iterator); other != 0; other = (uint8_t *)data_iterator_next(&iterator)) {
        if (*(datum_index *)(other + 0x88) == player_index) {
            *(int32_t *)(other + 0x88) = -1;
        }
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
