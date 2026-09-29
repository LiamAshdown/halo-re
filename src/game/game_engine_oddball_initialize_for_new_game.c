// game_engine_oddball_initialize_for_new_game  (not a Ghidra function; the oddball game engine definition's +0x0c slot (initialize_for_new_game); no C existed, so that
//   stored pointer trapped as unlisted_46c080)
// address 0x46c080, size 185 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x46c080..0x46c138: zeroes the 0x51 dwords from 0x006b1148 and from 0x0087a680;
//   the score target becomes the variant score limit, times 0x708 unless variant +0x8c is 2; the 16 hill occupants
//   and their last ticks become -1. As the server with variant +0x8c 1 or 2: each of the first variant +0x90 timers
//   (0x006b11cc) is cleared and the hill marker relocated; otherwise they get 0x1c2, 0x384, ... Returns 1.
// blam-cc: cdecl (called through the engine definition)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <wchar.h>
#include <string.h>

extern game_variant game_engine_variant; // 0x006f1c88
extern int16_t network_game_mode; // 0x00719720
extern int32_t king_alt_score_target; // 0x006b1148, the first of 0x51 dwords up to 0x006b128c
extern int32_t king_alt_team_scores_network[16]; // 0x0087a680, the first of 0x51 dwords
extern int32_t oddball_ball_timers_006b11cc[16]; // 0x006b11cc, UNSURE name
extern int32_t king_hill_occupant_last_tick[16]; // 0x006b124c
extern uint32_t king_hill_occupant_table[16]; // 0x006b120c
extern void game_engine_koth_relocate_hill_marker(int32_t ball_index); // 0x46bfe0, blam-cc: ESI ball_index

uint8_t game_engine_oddball_initialize_for_new_game(void)
{
    int32_t mode = game_engine_variant.unknown_8c;
    int32_t target;
    int32_t i;

    memset(&king_alt_score_target, 0, 0x51 * 4);
    memset(king_alt_team_scores_network, 0, 0x51 * 4);
    target = game_engine_variant.score_limit;
    if (mode != 2) {
        target *= 0x708;
    }
    king_alt_score_target = target;
    for (i = 0; i < 0x10; i++) {
        king_hill_occupant_table[i] = 0xffffffff;
        king_hill_occupant_last_tick[i] = -1;
    }
    if (network_game_mode == 2) {
        if (mode > 0 && mode <= 2) {
            for (i = 0; i < game_engine_variant.tracked_slot_count; i++) {
                oddball_ball_timers_006b11cc[i] = 0;
                game_engine_koth_relocate_hill_marker(i);
            }
        } else {
            int32_t delay = 0;

            for (i = 0; i < game_engine_variant.tracked_slot_count; i++) {
                delay += 0x1c2;
                oddball_ball_timers_006b11cc[i] = delay;
            }
        }
    }
    return 1;
}
