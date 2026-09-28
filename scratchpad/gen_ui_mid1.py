exec(open(r'C:\Users\Liam-\halo-re\scratchpad\uigen_lib.py').read())

DISPOSE_SERVER = ('    if (network_server_pointer != 0) {\n        network_game_server_host_dispose(network_server_pointer);\n'
                  '        network_server_pointer = 0;\n        unknown_0071c2dd = 0;\n    }\n')
SRV = ['network_server_pointer', 'network_game_server_host_dispose', 'unknown_0071c2dd']

SPECS = [
 (0x49d1b0, 87, 'disposes the network client globals and any server host, tears down the game setup widget, then creates a network client session; with one, network_game_mode = 1, the host handoff request is cleared and 1 returned, else 0.',
  ['network_client_globals_dispose'] + SRV + ['network_game_setup_teardown', 'network_session_create', 'network_client', 'network_game_mode', 'network_host_handoff_requested'],
  '    network_client_globals_dispose();\n' + DISPOSE_SERVER + '    network_game_setup_teardown();\n'
  '    network_client = (uint8_t *)network_session_create();\n    if (network_client == 0) {\n        return 0;\n    }\n'
  '    network_game_mode = 1;\n    network_host_handoff_requested = 0;\n    return 1;\n'),
 (0x49d480, 155, 'clears 0x0071c2dc; without a server, makes sure the variant history has an entry and creates the server host (its low byte is the result): on 1 forgets the current variant history entry, applies the current custom variant, syncs the variant defaults and sets network_game_mode = 2. While still good, creates the client session unless there is one (clearing the handoff request) and the result becomes whether one exists. On failure disposes the server host, the client globals and the game setup widget. Returns the result.',
  SRV + ['network_session_starting_0071c2dc', 'game_engine_ensure_variant_history_has_entry', 'network_game_server_host_create',
         'game_variant_history_current', 'game_engine_apply_current_custom_variant', 'game_engine_sync_variant_defaults',
         'network_game_mode', 'network_client', 'network_session_create', 'network_host_handoff_requested',
         'network_client_globals_dispose', 'network_game_setup_teardown'],
  '    uint8_t ok = 1;\n\n    network_session_starting_0071c2dc = 0;\n    if (network_server_pointer == 0) {\n'
  '        game_engine_ensure_variant_history_has_entry();\n        ok = (uint8_t)network_game_server_host_create();\n'
  '        if (ok == 1) {\n            game_variant_history_current = -1;\n            game_engine_apply_current_custom_variant();\n'
  '            game_engine_sync_variant_defaults();\n            network_game_mode = 2;\n        }\n    }\n'
  '    if (ok != 0 && network_client == 0) {\n        network_client = (uint8_t *)network_session_create();\n'
  '        if (network_client != 0) {\n            network_host_handoff_requested = 0;\n        }\n        ok = (uint8_t)(network_client != 0);\n    }\n'
  '    if (ok == 0) {\n' + DISPOSE_SERVER.replace('\n    ', '\n        ').replace('    if', '        if', 1) +
  '        network_client_globals_dispose();\n        network_game_setup_teardown();\n    }\n    return ok;\n'),
 (0x49d540, 111, 'clears 0x00714dd8 and the first byte of 0x00714ddc, tears down the game setup widget, disposes the client globals and any server host, then clears 0x0071c2dc, network_game_mode and 0x00719010, one local player, no selected saved item; restarts the title music unless it is pending; returns 1.',
  ['local_team_00714dd8', 'coop_profile_globals_block_00714ddc', 'network_game_setup_teardown', 'network_client_globals_dispose'] + SRV +
  ['main_menu_music_pending', 'network_session_starting_0071c2dc', 'network_game_mode', 'save_in_progress_00719010',
   'local_player_count', 'selected_saved_item', 'main_menu_play_title_music'],
  '    uint8_t music_pending;\n\n    local_team_00714dd8 = 0;\n    coop_profile_globals_block_00714ddc[0] = 0;\n    network_game_setup_teardown();\n'
  '    network_client_globals_dispose();\n' + DISPOSE_SERVER + '    music_pending = main_menu_music_pending;\n'
  '    network_session_starting_0071c2dc = 0;\n    network_game_mode = 0;\n    save_in_progress_00719010 = 0;\n'
  '    local_player_count = 1;\n    selected_saved_item = -1;\n    if (music_pending == 0) {\n        main_menu_play_title_music();\n    }\n    return 1;\n'),
 (0x49e170, 145, "forgets the cached profile slot, maps the widget's committed selection (int16 +0x3c) to its ui list id (-1 when out of range) and reads the widget list (+0x44) at that id (index -1 included, as the binary does). -1: sound 4, returns 0. Negative: selects that saved item, returns 1. Otherwise raises quit confirm error 0x1f when none is up, sound 4, returns 0.",
  ['profile_slot_lookup_cache_00692ac8', 'ui_list_current', 'ui_lists', 'saved_item_select', 'quit_confirm_error', 'widget_play_sound_effect'],
  '    int32_t id = list_item_id(*(int16_t *)((uint8_t *)widget + 0x3c));\n    int32_t item;\n\n'
  '    profile_slot_lookup_cache_00692ac8 = -1;\n    item = ((int32_t *)widget->list_items)[id];\n'
  '    if (item != -1) {\n        if (item < 0) {\n            saved_item_select(item);\n            return 1;\n        }\n'
  '        if (quit_confirm_error_string_index == -1) {\n' + QUIT_ERR.replace('        ', '            ') % '0x1f' + '        }\n    }\n'
  '    widget_play_sound_effect(4);\n    return 0;\n', LIST_ID),
 (0x49e220, 131, "for a selected variant (low nibble 1): the parent's selection 0..4 maps to game type 1 (also setting variant +0x34 = 1), 4, 2, 3, 5 (any other keeps +0x30); a changed game type (+0x30) zeroes the 0x18 bytes at +0x7c. Returns 1, or 0 without a variant.",
  ['selected_saved_item', 'saved_item_working_copy'],
  VARIANT + '    int32_t type;\n\n    if (variant == 0) {\n        return 0;\n    }\n    switch (widget->parent->selection_index) {\n'
  '    case 0:\n        type = 1;\n        variant[0x34] = 1;\n        break;\n    case 1:\n        type = 4;\n        break;\n'
  '    case 2:\n        type = 2;\n        break;\n    case 3:\n        type = 3;\n        break;\n    case 4:\n        type = 5;\n        break;\n'
  '    default:\n        type = *(int32_t *)(variant + 0x30);\n        break;\n    }\n'
  '    if (type != *(int32_t *)(variant + 0x30)) {\n        memset(variant + 0x7c, 0, 0x18);\n    }\n'
  '    *(int32_t *)(variant + 0x30) = type;\n    return 1;\n'),
 (0x49e2c0, 64, 'for a selected variant, opens the virtual keyboard on its name (working copy +0, 0x30 characters, field kind 9) and on success sets the edit field (0x00719410) to 2; returns 1 either way, 0 without a variant.',
  ['selected_saved_item', 'saved_item_working_copy', 'virtual_keyboard_open', 'network_host_edit_field_00719410'],
  VARIANT + '\n    if (variant == 0) {\n        return 0;\n    }\n    if (virtual_keyboard_open((uint16_t *)variant, 0x30, 9) != 0) {\n'
  '        network_host_edit_field_00719410 = 2;\n    }\n    return 1;\n'),
 (0x49f560, 97, "for a selected variant, maps its game type (+0x30) 1..5 to selection 0, 2, 3, 1, 4 (0 otherwise), then focuses that child (walking next siblings, stopping at the end); returns 1, or 0 without a variant.",
  ['selected_saved_item', 'saved_item_working_copy'],
  VARIANT + '    widget_instance *child;\n    int32_t i;\n\n    if (variant == 0) {\n        return 0;\n    }\n'
  '    switch (*(int32_t *)(variant + 0x30)) {\n    case 2:\n        widget->selection_index = 2;\n        break;\n'
  '    case 3:\n        widget->selection_index = 3;\n        break;\n    case 4:\n        widget->selection_index = 1;\n        break;\n'
  '    case 5:\n        widget->selection_index = 4;\n        break;\n    default:\n        widget->selection_index = 0;\n        break;\n    }\n'
  '    child = widget->first_child;\n    for (i = 0; i < widget->selection_index && child != 0; i++) {\n        child = child->next_sibling;\n    }\n'
  '    widget->focused_child = child;\n    return 1;\n'),
 (0x49f610, 99, "for a selected variant, grows the widget text block (+0x3c) to 0x100 bytes in the widget pool and copies up to 0x7f characters of the variant name into it, terminated at [0x7f]; returns 1, or 0 without a variant.",
  ['selected_saved_item', 'saved_item_working_copy', 'widget_memory_pool', 'heap_reallocate'],
  VARIANT + '    uint16_t *text;\n\n    if (variant == 0) {\n        return 0;\n    }\n'
  '    text = (uint16_t *)heap_reallocate(widget->text, 0x100, widget_memory_pool);\n    widget->text = text;\n'
  '    if (text != 0) {\n        wcsncpy((wchar_t *)text, (const wchar_t *)variant, 0x7f);\n        text[0x7f] = 0;\n    }\n    return 1;\n'),
 (0x4a07e0, 127, "with unsaved changes: a profile or variant selection with bit 30 set whose name did not change starts a name edit and returns 0; any other case returns the save result (0x495d40, a tail jump). Without changes: no selected item, closes the root widget, *out_handled = 1, returns 0.",
  ['saved_item_has_unsaved_changes', 'selected_saved_item', 'saved_item_name_changed', 'saved_item_name_edit_begin',
   'player_profile_save', 'widget_close'],
  '    widget_instance *root;\n\n    if (saved_item_has_unsaved_changes() != 0) {\n        int32_t item = selected_saved_item;\n\n'
  '        if (item != -1 && (item & 0xf) <= 1 && ((item >> 30) & 1) != 0 && (uint8_t)saved_item_name_changed() == 0) {\n'
  '            saved_item_name_edit_begin();\n            return 0;\n        }\n        return player_profile_save();\n    }\n'
  '    selected_saved_item = -1;\n' + ROOT_CLOSE + '    return 0;\n'),
 (0x4a0a80, 91, "for a selected profile, stores the ui list id of the widget's committed selection (+0x3c; -1 when out of range) as the profile word +0x11a and returns 1; 0 without a profile.",
  ['selected_saved_item', 'saved_item_working_copy', 'ui_list_current', 'ui_lists'],
  PROFILE + '    int32_t id = list_item_id(*(int16_t *)((uint8_t *)widget + 0x3c));\n\n    if (profile == 0) {\n        return 0;\n    }\n'
  '    *(int16_t *)(profile + 0x11a) = (int16_t)id;\n    return 1;\n', LIST_ID),
 (0x4a0ae0, 100, "forgets the cached profile slot and reads the first child's list (+0x44) at its selection (+0x40). -1: sound 4, returns 0. Negative: selects that saved item, returns 1. Otherwise raises quit confirm error 0x1f when none is up, sound 4, returns 0.",
  ['profile_slot_lookup_cache_00692ac8', 'saved_item_select', 'quit_confirm_error', 'widget_play_sound_effect'],
  '    widget_instance *list = widget->first_child;\n    int32_t item = ((int32_t *)list->list_items)[list->selection_index];\n\n'
  '    profile_slot_lookup_cache_00692ac8 = -1;\n'
  '    if (item != -1) {\n        if (item < 0) {\n            saved_item_select(item);\n            return 1;\n        }\n'
  '        if (quit_confirm_error_string_index == -1) {\n' + QUIT_ERR.replace('        ', '            ') % '0x1f' + '        }\n    }\n'
  '    widget_play_sound_effect(4);\n    return 0;\n'),
 (0x4a0c00, 83, 'sound 2; with unsaved changes returns the save result when nonzero; otherwise (or unsaved and the save failed) no selected item, closes the root widget, *out_handled = 1, returns 0.',
  ['widget_play_sound_effect', 'saved_item_has_unsaved_changes', 'player_profile_save', 'selected_saved_item', 'widget_close'],
  '    widget_instance *root;\n\n    widget_play_sound_effect(2);\n    if (saved_item_has_unsaved_changes() != 0) {\n'
  '        uint8_t saved = player_profile_save();\n\n        if (saved != 0) {\n            return saved;\n        }\n    }\n'
  '    selected_saved_item = -1;\n' + ROOT_CLOSE + '    return 0;\n'),
 (0x4a1180, 93, "maps the second child's committed selection (+0x3c) to its ui list id; when valid, caches that child's list (+0x44) entry at the id as the profile slot and returns 1 unless it is -1; otherwise sound 4 and 0.",
  ['ui_list_current', 'ui_lists', 'profile_slot_lookup_cache_00692ac8', 'widget_play_sound_effect'],
  '    widget_instance *list = widget->first_child->next_sibling;\n    int32_t id = list_item_id(*(int16_t *)((uint8_t *)list + 0x3c));\n\n'
  '    if (id != -1) {\n        profile_slot_lookup_cache_00692ac8 = ((int32_t *)list->list_items)[id];\n'
  '        if (profile_slot_lookup_cache_00692ac8 != -1) {\n            return 1;\n        }\n    }\n'
  '    widget_play_sound_effect(4);\n    return 0;\n', LIST_ID),
 (0x4a11e0, 153, "caches the widget list (+0x44) entry at the ui list id of its committed selection (index -1 when out of range) as the profile slot. -1: sound 4, returns 0. Bit 30 set: sound 4, raises quit confirm error 0x1a when none is up, returns 0. Otherwise 1.",
  ['ui_list_current', 'ui_lists', 'profile_slot_lookup_cache_00692ac8', 'widget_play_sound_effect', 'quit_confirm_error'],
  '    int32_t id = list_item_id(*(int16_t *)((uint8_t *)widget + 0x3c));\n    int32_t item = ((int32_t *)widget->list_items)[id];\n\n'
  '    profile_slot_lookup_cache_00692ac8 = item;\n    if (item == -1) {\n        widget_play_sound_effect(4);\n        return 0;\n    }\n'
  '    if ((item & 0x40000000) == 0) {\n        return 1;\n    }\n    widget_play_sound_effect(4);\n'
  '    if (quit_confirm_error_string_index == -1) {\n' + QUIT_ERR % '0x1a' + '    }\n    return 0;\n', LIST_ID),
 (0x4a1280, 64, 'for a cached profile slot without bit 30 and with low nibble 0: deletes that saved game (unless -1); when the cached slot is the current profile handle (0x00714dd4, read before the delete) picks a profile automatically; returns 1. Otherwise 0.',
  ['profile_slot_lookup_cache_00692ac8', 'saved_player_profile_slots_handle', 'saved_game_delete_by_handle', 'player_profile_auto_select'],
  '    int32_t handle = profile_slot_lookup_cache_00692ac8;\n    int32_t current;\n\n'
  '    if ((handle & 0x40000000) != 0 || (handle & 0xf) != 0) {\n        return 0;\n    }\n'
  '    current = saved_player_profile_slots_handle;\n    if (handle != -1) {\n        saved_game_delete_by_handle(handle);\n'
  '        handle = profile_slot_lookup_cache_00692ac8;\n    }\n    if (handle == current) {\n        player_profile_auto_select();\n    }\n    return 1;\n'),
 (0x4a1570, 103, "with a client, finds among its 16 player entries (0x20 each from +0xcb6) a valid one whose machine byte (+0x1c) matches the client's word +0 and player byte (+0x1d) matches event word 1, and commits the staged message with AX = 1; returns 1.",
  ['network_client', 'network_player_entry_validate', 'network_staged_message_commit'],
  '    uint8_t *client = network_client;\n    int32_t i;\n\n    if (client == 0) {\n        return 1;\n    }\n'
  '    for (i = 0; i < 0x10; i++) {\n        uint8_t *entry = client + 0xcb6 + i * 0x20;\n\n'
  '        if (network_player_entry_validate(entry) != 0 && (int16_t)(int8_t)entry[0x1c] == *(int16_t *)client &&\n'
  '            (int16_t)(int8_t)entry[0x1d] == event[1]) {\n            network_staged_message_commit(client, 1);\n            return 1;\n        }\n    }\n    return 1;\n'),
 (0x4a15e0, 100, 'the same search as 0x4a1570, committing the staged message with AX = 0; returns 1.',
  ['network_client', 'network_player_entry_validate', 'network_staged_message_commit'],
  '    uint8_t *client = network_client;\n    int32_t i;\n\n    if (client == 0) {\n        return 1;\n    }\n'
  '    for (i = 0; i < 0x10; i++) {\n        uint8_t *entry = client + 0xcb6 + i * 0x20;\n\n'
  '        if (network_player_entry_validate(entry) != 0 && (int16_t)(int8_t)entry[0x1c] == *(int16_t *)client &&\n'
  '            (int16_t)(int8_t)entry[0x1d] == event[1]) {\n            network_staged_message_commit(client, 0);\n            return 1;\n        }\n    }\n    return 1;\n'),
 (0x4a1740, 80, "for a widget with no items (+0x48) and a client: a client state word (+0xeda) of 1 reads the performance counter (result unused); a state of 0 returns the host session start result (0x49d210, which ignores the three pushed arguments); otherwise 0.",
  ['network_client', 'time_query_performance_counter_ms', 'multiplayer_host_session_start'],
  '    int16_t *state;\n\n    if (widget->item_count != 0 || network_client == 0) {\n        return 0;\n    }\n'
  '    state = (int16_t *)(network_client + 0xeda);\n    if (*state == 1) {\n        time_query_performance_counter_ms();\n    }\n'
  '    if (*state != 0) {\n        return 0;\n    }\n    return multiplayer_host_session_start();\n'),
 (0x4a1b00, 82, "pops the head of the widget history for the widget's controller (-1 means 0) and frees that node's widget pool block, updating the pool byte and allocation counts; returns 1.",
  ['ui_widget_history', 'widget_memory_pool', 'heap_unlink_block'],
  '    int16_t controller = widget->controller_index;\n    widget_history_node *node;\n\n    if (controller == -1) {\n        controller = 0;\n    }\n'
  '    node = ui_widget_history[controller];\n    if (node != 0) {\n        heap_block *block = (heap_block *)((uint8_t *)node - 0x10);\n'
  '        uint32_t size = block->size & 0x7fffffff;\n\n        ui_widget_history[controller] = node->next;\n'
  '        heap_unlink_block(block, widget_memory_pool);\n        widget_memory_pool->bytes_allocated -= (int32_t)size;\n'
  '        widget_memory_pool->allocation_count -= 1;\n    }\n    return 1;\n'),
 (0x4a1b60, 130, "when 0x0071916b is 1 and the level select path (0x00719068) matches 0x00719779 case-insensitively, selects and focuses child number level_select_frame (0x00719168; also the committed selection +0x3c); otherwise selects and focuses child 1. Returns 1.",
  ['level_select_flags_0071916b', '__stricmp', 'level_select_current_path_00719068', 'unknown_00719779', 'level_select_frame_00719168'],
  '    widget_instance *child = widget->first_child;\n    int16_t selection = 1;\n    int32_t i;\n\n'
  '    if (level_select_flags_0071916b == 1 && __stricmp(level_select_current_path_00719068, unknown_00719779) == 0) {\n'
  '        selection = level_select_frame_00719168;\n    }\n'
  '    for (i = 0; i < selection && child != 0; i++) {\n        child = child->next_sibling;\n    }\n'
  '    widget->selection_index = selection;\n    widget->focused_child = child;\n'
  '    *(int16_t *)((uint8_t *)widget + 0x3c) = widget->selection_index;\n    return 1;\n'),
 (0x4a2950, 70, "fills a local 0x1ffc byte profile with the default audio options; on success fills the grandparent as a controls input row from it and plays sound 2. Returns the fill result. (The binary leaves the rest of the local uninitialized; zeroed here.)",
  ['player_profile_set_default_audio_options', 'ui_controls_populate_input_row', 'widget_play_sound_effect'],
  '    uint8_t profile[0x1ffc];\n    uint8_t ok;\n\n    memset(profile, 0, sizeof(profile));\n'
  '    ok = player_profile_set_default_audio_options(profile);\n    if (ok != 0) {\n'
  '        ui_controls_populate_input_row(widget->parent->parent, profile);\n        widget_play_sound_effect(2);\n    }\n    return ok;\n'),
 (0x4a3000, 68, "the second child is shown at scale 1.0 when network_game_mode is 2 or the game engine has teams, else hidden at scale 0x3eaa7efa (about 1/3); returns 1.",
  ['network_game_mode', 'current_game_engine', 'game_engine_teams_enabled_flag'],
  '    widget_instance *child = widget->first_child->next_sibling;\n\n'
  '    if (network_game_mode == 2 || (current_game_engine != 0 && game_engine_teams_enabled_flag != 0)) {\n'
  '        child->hidden = 0;\n        child->scale = 1.0f;\n    } else {\n        child->hidden = 1;\n        *(uint32_t *)&child->scale = 0x3eaa7efa;\n    }\n    return 1;\n'),
 (0x4a3050, 141, "the first child is shown at scale 1.0 when the game engine has teams, else hidden at 0x3eaa7efa and its parent focuses its own second child; the next two children are shown at 1.0 when network_game_mode is 2, else hidden at 0x3eaa7efa. Returns 1.",
  ['current_game_engine', 'game_engine_teams_enabled_flag', 'network_game_mode'],
  '    widget_instance *child = widget->first_child;\n    int32_t i;\n\n'
  '    if (current_game_engine != 0 && game_engine_teams_enabled_flag != 0) {\n        child->hidden = 0;\n        child->scale = 1.0f;\n    } else {\n'
  '        child->hidden = 1;\n        *(uint32_t *)&child->scale = 0x3eaa7efa;\n'
  '        child->parent->focused_child = child->parent->first_child->next_sibling;\n    }\n'
  '    for (i = 0; i < 2; i++) {\n        child = child->next_sibling;\n        if (network_game_mode == 2) {\n'
  '            child->hidden = 0;\n            child->scale = 1.0f;\n        } else {\n            child->hidden = 1;\n'
  '            *(uint32_t *)&child->scale = 0x3eaa7efa;\n        }\n    }\n    return 1;\n'),
]
generate(SPECS)
