exec(open(r'C:\Users\Liam-\halo-re\scratchpad\uigen_lib.py').read())
for g in ('gen_ui_mid2.py', 'gen_ui_bigA.py', 'gen_ui_bigB.py', 'gen_ui_bigC.py'):
    load_ext(r'C:\Users\Liam-\halo-re\scratchpad\\' + g)

EXT.update({
    'input_controls_live_006b3a48': 'extern uint8_t input_controls_live_006b3a48[0x890]; // 0x006b3a48, UNSURE: the live controls configuration the profile copy mirrors',
    'controls_capture': ('extern uint8_t controls_input_capture_flags; // 0x00712542, UNSURE\n'
                         'extern uint8_t unknown_00712544[0xa0 * 4]; // 0x00712544, UNSURE identity (zeroed 0xa0 dwords)'),
    'ui_split_screen': 'extern uint8_t ui_split_screen; // 0x00718fc9',
    'network_channel_port_a': 'extern uint32_t network_channel_port_a; // 0x00698208, UNSURE name',
    'widget_extended_description_sync_selection': 'extern void widget_extended_description_sync_selection(widget_instance *widget); // 0x4a66b0',
    'network_session_info_packet_send': 'extern char network_session_info_packet_send(const uint32_t *source, void *client); // 0x4d9050, blam-cc: EAX source, stack client',
    'network_server_reset_game_stats': 'extern uint32_t network_server_reset_game_stats(void); // 0x4a1670',
    'local_player_profile_blocks_00714dd8': 'extern uint8_t local_player_profile_blocks_00714dd8[]; // 0x00714dd8, 0x2004 bytes per local player',
})

TEXT_OPTION = ('static void set_option_text(widget_instance *row, uint32_t value, uint8_t hidden)\n{\n'
               '    widget_instance *label = row->first_child->next_sibling;\n    uint16_t *text;\n\n'
               '    text = (uint16_t *)heap_reallocate(label->text, 0x10, widget_memory_pool);\n    label->text = text;\n'
               '    if (text != 0) {\n        string_format_wide_va(text, (const uint16_t *)L"%d", value);\n        text[7] = 0;\n    }\n'
               '    if (hidden) {\n        row->hidden = 1;\n        *(uint32_t *)&row->scale = 0x3eaa7efa;\n    } else {\n'
               '        row->hidden = 0;\n        row->scale = 1.0f;\n    }\n}\n')

SPECS = [
 (0x4b4af0, 333, "unless 0x00719444 is set, a selected profile's controls section is copied from the live controls block at 0x006b3a48 (working copy +0x134..+0x95d: keyboard scan table, mouse/gamepad tables and scalars). Then empties the second child's nested list (first child -> next -> first -> first -> next: +0x48 = 0, +0x44 = 0), clears 0x00719444 and, when 0x006953e8 is not -1, drops input capture bit 3 (0x00712542), zeroes the 0x280 bytes at 0x00712544 and resets 0x006953e8 to -1. Returns 1.",
  ['ui_flag_00719444', 'selected_saved_item', 'saved_item_working_copy', 'input_controls_live_006b3a48', 'chat_state_006953e8', 'controls_capture'],
  '    const uint8_t *live = input_controls_live_006b3a48;\n    widget_instance *list;\n\n'
  '    if (ui_flag_00719444 == 0 && (selected_saved_item & 0xf) == 0) {\n        uint8_t *profile = saved_item_working_copy;\n\n'
  '        memcpy(profile + 0x134, live + 0x220, 0xda);\n        memcpy(profile + 0x20e, live + 0x10, 0x10);\n'
  '        memcpy(profile + 0x21e, live + 0x880, 0xc);\n        memcpy(profile + 0x22a, live + 0x380, 0x100);\n'
  '        memcpy(profile + 0x32a, live + 0x0, 0x10);\n        memcpy(profile + 0x33a, live + 0x20, 0x200);\n'
  '        memcpy(profile + 0x53a, live + 0x480, 0x400);\n        memcpy(profile + 0x956, live + 0x2fc, 4);\n'
  '        memcpy(profile + 0x95a, live + 0x88c, 4);\n    }\n'
  '    list = widget->first_child->next_sibling->first_child->first_child->next_sibling;\n    list->item_count = 0;\n    list->list_items = 0;\n'
  '    ui_flag_00719444 = 0;\n    if (chat_state_006953e8 != -1) {\n        controls_input_capture_flags &= 0xf7;\n'
  '        memset(unknown_00712544, 0, sizeof(unknown_00712544));\n        chat_state_006953e8 = -1;\n    }\n    return 1;\n'),
 (0x4a3b70, 363, "commits the number being typed (0x00719218: 1 option a, 2 option b) from 0x0071921c with _wtoi, clamped to 0xffff; an empty entry gives option a the default 0x00698208 and option b 0. Then the second and third children show options a and b as L\"%d\" in their second child's text (0x10 bytes), hidden at 0x3eaa7efa in split screen, else shown at 1.0; syncs the extended description selection and clears 0x00719218. (The binary returns 0x4a66b0's leftover AL; this table ignores it.)",
  ['ui_split_screen', 'network_host_number', 'network_channel_port_a', 'network_game_options', 'widget_memory_pool', 'heap_reallocate',
   'string_format_wide_va', 'widget_extended_description_sync_selection'],
  '    uint8_t hidden = (uint8_t)(ui_split_screen == 0);\n    widget_instance *row;\n\n'
  '    if (network_host_number_field_00719218 == 1) {\n        if (network_host_number_text_0071921c[0] == 0) {\n'
  '            network_game_option_a_00719210 = network_channel_port_a;\n        } else {\n'
  '            network_game_option_a_00719210 = (uint32_t)_wtoi((const wchar_t *)network_host_number_text_0071921c);\n'
  '            if (network_game_option_a_00719210 > 0xffff) {\n                network_game_option_a_00719210 = 0xffff;\n            }\n        }\n'
  '    } else if (network_host_number_field_00719218 == 2) {\n        if (network_host_number_text_0071921c[0] == 0) {\n'
  '            network_game_option_b_00719214 = 0;\n        } else {\n'
  '            network_game_option_b_00719214 = (uint32_t)_wtoi((const wchar_t *)network_host_number_text_0071921c);\n'
  '            if (network_game_option_b_00719214 > 0xffff) {\n                network_game_option_b_00719214 = 0xffff;\n            }\n        }\n    }\n'
  '    row = widget->first_child->next_sibling;\n    set_option_text(row, network_game_option_a_00719210, hidden);\n'
  '    row = row->next_sibling;\n    set_option_text(row, network_game_option_b_00719214, hidden);\n'
  '    widget_extended_description_sync_selection(widget);\n    network_host_number_field_00719218 = 0;\n', TEXT_OPTION),
 (0x4a1790, 363, "with a client in state 2 (a state of 1 first reads the performance counter, unused) and a machine word other than -1: counts the valid player entries (16 of 0x20 bytes from client +0xcb6) of this machine, remembering the one whose player byte matches event word 1. None: returns 1. With a match, sends it as a session info packet and clears that local player's profile byte (0x00714dd8 + player * 0x2004). Exactly one entry: with a server up and 0x0071c2dc not 1 returns the game stats reset result (0x4a1670, the pushed arguments are not read); otherwise disposes the client globals and the server host and returns 1; either way 0x00714dd8 takes the byte at 0x00714ddc. More than one entry returns 0. Without a client, or not in state 2, returns 1.",
  ['network_client', 'time_query_performance_counter_ms', 'network_player_entry_validate', 'network_session_info_packet_send',
   'local_player_profile_blocks_00714dd8', 'network_server_pointer', 'network_session_starting_0071c2dc', 'network_server_reset_game_stats',
   'coop_profile_globals_block_00714ddc', 'network_client_globals_dispose', 'network_game_server_host_dispose', 'unknown_0071c2dd'],
  '    uint8_t *client = network_client;\n    uint8_t *found = 0;\n    int32_t count = 0;\n    int16_t key;\n    int32_t i;\n    int16_t *state;\n\n'
  '    if (client == 0) {\n        return 1;\n    }\n    state = (int16_t *)(client + 0xeda);\n    if (*state == 1) {\n        time_query_performance_counter_ms();\n    }\n'
  '    if (*state != 2) {\n        return 1;\n    }\n    key = network_client != 0 ? *(int16_t *)network_client : -1;\n    if (key == -1) {\n        return 1;\n    }\n'
  '    for (i = 0; i < 0x10; i++) {\n        uint8_t *entry = client + 0xcb6 + i * 0x20;\n\n'
  '        if (network_player_entry_validate(entry) != 0 && (int16_t)(int8_t)entry[0x1c] == key) {\n            count++;\n'
  '            if ((int16_t)(int8_t)entry[0x1d] == event[1]) {\n                found = entry;\n            }\n        }\n    }\n'
  '    if (count <= 0) {\n        return 1;\n    }\n    if (found != 0) {\n        network_session_info_packet_send((const uint32_t *)found, client);\n'
  '        local_player_profile_blocks_00714dd8[(int8_t)found[0x1d] * 0x2004] = 0;\n    }\n    if (count != 1) {\n        return 0;\n    }\n'
  '    if (network_server_pointer != 0 && network_session_starting_0071c2dc != 1) {\n'
  '        uint8_t result = (uint8_t)network_server_reset_game_stats();\n\n'
  '        local_player_profile_blocks_00714dd8[0] = coop_profile_globals_block_00714ddc[0];\n        return result;\n    }\n'
  '    network_client_globals_dispose();\n    if (network_server_pointer != 0) {\n        network_game_server_host_dispose(network_server_pointer);\n'
  '        network_server_pointer = 0;\n        unknown_0071c2dd = 0;\n    }\n'
  '    local_player_profile_blocks_00714dd8[0] = coop_profile_globals_block_00714ddc[0];\n    return 1;\n'),
]
generate(SPECS)
