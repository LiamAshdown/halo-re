// game_engine_oddball_reset_objects  (not a Ghidra function; the oddball game engine definition's +0xac slot (reset_objects); no C existed, so that
//   stored pointer trapped as unlisted_46d450)
// address 0x46d450, size 203 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x46d450..0x46d51a: as the server: zeroes the 16 team scores and the first 16
//   player scores, sets the 16 occupant entries and last ticks to -1, then with variant +0x8c in 1..2 zeroes each of
//   the first variant +0x90 ball timers and relocates the marker once per ball, otherwise gives them cumulative 0x1c2
//   tick delays. Always zeroes the first variant +0x90 custom waypoints.
// blam-cc: cdecl (called through the engine definition)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include <wchar.h>
#include <string.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int16_t network_game_mode; // 0x00719720
extern game_variant game_engine_variant; // 0x006f1c88
extern int32_t king_alt_team_score[16]; // 0x006b114c (oddball team score)
extern int32_t king_alt_player_score[]; // 0x006b118c (oddball player score)
extern uint32_t king_hill_occupant_table[16]; // 0x006b120c
extern int32_t king_hill_occupant_last_tick[16]; // 0x006b124c
extern int32_t oddball_ball_timers_006b11cc[16]; // 0x006b11cc
extern void game_engine_koth_relocate_hill_marker(int32_t ball_index); // 0x46bfe0, blam-cc: ESI ball_index
extern uint8_t custom_waypoints[]; // 0x006f1888 (custom_waypoint, 0x20 bytes each)

void game_engine_oddball_reset_objects(void)
{
    int32_t count = game_engine_variant.engine.oddball.ball_count;
    int32_t i;

    if (network_game_mode == 2) {
        for (i = 0; i < 16; i++) {
            king_alt_player_score[i] = 0;
            king_alt_team_score[i] = 0;
        }
        for (i = 0; i < 16; i++) {
            king_hill_occupant_table[i] = 0xffffffff;
            king_hill_occupant_last_tick[i] = -1;
        }
        if (game_engine_variant.engine.oddball.ball_type > 0 && game_engine_variant.engine.oddball.ball_type <= 2) {
            for (i = 0; i < count; i++) {
                oddball_ball_timers_006b11cc[i] = 0;
                game_engine_koth_relocate_hill_marker(i);
            }
            count = game_engine_variant.engine.oddball.ball_count;
        } else {
            int32_t delay = 0;

            for (i = 0; i < count; i++) {
                delay += 0x1c2;
                oddball_ball_timers_006b11cc[i] = delay;
            }
        }
    }
    for (i = 0; i < count; i++) {
        memset(custom_waypoints + (int16_t)i * 0x20, 0, 0x20);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
