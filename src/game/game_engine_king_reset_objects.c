// game_engine_king_reset_objects  (not a Ghidra function; the king game engine definition's +0xac slot (reset_objects); no C existed, so that
//   stored pointer trapped as unlisted_46bb70)
// address 0x46bb70, size 86 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x46bb70..0x46bbc5: as the server: clears the 16 hill ticks, last credit ticks
//   and in-hill flags, sets the starting location type 0, 0x006b1068 = 0x708, 0x006b1058 = -1, the first two king
//   globals 0, and rebuilds the hill boundary (tail call).
// blam-cc: cdecl (called through the engine definition)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "fn_game.h"
#include <wchar.h>

extern int16_t network_game_mode; // 0x00719720
extern int32_t king_bucket_credit_ticks[16]; // 0x006b0ec0
extern int32_t king_bucket_last_credit_tick[16]; // 0x006b0f00
extern uint8_t king_hill_player_in_hill[16]; // 0x006b0f40
extern int32_t king_starting_location_type; // 0x006b1064
extern int32_t king_hill_move_ticks_006b1068; // 0x006b1068, UNSURE name
extern int32_t king_hill_index_006b1058; // 0x006b1058, UNSURE name
extern int32_t king_hill_state_globals; // 0x006b1050 (king_globals +0x00)
extern int32_t king_hill_state_006b1054; // 0x006b1054 (king_globals +0x04)


void game_engine_king_reset_objects(void)
{
    int32_t i;

    if (network_game_mode != 2) {
        return;
    }
    for (i = 0; i < 0x10; i++) {
        king_bucket_credit_ticks[i] = 0;
        king_bucket_last_credit_tick[i] = 0;
        king_hill_player_in_hill[i] = 0;
    }
    king_starting_location_type = 0;
    king_hill_move_ticks_006b1068 = 0x708;
    king_hill_index_006b1058 = -1;
    king_hill_state_globals = 0;
    king_hill_state_006b1054 = 0;
    game_engine_koth_build_hill_boundary();
}
