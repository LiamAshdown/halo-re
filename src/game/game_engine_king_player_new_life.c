// game_engine_king_player_new_life  (not a Ghidra function; the king game engine definition's +0x14 slot (player_new_life); no C existed, so that
//   stored pointer trapped as unlisted_46a5e0)
// address 0x46a5e0, size 76 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x46a5e0..0x46a62b: as the server without teams, clears the hill ticks and last
//   credit tick of the player's team (+0x20).
// blam-cc: cdecl (called through the engine definition)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <wchar.h>

extern data_array *player_data; // 0x0087a480
extern int16_t network_game_mode; // 0x00719720
extern game_engine_definition *current_game_engine;
extern uint8_t game_engine_teams_enabled_flag; // 0x006f1cbc
extern int32_t king_bucket_credit_ticks[16]; // 0x006b0ec0
extern int32_t king_bucket_last_credit_tick[16]; // 0x006b0f00

void game_engine_king_player_new_life(datum_index player_index)
{
    if (network_game_mode == 2 && (current_game_engine == 0 || game_engine_teams_enabled_flag == 0)) {
        int32_t team = *(int32_t *)(((uint8_t *)player_data->data + ((player_index) & 0xffff) * 0x200) + 0x20);

        king_bucket_credit_ticks[team] = 0;
        king_bucket_last_credit_tick[team] = 0;
    }
}
