// game_engine_ctf_build_team_score_text  (not a Ghidra function; the ctf game engine definition's +0x5c slot (build_team_score_text); no C existed, so that
//   stored pointer trapped as unlisted_469ab0)
// address 0x469ab0, size 36 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x469ab0..0x469ad3: formats the team's flag touch count as L"%d" (0x006607a0)
//   into the buffer and returns it.
// blam-cc: cdecl (called through the engine definition)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <wchar.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int32_t ctf_team_flag_touch_count[2]; // 0x006b0e98
extern void string_format_wide_va(uint16_t *dest, const uint16_t *format, ...); // 0x557930, blam-cc: EDX dest

wchar_t *game_engine_ctf_build_team_score_text(int32_t team, wchar_t *buffer)
{
    string_format_wide_va((uint16_t *)buffer, (const uint16_t *)L"%d", ctf_team_flag_touch_count[team]);
    return buffer;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
