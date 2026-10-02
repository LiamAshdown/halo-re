// game_engine_slayer_get_score  (not a Ghidra function; the slayer game engine definition's +0x4c slot (get_score); no C existed, so that
//   stored pointer trapped as unlisted_46f980)
// address 0x46f980, size 58 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x46f980..0x46f9b9: team_mode 1: the team score of the player's team; otherwise
//   the player score.
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
extern int32_t slayer_team_score[16]; // 0x006b13d8
extern int32_t slayer_player_score[16]; // 0x006b1418
extern int32_t slayer_unknown_0087a4a0[16]; // 0x0087a4a0, UNSURE
extern int32_t slayer_unknown_0087a4e0[16]; // 0x0087a4e0, UNSURE

int32_t game_engine_slayer_get_score(datum_index player, int32_t team_mode)
{
    if (team_mode == 1) {
        return slayer_team_score[*(int32_t *)(((uint8_t *)player_data->data + ((player) & 0xffff) * 0x200) + 0x20)];
    }
    return slayer_player_score[player & 0xffff];
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
