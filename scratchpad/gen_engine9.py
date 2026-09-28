exec(open(r'C:\Users\Liam-\halo-re\scratchpad\engine_lib.py').read())
EXT.update({
    'ctf_update': ('extern int32_t game_engine_state_value; // 0x0087aa10\n'
                   'extern datum_index ctf_team_flag_object[2]; // 0x006b0e90\n'
                   'extern uint8_t unit_has_must_be_readied_weapon(uint32_t player_index); // 0x463300, blam-cc: ECX player_index\n'
                   'extern void unit_reset_gauge_if_flagged(uint32_t player_index); // 0x4633a0, blam-cc: EAX player_index\n'
                   'extern uint32_t weapon_must_be_readied(datum_index item_index); // 0x4c2ea0, blam-cc: EAX item_index\n'
                   'extern uint8_t game_engine_ctf_point_within_team_flag_radius(float radius, int32_t team, real_point3d *point); // 0x468990, blam-cc: EAX team, ECX point\n'
                   'extern void game_engine_ctf_notify_flag_carried_throttled(int32_t target_player); // 0x4689e0, blam-cc: EDI target_player\n'
                   'extern void game_engine_ctf_player_touch_flag(uint32_t player_index, int32_t team); // 0x468910, blam-cc: EAX team\n'
                   'extern void game_engine_ctf_player_drop_flag(uint32_t player_index, datum_index flag_object_index); // 0x4688b0, blam-cc: EAX player_index'),
    'oddball_kill': ('extern int32_t oddball_ball_timers_006b11cc[16]; // 0x006b11cc\n'
                     'extern uint8_t game_engine_is_inactive(void); // 0x461610\n'
                     'extern void game_engine_koth_alt_scorer_tick(uint32_t player_index); // 0x46c230, blam-cc: EAX player_index\n'
                     'extern void game_engine_broadcast_kill_feed_by_relationship(uint32_t source_player, int32_t no_source_message,\n'
                     '    int32_t message_a, int32_t message_b, uint32_t subject, uint8_t broadcast); // 0x460c10, blam-cc: BL broadcast'),
})

emit(0x468a20, 295, 'game_engine_ctf_update',
     'resets the unit gauge when its weapon must be readied; then as the server, for a unit holding a must-be-readied weapon (the enemy flag) while the game runs, within 1.0 of its own team\'s flag (the unit origin +0x5c): with variant +0x7f set and +0x80 zero, a team flag away from its stand (bit 6 of +0x22c) only triggers the throttled carried notice; otherwise the player touches the flag and drops the carried one (the weapon).',
     ['player_data', 'object_data', 'current_game_engine', 'network_game_mode', 'variant', 'ctf_update'], 'void %s(datum_index player_index)',
     '    uint8_t *player = %s;\n    datum_index unit_index;\n    uint8_t *unit;\n    int16_t weapon_slot;\n    datum_index weapon;\n    int32_t team;\n\n' % P('player_index') +
     '    if (unit_has_must_be_readied_weapon(player_index) != 0) {\n        unit_reset_gauge_if_flagged(player_index);\n    }\n'
     '    if (network_game_mode != 2) {\n        return;\n    }\n'
     '    unit_index = *(datum_index *)(player + 0x34);\n    if (unit_index == 0xffffffff) {\n        return;\n    }\n'
     '    unit = *(uint8_t **)((uint8_t *)object_data->data + (unit_index & 0xffff) * 12 + 8);\n'
     '    weapon_slot = *(int16_t *)(unit + 0x2f2);\n    if (weapon_slot == -1) {\n        return;\n    }\n'
     '    weapon = *(datum_index *)(unit + 0x2f8 + weapon_slot * 4);\n    if (weapon == 0xffffffff) {\n        return;\n    }\n'
     '    if (current_game_engine != 0 && game_engine_state_value != 0) {\n        return;\n    }\n'
     '    if (weapon_must_be_readied(weapon) == 0) {\n        return;\n    }\n'
     '    team = *(int32_t *)(player + 0x20);\n'
     '    if (game_engine_ctf_point_within_team_flag_radius(1.0f, team, (real_point3d *)(unit + 0x5c)) == 0) {\n        return;\n    }\n'
     '    if (game_engine_variant.ctf_option_7f != 0 && game_engine_variant.ctf_value_80 == 0) {\n'
     '        uint8_t *flag = *(uint8_t **)((uint8_t *)object_data->data + (ctf_team_flag_object[team] & 0xffff) * 12 + 8);\n\n'
     '        if (((*(uint32_t *)(flag + 0x22c) >> 6) & 1) != 0) {\n'
     '            game_engine_ctf_notify_flag_carried_throttled((int32_t)player_index);\n            return;\n        }\n    }\n'
     '    game_engine_ctf_player_touch_flag(player_index, team);\n    game_engine_ctf_player_drop_flag(player_index, weapon);\n')

HELP = '''// 0x46c8e0 (ESI player): whether the player holds one of the variant +0x90 balls.
static uint8_t oddball_is_carrier(datum_index player_index)
{
    int32_t i;

    for (i = 0; i < game_engine_variant.unknown_90; i++) {
        if (king_hill_occupant_table[i] == player_index) {
            return 1;
        }
    }
    return 0;
}

// 0x46c910: whether some ball has run out its timer and has no carrier.
static uint8_t oddball_any_ball_free(void)
{
    int32_t i;

    for (i = 0; i < game_engine_variant.unknown_90; i++) {
        if (oddball_ball_timers_006b11cc[i] == 0 && king_hill_occupant_table[i] == 0xffffffff) {
            return 1;
        }
    }
    return 0;
}
'''

emit(0x46c940, 374, 'game_engine_oddball_player_killed',
     'fired by game_engine_on_player_death with (killer, death object, victim, suicide); with variant +0x8c in 1..2 and as the server. For a real killer that is not a suicide: killing a carrier counts in the killer\'s +0xc6, a carrier\'s kill in its +0xc8 (both then score through game_engine_koth_alt_scorer_tick only when +0x8c is 2 and game_engine_is_inactive), otherwise the killer scores when a ball lies free; a killer with a unit then takes the victim\'s ball (or the first free one), with a kill feed (-1 / 0x24 / 0x25, BL 0). Finally every ball still held by the victim is dropped (-1). The helpers 0x46c8e0 / 0x46c910 / 0x46cef0 exist only for this function and are statics / inlined here.',
     ['player_data', 'network_game_mode', 'variant', 'king_hill_occupant_table', 'oddball_kill'],
     'void %s(datum_index killer, datum_index death_object, datum_index victim, uint8_t is_suicide)',
     '    int32_t count;\n    int32_t found = -1;\n    int32_t i;\n\n    (void)death_object;\n'
     '    if (game_engine_variant.unknown_8c <= 0 || game_engine_variant.unknown_8c > 2 || network_game_mode != 2) {\n        return;\n    }\n'
     '    count = game_engine_variant.unknown_90;\n'
     '    if (killer != 0xffffffff && is_suicide == 0) {\n'
     '        uint8_t *killer_player = %s;\n        uint8_t score;\n\n' % P('killer') +
     '        if (oddball_is_carrier(victim) || oddball_is_carrier(killer)) {\n'
     '            if (oddball_is_carrier(victim)) {\n                (*(int16_t *)(killer_player + 0xc6))++;\n'
     '            } else {\n                (*(int16_t *)(killer_player + 0xc8))++;\n            }\n'
     '            score = game_engine_variant.unknown_8c == 2 ? game_engine_is_inactive() : 0; // 0x46cef0\n'
     '        } else {\n            score = oddball_any_ball_free();\n        }\n'
     '        if (score != 0) {\n            game_engine_koth_alt_scorer_tick(killer);\n        }\n'
     '        if (*(datum_index *)(killer_player + 0x34) != 0xffffffff) {\n'
     '            for (i = 0; i < count; i++) {\n'
     '                if (oddball_ball_timers_006b11cc[i] == 0 && found == -1 && king_hill_occupant_table[i] == 0xffffffff) {\n                    found = i;\n                }\n'
     '                if (king_hill_occupant_table[i] == victim) {\n                    found = i;\n                    break;\n                }\n            }\n'
     '            if (found != -1) {\n'
     '                int32_t message = (game_engine_variant.unknown_8c > 0 && game_engine_variant.unknown_8c <= 2) ? -1 : 0x23;\n\n'
     '                game_engine_broadcast_kill_feed_by_relationship(killer, message, 0x24, 0x25, killer, 0);\n'
     '                king_hill_occupant_table[found] = killer;\n            }\n        }\n    }\n'
     '    for (i = 0; i < count; i++) {\n        if (king_hill_occupant_table[i] == victim) {\n            king_hill_occupant_table[i] = 0xffffffff;\n        }\n    }\n',
     helper=HELP)
print('ok')
