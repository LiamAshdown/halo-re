// game_engine_oddball_time_scale_override  (not a Ghidra function; the oddball game engine definition's +0x88 slot (time_scale_override); no C existed, so that
//   stored pointer trapped as unlisted_46cf20)
// address 0x46cf20, size 65 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x46cf20..0x46cf60: without a value, false; when the player is among the first
//   variant +0x90 hill occupants (0x006b120c), whether the value is variant +0x84, else whether it is variant +0x88.
// blam-cc: cdecl (called through the engine definition)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <wchar.h>

extern game_variant game_engine_variant; // 0x006f1c88
extern uint32_t king_hill_occupant_table[16]; // 0x006b120c

uint8_t game_engine_oddball_time_scale_override(uint32_t player, int32_t value)
{
    int32_t i;

    if (value == 0) {
        return 0;
    }
    for (i = 0; i < game_engine_variant.ball_count; i++) {
        if (king_hill_occupant_table[i] == player) {
            return (uint8_t)(value == game_engine_variant.oddball_trait_with_ball);
        }
    }
    return (uint8_t)(value == game_engine_variant.oddball_trait_without_ball);
}
