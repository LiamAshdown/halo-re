exec(open(r'C:\Users\Liam-\halo-re\scratchpad\engine_lib.py').read())
EXT['check_bucket'] = 'extern void game_engine_check_bucket_scores_and_end_round(void); // 0x46db70'
EXT['flag_eligible'] = 'extern uint8_t game_engine_ctf_is_flag_eligible_for_capture(uint32_t team, int32_t flag_id); // 0x46df30, blam-cc: ECX team, EDI flag_id'

SC = 'int32_t %s(datum_index player, int32_t team_mode)'
TS = 'int32_t %s(int32_t team)'
PT = 'wchar_t *%s(datum_index player, wchar_t *buffer)'
TT = 'wchar_t *%s(int32_t team, wchar_t *buffer)'
QR = 'uint8_t %s(int32_t key, int32_t team, void *buffer)'
U84 = 'uint8_t %s(int32_t kind)'
V = 'void %s(void)'

def query(addr, engine, array, ext):
    emit(addr, 39, 'game_engine_%s_query_team_score' % engine,
         'GameSpy query report: for key 0x1d writes the team score (%s[team]) into the report (0x616640) and returns 1; other keys 0.' % array,
         [ext, 'gamespy_team_score'], QR,
         '    if (key != 0x1d) {\n        return 0;\n    }\n    FUN_00616640(buffer, %s[team]);\n    return 1;\n' % array)

def text_d(addr, size, name, what, ext, sig, value):
    emit(addr, size, name, 'formats %s as L"%%d" (0x006607a0) into the buffer and returns it.' % what, ext + ['format_d'], sig,
         '    string_format_wide_va((uint16_t *)buffer, (const uint16_t *)L"%%d", %s);\n    return buffer;\n' % value)

def text_time(addr, size, name, what, ext, sig, value):
    emit(addr, size, name, 'formats %s as minutes:seconds (0x466530) into the buffer and returns it.' % what, ext + ['format_time'], sig,
         '    game_time_format_minutes_seconds((uint32_t)(%s), 0x100, buffer);\n    return buffer;\n' % value)

def reset_sound(addr, size, name, note, body_sound):
    emit(addr, size, name, note, ['sound', 'current_game_engine', 'teams'], V,
         '    uint8_t teams = current_game_engine != 0 ? game_engine_teams_enabled_flag : 0;\n\n'
         '    (void)teams;\n    game_engine_queue_multiplayer_sound(%s);\n' % body_sound)

# ---- ctf
emit(0x468820, 23, 'game_engine_ctf_reset_round', 'queues multiplayer sound 0x16 (player -1, not broadcast).', ['sound'], V,
     '    game_engine_queue_multiplayer_sound(0x16);\n')
emit(0x469960, 36, 'game_engine_ctf_object_expired', 'the expiring flag object\'s +0xc0 dword becomes -1.', ['object_data'],
     'void %s(datum_index object_index)',
     '    uint8_t *object = (uint8_t *)((object_header *)object_data->data)[object_index & 0xffff].data;\n\n'
     '    *(int32_t *)(object + 0xc0) = -1;\n', '#include "objects.h"\n')
emit(0x469990, 50, 'game_engine_ctf_get_score', 'with team_mode, the flag touch count of the player\'s team (player +0x20); else the player\'s short +0xc8.',
     ['player_data', 'ctf_team_flag_touch_count'], SC,
     '    uint8_t *p = %s;\n\n    if (team_mode != 0) {\n        return ctf_team_flag_touch_count[*(int32_t *)(p + 0x20)];\n    }\n'
     '    return *(int16_t *)(p + 0xc8);\n' % P('player'))
emit(0x4699d0, 12, 'game_engine_ctf_get_team_score', 'the team\'s flag touch count.', ['ctf_team_flag_touch_count'], TS,
     '    return ctf_team_flag_touch_count[team];\n')
emit(0x4699e0, 13, 'game_engine_ctf_unknown_84', 'true for kind 0.', [], U84, '    return (uint8_t)(kind == 0);\n')
text_d(0x4699f0, 54, 'game_engine_ctf_build_player_text', 'the player\'s short +0xc8', ['player_data'], PT, '(int32_t)*(int16_t *)(%s + 0xc8)' % P('player'))
text_d(0x469ab0, 36, 'game_engine_ctf_build_team_score_text', 'the team\'s flag touch count', ['ctf_team_flag_touch_count'], TT, 'ctf_team_flag_touch_count[team]')
query(0x469fe0, 'ctf', 'ctf_team_flag_touch_count', 'ctf_team_flag_touch_count')

# ---- king
reset_sound(0x46a630, 51, 'game_engine_king_reset_round', 'queues multiplayer sound 0x20 with teams, 0x24 without.', 'teams ? 0x20 : 0x24')
emit(0x46b200, 50, 'game_engine_king_get_score', 'with team_mode, the hill ticks of the player\'s team (player +0x20); else the player\'s short +0xc4.',
     ['player_data', 'king_bucket_credit_ticks'], SC,
     '    uint8_t *p = %s;\n\n    if (team_mode != 0) {\n        return king_bucket_credit_ticks[*(int32_t *)(p + 0x20)];\n    }\n'
     '    return *(int16_t *)(p + 0xc4);\n' % P('player'))
emit(0x46b240, 12, 'game_engine_king_get_team_score', 'the team\'s hill ticks.', ['king_bucket_credit_ticks'], TS,
     '    return king_bucket_credit_ticks[team];\n')
text_time(0x46b6e0, 52, 'game_engine_king_build_player_text', 'the player\'s short +0xc4', ['player_data'], PT, '(int32_t)*(int16_t *)(%s + 0xc4)' % P('player'))
text_time(0x46b7a0, 34, 'game_engine_king_build_team_score_text', 'the team\'s hill ticks', ['king_bucket_credit_ticks'], TT, 'king_bucket_credit_ticks[team]')
emit(0x46b7d0, 21, 'game_engine_king_waypoint_filter', 'true when the player is not in the hill.', ['king_hill_player_in_hill'],
     'uint8_t %s(datum_index player)', '    return (uint8_t)(king_hill_player_in_hill[player & 0xffff] == 0);\n')
query(0x46bb40, 'king', 'king_bucket_credit_ticks', 'king_bucket_credit_ticks')

# ---- oddball
emit(0x46cea0, 58, 'game_engine_oddball_get_score', 'team_mode 1: the team score of the player\'s team; otherwise the player score.',
     ['player_data', 'king_alt_team_score', 'king_alt_player_score'], SC,
     '    if (team_mode == 1) {\n        return king_alt_team_score[*(int32_t *)(%s + 0x20)];\n    }\n'
     '    return king_alt_player_score[player & 0xffff];\n' % P('player'))
emit(0x46cee0, 12, 'game_engine_oddball_get_team_score', 'the team score.', ['king_alt_team_score'], TS, '    return king_alt_team_score[team];\n')
emit(0x46cf00, 25, 'game_engine_oddball_unknown_84', 'true for kind 1 when the variant\'s +0x8c is 2.', ['variant'], U84,
     '    return (uint8_t)(kind == 1 && game_engine_variant.unknown_8c == 2);\n')
emit(0x46cf20, 65, 'game_engine_oddball_time_scale_override',
     'without a value, false; when the player is among the first variant +0x90 hill occupants (0x006b120c), whether the value is variant +0x84, else whether it is variant +0x88.',
     ['variant', 'king_hill_occupant_table'], 'uint8_t %s(uint32_t player, int32_t value)',
     '    int32_t i;\n\n    if (value == 0) {\n        return 0;\n    }\n    for (i = 0; i < game_engine_variant.unknown_90; i++) {\n'
     '        if (king_hill_occupant_table[i] == player) {\n            return (uint8_t)(value == game_engine_variant.unknown_84);\n        }\n    }\n'
     '    return (uint8_t)(value == game_engine_variant.unknown_88);\n')
emit(0x46cf70, 69, 'game_engine_oddball_build_player_text',
     'the player score, as L"%d" when the variant\'s +0x8c is 2, else as minutes:seconds; returns the buffer.',
     ['variant', 'king_alt_player_score', 'format_d', 'format_time'], PT,
     '    int32_t score = king_alt_player_score[player & 0xffff];\n\n    if (game_engine_variant.unknown_8c == 2) {\n'
     '        string_format_wide_va((uint16_t *)buffer, (const uint16_t *)L"%d", score);\n    } else {\n'
     '        game_time_format_minutes_seconds((uint32_t)score, 0x100, buffer);\n    }\n    return buffer;\n')
emit(0x46d020, 64, 'game_engine_oddball_build_team_score_text',
     'the team score, as L"%d" when the variant\'s +0x8c is 2, else as minutes:seconds; returns the buffer.',
     ['variant', 'king_alt_team_score', 'format_d', 'format_time'], TT,
     '    int32_t score = king_alt_team_score[team];\n\n    if (game_engine_variant.unknown_8c == 2) {\n'
     '        string_format_wide_va((uint16_t *)buffer, (const uint16_t *)L"%d", score);\n    } else {\n'
     '        game_time_format_minutes_seconds((uint32_t)score, 0x100, buffer);\n    }\n    return buffer;\n')
query(0x46d420, 'oddball', 'king_alt_team_score', 'king_alt_team_score')

# ---- race
emit(0x46dab0, 68, 'game_engine_race_player_new_life',
     'stamps the player\'s +0x88 with the game tick, clears its captured-flags mask (0x006b12d4) and, as the server, checks the bucket scores for the end of the round (tail call).',
     ['player_data', 'game_time', 'network_game_mode', 'ctf_team_captured_flags_mask', 'check_bucket'], 'void %s(datum_index player)',
     '    *(int32_t *)(%s + 0x88) = *(int32_t *)(game_time + 0xc);\n    ctf_team_captured_flags_mask[player & 0xffff] = 0;\n'
     '    if (network_game_mode == 2) {\n        game_engine_check_bucket_scores_and_end_round();\n    }\n' % P('player'))
emit(0x46e9d0, 37, 'game_engine_race_waypoint_filter',
     'when the team\'s bit is set in the flag mask (0x006b1290), whether its flag is eligible for capture (0x46df30 with the team as both team and flag); else false.',
     ['ctf_globals_live', 'flag_eligible'], 'uint8_t %s(datum_index player, int32_t team)',
     '    (void)player;\n    if ((ctf_globals_live & (1u << (team & 0x1f))) == 0) {\n        return 0;\n    }\n'
     '    return game_engine_ctf_is_flag_eligible_for_capture((uint32_t)team, team);\n')
emit(0x46eaa0, 12, 'game_engine_race_get_team_score', 'the team\'s bucket score.', ['bucket_scores'], TS, '    return game_engine_bucket_scores[team];\n')
text_d(0x46eab0, 54, 'game_engine_race_build_player_text', 'the player\'s short +0xc6', ['player_data'], PT, '(int32_t)*(int16_t *)(%s + 0xc6)' % P('player'))
text_d(0x46eb50, 36, 'game_engine_race_build_team_score_text', 'the team\'s bucket score', ['bucket_scores'], TT, 'game_engine_bucket_scores[team]')
query(0x46efb0, 'race', 'game_engine_bucket_scores', 'bucket_scores')

# ---- slayer
emit(0x46f380, 55, 'game_engine_slayer_initialize_for_new_game', 'zeroes the slayer team and player scores and the two arrays at 0x0087a4a0 / 0x0087a4e0; returns 1.',
     ['slayer_scores'], 'uint8_t %s(void)',
     '    memset(slayer_team_score, 0, sizeof(slayer_team_score));\n    memset(slayer_player_score, 0, sizeof(slayer_player_score));\n'
     '    memset(slayer_unknown_0087a4a0, 0, sizeof(slayer_unknown_0087a4a0));\n    memset(slayer_unknown_0087a4e0, 0, sizeof(slayer_unknown_0087a4e0));\n    return 1;\n',
     '#include <string.h>\n')
reset_sound(0x46f420, 47, 'game_engine_slayer_reset_round', 'queues multiplayer sound 0x23 with teams, 0x15 without.', 'teams ? 0x23 : 0x15')
emit(0x46f980, 58, 'game_engine_slayer_get_score', 'team_mode 1: the team score of the player\'s team; otherwise the player score.',
     ['player_data', 'slayer_scores'], SC,
     '    if (team_mode == 1) {\n        return slayer_team_score[*(int32_t *)(%s + 0x20)];\n    }\n'
     '    return slayer_player_score[player & 0xffff];\n' % P('player'))
emit(0x46f9c0, 12, 'game_engine_slayer_get_team_score', 'the team score.', ['slayer_scores'], TS, '    return slayer_team_score[team];\n')
emit(0x46f9d0, 12, 'game_engine_slayer_unknown_84', 'true for kind 1.', [], U84, '    return (uint8_t)(kind == 1);\n')
text_d(0x46f9e0, 41, 'game_engine_slayer_build_player_text', 'the player score', ['slayer_scores'], PT, 'slayer_player_score[player & 0xffff]')
text_d(0x46fa10, 36, 'game_engine_slayer_build_team_score_text', 'the team score', ['slayer_scores'], TT, 'slayer_team_score[team]')
query(0x46fdb0, 'slayer', 'slayer_team_score', 'slayer_scores')
emit(0x46fde0, 39, 'game_engine_slayer_reset_objects', 'as the server, zeroes the slayer team and player scores.',
     ['network_game_mode', 'slayer_scores'], V,
     '    if (network_game_mode == 2) {\n        memset(slayer_team_score, 0, sizeof(slayer_team_score));\n'
     '        memset(slayer_player_score, 0, sizeof(slayer_player_score));\n    }\n', '#include <string.h>\n')
print('ok')
