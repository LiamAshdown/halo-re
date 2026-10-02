// game_engine_ctf_build_player_text  (not a Ghidra function; the ctf game engine definition's +0x54 slot (unknown_54_build_player_text); no C existed, so that
//   stored pointer trapped as unlisted_4699f0)
// address 0x4699f0, size 54 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4699f0..0x469a25: formats the player's short +0xc8 as L"%d" (0x006607a0) into
//   the buffer and returns it.
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
extern void string_format_wide_va(uint16_t *dest, const uint16_t *format, ...); // 0x557930, blam-cc: EDX dest

wchar_t *game_engine_ctf_build_player_text(datum_index player, wchar_t *buffer)
{
    string_format_wide_va((uint16_t *)buffer, (const uint16_t *)L"%d", (int32_t)*(int16_t *)(((uint8_t *)player_data->data + ((player) & 0xffff) * 0x200) + 0xc8));
    return buffer;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
