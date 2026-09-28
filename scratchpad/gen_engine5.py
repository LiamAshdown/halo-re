exec(open(r'C:\Users\Liam-\halo-re\scratchpad\engine_lib.py').read())
EXT.update({
    'end_game': ('extern uint8_t *network_server_pointer; // 0x0071c2d4 (network_server_globals *; +0xa0f set when the game ends)\n'
                 'extern int32_t game_engine_state_value; // 0x0087aa10\n'
                 'extern float game_engine_end_game_timer; // 0x0087aa08\n'
                 'extern void widget_close_all(void); // 0x498650\n'
                 'extern void game_engine_send_end_game_notification(uint32_t reason); // 0x4671d0, blam-cc: EAX reason'),
    'ctf_credit': ('extern int32_t ctf_flag_capture_limit_006b0ea0; // 0x006b0ea0, UNSURE name: the captures that end the game\n'
                   'extern uint8_t ctf_team_return_credit_active[2]; // 0x006b0ea4\n'
                   'extern int32_t ctf_team_return_credit_ticks[2]; // 0x006b0ea8'),
    'race_update': ('extern int32_t game_engine_state_value; // 0x0087aa10\n'
                    'extern int game_engine_find_valid_starting_locations(real_point3d *origin, float max_horizontal_dist,\n'
                    '    float max_height_delta, int16_t team, int16_t type, int32_t max_results, int32_t *results); // 0x461080, blam-cc: EBX origin\n'
                    'extern int32_t game_engine_find_one_valid_starting_location(int16_t type, int16_t team, real_point3d *origin,\n'
                    '    float max_horizontal_dist, float max_height_delta); // 0x461180, blam-cc: ECX type, EDX team, EBX origin\n'
                    'extern void game_engine_ctf_score_flag(uint32_t team, int32_t scenario_flag_index); // 0x46e080, blam-cc: EAX scenario_flag_index (the first argument is a player)'),
})

emit(0x469180, 236, 'game_engine_ctf_unknown_48',
     'as the server: when either team\'s touch count reached the limit at 0x6b0ea0 and the game has not ended, marks the server (+0xa0f), sets the ending state and a 7 s timer, queues sound 1, closes all widgets and sends end-game notification 1. Then for each team whose return credit is active the credit ticks count up, and past 0x258 queue sound 8 (team 0) / 0xb (team 1) and restart at 1.',
     ['network_game_mode', 'sound', 'ctf_team_flag_touch_count', 'end_game', 'ctf_credit'], 'void %s(void)',
     '    int32_t limit = ctf_flag_capture_limit_006b0ea0;\n\n'
     '    if (network_game_mode != 2) {\n        return;\n    }\n'
     '    if ((ctf_team_flag_touch_count[0] >= limit || ctf_team_flag_touch_count[1] >= limit) && game_engine_state_value == 0) {\n'
     '        network_server_pointer[0xa0f] = 1;\n        game_engine_state_value = 1;\n        game_engine_end_game_timer = 7.0f;\n'
     '        game_engine_queue_multiplayer_sound(1, 0xffffffff, 0);\n        widget_close_all();\n'
     '        game_engine_send_end_game_notification(1);\n    }\n'
     '    if (ctf_team_return_credit_active[0] != 0) {\n        int32_t ticks = ctf_team_return_credit_ticks[0];\n\n'
     '        if (ticks > 0x258) {\n            game_engine_queue_multiplayer_sound(8, 0xffffffff, 1);\n            ticks = 0;\n        }\n'
     '        ctf_team_return_credit_ticks[0] = ticks + 1;\n    }\n'
     '    if (ctf_team_return_credit_active[1] != 0) {\n        int32_t ticks = ctf_team_return_credit_ticks[1];\n\n'
     '        if (ticks > 0x258) {\n            game_engine_queue_multiplayer_sound(0xb, 0xffffffff, 1);\n            ticks = 0;\n        }\n'
     '        ctf_team_return_credit_ticks[1] = ticks + 1;\n    }\n')

emit(0x46e160, 231, 'game_engine_race_update',
     'the player\'s +0x74 becomes 0x16 and +0x78 its own handle; with a unit, while the game has not ended and as the server, looks for the race flag (scenario player starting location) near the unit -- within 2.5 of its parent\'s origin (+0xa0) through 0x461080 when it rides something, else within 1.5 / 0.6 of its own origin through 0x461180 -- and scores it with game_engine_ctf_score_flag.',
     ['player_data', 'object_data', 'current_game_engine', 'network_game_mode', 'race_update'], 'void %s(datum_index player_index)',
     '    uint8_t *player = %s;\n    datum_index unit_index;\n    uint8_t *unit;\n    datum_index parent_index;\n    int32_t result;\n\n' % P('player_index') +
     '    *(int32_t *)(player + 0x74) = 0x16;\n    *(datum_index *)(player + 0x78) = player_index;\n'
     '    unit_index = *(datum_index *)(player + 0x34);\n    if (unit_index == 0xffffffff) {\n        return;\n    }\n'
     '    if (current_game_engine != 0 && game_engine_state_value != 0) {\n        return;\n    }\n'
     '    if (network_game_mode != 2) {\n        return;\n    }\n'
     '    unit = *(uint8_t **)((uint8_t *)object_data->data + (unit_index & 0xffff) * 12 + 8);\n'
     '    parent_index = *(datum_index *)(unit + 0x11c);\n'
     '    if (parent_index != 0xffffffff) {\n'
     '        uint8_t *parent = *(uint8_t **)((uint8_t *)object_data->data + (parent_index & 0xffff) * 12 + 8);\n\n'
     '        result = -1;\n'
     '        game_engine_find_valid_starting_locations((real_point3d *)(parent + 0xa0), 2.5f, 0.0f, 3, -1, 1, &result);\n'
     '    } else {\n'
     '        result = game_engine_find_one_valid_starting_location(-1, 3, (real_point3d *)(unit + 0xa0), 1.5f, 0.6f);\n    }\n'
     '    if (result != -1) {\n        game_engine_ctf_score_flag(player_index, result);\n    }\n')

# game_engine_send_end_game_notification takes EAX reason: 1 at 0x45fddd, 3 at 0x45ff17, 2 at 0x4601bf.
for path, reason, where in (('src/game/game_engine_begin_end_game_sequence.c', 1, '0x45fddd'),
                            ('src/game/game_engine_update_end_game_sequence.c', 3, '0x45ff17'),
                            ('src/game/game_engine_tick.c', 2, '0x4601bf')):
    s = open(path, encoding='utf-8').read()
    head, sep, tail = s.partition('#if 0')
    old_decl = 'extern void game_engine_send_end_game_notification(void);'
    assert head.count(old_decl) == 1, path
    head = head.replace(old_decl, 'extern void game_engine_send_end_game_notification(uint32_t reason); // blam-cc: EAX reason;')
    assert head.count('game_engine_send_end_game_notification();') == 1, path
    head = head.replace('game_engine_send_end_game_notification();',
                        'game_engine_send_end_game_notification(%d); // FIXED 2026-09-28: %s loads EAX = %d' % (reason, where, reason))
    open(path, 'w', encoding='utf-8').write(head + sep + tail)
print('ok')
