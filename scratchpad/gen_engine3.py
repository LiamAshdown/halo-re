exec(open(r'C:\Users\Liam-\halo-re\scratchpad\engine_lib.py').read())
exec(open(r'C:\Users\Liam-\halo-re\scratchpad\gen_engine2.py').read().split("HEADER_TEXT = (")[0].split("exec(open(r'C:\\Users\\Liam-\\halo-re\\scratchpad\\engine_lib.py').read())")[1])
EXT.update({
    'is_winning': 'extern uint32_t game_engine_is_object_winning(uint32_t handle); // 0x463660, blam-cc: EAX',
    'iterator_next': 'extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, blam-cc: EDI',
})

emit(0x46eb80, 132, 'game_engine_race_is_winner',
     'with teams: when exactly one team still has scoring capacity (0x46e250 for teams 0 and 1), whether the player\'s team is that one; when neither has, -1; when both have, game_engine_is_object_winning. Without teams game_engine_is_object_winning.',
     ['player_data', 'current_game_engine', 'teams', 'scoring_capacity', 'is_winning'], 'uint32_t %s(datum_index player)',
     '    uint8_t capacity[2];\n\n    if (current_game_engine == 0 || game_engine_teams_enabled_flag == 0) {\n'
     '        return game_engine_is_object_winning(player);\n    }\n'
     '    capacity[0] = game_engine_team_has_scoring_capacity(0);\n    capacity[1] = game_engine_team_has_scoring_capacity(1);\n'
     '    if (capacity[0] != capacity[1]) {\n        return (uint32_t)(capacity[*(int32_t *)(%s + 0x20)] != 0);\n    }\n'
     '    if (capacity[0] == 0) {\n        return 0xffffffff;\n    }\n    return game_engine_is_object_winning(player);\n' % P('player'))
emit(0x46ef30, 113, 'game_engine_race_query_player_score',
     'GameSpy player query: for key 0x16 and an active player at the index, writes its short +0xc6 into the report (0x616640) and returns 1; else 0.',
     ['player_data', 'datum_get', 'active_by_index', 'gamespy_team_score'], 'uint8_t %s(int32_t key, int32_t index, void *buffer)',
     '    uint8_t *player = (uint8_t *)datum_get(players_get_active_by_index(index), player_data);\n\n'
     '    if (player == 0 || key != 0x16) {\n        return 0;\n    }\n    FUN_00616640(buffer, *(int16_t *)(player + 0xc6));\n    return 1;\n')
emit(0x46fd30, 120, 'game_engine_slayer_query_player_score',
     'GameSpy player query: for key 0x16 and an active player at the index, writes its slayer score into the report (0x616640) and returns 1; else 0.',
     ['player_data', 'datum_get', 'active_by_index', 'gamespy_team_score', 'slayer_scores'], 'uint8_t %s(int32_t key, int32_t index, void *buffer)',
     '    uint32_t handle = players_get_active_by_index(index);\n    uint8_t *player = (uint8_t *)datum_get(handle, player_data);\n\n'
     '    if (player == 0 || key != 0x16) {\n        return 0;\n    }\n    FUN_00616640(buffer, slayer_player_score[handle & 0xffff]);\n    return 1;\n')
emit(0x46f3c0, 84, 'game_engine_slayer_player_new_life',
     'the player\'s +0x88 becomes -1; as the server its score is cleared and, without teams, the score of its team.',
     ['player_data', 'network_game_mode', 'current_game_engine', 'teams', 'slayer_scores'], 'void %s(datum_index player_index)',
     '    uint8_t *player = %s;\n\n    *(int32_t *)(player + 0x88) = -1;\n    if (network_game_mode != 2) {\n        return;\n    }\n'
     '    slayer_player_score[player_index & 0xffff] = 0;\n    if (current_game_engine == 0 || game_engine_teams_enabled_flag == 0) {\n'
     '        slayer_team_score[*(int32_t *)(player + 0x20)] = 0;\n    }\n' % P('player_index'))
emit(0x46fc70, 178, 'game_engine_slayer_player_round_reset',
     'for a valid player: its +0x88 becomes -1 and its score 0, and every player whose +0x88 names it is reset to -1 too (a player data iterator).',
     ['player_data', 'datum_get', 'slayer_scores', 'iterator_next'], 'void %s(datum_index player_index)',
     '    uint8_t *player = (uint8_t *)datum_get(player_index, player_data);\n    data_iterator iterator;\n    uint8_t *other;\n\n'
     '    if (player == 0) {\n        return;\n    }\n    *(int32_t *)(player + 0x88) = -1;\n    slayer_player_score[player_index & 0xffff] = 0;\n'
     '    iterator.data = player_data;\n    iterator.next_index = 0;\n    iterator.index = 0xffffffff;\n'
     '    iterator.signature = (uint32_t)player_data ^ 0x69746572; // \'reti\'\n'
     '    for (other = (uint8_t *)data_iterator_next(&iterator); other != 0; other = (uint8_t *)data_iterator_next(&iterator)) {\n'
     '        if (*(datum_index *)(other + 0x88) == player_index) {\n            *(int32_t *)(other + 0x88) = -1;\n        }\n    }\n')
print('ok')
