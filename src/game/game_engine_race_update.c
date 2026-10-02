// game_engine_race_update  (not a Ghidra function; the race game engine definition's +0x38 slot (update); no C existed, so that
//   stored pointer trapped as unlisted_46e160)
// address 0x46e160, size 231 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x46e160..0x46e246: the player's +0x74 becomes 0x16 and +0x78 its own handle;
//   with a unit, while the game has not ended and as the server, looks for the race flag (scenario player starting
//   location) near the unit -- within 2.5 of its parent's origin (+0xa0) through 0x461080 when it rides something,
//   else within 1.5 / 0.6 of its own origin through 0x461180 -- and scores it with game_engine_ctf_score_flag.
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
extern data_array *object_data; // 0x008603b0
extern game_engine_definition *current_game_engine;
extern int16_t network_game_mode; // 0x00719720
extern int32_t game_engine_state_value; // 0x0087aa10
extern int game_engine_find_valid_starting_locations(real_point3d *origin, float max_horizontal_dist,
    float max_height_delta, int16_t team, int16_t type, int32_t max_results, int32_t *results); // 0x461080, blam-cc: EBX origin
extern int32_t game_engine_find_one_valid_starting_location(int16_t type, int16_t team, real_point3d *origin,
    float max_horizontal_dist, float max_height_delta); // 0x461180, blam-cc: ECX type, EDX team, EBX origin
extern void game_engine_ctf_score_flag(uint32_t team, int32_t scenario_flag_index); // 0x46e080, blam-cc: EAX scenario_flag_index (the first argument is a player)

void game_engine_race_update(datum_index player_index)
{
    uint8_t *player = ((uint8_t *)player_data->data + ((player_index) & 0xffff) * 0x200);
    datum_index unit_index;
    uint8_t *unit;
    datum_index parent_index;
    int32_t result;

    *(int32_t *)&((struct player *)player)->hud_message_index = 0x16;
    ((struct player *)player)->hud_message_player = player_index;
    unit_index = ((struct player *)player)->unit;
    if (unit_index == 0xffffffff) {
        return;
    }
    if (current_game_engine != 0 && game_engine_state_value != 0) {
        return;
    }
    if (network_game_mode != 2) {
        return;
    }
    unit = *(uint8_t **)((uint8_t *)object_data->data + (unit_index & 0xffff) * 12 + 8);
    parent_index = ((unit_object *)unit)->base.parent_object;
    if (parent_index != 0xffffffff) {
        uint8_t *parent = *(uint8_t **)((uint8_t *)object_data->data + (parent_index & 0xffff) * 12 + 8);

        result = -1;
        game_engine_find_valid_starting_locations((real_point3d *)(parent + 0xa0), 2.5f, 0.0f, 3, -1, 1, &result);
    } else {
        result = game_engine_find_one_valid_starting_location(-1, 3, (real_point3d *)(unit + 0xa0), 1.5f, 0.6f);
    }
    if (result != -1) {
        game_engine_ctf_score_flag(player_index, result);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
