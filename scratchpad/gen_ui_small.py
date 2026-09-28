import os, textwrap
os.chdir(r'C:\Users\Liam-\halo-re')

EXT = {
    'level_select_entries': 'extern uint8_t level_select_entries[0x50]; // 0x00719018',
    'pending_difficulty': 'extern int16_t pending_difficulty; // 0x00696564',
    'local_player_count': 'extern int16_t local_player_count; // 0x006894b8',
    'save_in_progress_00719010': 'extern uint8_t save_in_progress_00719010; // 0x00719010',
    'main_globals_byte_0071973c': 'extern uint8_t main_globals_byte_0071973c; // 0x0071973c',
    'main_globals_byte_0071974f': 'extern uint8_t main_globals_byte_0071974f; // 0x0071974f',
    'main_globals_byte_0071973a': 'extern uint8_t main_globals_byte_0071973a; // 0x0071973a',
    'unknown_00719738': 'extern uint8_t unknown_00719738; // 0x00719738, UNSURE',
    'split_screen_quit_prompt_string': 'extern uint16_t split_screen_quit_prompt_string; // 0x00719754, word stores',
    'split_screen_quit_prompt_armed': 'extern uint8_t split_screen_quit_prompt_armed; // 0x00719757',
    'network_server_pointer': 'extern void *network_server_pointer; // 0x0071c2d4 (network_server_globals *)',
    'network_client': 'extern uint8_t *network_client; // 0x0071c2d8 (network_client_globals *)',
    'unknown_0071c2dd': 'extern uint8_t unknown_0071c2dd; // 0x0071c2dd, UNSURE identity',
    'local_team_00714dd8': 'extern uint8_t local_team_00714dd8; // 0x00714dd8, TYPES-GAP',
    'coop_profile_globals_block_00714ddc': 'extern uint8_t coop_profile_globals_block_00714ddc[0x1ffc]; // 0x00714ddc, TYPES-GAP',
    'profile_slot_lookup_cache_00692ac8': 'extern int32_t profile_slot_lookup_cache_00692ac8; // 0x00692ac8, TYPES-GAP',
    'selected_saved_item': 'extern int32_t selected_saved_item; // 0x00714e7c, low nibble: 0 profile, 1 variant',
    'saved_item_working_copy': 'extern uint8_t saved_item_working_copy[0x1ffc]; // 0x00714e80, the record itself',
    'pending_delete_saved_game_name_00718fd0': 'extern char pending_delete_saved_game_name_00718fd0[]; // 0x00718fd0, UNSURE name',
    'profile_slot_id': 'extern int16_t profile_slot_id[]; // 0x00714dde',
    'main_menu_music_pending': 'extern uint8_t main_menu_music_pending; // 0x00718fc6',
    'network_host_name_00719170': 'extern uint16_t network_host_name_00719170[0x40]; // 0x00719170',
    'network_host_subname_007191f0': 'extern uint16_t network_host_subname_007191f0[9]; // 0x007191f0',
    'network_host_edit_field_00719410': 'extern int32_t network_host_edit_field_00719410; // 0x00719410, UNSURE name (3 after the name, 0 after the subname)',
    'unknown_00879f34': 'extern uint32_t unknown_00879f34; // 0x00879f34, UNSURE',
    'unknown_00879f38': 'extern uint32_t unknown_00879f38; // 0x00879f38, UNSURE',
    'unknown_00719208': 'extern uint32_t unknown_00719208; // 0x00719208, UNSURE',
    'autopatch_status_state_00719234': 'extern uint8_t autopatch_status_state_00719234; // 0x00719234, TYPES-GAP',
    'joystick_slot_devices': 'extern int32_t joystick_slot_devices[4]; // 0x006b2ce8, input.h',
    'ui_flag_00719444': 'extern uint8_t ui_flag_00719444; // 0x00719444, UNSURE (only ever set to 1 here)',
    'ui_flag_007196d2': 'extern uint8_t ui_flag_007196d2; // 0x007196d2, UNSURE (only ever set to 1 here)',
    # functions
    'ui_list_free_all': 'extern void ui_list_free_all(void); // 0x4a7b20',
    'widget_play_sound_effect': 'extern void widget_play_sound_effect(int16_t effect_id); // 0x498e90, blam-cc: AX effect_id',
    'ui_start_campaign_from_level_one': 'extern uint32_t ui_start_campaign_from_level_one(void *widget, int16_t *event); // 0x49cfd0',
    'network_client_globals_dispose': 'extern void network_client_globals_dispose(void); // 0x4dde70',
    'network_game_server_host_dispose': 'extern void network_game_server_host_dispose(void *host); // 0x4deda0',
    'network_game_setup_teardown': 'extern void network_game_setup_teardown(void); // 0x495520',
    'network_dispatch_initialize': 'extern void network_dispatch_initialize(void); // 0x4414c0',
    'game_engine_ensure_variant_history_has_entry': 'extern uint32_t game_engine_ensure_variant_history_has_entry(void); // 0x463b20',
    'game_engine_apply_current_custom_variant': 'extern void game_engine_apply_current_custom_variant(void); // 0x463b90',
    'virtual_keyboard_open': 'extern uint8_t virtual_keyboard_open(uint16_t *destination, uint16_t maximum_length, int16_t field_kind); // 0x4a89a0, blam-cc: ESI destination',
    'network_client_rejoin_check': 'extern void network_client_rejoin_check(int8_t machine_player_index); // 0x4de390',
    'saved_game_delete_by_handle': 'extern uint8_t saved_game_delete_by_handle(int32_t handle); // 0x53c960, blam-cc: EDI handle',
    'display_error': 'extern void display_error(int16_t error_string_index, int32_t player_index, uint8_t modal, uint8_t is_error); // 0x498f20',
    'tag_lookup': 'extern datum_index tag_lookup(tag_group group, char *path); // 0x442550, blam-cc: EDI group',
    'sound_looping_stop': 'extern void sound_looping_stop(datum_index looping_definition); // 0x544120, blam-cc: EAX',
    'ui_controls_populate_sensitivity_row': 'extern void ui_controls_populate_sensitivity_row(widget_instance *widget, const uint8_t *profile_record); // 0x4a20f0, blam-cc: EAX widget, ESI profile_record',
    'ui_controls_populate_input_row': 'extern void ui_controls_populate_input_row(widget_instance *widget, const uint8_t *profile_record); // 0x4a22e0, blam-cc: EAX widget, EDI profile_record',
    'game_engine_send_team_allegiance_message': 'extern void game_engine_send_team_allegiance_message(char broadcast); // 0x4704d0',
    'ui_network_game_options_populate': 'extern uint8_t ui_network_game_options_populate(widget_instance *widget, const uint8_t *options_record); // 0x4a3960, blam-cc: ECX widget, ESI options_record',
    'autopatch_launch_updater': 'extern uint8_t autopatch_launch_updater(void); // 0x577310',
    'saved_game_delete_files': 'extern uint8_t saved_game_delete_files(char *name); // 0x5388c0, blam-cc: EDI name',
    'game_checkpoint_save_new': 'extern uint8_t game_checkpoint_save_new(void); // 0x538db0',
    'controls_binding_rows_toggle_device_mode': 'extern void controls_binding_rows_toggle_device_mode(widget_instance *widget, uint8_t mode); // 0x4b53a0, blam-cc: ESI widget',
}

PROFILE = '    const uint8_t *profile = (selected_saved_item & 0xf) == 0 ? saved_item_working_copy : 0;\n\n'

# (addr, size, table index, note, externs, body)
EV = [
 (0x49cdd0, 37, 7, 'clears the ten level select entries (0x50 bytes at 0x00719018), empties the widget list (+0x44, +0x48), frees every ui list; returns 1.',
  ['level_select_entries', 'ui_list_free_all'],
  '    memset(level_select_entries, 0, 0x50);\n    widget->list_items = 0;\n    widget->item_count = 0;\n    ui_list_free_all();\n    return 1;\n'),
 (0x49cfa0, 44, 9, "reads the grandparent list's committed selection (int16 at +0x3c); below 4 it becomes the pending difficulty (when not negative) and sound effect 2 plays; returns 1.",
  ['pending_difficulty', 'widget_play_sound_effect'],
  '    int16_t selection = *(int16_t *)((uint8_t *)widget->parent->parent + 0x3c);\n\n'
  '    if (selection < 4) {\n        if (selection >= 0) {\n            pending_difficulty = selection;\n        }\n'
  '        widget_play_sound_effect(2);\n    }\n    return 1;\n'),
 (0x49d0d0, 42, 104, 'one local player, marks 0x00719010, then starts the campaign from level one with the same arguments (0x49cfd0); returns 1.',
  ['local_player_count', 'save_in_progress_00719010', 'ui_start_campaign_from_level_one'],
  '    local_player_count = 1;\n    save_in_progress_00719010 = 1;\n    ui_start_campaign_from_level_one(widget, event);\n    return 1;\n'),
 (0x49d100, 31, 11, 'clears 0x0071973c and 0x0071974f, resets the split screen quit prompt string to -1, sets 0x0071973a; returns 1.',
  ['main_globals_byte_0071973c', 'main_globals_byte_0071974f', 'split_screen_quit_prompt_string', 'main_globals_byte_0071973a'],
  '    main_globals_byte_0071973c = 0;\n    main_globals_byte_0071974f = 0;\n    split_screen_quit_prompt_string = 0xffff;\n    main_globals_byte_0071973a = 1;\n    return 1;\n'),
 (0x49d120, 31, 12, 'clears 0x0071973c and 0x0071974f, resets the split screen quit prompt string to -1, sets 0x00719738; returns 1.',
  ['main_globals_byte_0071973c', 'main_globals_byte_0071974f', 'split_screen_quit_prompt_string', 'unknown_00719738'],
  '    main_globals_byte_0071973c = 0;\n    main_globals_byte_0071974f = 0;\n    split_screen_quit_prompt_string = 0xffff;\n    unknown_00719738 = 1;\n    return 1;\n'),
 (0x49d140, 24, 13, 'resets the split screen quit prompt string to -1, clears 0x0071973c, arms the quit prompt (0x00719757); returns 1.',
  ['split_screen_quit_prompt_string', 'main_globals_byte_0071973c', 'split_screen_quit_prompt_armed'],
  '    split_screen_quit_prompt_string = 0xffff;\n    main_globals_byte_0071973c = 0;\n    split_screen_quit_prompt_armed = 1;\n    return 1;\n'),
 (0x49d160, 59, 14, 'disposes the network client globals, then any server host (clearing the pointer and 0x0071c2dd), clears 0x00714dd8 and the first byte of 0x00714ddc, tears down the game setup widget; returns 1.',
  ['network_client_globals_dispose', 'network_server_pointer', 'network_game_server_host_dispose', 'unknown_0071c2dd',
   'local_team_00714dd8', 'coop_profile_globals_block_00714ddc', 'network_game_setup_teardown'],
  '    network_client_globals_dispose();\n    if (network_server_pointer != 0) {\n        network_game_server_host_dispose(network_server_pointer);\n'
  '        network_server_pointer = 0;\n        unknown_0071c2dd = 0;\n    }\n    local_team_00714dd8 = 0;\n    coop_profile_globals_block_00714ddc[0] = 0;\n'
  '    network_game_setup_teardown();\n    return 1;\n'),
 (0x49d1a0, 8, 15, 'tears down the multiplayer game setup widget; returns 1.', ['network_game_setup_teardown'],
  '    network_game_setup_teardown();\n    return 1;\n'),
 (0x49d440, 16, 18, 'empties the widget list (+0x44 = 0, +0x48 = 0); returns 1.', [],
  '    widget->list_items = 0;\n    widget->item_count = 0;\n    return 1;\n'),
 (0x49d450, 48, 19, 'disposes any server host (clearing the pointer and 0x0071c2dd), then the network client globals, tears down the game setup widget; returns 1.',
  ['network_server_pointer', 'network_game_server_host_dispose', 'unknown_0071c2dd', 'network_client_globals_dispose', 'network_game_setup_teardown'],
  '    if (network_server_pointer != 0) {\n        network_game_server_host_dispose(network_server_pointer);\n'
  '        network_server_pointer = 0;\n        unknown_0071c2dd = 0;\n    }\n    network_client_globals_dispose();\n'
  '    network_game_setup_teardown();\n    return 1;\n'),
 (0x49d520, 17, 22, 'one local player and marks 0x00719010; returns 1 (AL from the stored EAX = 1).',
  ['local_player_count', 'save_in_progress_00719010'],
  '    local_player_count = 1;\n    save_in_progress_00719010 = 1;\n    return 1;\n'),
 (0x49d5b0, 24, 24, 'one local player, clears 0x00719010, initializes network dispatch; returns 1.',
  ['local_player_count', 'save_in_progress_00719010', 'network_dispatch_initialize'],
  '    local_player_count = 1;\n    save_in_progress_00719010 = 0;\n    network_dispatch_initialize();\n    return 1;\n'),
 (0x49d5d0, 18, 25, 'makes sure the variant history has an entry, applies the current custom variant, sets 0x0071c2dd; returns 1.',
  ['game_engine_ensure_variant_history_has_entry', 'game_engine_apply_current_custom_variant', 'unknown_0071c2dd'],
  '    game_engine_ensure_variant_history_has_entry();\n    game_engine_apply_current_custom_variant();\n    unknown_0071c2dd = 1;\n    return 1;\n'),
 (0x49d7a0, 21, 27, 'empties the widget list (+0x44, +0x48) and frees every ui list; returns 1.', ['ui_list_free_all'],
  '    widget->list_items = 0;\n    widget->item_count = 0;\n    ui_list_free_all();\n    return 1;\n'),
 (0x49e210, 16, 39, 'forgets the cached profile slot (0x00692ac8) and the selected saved item (both -1); returns 1.',
  ['profile_slot_lookup_cache_00692ac8', 'selected_saved_item'],
  '    profile_slot_lookup_cache_00692ac8 = -1;\n    selected_saved_item = -1;\n    return 1;\n'),
 (0x4a0bc0, 54, 66, 'when the selected saved item is a profile (low nibble 0), opens the virtual keyboard on its name (working copy +2, 0x18 characters, field kind 8); returns whether the keyboard opened (0 when not a profile).',
  ['selected_saved_item', 'saved_item_working_copy', 'virtual_keyboard_open'],
  PROFILE + '    if (profile == 0 || virtual_keyboard_open((uint16_t *)(profile + 2), 0x18, 8) == 0) {\n        return 0;\n    }\n    return 1;\n'),
 (0x4a10f0, 22, 72, 'runs the network client rejoin check (0x4de390) for event word 1; returns 1. The binary passes the zero-extended word and the callee compares it as a dword against sign-extended bytes; the C callee takes int8_t, so values 0x80..0xffff differ (never seen: the word is a machine index).',
  ['network_client_rejoin_check'],
  '    network_client_rejoin_check((int8_t)event[1]);\n    return 1;\n'),
 (0x4a12c0, 35, 77, 'when the cached profile slot (0x00692ac8) is a profile entry (low nibble 1), deletes that saved game (unless -1) and returns 1; otherwise 0.',
  ['profile_slot_lookup_cache_00692ac8', 'saved_game_delete_by_handle'],
  '    int32_t handle = profile_slot_lookup_cache_00692ac8;\n\n    if ((handle & 0xf) != 1) {\n        return 0;\n    }\n'
  '    if (handle != -1) {\n        saved_game_delete_by_handle(handle);\n    }\n    return 1;\n'),
 (0x4a12f0, 20, 78, 'forgets the cached profile slot (-1) and the pending delete name (first byte of 0x00718fd0); returns 1.',
  ['profile_slot_lookup_cache_00692ac8', 'pending_delete_saved_game_name_00718fd0'],
  '    profile_slot_lookup_cache_00692ac8 = -1;\n    pending_delete_saved_game_name_00718fd0[0] = 0;\n    return 1;\n'),
 (0x4a1650, 31, 83, 'with a server up, sets bit 0 of its word +0x06 and byte +0xae0 of the record its first dword points at; returns 1.',
  ['network_server_pointer'],
  '    uint8_t *server = (uint8_t *)network_server_pointer;\n\n    if (server != 0) {\n        *(uint16_t *)(server + 6) |= 1;\n'
  '        (*(uint8_t **)server)[0xae0] = 1;\n    }\n    return 1;\n'),
 (0x4a16a0, 19, 85, 'with a server up, clears its byte +0x9d5; returns 1.', ['network_server_pointer'],
  '    if (network_server_pointer != 0) {\n        ((uint8_t *)network_server_pointer)[0x9d5] = 0;\n    }\n    return 1;\n'),
 (0x4a16c0, 15, 86, 'hides the widget (+0x12 = 1) and zeroes its state (+0x10); returns 1.', [],
  '    widget->hidden = 1;\n    widget->state = 0;\n    return 1;\n'),
 (0x4a16d0, 12, 88, 'profile_slot_id[0] = -1; returns 1.', ['profile_slot_id'],
  '    profile_slot_id[0] = -1;\n    return 1;\n'),
 (0x4a16e0, 18, 89, 'profile_slot_id[0] = event word 1; returns 1.', ['profile_slot_id'],
  '    profile_slot_id[0] = event[1];\n    return 1;\n'),
 (0x4a1700, 52, 90, 'event word 1 equal to profile_slot_id[0] shows error 0x12 (player -1, modal, not an error), sets *out_handled and returns 0; otherwise stores it in profile_slot_id[1] and returns 1.',
  ['profile_slot_id', 'display_error'],
  '    if (event[1] == profile_slot_id[0]) {\n        display_error(0x12, -1, 1, 0);\n        *out_handled = 1;\n        return 0;\n    }\n'
  '    profile_slot_id[1] = event[1];\n    return 1;\n'),
 (0x4a1900, 61, 94, 'when the selected saved item is neither a profile (0) nor a variant (1), finds the root widget and closes it (+0x1c auto close = 1 ms, +0x10 state = 0) and returns 0; otherwise returns 1.',
  ['selected_saved_item'],
  '    widget_instance *root = widget;\n    int32_t kind = selected_saved_item & 0xf;\n\n    if (kind == 0 || kind == 1) {\n        return 1;\n    }\n'
  '    while (root->parent != 0) {\n        root = root->parent;\n    }\n    root->milliseconds_to_auto_close = 1;\n    root->state = 0;\n    return 0;\n'),
 (0x4a1bf0, 53, 100, 'when the main menu music is pending (exactly 1), stops the looping sound sound\\\\music\\\\title1\\\\title1 (tag_lookup lsnd, path at 0x0066a044) if it is loaded and clears the flag; returns 1.',
  ['main_menu_music_pending', 'tag_lookup', 'sound_looping_stop'],
  '    if (main_menu_music_pending == 1) {\n'
  '        datum_index music = tag_lookup(0x6c736e64, "sound\\\\music\\\\title1\\\\title1"); // \'lsnd\', string at 0x0066a044\n\n'
  '        if (music != 0xffffffff) {\n            sound_looping_stop(music);\n        }\n        main_menu_music_pending = 0;\n    }\n    return 1;\n'),
 (0x4a2190, 45, 121, 'when the selected saved item is a profile, fills the widget as a controls sensitivity row from it and returns 1; otherwise 0.',
  ['selected_saved_item', 'saved_item_working_copy', 'ui_controls_populate_sensitivity_row'],
  PROFILE + '    if (profile == 0) {\n        return 0;\n    }\n    ui_controls_populate_sensitivity_row(widget, profile);\n    return 1;\n'),
 (0x4a2490, 45, 123, 'when the selected saved item is a profile, fills the widget as a controls input row from it and returns 1; otherwise 0.',
  ['selected_saved_item', 'saved_item_working_copy', 'ui_controls_populate_input_row'],
  PROFILE + '    if (profile == 0) {\n        return 0;\n    }\n    ui_controls_populate_input_row(widget, profile);\n    return 1;\n'),
 (0x4a2c50, 46, 142, 'opens the virtual keyboard on the network host name (0x00719170, 0x80 characters, field kind 0xb); if it opened, the edit field (0x00719410) becomes 3 and returns 1, else 0.',
  ['network_host_name_00719170', 'network_host_edit_field_00719410', 'virtual_keyboard_open'],
  '    if (virtual_keyboard_open(network_host_name_00719170, 0x80, 0xb) == 0) {\n        return 0;\n    }\n'
  '    network_host_edit_field_00719410 = 3;\n    return 1;\n'),
 (0x4a2c80, 43, 143, 'opens the virtual keyboard on the network host subname (0x007191f0, 0x12 characters, field kind 0xc); if it opened, the edit field (0x00719410) becomes 0 and returns 1, else 0.',
  ['network_host_subname_007191f0', 'network_host_edit_field_00719410', 'virtual_keyboard_open'],
  '    if (virtual_keyboard_open(network_host_subname_007191f0, 0x12, 0xc) == 0) {\n        return 0;\n    }\n'
  '    network_host_edit_field_00719410 = 0;\n    return 1;\n'),
 (0x4a3150, 47, 157, "the parent's first child sends the team allegiance message with 1, its second child with 0 (0x4704d0), returning 1; any other widget returns 0.",
  ['game_engine_send_team_allegiance_message'],
  '    widget_instance *first = widget->parent->first_child;\n\n    if (first == widget) {\n        game_engine_send_team_allegiance_message(1);\n        return 1;\n    }\n'
  '    if (first->next_sibling == widget) {\n        game_engine_send_team_allegiance_message(0);\n        return 1;\n    }\n    return 0;\n'),
 (0x4a3510, 48, 159, 'when the selected saved item is a variant (low nibble 1), copies the dwords at 0x00879f34, 0x00879f38 and 0x00719208 into the working copy +0x60..+0x6b (0x00714ee0); returns 1.',
  ['selected_saved_item', 'saved_item_working_copy', 'unknown_00879f34', 'unknown_00879f38', 'unknown_00719208'],
  '    if ((selected_saved_item & 0xf) == 1) {\n        uint32_t *out = (uint32_t *)(saved_item_working_copy + 0x60);\n\n'
  '        out[0] = unknown_00879f34;\n        out[1] = unknown_00879f38;\n        out[2] = unknown_00719208;\n    }\n    return 1;\n'),
 (0x4a39c0, 32, 166, 'passes the widget and the working copy when the selected saved item is a profile (0 otherwise) to 0x4a3960 and returns its result.',
  ['selected_saved_item', 'saved_item_working_copy', 'ui_network_game_options_populate'],
  PROFILE + '    return ui_network_game_options_populate(widget, profile);\n'),
 (0x4a4190, 8, 172, 'launches the autopatch updater; returns 1. Also slot 181.', ['autopatch_launch_updater'],
  '    autopatch_launch_updater();\n    return 1;\n'),
 (0x4a4570, 8, 177, 'frees every ui list; returns 1.', ['ui_list_free_all'],
  '    ui_list_free_all();\n    return 1;\n'),
 (0x4a45d0, 24, 189, 'with a pending delete name (0x00718fd0 non-empty), deletes those saved game files; returns 1.',
  ['pending_delete_saved_game_name_00718fd0', 'saved_game_delete_files'],
  '    if (pending_delete_saved_game_name_00718fd0[0] != 0) {\n        saved_game_delete_files(pending_delete_saved_game_name_00718fd0);\n    }\n    return 1;\n'),
 (0x4a47b0, 8, 179, 'saves a new checkpoint; returns 1.', ['game_checkpoint_save_new'],
  '    game_checkpoint_save_new();\n    return 1;\n'),
 (0x4a4870, 6, 182, 'returns the autopatch status byte (0x00719234).', ['autopatch_status_state_00719234'],
  '    return autopatch_status_state_00719234;\n'),
 (0x4b4c40, 8, 127, 'sets 0x00719444; returns 1.', ['ui_flag_00719444'],
  '    ui_flag_00719444 = 1;\n    return 1;\n'),
 (0x4b54a0, 28, 153, 'switches the controls binding rows of the great-grandparent widget to device mode 1; returns 1.',
  ['controls_binding_rows_toggle_device_mode'],
  '    controls_binding_rows_toggle_device_mode(widget->parent->parent->parent, 1);\n    return 1;\n'),
 (0x4bba80, 8, 147, 'sets 0x007196d2; returns 1.', ['ui_flag_007196d2'],
  '    ui_flag_007196d2 = 1;\n    return 1;\n'),
]

SERVER_OR_CLIENT = ('    uint8_t *game = network_server_pointer != 0 ? (uint8_t *)network_server_pointer + 8\n'
                    '                  : network_client != 0 ? network_client + 0xb14 : 0;\n\n')
GDI = [
 (0x4a4c70, 58, 1, "counts the widget's children up to its focused child (all of them when none is focused; 0 with no children) and stores the count as the background frame of the extended description's first child and the selection of that child's next sibling (skipped when the count is 0xffff).",
  [],
  '    widget_instance *label = widget->extended_description->first_child;\n    widget_instance *list = label->next_sibling;\n'
  '    widget_instance *child = widget->first_child;\n    int16_t index = 0;\n\n'
  '    if (child != 0) {\n        for (; child != 0 && child != widget->focused_child; child = child->next_sibling) {\n            index++;\n        }\n'
  '        if ((uint16_t)index == 0xffff) {\n            return;\n        }\n    }\n'
  '    label->background_bitmap_frame = index;\n    list->selection_index = index;\n'),
 (0x4a6e50, 58, 29, 'takes the server game (server +8) or else the client game (client +0xb14); when there is one, the selection (+0x40) becomes 0xc plus (its byte +0x138 != 1).',
  ['network_server_pointer', 'network_client'],
  SERVER_OR_CLIENT + '    if (game != 0) {\n        widget->selection_index = (int16_t)((game[0x138] != 1) + 0xc);\n    }\n'),
 (0x4a7300, 55, 38, 'takes the server game (server +8) or else the client game (client +0xb14); when there is one, the background frame (+0x58) becomes (its byte +0x138 != 1).',
  ['network_server_pointer', 'network_client'],
  SERVER_OR_CLIENT + '    if (game != 0) {\n        widget->background_bitmap_frame = (int16_t)(game[0x138] != 1);\n    }\n'),
 (0x4a7340, 15, 39, "zeroes the state (+0x10) of the widget's second child.", [],
  '    widget->first_child->next_sibling->state = 0;\n'),
 (0x4a7350, 12, 40, 'scale (+0x24) = 1.0.', [],
  '    widget->scale = 1.0f;\n'),
 (0x4a73d0, 47, 42, 'counts the connected joysticks, which is only slot 0 here (0x006b2ce8 != -1); two or more give scale 1.0, otherwise 0x3eaa7efa (about 1/3). So always 1/3 in retail.',
  ['joystick_slot_devices'],
  '    int32_t count = joystick_slot_devices[0] != -1 ? 1 : 0;\n\n'
  '    if (count >= 2) {\n        widget->scale = 1.0f;\n        return;\n    }\n    *(uint32_t *)&widget->scale = 0x3eaa7efa;\n'),
]

HDR_EV = '''// ui_event_%(a)x  (not a Ghidra function; ui_event_function_table[%(i)d])
// address 0x%(a)x, size %(s)d bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x%(slot)08x (index %(i)d); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_%(a)x.
%(note)s// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL
'''
HDR_GDI = '''// ui_game_data_input_%(a)x  (not a Ghidra function; game_data_input_function_table[%(i)d])
// address 0x%(a)x, size %(s)d bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: game_data_input_function_table 0x00692b18 slot 0x%(slot)08x (index %(i)d); widget_instance_render
//   0x49a8c0 runs it once per frame for each game_data_inputs entry. Only reachable through that table; no C
//   existed, so it trapped as unlisted_%(a)x.
%(note)s// blam-cc: stack -> widget (cdecl)
'''

def note(a, s, text):
    lines = textwrap.wrap('WRITTEN 2026-09-28 from objdump 0x%x..0x%x: %s' % (a, a + s - 1, text), 113)
    return ''.join(('// ' if k == 0 else '//   ') + l + '\n' for k, l in enumerate(lines))

def emit(name, hdr, ext, sig, body, needs_string):
    inc = '#include "tags.h"\n#include "memory.h"\n#include "math.h"\n#include "cache.h"\n#include "interface.h"\n'
    if needs_string:
        inc += '#include <string.h>\n'
    ex = ''.join(EXT[e] + '\n' for e in ext)
    src = hdr + '\n' + inc + ('\n' + ex if ex else '') + '\n' + sig + '\n{\n' + body + '}\n'
    p = 'src/interface/%s.c' % name
    assert not os.path.exists(p), p
    open(p, 'w', encoding='utf-8').write(src)
    print('wrote', p)

for a, s, i, n, ext, body in EV:
    name = 'ui_event_%x' % a
    hdr = HDR_EV % dict(a=a, i=i, s=s, slot=0x6927d0 + 4 * i, note=note(a, s, n))
    emit(name, hdr, ext, 'uint8_t %s(widget_instance *widget, int16_t *event, uint8_t *out_handled)' % name, body, 'memset' in body)
for a, s, i, n, ext, body in GDI:
    name = 'ui_game_data_input_%x' % a
    hdr = HDR_GDI % dict(a=a, i=i, s=s, slot=0x692b18 + 4 * i, note=note(a, s, n))
    emit(name, hdr, ext, 'void %s(widget_instance *widget)' % name, body, False)
print(len(EV) + len(GDI))
