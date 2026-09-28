exec(open(r'C:\Users\Liam-\halo-re\scratchpad\hsgen_lib.py').read())

R0 = 'hs_thread_return(0, thread_index);\n'
CONSOLE = X['console_out']

def simple(addr, size, decl, call, args=True):
    emit(addr, size, 'calls %s; returns 0.' % call.split('(')[0], decl, call + ';\n' + R0, args=args)

def server_only(addr, size, text, text_addr, args):
    emit(addr, size, 'prints "%s" (no colour); returns 0.' % text, CONSOLE,
         'chimera__console_out(0, "%s"); // 0x%08x\n' % (text, text_addr) + R0, args=args)

VARIADIC = ('extern char hs_evaluate_variadic_arguments(uint32_t thread_index, int32_t value, uint32_t *out_count, int32_t **out_values); // 0x48ad60\n')

emit(0x4823e0, 74, 'hs_bind_control (0x48b750) with the device class, input and action names; returns 0.',
     'extern void hs_bind_control(const char *device_class_name, const char *input_name, const char *action_name); // 0x48b750, blam-cc: EAX device, stack input, action\n',
     'hs_bind_control((const char *)arguments[0], (const char *)arguments[1], (const char *)arguments[2]);\n' + R0)
emit(0x482430, 70, 'hs_unbind_control (0x48b8d0) with the device class and input names; returns 0.',
     'extern void hs_unbind_control(const char *device_class_name, const char *input_name); // 0x48b8d0, blam-cc: EDI, EBX\n',
     'hs_unbind_control((const char *)arguments[0], (const char *)arguments[1]);\n' + R0)
simple(0x482480, 16, 'extern void input_print_bound_controls(void); // 0x48bea0\n', 'input_print_bound_controls()', args=False)
emit(0x482490, 69, 'sends the team allegiance message (0x4704d0) with the low byte of the short, zero-extended; returns 0.',
     'extern void game_engine_send_team_allegiance_message(char broadcast); // 0x4704d0\n',
     'game_engine_send_team_allegiance_message((char)(uint8_t)arguments[0]);\n' + R0)
emit(0x4824e0, 35, 'while message delta parameters are enabled (0x0071cfa8): reloads them from the config file, reapplies the field bindings and sends an update; returns 0.',
     'extern uint8_t message_delta_parameters_enabled; // 0x0071cfa8\n'
     'extern void message_delta_parameters_protocol_reload_from_config_file(void); // 0x4ebda0\n'
     'extern void message_delta_definitions_invoke_field_bindings(void); // 0x4ec390\n'
     'extern void message_delta_parameters_protocol_send_update(void); // 0x4ebf50\n',
     'if (message_delta_parameters_enabled != 0) {\n    message_delta_parameters_protocol_reload_from_config_file();\n'
     '    message_delta_definitions_invoke_field_bindings();\n    message_delta_parameters_protocol_send_update();\n}\n' + R0, args=False)
emit(0x482510, 70, 'when message delta parameters are enabled with exactly 1: writes the config text buffer (0x00860b40, used as the format) to "parameters.cfg" opened "wb"; returns 0.',
     'extern uint8_t message_delta_parameters_enabled; // 0x0071cfa8\n'
     'extern char message_delta_config_text_buffer[]; // 0x00860b40\n',
     'if (message_delta_parameters_enabled != 0 && message_delta_parameters_enabled == 1) {\n'
     '    FILE *file = fopen("parameters.cfg", "wb"); // 0x0066e664, mode 0x0066e674\n\n    if (file != 0) {\n'
     '        fprintf(file, message_delta_config_text_buffer);\n        fclose(file);\n    }\n}\n' + R0, args=False)
emit(0x482560, 65, 'player_update_history_log_set_name_filter (0x4e5f80) with the name; returns 0.',
     'extern void player_update_history_log_set_name_filter(char *name); // 0x4e5f80, blam-cc: ESI\n',
     'player_update_history_log_set_name_filter((char *)arguments[0]);\n' + R0)
emit(0x4825b0, 63, 'player_update_queue_flush_by_name (0x4e5fe0) with the name; returns 0.',
     'extern void player_update_queue_flush_by_name(char *name); // 0x4e5fe0, blam-cc: EAX\n',
     'player_update_queue_flush_by_name((char *)arguments[0]);\n' + R0)
emit(0x4825f0, 65, 'game_engine_find_player_by_name (0x473430) with the name (EBX); returns 0.',
     'extern void game_engine_find_player_by_name(char *source_name); // 0x473430, blam-cc: EBX\n',
     'game_engine_find_player_by_name((char *)arguments[0]);\n' + R0)
emit(0x482640, 65, 'Sleep for the long milliseconds; returns 0.', 'extern void Sleep(uint32_t milliseconds); // 0x0063a29c IAT\n',
     'Sleep((uint32_t)arguments[0]);\n' + R0)
simple(0x482690, 16, 'extern uint8_t game_checkpoint_save_new(void); // 0x538db0\n', 'game_checkpoint_save_new()', args=False)
emit(0x4826a0, 63, 'saved_game_load_checkpoint (0x539290) with the name; returns 0.',
     'extern uint8_t saved_game_load_checkpoint(char *name); // 0x539290, blam-cc: EAX\n',
     'saved_game_load_checkpoint((char *)arguments[0]);\n' + R0)
emit(0x482720, 71, 'network_game_client_connect_to_address_async (0x4c8500) with the address and password; returns 0.',
     'extern uint8_t network_game_client_connect_to_address_async(char *address, char *password); // 0x4c8500\n',
     'network_game_client_connect_to_address_async((char *)arguments[0], (char *)arguments[1]);\n' + R0)
emit(0x482770, 40, 'as a client in network_game_mode 1: network_client_rejoin_check(0) (0x4de390); returns 0.',
     'extern uint8_t *network_client; // 0x0071c2d8\nextern int16_t network_game_mode; // 0x00719720\n'
     'extern void network_client_rejoin_check(int8_t machine_player_index); // 0x4de390\n',
     'if (network_client != 0 && network_game_mode == 1) {\n    network_client_rejoin_check(0);\n}\n' + R0, args=False)
emit(0x4827a0, 63, 'saved_game_delete_by_display_name (0x53b9b0) with the name (ECX); returns 0.',
     'extern void saved_game_delete_by_display_name(const char *name); // 0x53b9b0, blam-cc: ECX\n',
     'saved_game_delete_by_display_name((const char *)arguments[0]);\n' + R0)
emit(0x4827e0, 63, 'hs_help_print_function (0x4841b0) with the name (EDX); returns 0.',
     'extern void hs_help_print_function(char *name); // 0x4841b0, blam-cc: EDX\n',
     'hs_help_print_function((char *)arguments[0]);\n' + R0)
emit(0x482820, 33, 'quit: sets 0x0071975b, movie_playback_abort (0x007196d4) = 1 and disarms the split screen quit prompt; returns 0.',
     'extern uint8_t ui_event_byte_0071975b; // 0x0071975b\nextern int32_t movie_playback_abort; // 0x007196d4\n'
     'extern uint8_t split_screen_quit_prompt_armed; // 0x00719757\n',
     'ui_event_byte_0071975b = 1;\nmovie_playback_abort = 1;\nsplit_screen_quit_prompt_armed = 0;\n' + R0, args=False)
emit(0x482850, 66, 'stores the boolean in the ai enabled byte (ai_globals +0); returns 0.', X['ai_global_data'],
     'ai_global_data[0] = (uint8_t)arguments[0];\n' + R0)
emit(0x482980, 63, 'main_queue_map_change (0x4c8740) with the name; returns 0.',
     'extern void main_queue_map_change(char *map_name); // 0x4c8740, blam-cc: EAX\n',
     'main_queue_map_change((char *)arguments[0]);\n' + R0)
emit(0x4829c0, 78, 'evaluates the variadic string arguments (0x48ad60) and, when done, runs rcon (0x4e4c00) with their count and values; returns 0 then.',
     VARIADIC + 'extern void rcon(int32_t argument_count, char **arguments); // 0x4e4c00\n',
     'uint32_t count = 0;\nint32_t *values = 0;\n\n'
     'if (hs_evaluate_variadic_arguments(thread_index, (int32_t)first, &count, &values) != 0) {\n'
     '    rcon((int32_t)count, (char **)values);\n    hs_thread_return(0, thread_index);\n}\n', args=False)
simple(0x482a10, 16, 'extern void network_banlist_print(void); // 0x4e34e0\n', 'network_banlist_print()', args=False)
emit(0x482a20, 116, 'for a ban list index in range (0x006b859c, 0x38 byte entries): prints "Unbanning %s." with the entry, removes it and saves the ban list; returns 0.',
     CONSOLE + 'extern growable_array ban_list; // 0x006b859c\n'
     'extern void growable_array_remove_element(growable_array *array, uint32_t index); // 0x4cf890, blam-cc: ESI, EDI\n'
     'extern void network_banlist_save(void); // 0x4e3380\n',
     'int32_t index = arguments[0];\n\nif (index >= 0 && index < ban_list.count) {\n'
     '    chimera__console_out(0, "Unbanning %s.", (uint8_t *)ban_list.data + index * 0x38); // 0x0066d71c\n'
     '    growable_array_remove_element(&ban_list, (uint32_t)index);\n    network_banlist_save();\n    hs_thread_return(0, thread_index);\n}\n')
server_only(0x482aa0, 26, 'sv_map_next is a dedicated server-only function!', 0x66dc7c, False)
simple(0x482ac0, 16, 'extern void sv_map_reset(void); // 0x4e2aa0\n', 'sv_map_reset()', args=False)
emit(0x482ad0, 68, 'sv_map (0x4e2b20) with the two strings as its (EAX, EBX) pair; returns 0.',
     'extern void sv_map(uint32_t argument_count, uint16_t **arguments); // 0x4e2b20, blam-cc: EAX, EBX\n',
     'sv_map((uint32_t)arguments[0], (uint16_t **)arguments[1]);\n' + R0)
emit(0x482bc0, 58, 'as the server (network_game_mode 2): sets 0x006f1d25, begins the end game sequence and prints "Server is stopping the game..."; otherwise prints "sv_end_game is a server-only function!"; both in the console colour at 0x006851fc. Returns 0.',
     CONSOLE + 'extern int16_t network_game_mode; // 0x00719720\nextern uint8_t g_006f1d25; // 0x006f1d25\n'
     'extern void game_engine_begin_end_game_sequence(void); // 0x45fd90\nextern void *console_color_006851fc; // 0x006851fc\n',
     'if (network_game_mode == 2) {\n    g_006f1d25 = 1;\n    game_engine_begin_end_game_sequence();\n'
     '    chimera__console_out(console_color_006851fc, "Server is stopping the game..."); // 0x0066dbd4\n} else {\n'
     '    chimera__console_out(console_color_006851fc, "sv_end_game is a server-only function!"); // 0x0066dbac\n}\n' + R0, args=False)
simple(0x482c00, 16, 'extern void sv_players(void); // 0x4e2c70\n', 'sv_players()', args=False)
emit(0x482c10, 63, 'sv_kick (0x4e3910) with the name or index; returns 0.', 'extern void sv_kick(char *name_or_index); // 0x4e3910, blam-cc: EAX\n',
     'sv_kick((char *)arguments[0]);\n' + R0)
simple(0x482c50, 16, 'extern void sv_status(void); // 0x4e2e50\n', 'sv_status()', args=False)
emit(0x482c60, 73, 'evaluates the variadic arguments (0x48ad60) and, when done, runs sv_ban (0x4e3990) with their count and values; returns 0 then.',
     VARIADIC + 'extern void sv_ban(uint32_t argument_count, int32_t *arguments); // 0x4e3990, blam-cc: EAX, ECX\n',
     'uint32_t count = 0;\nint32_t *values = 0;\n\n'
     'if (hs_evaluate_variadic_arguments(thread_index, (int32_t)first, &count, &values) != 0) {\n'
     '    sv_ban(count, values);\n    hs_thread_return(0, thread_index);\n}\n', args=False)
server_only(0x482df0, 26, 'sv_mapcycle_begin is a dedicated server-only function!', 0x66dd54, False)
server_only(0x482e10, 26, 'sv_mapcycle is a dedicated server-only function!', 0x66dd20, False)
server_only(0x482e30, 71, 'sv_mapcycle_add is a dedicated server-only function!', 0x66dce8, True)
server_only(0x482e80, 71, 'sv_mapcycle_del is a dedicated server-only function!', 0x66dcb0, True)
print('part 2 done')
