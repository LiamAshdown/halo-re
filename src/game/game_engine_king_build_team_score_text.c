// game_engine_king_build_team_score_text  (not a Ghidra function; the king game engine definition's +0x5c slot (build_team_score_text); no C existed, so that
//   stored pointer trapped as unlisted_46b7a0)
// address 0x46b7a0, size 34 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x46b7a0..0x46b7c1: formats the team's hill ticks as minutes:seconds (0x466530)
//   into the buffer and returns it.
// blam-cc: cdecl (called through the engine definition)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <wchar.h>

extern int32_t king_bucket_credit_ticks[16]; // 0x006b0ec0
extern void game_time_format_minutes_seconds(uint32_t ticks, uint32_t unused, wchar_t *dest); // 0x466530, blam-cc: ECX ticks

wchar_t *game_engine_king_build_team_score_text(int32_t team, wchar_t *buffer)
{
    game_time_format_minutes_seconds((uint32_t)(king_bucket_credit_ticks[team]), 0x100, buffer);
    return buffer;
}
