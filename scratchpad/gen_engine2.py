exec(open(r'C:\Users\Liam-\halo-re\scratchpad\engine_lib.py').read())
EXT.update({
    'datum_get': 'extern void *datum_get(datum_index handle, data_array *array); // 0x4d0680, blam-cc: EDX handle, ESI array',
    'active_by_index': 'extern uint32_t players_get_active_by_index(int32_t index); // 0x45c6f0, blam-cc: EAX',
    'gamespy_string': 'extern void FUN_00615590(void *buffer, const char *value); // 0x615590, GameSpy query-report string writer (networking phase)',
    'time_ascii': 'extern void game_time_format_minutes_seconds_ascii(uint32_t ticks, uint32_t count, char *dest); // 0x466600, blam-cc: ECX ticks',
    'tag_lookup': 'extern datum_index tag_lookup(tag_group group, char *path); // 0x442550, blam-cc: EDI group',
    'tag_instances': 'extern tag_instance *tag_instances; // 0x0087bc14',
    'missing_string_text': 'extern uint16_t missing_string_text[]; // 0x00671fac',
    'get_string': 'extern uint16_t *text_string_list_get_string(datum_index list_id, int16_t index); // 0x5578c0, blam-cc: ECX list, EDX index',
    'king_last': 'extern int32_t king_bucket_last_credit_tick[16]; // 0x006b0f00',
    'king_state': ('extern int32_t king_starting_location_type; // 0x006b1064\n'
                   'extern int32_t king_hill_move_ticks_006b1068; // 0x006b1068, UNSURE name\n'
                   'extern int32_t king_hill_index_006b1058; // 0x006b1058, UNSURE name\n'
                   'extern int32_t king_hill_state_006b1050; // 0x006b1050 (king_globals +0x00)\n'
                   'extern int32_t king_hill_state_006b1054; // 0x006b1054 (king_globals +0x04)'),
    'koth_boundary': 'extern void game_engine_koth_build_hill_boundary(void); // 0x46a240',
    'oddball_block': ('extern int32_t king_alt_score_target; // 0x006b1148, the first of 0x51 dwords up to 0x006b128c\n'
                      'extern int32_t king_alt_team_scores_network[16]; // 0x0087a680, the first of 0x51 dwords\n'
                      'extern int32_t oddball_ball_timers_006b11cc[16]; // 0x006b11cc, UNSURE name\n'
                      'extern int32_t king_hill_occupant_last_tick[16]; // 0x006b124c'),
    'relocate': 'extern void game_engine_koth_relocate_hill_marker(void); // 0x46bfe0',
    'bucket_extra': 'extern int32_t game_engine_bucket_scores_extra[16]; // 0x006b1358',
    'check_bucket': 'extern void game_engine_check_bucket_scores_and_end_round(void); // 0x46db70',
    'scoring_capacity': 'extern uint8_t game_engine_team_has_scoring_capacity(int32_t team); // 0x46e250',
    'end_game': 'extern void game_engine_begin_end_game_sequence(void); // 0x45fd90',
    'catchup': 'extern void game_engine_apply_catchup_speed_boost(void); // 0x46e310',
    'player_from_unit': 'extern datum_index player_index_from_unit_index(datum_index unit_index); // 0x474db0',
    'object_try_and_get': 'extern void *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, blam-cc: ECX, stack',
    'must_ready': 'extern uint32_t weapon_must_be_readied(datum_index item_index); // 0x4c2ea0, blam-cc: EAX',
})

HEADER_TEXT = ('static uint16_t *multiplayer_text(int16_t index)\n{\n'
               '    datum_index list = tag_lookup(0x75737472, "ui\\\\multiplayer_game_text"); // \'ustr\', 0x00660c38\n\n'
               '    if (list == 0xffffffff) {\n        return (uint16_t *)L""; // 0x00660c34\n    }\n'
               '    {\n        uint8_t *strings = (uint8_t *)tag_instances[list & 0xffff].data;\n\n'
               '        if (*(int32_t *)strings > index) {\n            uint8_t *element = *(uint8_t **)(strings + 4) + index * 0x14;\n'
               '            uint32_t size = *(uint32_t *)element;\n\n            if ((int32_t)size > 0) {\n'
               '                uint16_t *text = *(uint16_t **)(element + 0xc);\n\n                text[(size >> 1) - 1] = 0;\n'
               '                return text;\n            }\n        }\n        return missing_string_text;\n    }\n}\n')
HT_EXT = ['tag_lookup', 'tag_instances', 'missing_string_text']
LOOKUP = ('static uint16_t *multiplayer_text(int16_t index)\n{\n'
          '    datum_index list = tag_lookup(0x75737472, "ui\\\\multiplayer_game_text"); // \'ustr\', 0x00660c38\n\n'
          '    return list == 0xffffffff ? (uint16_t *)L"" : text_string_list_get_string(list, index);\n}\n')

def emitx(addr, size, name, note, ext, sig, body, helper=''):
    emit(addr, size, name, note, ext, sig, body, '#include <string.h>\n', helper)

W = 'wchar_t *%s(wchar_t *buffer)'
# score header texts (inlined string list access, string index 0x9a / 0x9e)
for addr, name, index in ((0x469a30, 'game_engine_ctf_build_score_header_text', 0x9a), (0x46b720, 'game_engine_king_build_score_header_text', 0x9e)):
    emitx(addr, 120, name, 'copies string 0x%x of ui\\multiplayer_game_text (inlined lookup: missing -> L"<missing string>", no tag -> L"") into the buffer and returns it.' % index,
          HT_EXT, W, '    wcscpy(buffer, (const wchar_t *)multiplayer_text(0x%x));\n    return buffer;\n' % index, HEADER_TEXT)
emitx(0x46cfc0, 88, 'game_engine_oddball_build_score_header_text',
      'copies string 0x9a (variant +0x8c == 2) or 0x9e of ui\\multiplayer_game_text (text_string_list_get_string) into the buffer and returns it.',
      ['variant', 'tag_lookup', 'get_string'], W,
      '    wcscpy(buffer, (const wchar_t *)multiplayer_text((int16_t)(game_engine_variant.unknown_8c == 2 ? 0x9a : 0x9e)));\n    return buffer;\n', LOOKUP)
emitx(0x46eaf0, 91, 'game_engine_race_build_score_header_text',
      'copies string 0xb2 (the variant dword +0x7c is 2) or 0x19 of ui\\multiplayer_game_text into the buffer and returns it.',
      ['variant', 'tag_lookup', 'get_string'], W,
      '    wcscpy(buffer, (const wchar_t *)multiplayer_text((int16_t)(*(int32_t *)&game_engine_variant.ctf_option_7c == 2 ? 0xb2 : 0x19)));\n    return buffer;\n', LOOKUP)

# player round resets
for addr, name, size, stmt, ext in ((0x469f10, 'game_engine_ctf_player_round_reset', 74, '*(int16_t *)(player + 0xc8) = 0;', []),
                                    (0x46ba40, 'game_engine_king_player_round_reset', 74, '*(int16_t *)(player + 0xc4) = 0;', []),
                                    (0x46d310, 'game_engine_oddball_player_round_reset', 84, 'king_alt_player_score[player_index & 0xffff] = 0;', ['king_alt_player_score'])):
    emit(addr, size, name, 'for a valid player handle: %s' % stmt, ['player_data', 'datum_get'] + ext, 'void %s(datum_index player_index)',
         '    uint8_t *player = (uint8_t *)datum_get(player_index, player_data);\n\n    if (player != 0) {\n        %s\n    }\n' % stmt)

# GameSpy player query hooks
emit(0x469f60, 113, 'game_engine_ctf_query_player_score',
     'GameSpy player query: for key 0x16 and an active player at the index, writes its short +0xc8 into the report (0x616640) and returns 1; else 0.',
     ['player_data', 'datum_get', 'active_by_index', 'gamespy_team_score'], 'uint8_t %s(int32_t key, int32_t index, void *buffer)',
     '    uint8_t *player = (uint8_t *)datum_get(players_get_active_by_index(index), player_data);\n\n'
     '    if (player == 0 || key != 0x16) {\n        return 0;\n    }\n    FUN_00616640(buffer, *(int16_t *)(player + 0xc8));\n    return 1;\n')
for addr, name, size, value, ext in ((0x46ba90, 'game_engine_king_query_player_score', 162, '*(int16_t *)(player + 0xc4)', []),
                                     (0x46d370, 'game_engine_oddball_query_player_score', 169, 'king_alt_player_score[handle & 0xffff]', ['king_alt_player_score'])):
    emit(addr, size, name, 'GameSpy player query: for key 0x16 and an active player at the index, formats %s as ASCII minutes:seconds (0x466600, 0x100 characters) and writes it into the report (0x615590); returns 1, else 0.' % value,
         ['player_data', 'datum_get', 'active_by_index', 'gamespy_string', 'time_ascii'] + ext, 'uint8_t %s(int32_t key, int32_t index, void *buffer)',
         '    uint32_t handle = players_get_active_by_index(index);\n    uint8_t *player = (uint8_t *)datum_get(handle, player_data);\n    char text[0x100];\n\n'
         '    if (player == 0 || key != 0x16) {\n        return 0;\n    }\n'
         '    game_time_format_minutes_seconds_ascii((uint32_t)(%s), 0x100, text);\n    FUN_00615590(buffer, text);\n    return 1;\n' % value)

emit(0x469270, 139, 'game_engine_ctf_unknown_60',
     'false only as the server when the unit\'s player may not pick up the weapon: the item is a live weapon that must be readied, its flag bit 6 (+0x22c) is clear, and its owner team (+0xb8) is the player\'s team; true otherwise (also without a player or item).',
     ['player_data', 'network_game_mode', 'player_from_unit', 'object_try_and_get', 'must_ready'], 'uint8_t %s(datum_index unit_index, datum_index item_index)',
     '    datum_index player = player_index_from_unit_index(unit_index);\n    uint8_t *weapon;\n\n'
     '    if (player == 0xffffffff || item_index == 0xffffffff || network_game_mode != 2) {\n        return 1;\n    }\n'
     '    weapon = (uint8_t *)object_try_and_get(item_index, 4);\n'
     '    if (weapon != 0 && (uint8_t)weapon_must_be_readied(item_index) != 0 && (weapon[0x22c] & 0x40) == 0 &&\n'
     '        *(int16_t *)(weapon + 0xb8) == *(int32_t *)(%s + 0x20)) {\n        return 0;\n    }\n    return 1;\n' % P('player'))

emit(0x46a5e0, 76, 'game_engine_king_player_new_life', 'as the server without teams, clears the hill ticks and last credit tick of the player\'s team (+0x20).',
     ['player_data', 'network_game_mode', 'current_game_engine', 'teams', 'king_bucket_credit_ticks', 'king_last'], 'void %s(datum_index player_index)',
     '    if (network_game_mode == 2 && (current_game_engine == 0 || game_engine_teams_enabled_flag == 0)) {\n'
     '        int32_t team = *(int32_t *)(%s + 0x20);\n\n        king_bucket_credit_ticks[team] = 0;\n        king_bucket_last_credit_tick[team] = 0;\n    }\n' % P('player_index'))
emit(0x46bb70, 86, 'game_engine_king_reset_objects',
     'as the server: clears the 16 hill ticks, last credit ticks and in-hill flags, sets the starting location type 0, 0x006b1068 = 0x708, 0x006b1058 = -1, the first two king globals 0, and rebuilds the hill boundary (tail call).',
     ['network_game_mode', 'king_bucket_credit_ticks', 'king_last', 'king_hill_player_in_hill', 'king_state', 'koth_boundary'], 'void %s(void)',
     '    int32_t i;\n\n    if (network_game_mode != 2) {\n        return;\n    }\n    for (i = 0; i < 0x10; i++) {\n'
     '        king_bucket_credit_ticks[i] = 0;\n        king_bucket_last_credit_tick[i] = 0;\n        king_hill_player_in_hill[i] = 0;\n    }\n'
     '    king_starting_location_type = 0;\n    king_hill_move_ticks_006b1068 = 0x708;\n    king_hill_index_006b1058 = -1;\n'
     '    king_hill_state_006b1050 = 0;\n    king_hill_state_006b1054 = 0;\n    game_engine_koth_build_hill_boundary();\n')
emitx(0x46c080, 185, 'game_engine_oddball_initialize_for_new_game',
      'zeroes the 0x51 dwords from 0x006b1148 and from 0x0087a680; the score target becomes the variant score limit, times 0x708 unless variant +0x8c is 2; the 16 hill occupants and their last ticks become -1. As the server with variant +0x8c 1 or 2: each of the first variant +0x90 timers (0x006b11cc) is cleared and the hill marker relocated; otherwise they get 0x1c2, 0x384, ... Returns 1.',
      ['variant', 'network_game_mode', 'oddball_block', 'king_hill_occupant_table', 'relocate'], 'uint8_t %s(void)',
      '    int32_t mode = game_engine_variant.unknown_8c;\n    int32_t target;\n    int32_t i;\n\n'
      '    memset(&king_alt_score_target, 0, 0x51 * 4);\n    memset(king_alt_team_scores_network, 0, 0x51 * 4);\n'
      '    target = game_engine_variant.score_limit;\n    if (mode != 2) {\n        target *= 0x708;\n    }\n    king_alt_score_target = target;\n'
      '    for (i = 0; i < 0x10; i++) {\n        king_hill_occupant_table[i] = 0xffffffff;\n        king_hill_occupant_last_tick[i] = -1;\n    }\n'
      '    if (network_game_mode == 2) {\n        if (mode > 0 && mode <= 2) {\n            for (i = 0; i < game_engine_variant.unknown_90; i++) {\n'
      '                oddball_ball_timers_006b11cc[i] = 0;\n                game_engine_koth_relocate_hill_marker();\n            }\n        } else {\n'
      '            int32_t delay = 0;\n\n            for (i = 0; i < game_engine_variant.unknown_90; i++) {\n                delay += 0x1c2;\n'
      '                oddball_ball_timers_006b11cc[i] = delay;\n            }\n        }\n    }\n    return 1;\n')
emit(0x46c150, 70, 'game_engine_oddball_player_new_life', 'as the server clears the player score and, without teams, the score of the player\'s team.',
     ['player_data', 'network_game_mode', 'current_game_engine', 'teams', 'king_alt_player_score', 'king_alt_team_score'], 'void %s(datum_index player_index)',
     '    if (network_game_mode != 2) {\n        return;\n    }\n    king_alt_player_score[player_index & 0xffff] = 0;\n'
     '    if (current_game_engine == 0 || game_engine_teams_enabled_flag == 0) {\n'
     '        king_alt_team_score[*(int32_t *)(%s + 0x20)] = 0;\n    }\n' % P('player_index'))
emit(0x46db00, 106, 'game_engine_race_player_changed_object',
     'as the server: for a valid player when the variant dword +0x80 is 2, adds its short +0xc6 to the extra bucket score of its team; then checks the bucket scores for the end of the round (tail call).',
     ['player_data', 'network_game_mode', 'variant', 'datum_get', 'bucket_extra', 'check_bucket'], 'void %s(datum_index player_index)',
     '    uint8_t *player;\n\n    if (network_game_mode != 2) {\n        return;\n    }\n    player = (uint8_t *)datum_get(player_index, player_data);\n'
     '    if (player != 0 && game_engine_variant.ctf_value_80 == 2) {\n'
     '        game_engine_bucket_scores_extra[*(int32_t *)(player + 0x20)] += *(int16_t *)(player + 0xc6);\n    }\n'
     '    game_engine_check_bucket_scores_and_end_round();\n')
emit(0x46e400, 118, 'game_engine_race_unknown_48',
     'at game tick 2 queues sound 0x22 (teams) or 0x14; with teams, a team without scoring capacity (0x46e250, team 0 then 1) begins the end game sequence; then the catch-up speed boost (tail call).',
     ['game_time', 'current_game_engine', 'teams', 'sound', 'scoring_capacity', 'end_game', 'catchup'], 'void %s(void)',
     '    uint8_t teams = current_game_engine != 0 ? game_engine_teams_enabled_flag : 0;\n\n'
     '    if (*(int32_t *)(game_time + 0xc) == 2) {\n        game_engine_queue_multiplayer_sound(teams ? 0x22 : 0x14);\n    }\n'
     '    if (current_game_engine != 0 && game_engine_teams_enabled_flag != 0) {\n'
     '        if (game_engine_team_has_scoring_capacity(0) == 0) {\n            game_engine_begin_end_game_sequence();\n        }\n'
     '        if (game_engine_team_has_scoring_capacity(1) == 0) {\n            game_engine_begin_end_game_sequence();\n        }\n    }\n'
     '    game_engine_apply_catchup_speed_boost();\n')
emit(0x46ea00, 151, 'game_engine_race_get_score',
     'team_mode 1: the bucket score of the player\'s team; otherwise the player\'s short +0xc6 times 0x21 plus the number of bits set in its team\'s captured-flags mask (0x006b12d4).',
     ['player_data', 'bucket_scores', 'ctf_team_captured_flags_mask'], 'int32_t %s(datum_index player, int32_t team_mode)',
     '    uint8_t *p = %s;\n    uint32_t mask;\n    int32_t bits = 0;\n    int32_t i;\n\n'
     '    if (team_mode == 1) {\n        return game_engine_bucket_scores[*(int32_t *)(p + 0x20)];\n    }\n'
     '    mask = ctf_team_captured_flags_mask[*(int32_t *)(p + 0x20)];\n    for (i = 0; i < 0x20; i++) {\n'
     '        if ((mask & (1u << i)) != 0) {\n            bits++;\n        }\n    }\n    return *(int16_t *)(p + 0xc6) * 0x21 + bits;\n' % P('player'))
print('ok')
