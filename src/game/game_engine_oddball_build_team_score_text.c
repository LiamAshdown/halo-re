// game_engine_oddball_build_team_score_text  (not a Ghidra function; the oddball game engine definition's +0x5c slot (build_team_score_text); no C existed, so that
//   stored pointer trapped as unlisted_46d020)
// address 0x46d020, size 64 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x46d020..0x46d05f: the team score, as L"%d" when the variant's +0x8c is 2, else
//   as minutes:seconds; returns the buffer.
// blam-cc: cdecl (called through the engine definition)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <wchar.h>

extern game_variant game_engine_variant; // 0x006f1c88
extern int32_t king_alt_team_score[16]; // 0x006b114c (oddball team score)
extern void string_format_wide_va(uint16_t *dest, const uint16_t *format, ...); // 0x557930, blam-cc: EDX dest
extern void game_time_format_minutes_seconds(uint32_t ticks, uint32_t unused, wchar_t *dest); // 0x466530, blam-cc: ECX ticks

wchar_t *game_engine_oddball_build_team_score_text(int32_t team, wchar_t *buffer)
{
    int32_t score = king_alt_team_score[team];

    if (game_engine_variant.oddball_style == 2) {
        string_format_wide_va((uint16_t *)buffer, (const uint16_t *)L"%d", score);
    } else {
        game_time_format_minutes_seconds((uint32_t)score, 0x100, buffer);
    }
    return buffer;
}
