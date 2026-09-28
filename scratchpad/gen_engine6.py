exec(open(r'C:\Users\Liam-\halo-re\scratchpad\engine_lib.py').read())
EXT.update({
    'king_move': ('extern int32_t game_engine_state_value; // 0x0087aa10\n'
                  'extern int32_t king_hill_move_ticks_006b1068; // 0x006b1068\n'
                  'extern int32_t king_starting_location_type; // 0x006b1064\n'
                  'extern int32_t king_starting_location_count; // 0x006b0f50\n'
                  'extern real_point3d king_hill_boundary_center; // 0x006b1044\n'
                  'extern int32_t game_engine_pick_random_recent_location(int32_t exclude_value, int32_t fallback); // 0x46a1b0, blam-cc: ECX fallback\n'
                  'extern void game_engine_koth_build_hill_boundary(void); // 0x46a240\n'
                  'extern void custom_waypoint_register(datum_index owner, int16_t slot, real_point3d *position, const char *icon_name,\n'
                  '    float height_offset, datum_index player_filter, int16_t team_filter); // 0x462260, blam-cc: EAX owner, CX slot, EBX position, EDI icon\n'
                  'extern void console_print_error_va(uint8_t clear_first, const char *format, ...); // 0x4c67c0, blam-cc: AL clear_first\n'
                  'extern void game_engine_koth_update_hill_occupancy_state(void); // 0x46acb0'),
    'flag_stand': ('extern real_point3d *ctf_team_flag_stand_position[2]; // 0x006b0e88\n'
                   'extern double pow(double x, double y); // C runtime (libcmt; the retail copy is the CRT pow)'),
})

emit(0x46aee0, 286, 'game_engine_king_unknown_48',
     'while the game runs, as the server and with variant +0x7c set (moving hill), counts the hill move ticks down; at zero they restart at 0x708, a new hill location differing from the current one is picked (0x46a1b0), the boundary rebuilt and sound 0x1e queued, repeating while the new hill has no starting locations. Then registers the "crown_blue" waypoint (slot 0) at the hill centre, or prints FAILED TO FIND HILL, and as the server updates the hill occupancy. UNSURE: 0x46a1b0\'s fallback is the caller\'s ECX, which is not set here (whatever the engine update left); it is modeled as the current location, i.e. no move.',
     ['current_game_engine', 'network_game_mode', 'variant', 'sound', 'king_move'], 'void %s(void)',
     '    if ((current_game_engine == 0 || game_engine_state_value == 0) && network_game_mode == 2 &&\n'
     '        game_engine_variant.ctf_option_7c != 0 && --king_hill_move_ticks_006b1068 == 0) {\n'
     '        king_hill_move_ticks_006b1068 = 0x708;\n'
     '        king_starting_location_type = game_engine_pick_random_recent_location(king_starting_location_type, king_starting_location_type);\n'
     '        game_engine_koth_build_hill_boundary();\n        game_engine_queue_multiplayer_sound(0x1e, 0xffffffff, 1);\n'
     '        while (king_starting_location_count == 0 && king_starting_location_type != 0) {\n'
     '            king_starting_location_type = game_engine_pick_random_recent_location(king_starting_location_type, king_starting_location_type);\n'
     '            game_engine_koth_build_hill_boundary();\n            game_engine_queue_multiplayer_sound(0x1e, 0xffffffff, 1);\n        }\n    }\n'
     '    if (king_starting_location_count > 0) {\n        real_point3d position = king_hill_boundary_center;\n\n'
     '        custom_waypoint_register(0xffffffff, 0, &position, "crown_blue", 0.0f, 0xffffffff, -1);\n'
     '    } else {\n        console_print_error_va(0, "FAILED TO FIND HILL");\n    }\n'
     '    if (network_game_mode == 2) {\n        game_engine_koth_update_hill_occupancy_state();\n    }\n')

emit(0x469ae0, 263, 'game_engine_ctf_unknown_70',
     'a respawn weighting for a position: 1.0 unless variant +0x7c is set; else the squared distance to the enemy flag stand, clamped to 0.5..10, inverted; after the first second (tick > 30) a distance above 1 raises it to the power 0.33 and the result is clamped to 0.5..2.0.',
     ['player_data', 'game_time', 'variant', 'flag_stand'], 'float %s(datum_index player_index, real_point3d *position)',
     '    real_point3d *stand;\n    float dx, dy, dz, distance_squared, weight;\n    int32_t other_team;\n\n'
     '    if (game_engine_variant.ctf_option_7c == 0) {\n        return 1.0f;\n    }\n'
     '    other_team = (*(int32_t *)(%s + 0x20) + 1) %% 2;\n' % P('player_index') +
     '    stand = ctf_team_flag_stand_position[other_team];\n'
     '    dx = stand->x - position->x;\n    dy = stand->y - position->y;\n    dz = stand->z - position->z;\n'
     '    distance_squared = dz * dz + dx * dx + dy * dy;\n'
     '    if (distance_squared < 0.5f) {\n        distance_squared = 0.5f;\n    } else if (distance_squared > 10.0f) {\n        distance_squared = 10.0f;\n    }\n'
     '    weight = 1.0f / distance_squared;\n'
     '    if (*(int32_t *)(game_time + 0xc) <= 0x1e) {\n        return weight;\n    }\n'
     '    if (distance_squared > 1.0f) {\n        weight = (float)pow((double)weight, (double)0.33f);\n    }\n'
     '    if (weight < 0.5f) {\n        return 0.5f;\n    }\n    if (weight > 2.0f) {\n        return 2.0f;\n    }\n    return weight;\n')
print('ok')
