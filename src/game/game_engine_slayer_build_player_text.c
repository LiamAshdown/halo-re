// game_engine_slayer_build_player_text  (not a Ghidra function; the slayer game engine definition's +0x54 slot (unknown_54_build_player_text); no C existed, so that
//   stored pointer trapped as unlisted_46f9e0)
// address 0x46f9e0, size 41 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x46f9e0..0x46fa08: formats the player score as L"%d" (0x006607a0) into the
//   buffer and returns it.
// blam-cc: cdecl (called through the engine definition)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <wchar.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int32_t slayer_team_score[16]; // 0x006b13d8
extern int32_t slayer_player_score[16]; // 0x006b1418
extern int32_t slayer_unknown_0087a4a0[16]; // 0x0087a4a0, UNSURE
extern int32_t slayer_unknown_0087a4e0[16]; // 0x0087a4e0, UNSURE
extern void string_format_wide_va(uint16_t *dest, const uint16_t *format, ...); // 0x557930, blam-cc: EDX dest

wchar_t *game_engine_slayer_build_player_text(datum_index player, wchar_t *buffer)
{
    string_format_wide_va((uint16_t *)buffer, (const uint16_t *)L"%d", slayer_player_score[player & 0xffff]);
    return buffer;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
