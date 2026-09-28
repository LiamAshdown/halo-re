// game_engine_ctf_initialize_for_new_game  (not a Ghidra function; the ctf game engine definition's +0x0c slot (initialize_for_new_game); no C existed, so that
//   stored pointer trapped as unlisted_4684a0)
// address 0x4684a0, size 886 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4684a0..0x468815: clears the 13 ctf global dwords from 0x6b0e88 and the
//   replicated touch counts, flag objects -1, reset ticks 60. Each team's flag stand is the first starting location
//   of type = team (0x461080), swapped between teams when variant +0x7c is set. As the server: in single-flag mode
//   (+0x80 positive) a random team gets the flag, becomes the active team, the teams hear 0x2f / 0x2e (BL 1) and the
//   auto-return ticks take +0x80; otherwise both teams get their flag. A client with +0x80 positive still advances
//   the random seed. The capture limit takes the score limit. Netgame equipment of type 0 or 1 that belongs to this
//   game (any game type 1 or 12 in +0x14..+0x1a with an engine; all four zero without) becomes type 3 when it is
//   nearer the other stand (with +0x7c: nearer its own). The single flag mode byte takes 0x71c306. Returns 1.
// blam-cc: cdecl (called through the engine definition)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include <wchar.h>
#include <string.h>

extern void *current_game_engine; // 0x006f1d20
extern int16_t network_game_mode; // 0x00719720
extern game_variant game_engine_variant; // 0x006f1c88
extern int32_t ctf_team_flag_touch_count[2]; // 0x006b0e98
extern uint8_t *global_scenario; // 0x00746f8c (+0x354 / +0x358 the 0x34 byte netgame equipment, +0x37c the starting locations)
extern uint32_t random_seed_global; // 0x00719cd0
extern real_point3d *ctf_team_flag_stand_position[2]; // 0x006b0e88 (the ctf globals start here, 13 dwords)
extern datum_index ctf_team_flag_object[2]; // 0x006b0e90
extern int32_t ctf_flag_capture_limit_006b0ea0; // 0x006b0ea0
extern int32_t ctf_flag_auto_return_ticks; // 0x006b0eb0
extern uint8_t ctf_active_team; // 0x006b0eb8
extern uint8_t ctf_single_flag_mode; // 0x006b0ebc
extern int32_t ctf_touch_counts_network[3]; // 0x0087a9e0
extern int32_t game_engine_ctf_reset_ticks; // 0x0087aa24
extern uint8_t network_single_flag_force_reset_value; // 0x0071c306
extern int game_engine_find_valid_starting_locations(real_point3d *origin, float max_horizontal_dist,
    float max_height_delta, int16_t team, int16_t type, int32_t max_results, int32_t *results); // 0x461080, blam-cc: EBX origin
extern datum_index game_engine_ctf_create_flag_object(real_point3d *position, uint16_t name_index); // 0x468360, blam-cc: EAX position
extern void game_engine_broadcast_kill_feed_to_team(int32_t message_type, int32_t team, uint8_t broadcast); // 0x460ba0, blam-cc: ESI message_type, BL broadcast

static float distance_squared(const real_point3d *a, const real_point3d *b)
{
    float dx = a->x - b->x;
    float dy = a->y - b->y;
    float dz = a->z - b->z;

    return dz * dz + dy * dy + dx * dx;
}

uint8_t game_engine_ctf_initialize_for_new_game(void)
{
    int32_t team;
    int16_t count;
    int16_t i;

    memset(ctf_team_flag_stand_position, 0, 0xd * 4);
    ctf_touch_counts_network[0] = 0;
    ctf_touch_counts_network[1] = 0;
    ctf_touch_counts_network[2] = 0;
    ctf_team_flag_object[0] = 0xffffffff;
    ctf_team_flag_object[1] = 0xffffffff;
    game_engine_ctf_reset_ticks = 0x3c;
    for (team = 0; team < 2; team++) {
        int32_t index = -1;
        int32_t slot;

        game_engine_find_valid_starting_locations((real_point3d *)0, 0.0f, 0.0f, 0, (int16_t)team, 1, &index);
        ctf_team_flag_touch_count[team] = 0;
        slot = game_engine_variant.ctf_option_7c != 0 ? (team + 1) % 2 : team;
        ctf_team_flag_stand_position[slot] = 0;
        if (index != -1) {
            ctf_team_flag_stand_position[slot] = (real_point3d *)(*(uint8_t **)(global_scenario + 0x37c) + index * 0x94);
        }
    }
    if (network_game_mode == 2) {
        if (game_engine_variant.ctf_value_80 > 0) {
            int32_t active;

            random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
            active = (int16_t)((((random_seed_global >> 16) << 1) & 0xffffffff) >> 16);
            if (ctf_team_flag_stand_position[active] != 0) {
                datum_index flag = game_engine_ctf_create_flag_object(ctf_team_flag_stand_position[active], (uint16_t)active);

                if (flag != 0xffffffff) {
                    ctf_team_flag_object[active] = flag;
                }
            }
            ctf_active_team = (uint8_t)active;
            game_engine_broadcast_kill_feed_to_team(0x2f, active % 2, 1);
            game_engine_broadcast_kill_feed_to_team(0x2e, (active + 1) % 2, 1);
            ctf_flag_auto_return_ticks = game_engine_variant.ctf_value_80;
        } else {
            for (team = 0; team < 2; team++) {
                if (ctf_team_flag_stand_position[team] != 0) {
                    datum_index flag = game_engine_ctf_create_flag_object(ctf_team_flag_stand_position[team], (uint16_t)team);

                    if (flag != 0xffffffff) {
                        ctf_team_flag_object[team] = flag;
                    }
                }
            }
        }
    } else if (game_engine_variant.ctf_value_80 > 0) {
        random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
    }
    ctf_flag_capture_limit_006b0ea0 = game_engine_variant.score_limit;
    count = *(int16_t *)(global_scenario + 0x354);
    for (i = 0; i < count; i++) {
        uint8_t *equipment = (uint8_t *)0;
        int16_t type;
        uint8_t belongs;

        if (i >= 0 && i < *(int32_t *)(global_scenario + 0x354)) {
            equipment = *(uint8_t **)(global_scenario + 0x358) + i * 0x34;
        }
        type = *(int16_t *)(equipment + 0x10);
        if (type != 0 && type != 1) {
            continue;
        }
        if (current_game_engine != 0) {
            int32_t k;

            belongs = 0;
            for (k = 0; k < 4; k++) {
                int16_t game_type = *(int16_t *)(equipment + 0x14 + k * 2);

                if (game_type == 1 || game_type == 12) {
                    belongs = 1;
                }
            }
        } else {
            belongs = *(int16_t *)(equipment + 0x14) == 0 && *(int16_t *)(equipment + 0x16) == 0 &&
                *(int16_t *)(equipment + 0x18) == 0 && *(int16_t *)(equipment + 0x1a) == 0;
        }
        if (belongs) {
            float own = distance_squared((real_point3d *)equipment, ctf_team_flag_stand_position[type % 2]);
            float other = distance_squared((real_point3d *)equipment, ctf_team_flag_stand_position[(type + 1) % 2]);

            if (game_engine_variant.ctf_option_7c != 0 ? own < other : own > other) {
                *(int16_t *)(equipment + 0x10) = 3;
            }
        }
    }
    ctf_single_flag_mode = network_single_flag_force_reset_value;
    return 1;
}
