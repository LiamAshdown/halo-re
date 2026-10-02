// game_engine_ctf_get_score  (not a Ghidra function; the ctf game engine definition's +0x4c slot (get_score); no C existed, so that
//   stored pointer trapped as unlisted_469990)
// address 0x469990, size 50 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x469990..0x4699c1: with team_mode, the flag touch count of the player's team
//   (player +0x20); else the player's short +0xc8.
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
extern int32_t ctf_team_flag_touch_count[2]; // 0x006b0e98

int32_t game_engine_ctf_get_score(datum_index player, int32_t team_mode)
{
    uint8_t *p = ((uint8_t *)player_data->data + ((player) & 0xffff) * 0x200);

    if (team_mode != 0) {
        return ctf_team_flag_touch_count[*(int32_t *)(p + 0x20)];
    }
    return *(int16_t *)(p + 0xc8);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
