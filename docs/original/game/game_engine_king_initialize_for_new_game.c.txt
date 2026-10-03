// game_engine_king_initialize_for_new_game  (not a Ghidra function; the king game engine definition's +0x0c slot (initialize_for_new_game); no C existed, so that
//   stored pointer trapped as unlisted_46a510)
// address 0x46a510, size 202 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x46a510..0x46a5d9: zeroes 0x6b dwords at 0x6b0ec0 and at 0x87a7e0; collects the
//   distinct words +0x12 of every scenario player starting location whose word +0x10 is 8 into the recent location
//   table (count at 0x6b106c); then starting location type 0, hill move ticks 0x708, hill index -1, hill state 0,
//   builds the hill boundary, resets the hill marker history and returns 1.
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
extern Scenario *global_scenario;
extern int32_t king_bucket_credit_ticks[16]; // 0x006b0ec0
extern int32_t king_team_hill_seconds_network[16]; // 0x0087a7e0
extern int16_t game_engine_recent_location_count; // 0x006b106c
extern int16_t game_engine_recent_location_table[]; // 0x006b1070
extern int32_t king_starting_location_type; // 0x006b1064
extern int32_t king_hill_move_ticks_006b1068; // 0x006b1068
extern int32_t king_hill_index_006b1058; // 0x006b1058
extern int32_t king_hill_state_globals; // 0x006b1050
extern void game_engine_koth_build_hill_boundary(void); // 0x46a240
extern void game_engine_koth_reset_hill_marker_history(void); // 0x46b250

uint8_t game_engine_king_initialize_for_new_game(void)
{
    int16_t count = 0;
    int16_t i;

    memset(king_bucket_credit_ticks, 0, 0x6b * 4);
    memset(king_team_hill_seconds_network, 0, 0x6b * 4);
    game_engine_recent_location_count = 0;
    for (i = 0; i < *(int32_t *)&global_scenario->netgame_flags.count; i++) {
        uint8_t *location = (uint8_t *)global_scenario->netgame_flags.pointer + i * 0x94;
        int16_t k;

        if (*(int16_t *)(location + 0x10) != 8) {
            continue;
        }
        for (k = 0; k < count; k++) {
            if (game_engine_recent_location_table[k] == *(int16_t *)(location + 0x12)) {
                break;
            }
        }
        if (k == count) {
            game_engine_recent_location_table[count] = *(int16_t *)(location + 0x12);
            count++;
        }
    }
    if (*(int32_t *)&global_scenario->netgame_flags.count > 0) {
        game_engine_recent_location_count = count;
    }
    king_starting_location_type = 0;
    king_hill_move_ticks_006b1068 = 0x708;
    king_hill_index_006b1058 = -1;
    king_hill_state_globals = 0;
    game_engine_koth_build_hill_boundary();
    game_engine_koth_reset_hill_marker_history();
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
