// game_engine_oddball_build_player_text  (not a Ghidra function; the oddball game engine definition's +0x54 slot (unknown_54_build_player_text); no C existed, so that
//   stored pointer trapped as unlisted_46cf70)
// address 0x46cf70, size 69 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x46cf70..0x46cfb4: the player score, as L"%d" when the variant's +0x8c is 2,
//   else as minutes:seconds; returns the buffer.
// blam-cc: cdecl (called through the engine definition)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <wchar.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern game_variant game_engine_variant; // 0x006f1c88
extern int32_t king_alt_player_score[]; // 0x006b118c (oddball player score)
extern void string_format_wide_va(uint16_t *dest, const uint16_t *format, ...); // 0x557930, blam-cc: EDX dest
extern void game_time_format_minutes_seconds(uint32_t ticks, uint32_t unused, wchar_t *dest); // 0x466530, blam-cc: ECX ticks

wchar_t *game_engine_oddball_build_player_text(datum_index player, wchar_t *buffer)
{
    int32_t score = king_alt_player_score[player & 0xffff];

    if (game_engine_variant.engine.oddball.ball_type == 2) {
        string_format_wide_va((uint16_t *)buffer, (const uint16_t *)L"%d", score);
    } else {
        game_time_format_minutes_seconds((uint32_t)score, 0x100, buffer);
    }
    return buffer;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
