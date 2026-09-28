// game_engine_ctf_player_round_reset  (not a Ghidra function; the ctf game engine definition's +0x98 slot (player_round_reset); no C existed, so that
//   stored pointer trapped as unlisted_469f10)
// address 0x469f10, size 74 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x469f10..0x469f59: for a valid player handle: *(int16_t *)(player + 0xc8) = 0;
// blam-cc: cdecl (called through the engine definition)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <wchar.h>

extern data_array *player_data; // 0x0087a480
extern void *datum_get(datum_index handle, data_array *array); // 0x4d0680, blam-cc: EDX handle, ESI array

void game_engine_ctf_player_round_reset(datum_index player_index)
{
    uint8_t *player = (uint8_t *)datum_get(player_index, player_data);

    if (player != 0) {
        *(int16_t *)(player + 0xc8) = 0;
    }
}
