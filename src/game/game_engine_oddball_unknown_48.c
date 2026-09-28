// game_engine_oddball_unknown_48  (not a Ghidra function; the oddball game engine definition's +0x48 slot (unknown_48); no C existed, so that
//   stored pointer trapped as unlisted_46c750)
// address 0x46c750, size 390 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x46c750..0x46c8d5: also the race engine's +0xf8 slot. At tick 60 queues sound
//   0x21 with teams, 0x13 without. As the server each running ball timer counts down and at zero queues sound 0 and
//   respawns that ball (0x46bfe0). With variant +0x8c in 1..2 each ball waypoint follows its carrier: cleared when
//   uncarried, else owner = the carrier, arrow "target_blue", visible, at the carrier unit's origin 0.63 higher, both
//   filters -1.
// blam-cc: cdecl (called through the engine definition)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include <wchar.h>
#include <string.h>

extern data_array *object_data; // 0x008603b0
extern data_array *player_data; // 0x0087a480
extern game_time_globals *game_time; // 0x006f1d6c
extern game_engine_definition *current_game_engine;
extern uint8_t game_engine_teams_enabled_flag; // 0x006f1cbc
extern int16_t network_game_mode; // 0x00719720
extern game_variant game_engine_variant; // 0x006f1c88
extern void game_engine_queue_multiplayer_sound(int32_t sound_index, datum_index player, uint8_t broadcast); // 0x46be40, blam-cc: ESI sound, EDI player, stack broadcast
extern uint32_t king_hill_occupant_table[16]; // 0x006b120c
extern int32_t oddball_ball_timers_006b11cc[16]; // 0x006b11cc
extern void game_engine_koth_relocate_hill_marker(int32_t ball_index); // 0x46bfe0, blam-cc: ESI ball_index
extern uint8_t custom_waypoints[]; // 0x006f1888 (0x20 bytes each)
extern int16_t hud_waypoint_arrow_find(const char *name); // 0x4af070, blam-cc: EDI name

void game_engine_oddball_unknown_48(void)
{
    int32_t count;
    int32_t i;

    if (game_time->game_time == 0x3c) {
        uint8_t teams = current_game_engine != 0 ? game_engine_teams_enabled_flag : 0;

        game_engine_queue_multiplayer_sound(teams != 0 ? 0x21 : 0x13, 0xffffffff, 0);
    }
    count = game_engine_variant.unknown_90;
    if (network_game_mode == 2) {
        for (i = 0; i < count; i++) {
            if (oddball_ball_timers_006b11cc[i] > 0 && --oddball_ball_timers_006b11cc[i] == 0) {
                game_engine_queue_multiplayer_sound(0, 0xffffffff, 0);
                game_engine_koth_relocate_hill_marker(i);
            }
        }
    }
    if (game_engine_variant.unknown_8c <= 0 || game_engine_variant.unknown_8c > 2) {
        return;
    }
    for (i = 0; i < count; i++) {
        uint8_t *waypoint = custom_waypoints + (int16_t)i * 0x20;
        datum_index carrier = king_hill_occupant_table[i];
        datum_index unit_index;
        uint8_t *unit;

        if (carrier == 0xffffffff) {
            memset(waypoint, 0, 0x20);
            continue;
        }
        unit_index = *(datum_index *)(((uint8_t *)player_data->data + ((carrier) & 0xffff) * 0x200) + 0x34);
        if (unit_index == 0xffffffff) {
            continue;
        }
        unit = *(uint8_t **)((uint8_t *)object_data->data + (unit_index & 0xffff) * 12 + 8);
        *(datum_index *)(waypoint + 0x18) = carrier;
        *(int16_t *)(waypoint + 0x1c) = hud_waypoint_arrow_find("target_blue");
        waypoint[0x0c] = 1;
        *(real_point3d *)waypoint = *(real_point3d *)(unit + 0xa0);
        *(float *)(waypoint + 0x08) += 0.63f;
        *(int16_t *)(waypoint + 0x14) = -1;
        *(int32_t *)(waypoint + 0x10) = -1;
    }
}
