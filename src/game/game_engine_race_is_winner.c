// game_engine_race_is_winner  (not a Ghidra function; the race game engine definition's +0x8c slot (is_winner); no C existed, so that
//   stored pointer trapped as unlisted_46eb80)
// address 0x46eb80, size 132 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x46eb80..0x46ec03: with teams: when exactly one team still has scoring capacity
//   (0x46e250 for teams 0 and 1), whether the player's team is that one; when neither has, -1; when both have,
//   game_engine_is_object_winning. Without teams game_engine_is_object_winning.
// blam-cc: cdecl (called through the engine definition)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include <wchar.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *player_data; // 0x0087a480
extern game_engine_definition *current_game_engine;
extern uint8_t game_engine_teams_enabled_flag; // 0x006f1cbc
extern uint8_t game_engine_team_has_scoring_capacity(int32_t team); // 0x46e250
extern uint32_t game_engine_is_object_winning(uint32_t handle); // 0x463660, blam-cc: EAX

uint32_t game_engine_race_is_winner(datum_index player)
{
    uint8_t capacity[2];

    if (current_game_engine == 0 || game_engine_teams_enabled_flag == 0) {
        return game_engine_is_object_winning(player);
    }
    capacity[0] = game_engine_team_has_scoring_capacity(0);
    capacity[1] = game_engine_team_has_scoring_capacity(1);
    if (capacity[0] != capacity[1]) {
        return (uint32_t)(capacity[*(int32_t *)(((uint8_t *)player_data->data + ((player) & 0xffff) * 0x200) + 0x20)] != 0);
    }
    if (capacity[0] == 0) {
        return 0xffffffff;
    }
    return game_engine_is_object_winning(player);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
