exec(open(r'C:\Users\Liam-\halo-re\scratchpad\engine_lib.py').read())
exec(open(r'C:\Users\Liam-\halo-re\scratchpad\gen_engine4.py').read().split("emit(0x46a510")[0].split("exec(open(r'C:\\Users\\Liam-\\halo-re\\scratchpad\\engine_lib.py').read())")[1])
EXT.update({
    'ctf_state': ('extern int32_t ctf_touch_counts_network[3]; // 0x0087a9e0 (team 0 / team 1 touch counts, active team)\n'
                  'extern uint8_t ctf_active_team; // 0x006b0eb8\n'
                  'extern int32_t ctf_flag_auto_return_ticks; // 0x006b0eb0'),
    'ctf_reset': ('extern datum_index ctf_team_flag_object[2]; // 0x006b0e90\n'
                  'extern void game_engine_ctf_reset_team_return_credit(uint32_t object_index); // 0x468840, blam-cc: EAX object_index\n'
                  'extern uint8_t ctf_team_return_credit_active[2]; // 0x006b0ea4\n'
                  'extern int32_t ctf_team_return_credit_ticks[2]; // 0x006b0ea8\n'
                  'extern int32_t ctf_notify_throttle_tick; // 0x006b0eb4\n'
                  'extern uint8_t custom_waypoints[]; // 0x006f1888'),
})

emit(0x469bf0, 287, 'game_engine_ctf_profiles_updated',
     'mode 0 encodes a type 0x11 request for the replicated touch counts (0x87a9e0) with the flag auto-return ticks as the second field; otherwise encodes the live touch counts and active team against that replicated copy, then stores them into it. A positive bit count is broadcast when the machine is -1, else sent to that machine. The binary leaves the upper three bytes of the active team dword as stack garbage; they are zero here (the decoder reads the low byte).',
     ['ctf_team_flag_touch_count', 'net_send', 'ctf_state'], 'void %s(int32_t mode, int32_t machine_index)',
     '    int32_t ticks = ctf_flag_auto_return_ticks;\n    void *extra[1];\n    void *items[2];\n    int32_t bits;\n\n'
     '    extra[0] = &ticks;\n'
     '    if (mode == 0) {\n        items[0] = ctf_touch_counts_network;\n'
     '        bits = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 0x11, (int32_t)extra, items, 0, 1, 0);\n'
     '    } else {\n        int32_t live[3];\n        void *baseline[1];\n\n'
     '        live[0] = ctf_team_flag_touch_count[0];\n        live[1] = ctf_team_flag_touch_count[1];\n        live[2] = ctf_active_team;\n'
     '        items[0] = live;\n        items[1] = (void *)ticks;\n        baseline[0] = ctf_touch_counts_network;\n'
     '        bits = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 1, 0x11, (int32_t)extra, items,\n'
     '            (int32_t)baseline, 1, 0);\n'
     '        ctf_touch_counts_network[0] = live[0];\n        ctf_touch_counts_network[1] = live[1];\n        ctf_touch_counts_network[2] = live[2];\n    }\n'
     '    if (bits <= 0) {\n        return;\n    }\n'
     '    if (machine_index == -1) {\n'
     '        network_session_broadcast_to_flagged(bits, network_server_pointer, 1, network_message_scratch, 1, 0, 0, 3);\n'
     '    } else {\n'
     '        network_session_send_to_machine(machine_index, network_server_pointer, 1, network_message_scratch, bits, 1, 0, 0, 3);\n    }\n')

emit(0x46a010, 282, 'game_engine_ctf_reset_objects',
     'as the server: resets the return credit of each existing team flag object, the auto-return ticks take variant +0x80, and the touch counts, return credits, notify throttle and the first four custom waypoints (0x80 bytes) are cleared.',
     ['network_game_mode', 'variant', 'ctf_team_flag_touch_count', 'ctf_state', 'ctf_reset'], 'void %s(void)',
     '    if (network_game_mode != 2) {\n        return;\n    }\n'
     '    if (ctf_team_flag_object[0] != 0xffffffff) {\n        game_engine_ctf_reset_team_return_credit(ctf_team_flag_object[0]);\n    }\n'
     '    if (ctf_team_flag_object[1] != 0xffffffff) {\n        game_engine_ctf_reset_team_return_credit(ctf_team_flag_object[1]);\n    }\n'
     '    ctf_flag_auto_return_ticks = game_engine_variant.ctf_value_80;\n'
     '    ctf_team_flag_touch_count[0] = 0;\n    ctf_team_flag_touch_count[1] = 0;\n'
     '    ctf_team_return_credit_active[0] = 0;\n    ctf_team_return_credit_active[1] = 0;\n'
     '    ctf_team_return_credit_ticks[0] = 0;\n    ctf_team_return_credit_ticks[1] = 0;\n'
     '    ctf_notify_throttle_tick = 0;\n    memset(custom_waypoints, 0, 0x80);\n',
     extra_inc='#include <string.h>\n')
print('ok')
