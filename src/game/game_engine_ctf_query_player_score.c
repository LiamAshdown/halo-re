// game_engine_ctf_query_player_score  (not a Ghidra function; the ctf game engine definition's +0xa0 slot (unknown_a0); no C existed, so that
//   stored pointer trapped as unlisted_469f60)
// address 0x469f60, size 113 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x469f60..0x469fd0: GameSpy player query: for key 0x16 and an active player at
//   the index, writes its short +0xc8 into the report (0x616640) and returns 1; else 0.
// blam-cc: cdecl (called through the engine definition)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <wchar.h>
#include "objects.h"
#include "units.h"

extern data_array *player_data; // 0x0087a480
extern void *datum_get(datum_index handle, data_array *array); // 0x4d0680, blam-cc: EDX handle, ESI array
extern uint32_t players_get_active_by_index(int32_t index); // 0x45c6f0, blam-cc: EAX
extern void qr2_buffer_add_int(void *buffer, int32_t value); // 0x616640, GameSpy query-report field writer (networking phase)

uint8_t game_engine_ctf_query_player_score(int32_t key, int32_t index, void *buffer)
{
    uint8_t *player = (uint8_t *)datum_get(players_get_active_by_index(index), player_data);

    if (player == 0 || key != 0x16) {
        return 0;
    }
    qr2_buffer_add_int(buffer, ((struct player *)player)->unknown_c8);
    return 1;
}
