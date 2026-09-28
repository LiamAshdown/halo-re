exec(open(r'C:\Users\Liam-\halo-re\scratchpad\uigen_lib.py').read())
exec(open(r'C:\Users\Liam-\halo-re\scratchpad\gen_ui_mid2.py').read().split('GAME = (')[0].split("exec(open(")[1].split('\n', 1)[1])

EXT.update({
    'string_format_wide_va': 'extern void string_format_wide_va(uint16_t *dest, const uint16_t *format, ...); // 0x557930, blam-cc: EDX dest',
    'network_host_number': ('extern int32_t network_host_number_field_00719218; // 0x00719218, UNSURE (1 or 2: which option is being typed)\n'
                            'extern uint16_t network_host_number_text_0071921c[0x10]; // 0x0071921c, UNSURE'),
    'saved_game_file_exists': 'extern uint8_t saved_game_file_exists(char *name); // 0x538770',
    'game_checkpoint_enumerate_files': 'extern int32_t game_checkpoint_enumerate_files(uint8_t include_autosaves, uint8_t sort_newest_first, void *callback, void *user_data); // 0x538e70',
    'ui_restoring_previous_widget': 'extern uint8_t ui_restoring_previous_widget; // 0x00718fcb',
    'campaign_level_paths': 'extern char *campaign_level_paths[]; // 0x00696574',
    'main_queue_map_change': 'extern void main_queue_map_change(char *map_name); // 0x4c8740, blam-cc: EAX',
    'network_wait_flag_00719739': 'extern uint8_t network_wait_flag_00719739; // 0x00719739',
    'network_game_settings_ack_send': 'extern char network_game_settings_ack_send(uint8_t *client, int16_t template_row); // 0x4d9f50',
    'network_game_record_message_send': 'extern int32_t network_game_record_message_send(void *client, const uint32_t *source); // 0x4da130, blam-cc: EDX source, stack client',
    'player_profile_get': 'extern uint8_t player_profile_get(int32_t index, void *out_buffer); // 0x53a770, blam-cc: ECX out_buffer',
    'player_profile_load': 'extern void player_profile_load(int16_t player_index, void *source_profile, int32_t profile_id); // 0x495970, blam-cc: AX, EDX, stack',
})

GAME2 = ('    uint8_t *game = network_server_pointer != 0 ? (uint8_t *)network_server_pointer + 8\n'
         '                  : network_client != 0 ? network_client + 0xb14 : 0;\n')
SHOW = ('static void show(widget_instance *child, uint8_t visible)\n{\n'
        '    if (visible) {\n        child->scale = 1.0f;\n        child->hidden = 0;\n    } else {\n'
        '        *(uint32_t *)&child->scale = 0x3eaa7efa;\n        child->hidden = 1;\n    }\n}\n')

SPECS = [
 (0x4a21c0, 165, "for a selected profile: under the grandparent's first three children, the first spinner list (type 2) of each gives profile bytes +0x954 (selection + 1), +0x955 (selection + 1) and +0x12f (selection == 1). Returns 1, or 0 without a profile.",
  ['selected_saved_item', 'saved_item_working_copy'],
  PROFILE + '    widget_instance *group;\n\n    if (profile == 0) {\n        return 0;\n    }\n    group = widget->parent->parent->first_child;\n'
  '    profile[0x954] = (uint8_t)(first_list_child(group)->selection_index + 1);\n    group = group->next_sibling;\n'
  '    profile[0x955] = (uint8_t)(first_list_child(group)->selection_index + 1);\n    group = group->next_sibling;\n'
  '    profile[0x12f] = (uint8_t)(first_list_child(group)->selection_index == 1);\n    return 1;\n', FIRST_LIST),
 (0x4a3a70, 186, "with the grandparent's second child focused: formats option a (0x00719210) as L\"%d\" into 0x0071921c and opens the virtual keyboard on it (0x10 characters, field kind 0xd); on success edit field 4, 0x00719218 = 1, result 1. Otherwise, with the third child focused, the same for option b (edit field 5, 0x00719218 = 2) returning 1. Else 0.",
  ['network_game_options', 'string_format_wide_va', 'network_host_number', 'virtual_keyboard_open', 'network_host_edit_field_00719410'],
  '    widget_instance *second = widget->parent->parent->first_child->next_sibling;\n    widget_instance *third;\n    uint8_t result = 0;\n\n'
  '    if (second->parent->focused_child == second) {\n'
  '        string_format_wide_va(network_host_number_text_0071921c, (const uint16_t *)L"%d", network_game_option_a_00719210);\n'
  '        if (virtual_keyboard_open(network_host_number_text_0071921c, 0x10, 0xd) != 0) {\n'
  '            network_host_edit_field_00719410 = 4;\n            network_host_number_field_00719218 = 1;\n            result = 1;\n        }\n    }\n'
  '    third = second->next_sibling;\n    if (result == 0 && third->parent->focused_child == third) {\n'
  '        string_format_wide_va(network_host_number_text_0071921c, (const uint16_t *)L"%d", network_game_option_b_00719214);\n'
  '        if (virtual_keyboard_open(network_host_number_text_0071921c, 0x10, 0xd) != 0) {\n'
  '            network_host_edit_field_00719410 = 5;\n            network_host_number_field_00719218 = 2;\n            return 1;\n        }\n    }\n    return result;\n'),
 (0x4a0c60, 191, "for a selected profile: the first spinner list of the first child shows profile byte +0x12d (0..3, else 0), that of the second child byte +0x12c (0..4, else 0). Returns 1, or 0 without a profile.",
  ['selected_saved_item', 'saved_item_working_copy'],
  PROFILE + '    widget_instance *list;\n\n    if (profile == 0) {\n        return 0;\n    }\n'
  '    list = first_list_child(widget->first_child);\n    list->selection_index = (int16_t)(profile[0x12d] <= 3 ? profile[0x12d] : 0);\n'
  '    list = first_list_child(widget->first_child->next_sibling);\n    list->selection_index = (int16_t)(profile[0x12c] <= 4 ? profile[0x12c] : 0);\n    return 1;\n', FIRST_LIST),
 (0x4a41a0, 194, "shows the first child (scale 1, visible) when the 'savegame' file exists (0x538770, name at 0x0066a56c), else dims and hides it and focuses its next sibling; shows the third child when any checkpoint enumerates (> 0), else hides it and focuses the fourth. With either present returns 1. Otherwise: while restoring the previous widget, closes this one restoring the previous (returns 1); else queues the first campaign level (main_queue_map_change), clears 0x00719739 and returns whether a restore started meanwhile.",
  ['saved_game_file_exists', 'game_checkpoint_enumerate_files', 'ui_restoring_previous_widget', 'widget_instance_close_and_restore_previous',
   'campaign_level_paths', 'main_queue_map_change', 'network_wait_flag_00719739'],
  '    uint8_t has_save = saved_game_file_exists("savegame");\n'
  '    uint8_t has_checkpoints = (uint8_t)(game_checkpoint_enumerate_files(1, 1, 0, 0) > 0);\n    widget_instance *child = widget->first_child;\n\n'
  '    show(child, has_save);\n    if (!has_save) {\n        widget->focused_child = child->next_sibling;\n    }\n'
  '    child = child->next_sibling->next_sibling;\n    show(child, has_checkpoints);\n    if (!has_checkpoints) {\n        widget->focused_child = child->next_sibling;\n    }\n'
  '    if (has_save || has_checkpoints) {\n        return 1;\n    }\n    if (ui_restoring_previous_widget != 0) {\n'
  '        widget_instance_close_and_restore_previous(widget);\n        return 1;\n    }\n'
  '    main_queue_map_change(campaign_level_paths[0]);\n    network_wait_flag_00719739 = 0;\n'
  '    return (uint8_t)(ui_restoring_previous_widget != 0);\n', SHOW),
 (0x49dca0, 195, "with a client: a state word (+0xeda) of 1 reads the performance counter (result unused); in state 2, unless one of the 16 player entries of the server (+8) or client (+0xb14) game (0x20 each from +0x1a2) is valid and has machine byte (+0x1c) equal to the client word +0 and player byte (+0x1d) equal to event word 1, sends the game settings ack for event word 1 (zero-extended). Returns 1.",
  ['network_client', 'network_server_pointer', 'time_query_performance_counter_ms', 'network_player_entry_validate', 'network_game_settings_ack_send'],
  '    uint8_t *client = network_client;\n    int16_t *state;\n    uint8_t *game;\n\n    if (client == 0) {\n        return 1;\n    }\n'
  '    state = (int16_t *)(client + 0xeda);\n    if (*state == 1) {\n        time_query_performance_counter_ms();\n    }\n'
  '    if (*state != 2) {\n        return 1;\n    }\n'
  '    game = network_server_pointer != 0 ? (uint8_t *)network_server_pointer + 8 : network_client != 0 ? network_client + 0xb14 : 0;\n'
  '    if (network_client != 0 && *(int16_t *)network_client != -1) {\n        int16_t key = *(int16_t *)network_client;\n        int16_t i;\n\n'
  '        for (i = 0; i < 0x10; i++) {\n            uint8_t *entry = game + i * 0x20 + 0x1a2;\n\n'
  '            if (network_player_entry_validate(entry) != 0 && (int16_t)(int8_t)entry[0x1c] == key &&\n'
  '                (int16_t)(int8_t)entry[0x1d] == event[1]) {\n                return 1;\n            }\n        }\n    }\n'
  '    network_game_settings_ack_send(client, (int16_t)(uint16_t)event[1]);\n    return 1;\n'),
 (0x4a2a00, 195, "forgets the cached profile slot and reads the widget list (+0x44) at the ui list id of its committed selection (index -1 when out of range). Negative (not -1): fetches that profile into a 0x1ffc byte local and loads it for player 0 (0x495970), returning 1; a failed fetch returns 0. Not negative: raises quit confirm error 0x36 when none is up, sound 4, returns 0. -1 returns 0.",
  ['profile_slot_lookup_cache_00692ac8', 'ui_list_current', 'ui_lists', 'player_profile_get', 'player_profile_load', 'quit_confirm_error', 'widget_play_sound_effect'],
  '    uint8_t profile[0x1ffc];\n    int32_t id = list_item_id(*(int16_t *)((uint8_t *)widget + 0x3c));\n    int32_t item;\n\n'
  '    profile_slot_lookup_cache_00692ac8 = -1;\n    item = ((int32_t *)widget->list_items)[id];\n    if (item == -1) {\n        return 0;\n    }\n'
  '    if (item < 0) {\n        if (player_profile_get(item, profile) == 0) {\n            return 0;\n        }\n'
  '        player_profile_load(0, profile, item);\n        return 1;\n    }\n'
  '    if (quit_confirm_error_string_index == -1) {\n' + QUIT_ERR % '0x36' + '    }\n    widget_play_sound_effect(4);\n    return 0;\n', LIST_ID),
 (0x4a0590, 206, "for a selected variant: the first spinner list of the first child selects 1 when variant flag bit 2 (+0x38) is clear, else 0; that of the second child shows the dword +0x5c (0..13, else 0); that of the third child shows flag bit 5. Returns 1, or 0 without a variant.",
  ['selected_saved_item', 'saved_item_working_copy'],
  VARIANT + '    widget_instance *group;\n    uint32_t flags;\n    uint32_t value;\n\n    if (variant == 0) {\n        return 0;\n    }\n'
  '    flags = *(uint32_t *)(variant + 0x38);\n    group = widget->first_child;\n'
  '    first_list_child(group)->selection_index = (int16_t)(((flags >> 2) & 1) != 0 ? 0 : 1);\n    group = group->next_sibling;\n'
  '    value = *(uint32_t *)(variant + 0x5c);\n    first_list_child(group)->selection_index = (int16_t)(value <= 0xd ? value : 0);\n'
  '    group = group->next_sibling;\n    first_list_child(group)->selection_index = (int16_t)((*(uint32_t *)(variant + 0x38) >> 5) & 1);\n    return 1;\n', FIRST_LIST),
 (0x49dbc0, 211, "with a server (+8) or client (+0xb14) game whose byte +0x138 is 1 and a client with a machine word (+0) other than -1: finds the valid player entry (16 of 0x20 bytes from +0x1a2) with that machine byte (+0x1c) and event word 1 as player byte (+0x1d), copies it, toggles its byte +0x1e (becomes 1 when it was 0, else 0) and sends it as a game record message (0x4da130). Returns 1.",
  ['network_server_pointer', 'network_client', 'network_player_entry_validate', 'network_game_record_message_send'],
  GAME2 + '    int16_t key;\n    int32_t i;\n\n    if (game == 0 || game[0x138] != 1 || network_client == 0) {\n        return 1;\n    }\n'
  '    key = *(int16_t *)network_client;\n    if (key == -1) {\n        return 1;\n    }\n    for (i = 0; i < 0x10; i++) {\n'
  '        uint8_t *entry = game + 0x1a2 + i * 0x20;\n\n        if (network_player_entry_validate(entry) != 0 && (int16_t)(int8_t)entry[0x1c] == key &&\n'
  '            (int16_t)(int8_t)entry[0x1d] == event[1]) {\n            uint32_t copy[8];\n\n            memcpy(copy, entry, sizeof(copy));\n'
  '            ((uint8_t *)copy)[0x1e] = (uint8_t)(((uint8_t *)copy)[0x1e] == 0);\n'
  '            network_game_record_message_send(network_client, copy);\n            return 1;\n        }\n    }\n    return 1;\n'),
]
generate(SPECS)
