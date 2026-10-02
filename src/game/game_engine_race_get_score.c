// game_engine_race_get_score  (not a Ghidra function; the race game engine definition's +0x4c slot (get_score); no C existed, so that
//   stored pointer trapped as unlisted_46ea00)
// address 0x46ea00, size 151 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x46ea00..0x46ea96: team_mode 1: the bucket score of the player's team; otherwise
//   the player's short +0xc6 times 0x21 plus the number of bits set in its team's captured-flags mask (0x006b12d4).
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
extern int32_t game_engine_bucket_scores[16]; // 0x006b1318
extern uint32_t ctf_team_captured_flags_mask[]; // 0x006b12d4

int32_t game_engine_race_get_score(datum_index player, int32_t team_mode)
{
    uint8_t *p = ((uint8_t *)player_data->data + ((player) & 0xffff) * 0x200);
    uint32_t mask;
    int32_t bits = 0;
    int32_t i;

    if (team_mode == 1) {
        return game_engine_bucket_scores[*(int32_t *)(p + 0x20)];
    }
    mask = ctf_team_captured_flags_mask[*(int32_t *)(p + 0x20)];
    for (i = 0; i < 0x20; i++) {
        if ((mask & (1u << i)) != 0) {
            bits++;
        }
    }
    return *(int16_t *)(p + 0xc6) * 0x21 + bits;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
