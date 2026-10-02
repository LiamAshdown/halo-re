// game_engine_slayer_player_new_life  (not a Ghidra function; the slayer game engine definition's +0x14 slot (player_new_life); no C existed, so that
//   stored pointer trapped as unlisted_46f3c0)
// address 0x46f3c0, size 84 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x46f3c0..0x46f413: the player's +0x88 becomes -1; as the server its score is
//   cleared and, without teams, the score of its team.
// blam-cc: cdecl (called through the engine definition)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include <wchar.h>
#include "objects.h"
#include "units.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *player_data; // 0x0087a480
extern int16_t network_game_mode; // 0x00719720
extern game_engine_definition *current_game_engine;
extern uint8_t game_engine_teams_enabled_flag; // 0x006f1cbc
extern int32_t slayer_team_score[16]; // 0x006b13d8
extern int32_t slayer_player_score[16]; // 0x006b1418
extern int32_t slayer_unknown_0087a4a0[16]; // 0x0087a4a0, UNSURE
extern int32_t slayer_unknown_0087a4e0[16]; // 0x0087a4e0, UNSURE

void game_engine_slayer_player_new_life(datum_index player_index)
{
    uint8_t *player = ((uint8_t *)player_data->data + ((player_index) & 0xffff) * 0x200);

    ((struct player *)player)->slayer_target = -1;
    if (network_game_mode != 2) {
        return;
    }
    slayer_player_score[player_index & 0xffff] = 0;
    if (current_game_engine == 0 || game_engine_teams_enabled_flag == 0) {
        slayer_team_score[((struct player *)player)->team] = 0;
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
