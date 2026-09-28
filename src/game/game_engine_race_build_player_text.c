// game_engine_race_build_player_text  (not a Ghidra function; the race game engine definition's +0x54 slot (unknown_54_build_player_text); no C existed, so that
//   stored pointer trapped as unlisted_46eab0)
// address 0x46eab0, size 54 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x46eab0..0x46eae5: formats the player's short +0xc6 as L"%d" (0x006607a0) into
//   the buffer and returns it.
// blam-cc: cdecl (called through the engine definition)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <wchar.h>

extern data_array *player_data; // 0x0087a480
extern void string_format_wide_va(uint16_t *dest, const uint16_t *format, ...); // 0x557930, blam-cc: EDX dest

wchar_t *game_engine_race_build_player_text(datum_index player, wchar_t *buffer)
{
    string_format_wide_va((uint16_t *)buffer, (const uint16_t *)L"%d", (int32_t)*(int16_t *)(((uint8_t *)player_data->data + ((player) & 0xffff) * 0x200) + 0xc6));
    return buffer;
}
