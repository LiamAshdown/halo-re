// game_engine_king_get_score  (not a Ghidra function; the king game engine definition's +0x4c slot (get_score); no C existed, so that
//   stored pointer trapped as unlisted_46b200)
// address 0x46b200, size 50 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x46b200..0x46b231: with team_mode, the hill ticks of the player's team (player
//   +0x20); else the player's short +0xc4.
// blam-cc: cdecl (called through the engine definition)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <wchar.h>

extern data_array *player_data; // 0x0087a480
extern int32_t king_bucket_credit_ticks[16]; // 0x006b0ec0

int32_t game_engine_king_get_score(datum_index player, int32_t team_mode)
{
    uint8_t *p = ((uint8_t *)player_data->data + ((player) & 0xffff) * 0x200);

    if (team_mode != 0) {
        return king_bucket_credit_ticks[*(int32_t *)(p + 0x20)];
    }
    return *(int16_t *)(p + 0xc4);
}
