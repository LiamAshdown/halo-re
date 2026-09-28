exec(open(r'C:\Users\Liam-\halo-re\scratchpad\engine_lib.py').read())
EXT.update({
    'scenario': 'extern uint8_t *global_scenario; // 0x00746f8c (Scenario *; +0x378 count / +0x37c elements of the 0x94 byte player starting locations)',
    'king_init': ('extern int32_t king_team_hill_seconds_network[16]; // 0x0087a7e0\n'
                  'extern int16_t game_engine_recent_location_count; // 0x006b106c\n'
                  'extern int16_t game_engine_recent_location_table[]; // 0x006b1070\n'
                  'extern int32_t king_starting_location_type; // 0x006b1064\n'
                  'extern int32_t king_hill_move_ticks_006b1068; // 0x006b1068\n'
                  'extern int32_t king_hill_index_006b1058; // 0x006b1058\n'
                  'extern int32_t king_hill_state_006b1050; // 0x006b1050\n'
                  'extern void game_engine_koth_build_hill_boundary(void); // 0x46a240\n'
                  'extern void game_engine_koth_reset_hill_marker_history(void); // 0x46b250'),
    'oddball_reset': ('extern int32_t king_hill_occupant_last_tick[16]; // 0x006b124c\n'
                      'extern int32_t oddball_ball_timers_006b11cc[16]; // 0x006b11cc\n'
                      'extern void game_engine_koth_relocate_hill_marker(void); // 0x46bfe0\n'
                      'extern uint8_t custom_waypoints[]; // 0x006f1888 (custom_waypoint, 0x20 bytes each)'),
    'bucket_extra': 'extern int32_t game_engine_bucket_scores_extra[16]; // 0x006b1358',
    'check_bucket': 'extern void game_engine_check_bucket_scores_and_end_round(void); // 0x46db70',
    'pulse': 'extern void game_engine_animate_hill_pulse_icons(datum_index fading_player, datum_index growing_player); // 0x46f450, blam-cc: EAX fading, ECX growing',
    'random_target': 'extern void game_engine_player_select_random_target(datum_index player_or_all); // 0x46f1a0',
    'net_send': ('extern uint8_t network_message_scratch[0x7ff8]; // 0x00871de0\n'
                 'extern void *network_server_pointer; // 0x0071c2d4 (network_server_globals *)\n'
                 'extern int32_t message_delta_encode_message(int32_t extra_eax, int32_t extra_edx, int32_t flag, int32_t message_type,\n'
                 '    int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed); // 0x4ec940, blam-cc: EAX buffer, EDX size\n'
                 'extern char network_session_broadcast_to_flagged(int32_t body_bit_count, void *server, int32_t param_1,\n'
                 '    void *data, int32_t param_3, int32_t param_4, char force, int32_t param_6); // 0x4e1a80, blam-cc: EAX bits, ECX server\n'
                 'extern uint8_t network_session_send_to_machine(int32_t machine_id, void *server, uint32_t param_1, void *data,\n'
                 '    uint32_t param_3, uint32_t reliable, uint32_t unknown_a, char force, uint32_t priority); // 0x4e1930, blam-cc: EAX machine, ESI server'),
})

emit(0x46a510, 202, 'game_engine_king_initialize_for_new_game',
     'zeroes 0x6b dwords at 0x6b0ec0 and at 0x87a7e0; collects the distinct words +0x12 of every scenario player starting location whose word +0x10 is 8 into the recent location table (count at 0x6b106c); then starting location type 0, hill move ticks 0x708, hill index -1, hill state 0, builds the hill boundary, resets the hill marker history and returns 1.',
     ['scenario', 'king_bucket_credit_ticks', 'king_init'], 'uint8_t %s(void)',
     '    int16_t count = 0;\n    int16_t i;\n\n'
     '    memset(king_bucket_credit_ticks, 0, 0x6b * 4);\n    memset(king_team_hill_seconds_network, 0, 0x6b * 4);\n'
     '    game_engine_recent_location_count = 0;\n'
     '    for (i = 0; i < *(int32_t *)(global_scenario + 0x378); i++) {\n'
     '        uint8_t *location = *(uint8_t **)(global_scenario + 0x37c) + i * 0x94;\n        int16_t k;\n\n'
     '        if (*(int16_t *)(location + 0x10) != 8) {\n            continue;\n        }\n'
     '        for (k = 0; k < count; k++) {\n'
     '            if (game_engine_recent_location_table[k] == *(int16_t *)(location + 0x12)) {\n                break;\n            }\n        }\n'
     '        if (k == count) {\n            game_engine_recent_location_table[count] = *(int16_t *)(location + 0x12);\n            count++;\n        }\n    }\n'
     '    if (*(int32_t *)(global_scenario + 0x378) > 0) {\n        game_engine_recent_location_count = count;\n    }\n'
     '    king_starting_location_type = 0;\n    king_hill_move_ticks_006b1068 = 0x708;\n    king_hill_index_006b1058 = -1;\n'
     '    king_hill_state_006b1050 = 0;\n    game_engine_koth_build_hill_boundary();\n    game_engine_koth_reset_hill_marker_history();\n    return 1;\n',
     extra_inc='#include <string.h>\n')

emit(0x46d450, 203, 'game_engine_oddball_reset_objects',
     'as the server: zeroes the 16 team scores and the first 16 player scores, sets the 16 occupant entries and last ticks to -1, then with variant +0x8c in 1..2 zeroes each of the first variant +0x90 ball timers and relocates the marker once per ball, otherwise gives them cumulative 0x1c2 tick delays. Always zeroes the first variant +0x90 custom waypoints.',
     ['network_game_mode', 'variant', 'king_alt_team_score', 'king_alt_player_score', 'king_hill_occupant_table', 'oddball_reset'], 'void %s(void)',
     '    int32_t count = game_engine_variant.unknown_90;\n    int32_t i;\n\n'
     '    if (network_game_mode == 2) {\n'
     '        for (i = 0; i < 16; i++) {\n            king_alt_player_score[i] = 0;\n            king_alt_team_score[i] = 0;\n        }\n'
     '        for (i = 0; i < 16; i++) {\n            king_hill_occupant_table[i] = 0xffffffff;\n            king_hill_occupant_last_tick[i] = -1;\n        }\n'
     '        if (game_engine_variant.unknown_8c > 0 && game_engine_variant.unknown_8c <= 2) {\n'
     '            for (i = 0; i < count; i++) {\n                oddball_ball_timers_006b11cc[i] = 0;\n                game_engine_koth_relocate_hill_marker();\n            }\n'
     '            count = game_engine_variant.unknown_90;\n'
     '        } else {\n            int32_t delay = 0;\n\n'
     '            for (i = 0; i < count; i++) {\n                delay += 0x1c2;\n                oddball_ball_timers_006b11cc[i] = delay;\n            }\n        }\n    }\n'
     '    for (i = 0; i < count; i++) {\n        memset(custom_waypoints + (int16_t)i * 0x20, 0, 0x20);\n    }\n',
     extra_inc='#include <string.h>\n')

emit(0x46ee60, 207, 'game_engine_race_player_round_reset',
     'as the server, for a live player (index in range, salt 0 or matching): with variant +0x80 == 2 its word +0xc6 is added to the extra bucket score of its team (or, when the flag argument equals the team, of team flag != 1); its words +0xc4/+0xc6/+0xc8 are cleared, +0x88 takes the game tick and its 0x6b12d4 entry is cleared. Every server call ends with game_engine_check_bucket_scores_and_end_round.',
     ['player_data', 'network_game_mode', 'variant', 'game_time', 'bucket_extra', 'ctf_team_captured_flags_mask', 'check_bucket'],
     'void %s(datum_index player_index, uint8_t team_flag)',
     '    int16_t index = (int16_t)player_index;\n    int16_t salt = (int16_t)(player_index >> 16);\n\n'
     '    if (network_game_mode != 2) {\n        return;\n    }\n'
     '    if (player_index != 0xffffffff && index >= 0 && index < *(int16_t *)((uint8_t *)player_data + 0x20)) {\n'
     '        uint8_t *player = (uint8_t *)player_data->data + index * *(int16_t *)((uint8_t *)player_data + 0x22);\n'
     '        int16_t player_salt = *(int16_t *)player;\n\n'
     '        if (player_salt != 0 && (salt == 0 || player_salt == salt)) {\n'
     '            if (game_engine_variant.ctf_value_80 == 2) {\n'
     '                uint32_t team = *(uint32_t *)(player + 0x20);\n\n'
     '                if ((uint32_t)team_flag == team) {\n                    team = team_flag != 1;\n                }\n'
     '                game_engine_bucket_scores_extra[team] += *(int16_t *)(player + 0xc6);\n            }\n'
     '            *(int16_t *)(player + 0xc4) = 0;\n            *(int16_t *)(player + 0xc6) = 0;\n            *(int16_t *)(player + 0xc8) = 0;\n'
     '            *(int32_t *)(player + 0x88) = *(int32_t *)(game_time + 0xc);\n'
     '            ctf_team_captured_flags_mask[player_index & 0xffff] = 0;\n        }\n    }\n'
     '    game_engine_check_bucket_scores_and_end_round();\n')

emit(0x46f580, 204, 'game_engine_slayer_player_killed',
     'fired first by game_engine_on_player_death with (killer, death object, victim, suicide). Unless the victim is marked for deletion or there is no killer: a suicide takes a point from the killer; otherwise the pulse icons animate (killer fading, victim growing) and the killer scores a point, except that with variant +0x7e set as the server the kill only counts (and a new target is picked) when the victim was the killer\'s +0x88 target. The score helper 0x46f540 (EAX player, EDX delta) is not a separate function in the repo and is inlined here as a static.',
     ['player_data', 'network_game_mode', 'variant', 'slayer_scores', 'pulse', 'random_target'],
     'void %s(datum_index killer, datum_index death_object, datum_index victim, uint8_t is_suicide)',
     '    uint8_t *killer_player;\n\n    (void)death_object;\n'
     '    if (*(%s + 0xd5) != 0 || killer == 0xffffffff) {\n        return;\n    }\n' % P('victim') +
     '    killer_player = %s;\n' % P('killer') +
     '    if (is_suicide != 0) {\n        game_engine_slayer_add_score(killer, -1);\n        return;\n    }\n'
     '    game_engine_animate_hill_pulse_icons(killer, victim);\n'
     '    if (game_engine_variant.ctf_option_7e != 0 && network_game_mode == 2) {\n'
     '        if (*(datum_index *)(killer_player + 0x88) != victim) {\n            return;\n        }\n'
     '        game_engine_player_select_random_target(killer);\n    }\n'
     '    game_engine_slayer_add_score(killer, 1);\n',
     helper='// 0x46f540 (EAX player, EDX delta): unless a client, adds delta to the player\'s team score and its own score.\n'
            'static void game_engine_slayer_add_score(datum_index player_index, int32_t delta)\n{\n'
            '    if (network_game_mode == 1) {\n        return;\n    }\n'
            '    slayer_team_score[*(int32_t *)(%s + 0x20)] += delta;\n' % P('player_index') +
            '    slayer_player_score[player_index & 0xffff] += delta;\n}\n')

emit(0x46fa40, 214, 'game_engine_slayer_profiles_updated',
     'mode 0 encodes a type 0x10 request for the replicated scores at 0x87a4a0; otherwise encodes type 0x10 from the live team scores 0x6b13d8 against 0x87a4a0 and then copies the 0x20 live dwords (team and player scores) over the replicated copy. A positive bit count is broadcast when the machine is -1, else sent to that machine.',
     ['slayer_scores', 'net_send'], 'void %s(int32_t mode, int32_t machine_index)',
     '    void *items[2];\n    void *network_fields[1];\n    int32_t bits;\n\n'
     '    if (mode == 0) {\n        network_fields[0] = slayer_unknown_0087a4a0;\n'
     '        bits = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 0x10, 0, network_fields, 0, 1, 0);\n'
     '    } else {\n        items[0] = slayer_team_score;\n        items[1] = 0;\n        network_fields[0] = slayer_unknown_0087a4a0;\n'
     '        bits = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 1, 0x10, 0, items, (int32_t)network_fields, 1, 0);\n'
     '        memcpy(slayer_unknown_0087a4a0, slayer_team_score, 0x20 * 4);\n    }\n'
     '    if (bits <= 0) {\n        return;\n    }\n'
     '    if (machine_index == -1) {\n'
     '        network_session_broadcast_to_flagged(bits, network_server_pointer, 1, network_message_scratch, 1, 0, 0, 3);\n'
     '    } else {\n'
     '        network_session_send_to_machine(machine_index, network_server_pointer, 1, network_message_scratch, bits, 1, 0, 0, 3);\n    }\n',
     extra_inc='#include <string.h>\n')

# on_player_death: the +0x68 slot takes (killer, death_object, victim, is_suicide) -- 0x460247..0x46025b pushes them.
p = 'src/game/game_engine_on_player_death.c'
s = open(p, encoding='utf-8').read()
old = '        ((void (*)(void))current_game_engine->unknown_68)();\n'
new = ('        // FIXED 2026-09-28: 0x460247..0x46025b pushes is_suicide, victim, death_object, killer (the slayer\n'
       '        //   handler 0x46f580 reads all four); the call passed none.\n'
       '        ((void (*)(datum_index, datum_index, datum_index, char))current_game_engine->unknown_68)(killer, death_object,\n'
       '            victim, is_suicide);\n')
assert s.count(old) == 1
open(p, 'w', encoding='utf-8').write(s.replace(old, new))
print('ok')
