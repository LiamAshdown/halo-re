// game_engine_race_build_team_score_text  (not a Ghidra function; the race game engine definition's +0x5c slot (build_team_score_text); no C existed, so that
//   stored pointer trapped as unlisted_46eb50)
// address 0x46eb50, size 36 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x46eb50..0x46eb73: formats the team's bucket score as L"%d" (0x006607a0) into
//   the buffer and returns it.
// blam-cc: cdecl (called through the engine definition)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <wchar.h>

extern int32_t game_engine_bucket_scores[16]; // 0x006b1318
extern void string_format_wide_va(uint16_t *dest, const uint16_t *format, ...); // 0x557930, blam-cc: EDX dest

wchar_t *game_engine_race_build_team_score_text(int32_t team, wchar_t *buffer)
{
    string_format_wide_va((uint16_t *)buffer, (const uint16_t *)L"%d", game_engine_bucket_scores[team]);
    return buffer;
}
