// game_engine_king_build_player_text  (not a Ghidra function; the king game engine definition's +0x54 slot (unknown_54_build_player_text); no C existed, so that
//   stored pointer trapped as unlisted_46b6e0)
// address 0x46b6e0, size 52 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x46b6e0..0x46b713: formats the player's short +0xc4 as minutes:seconds
//   (0x466530) into the buffer and returns it.
// blam-cc: cdecl (called through the engine definition)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <wchar.h>

extern data_array *player_data; // 0x0087a480
extern void game_time_format_minutes_seconds(uint32_t ticks, uint32_t unused, wchar_t *dest); // 0x466530, blam-cc: ECX ticks

wchar_t *game_engine_king_build_player_text(datum_index player, wchar_t *buffer)
{
    game_time_format_minutes_seconds((uint32_t)((int32_t)*(int16_t *)(((uint8_t *)player_data->data + ((player) & 0xffff) * 0x200) + 0xc4)), 0x100, buffer);
    return buffer;
}
