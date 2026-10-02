// game_engine_oddball_get_score  (not a Ghidra function; the oddball game engine definition's +0x4c slot (get_score); no C existed, so that
//   stored pointer trapped as unlisted_46cea0)
// address 0x46cea0, size 58 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x46cea0..0x46ced9: team_mode 1: the team score of the player's team; otherwise
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
extern int32_t king_alt_team_score[16]; // 0x006b114c (oddball team score)
extern int32_t king_alt_player_score[]; // 0x006b118c (oddball player score)

int32_t game_engine_oddball_get_score(datum_index player, int32_t team_mode)
{
    if (team_mode == 1) {
        return king_alt_team_score[*(int32_t *)(((uint8_t *)player_data->data + ((player) & 0xffff) * 0x200) + 0x20)];
    }
    return king_alt_player_score[player & 0xffff];
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
