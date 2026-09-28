// game_engine_king_query_player_score  (not a Ghidra function; the king game engine definition's +0xa0 slot (unknown_a0); no C existed, so that
//   stored pointer trapped as unlisted_46ba90)
// address 0x46ba90, size 162 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x46ba90..0x46bb31: GameSpy player query: for key 0x16 and an active player at
//   the index, formats *(int16_t *)(player + 0xc4) as ASCII minutes:seconds (0x466600, 0x100 characters) and writes
//   it into the report (0x615590); returns 1, else 0.
// blam-cc: cdecl (called through the engine definition)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <wchar.h>

extern data_array *player_data; // 0x0087a480
extern void *datum_get(datum_index handle, data_array *array); // 0x4d0680, blam-cc: EDX handle, ESI array
extern uint32_t players_get_active_by_index(int32_t index); // 0x45c6f0, blam-cc: EAX
extern void qr2_buffer_add(void *buffer, const char *value); // 0x615590, GameSpy query-report string writer (networking phase)
extern void game_time_format_minutes_seconds_ascii(uint32_t ticks, uint32_t count, char *dest); // 0x466600, blam-cc: ECX ticks

uint8_t game_engine_king_query_player_score(int32_t key, int32_t index, void *buffer)
{
    uint32_t handle = players_get_active_by_index(index);
    uint8_t *player = (uint8_t *)datum_get(handle, player_data);
    char text[0x100];

    if (player == 0 || key != 0x16) {
        return 0;
    }
    game_time_format_minutes_seconds_ascii((uint32_t)(*(int16_t *)(player + 0xc4)), 0x100, text);
    qr2_buffer_add(buffer, text);
    return 1;
}
