exec(open(r'C:\Users\Liam-\halo-re\scratchpad\engine_lib.py').read())
EXT.update({
    'ctf_init': ('extern uint8_t *global_scenario; // 0x00746f8c (+0x354 / +0x358 the 0x34 byte netgame equipment, +0x37c the starting locations)\n'
                 'extern uint32_t random_seed_global; // 0x00719cd0\n'
                 'extern real_point3d *ctf_team_flag_stand_position[2]; // 0x006b0e88 (the ctf globals start here, 13 dwords)\n'
                 'extern datum_index ctf_team_flag_object[2]; // 0x006b0e90\n'
                 'extern int32_t ctf_flag_capture_limit_006b0ea0; // 0x006b0ea0\n'
                 'extern int32_t ctf_flag_auto_return_ticks; // 0x006b0eb0\n'
                 'extern uint8_t ctf_active_team; // 0x006b0eb8\n'
                 'extern uint8_t ctf_single_flag_mode; // 0x006b0ebc\n'
                 'extern int32_t ctf_touch_counts_network[3]; // 0x0087a9e0\n'
                 'extern int32_t game_engine_ctf_reset_ticks; // 0x0087aa24\n'
                 'extern uint8_t network_single_flag_force_reset_value; // 0x0071c306\n'
                 'extern int game_engine_find_valid_starting_locations(real_point3d *origin, float max_horizontal_dist,\n'
                 '    float max_height_delta, int16_t team, int16_t type, int32_t max_results, int32_t *results); // 0x461080, blam-cc: EBX origin\n'
                 'extern datum_index game_engine_ctf_create_flag_object(real_point3d *position, uint16_t name_index); // 0x468360, blam-cc: EAX position\n'
                 'extern void game_engine_broadcast_kill_feed_to_team(int32_t message_type, int32_t team, uint8_t broadcast); // 0x460ba0, blam-cc: ESI message_type, BL broadcast'),
})

HELP = '''static float distance_squared(const real_point3d *a, const real_point3d *b)
{
    float dx = a->x - b->x;
    float dy = a->y - b->y;
    float dz = a->z - b->z;

    return dz * dz + dy * dy + dx * dx;
}
'''

emit(0x4684a0, 886, 'game_engine_ctf_initialize_for_new_game',
     'clears the 13 ctf global dwords from 0x6b0e88 and the replicated touch counts, flag objects -1, reset ticks 60. Each team\'s flag stand is the first starting location of type = team (0x461080), swapped between teams when variant +0x7c is set. As the server: in single-flag mode (+0x80 positive) a random team gets the flag, becomes the active team, the teams hear 0x2f / 0x2e (BL 1) and the auto-return ticks take +0x80; otherwise both teams get their flag. A client with +0x80 positive still advances the random seed. The capture limit takes the score limit. Netgame equipment of type 0 or 1 that belongs to this game (any game type 1 or 12 in +0x14..+0x1a with an engine; all four zero without) becomes type 3 when it is nearer the other stand (with +0x7c: nearer its own). The single flag mode byte takes 0x71c306. Returns 1.',
     ['current_game_engine', 'network_game_mode', 'variant', 'ctf_team_flag_touch_count', 'ctf_init'], 'uint8_t %s(void)',
     '    int32_t team;\n    int16_t count;\n    int16_t i;\n\n'
     '    memset(ctf_team_flag_stand_position, 0, 0xd * 4);\n'
     '    ctf_touch_counts_network[0] = 0;\n    ctf_touch_counts_network[1] = 0;\n    ctf_touch_counts_network[2] = 0;\n'
     '    ctf_team_flag_object[0] = 0xffffffff;\n    ctf_team_flag_object[1] = 0xffffffff;\n    game_engine_ctf_reset_ticks = 0x3c;\n'
     '    for (team = 0; team < 2; team++) {\n        int32_t index = -1;\n        int32_t slot;\n\n'
     '        game_engine_find_valid_starting_locations((real_point3d *)0, 0.0f, 0.0f, 0, (int16_t)team, 1, &index);\n'
     '        ctf_team_flag_touch_count[team] = 0;\n'
     '        slot = game_engine_variant.ctf_option_7c != 0 ? (team + 1) % 2 : team;\n'
     '        ctf_team_flag_stand_position[slot] = 0;\n'
     '        if (index != -1) {\n'
     '            ctf_team_flag_stand_position[slot] = (real_point3d *)(*(uint8_t **)(global_scenario + 0x37c) + index * 0x94);\n        }\n    }\n'
     '    if (network_game_mode == 2) {\n'
     '        if (game_engine_variant.ctf_value_80 > 0) {\n            int32_t active;\n\n'
     '            random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;\n'
     '            active = (int16_t)((((random_seed_global >> 16) << 1) & 0xffffffff) >> 16);\n'
     '            if (ctf_team_flag_stand_position[active] != 0) {\n'
     '                datum_index flag = game_engine_ctf_create_flag_object(ctf_team_flag_stand_position[active], (uint16_t)active);\n\n'
     '                if (flag != 0xffffffff) {\n                    ctf_team_flag_object[active] = flag;\n                }\n            }\n'
     '            ctf_active_team = (uint8_t)active;\n'
     '            game_engine_broadcast_kill_feed_to_team(0x2f, active % 2, 1);\n'
     '            game_engine_broadcast_kill_feed_to_team(0x2e, (active + 1) % 2, 1);\n'
     '            ctf_flag_auto_return_ticks = game_engine_variant.ctf_value_80;\n'
     '        } else {\n'
     '            for (team = 0; team < 2; team++) {\n'
     '                if (ctf_team_flag_stand_position[team] != 0) {\n'
     '                    datum_index flag = game_engine_ctf_create_flag_object(ctf_team_flag_stand_position[team], (uint16_t)team);\n\n'
     '                    if (flag != 0xffffffff) {\n                        ctf_team_flag_object[team] = flag;\n                    }\n                }\n            }\n        }\n'
     '    } else if (game_engine_variant.ctf_value_80 > 0) {\n'
     '        random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;\n    }\n'
     '    ctf_flag_capture_limit_006b0ea0 = game_engine_variant.score_limit;\n'
     '    count = *(int16_t *)(global_scenario + 0x354);\n'
     '    for (i = 0; i < count; i++) {\n'
     '        uint8_t *equipment = (uint8_t *)0;\n        int16_t type;\n        uint8_t belongs;\n\n'
     '        if (i >= 0 && i < *(int32_t *)(global_scenario + 0x354)) {\n'
     '            equipment = *(uint8_t **)(global_scenario + 0x358) + i * 0x34;\n        }\n'
     '        type = *(int16_t *)(equipment + 0x10);\n'
     '        if (type != 0 && type != 1) {\n            continue;\n        }\n'
     '        if (current_game_engine != 0) {\n            int32_t k;\n\n            belongs = 0;\n'
     '            for (k = 0; k < 4; k++) {\n                int16_t game_type = *(int16_t *)(equipment + 0x14 + k * 2);\n\n'
     '                if (game_type == 1 || game_type == 12) {\n                    belongs = 1;\n                }\n            }\n'
     '        } else {\n'
     '            belongs = *(int16_t *)(equipment + 0x14) == 0 && *(int16_t *)(equipment + 0x16) == 0 &&\n'
     '                *(int16_t *)(equipment + 0x18) == 0 && *(int16_t *)(equipment + 0x1a) == 0;\n        }\n'
     '        if (belongs) {\n'
     '            float own = distance_squared((real_point3d *)equipment, ctf_team_flag_stand_position[type % 2]);\n'
     '            float other = distance_squared((real_point3d *)equipment, ctf_team_flag_stand_position[(type + 1) % 2]);\n\n'
     '            if (game_engine_variant.ctf_option_7c != 0 ? own < other : own > other) {\n'
     '                *(int16_t *)(equipment + 0x10) = 3;\n            }\n        }\n    }\n'
     '    ctf_single_flag_mode = network_single_flag_force_reset_value;\n    return 1;\n',
     extra_inc='#include <string.h>\n', helper=HELP)
print('ok')
