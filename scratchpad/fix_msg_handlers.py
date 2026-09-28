import re
R = "C:\\Users\\Liam-\\halo-re\\src\\networking\\"

NOTE = '''// FIXED 2026-09-28 (networking call audit, from the disassembly): the dispatcher (0x4db6b0) passes (client, record,
// length, sender) -- the length is an int, which the original reduces by 2 in its own argument slot and hands to
// data_packet_group_decode_packet by address (EAX) as the remaining length; that call takes 7 arguments (remaining,
// group, body, record + 2, out_type, out_version_used, expected class), not 8 (the extra one made the class 0);
// network_disconnect_notify_dropped_machines gets the client (EBX).
'''
DECL_OLD = re.compile(r"extern int32_t data_packet_group_decode_packet\(int16_t \*remaining_length, data_packet_group \*group,\s*"
                      r"void \*decoded_body, uint8_t \*buffer, int16_t \*out_type, byte_stream \*input,\s*"
                      r"uint16_t \*out_version_used, int16_t expected_class\); // 0x4d09d0")
DECL_NEW = ("extern int32_t data_packet_group_decode_packet(int16_t *remaining_length, data_packet_group *group,\n"
            "    void *decoded_body, uint8_t *buffer, int16_t *out_type, uint16_t *out_version_used,\n"
            "    int16_t expected_class); // 0x4d09d0, EAX remaining, stack x6")


def fix(name, extra):
    p = R + name + ".c"
    t = open(p, encoding="utf-8").read()
    cut = t.index("\n#if 0")
    h, tail = t[:cut], t[cut:]
    h, n = DECL_OLD.subn(DECL_NEW, h)
    assert n == 1, name
    h = h.replace("uint8_t *param_1,\n    int16_t *param_2, int32_t *param_3)", "uint8_t *param_1,\n    int32_t param_2, int32_t *param_3)")
    h, n = re.subn(r"data_packet_group_decode_packet\(param_2, &network_game_messages_group, decoded_body,(\s*)"
                   r"param_1 \+ 2, &out_type, &input, 0, (\d)\)",
                   r"data_packet_group_decode_packet((param_2 -= 2, (int16_t *)&param_2), &network_game_messages_group,\1"
                   r"decoded_body, param_1 + 2, &out_type, (uint16_t *)&input, \2)", h)
    assert n == 1, name
    h = h.replace("extern void network_disconnect_notify_dropped_machines(void);",
                  "extern void network_disconnect_notify_dropped_machines(network_client_globals *client);")
    h = h.replace("network_disconnect_notify_dropped_machines();", "network_disconnect_notify_dropped_machines(client);")
    for old, new in extra:
        assert h.count(old) == 1, (name, old[:60])
        h = h.replace(old, new)
    i = h.index("#include")
    h = h[:i] + NOTE + "\n" + h[i:]
    open(p, "w", encoding="utf-8", newline="\n").write(h + tail)
    print("fixed", name)


fix("network_game_client_decode_state_update_chunk", [])
fix("network_game_client_decode_player_join_chunk", [
    ("extern char network_player_join_finalize(network_client_globals *client);",
     "extern char network_player_join_finalize(network_client_globals *client, void *entry); // EDI client, EAX entry"),
    ("result = network_player_join_finalize(client);", "result = network_player_join_finalize(client, decoded_body); // 0x4dc2ac: EAX = the decoded body"),
])
fix("network_game_client_decode_player_slot_chunk", [
    ("extern uint8_t network_session_player_table_index_apply(network_client_globals *client, int32_t param_2);",
     "extern uint8_t network_session_player_table_index_apply(network_client_globals *client, int32_t table_index,\n    const uint8_t *candidate); // stack client, table index; EBX candidate"),
])
fix("network_game_message_decode_replicated_command", [
    ("extern void network_client_timer_schedule(network_client_globals *client, int32_t delay_ms, uint32_t event_id);",
     "extern void network_client_timer_schedule(int32_t delay_ms, int32_t context, network_client_globals *client); // stack x2, ESI client"),
    ("network_client_timer_schedule(client, decoded_body[0], decoded_body[1]);",
     "network_client_timer_schedule((int32_t)decoded_body[0], (int32_t)decoded_body[1], client);"),
])
# player_slot: the table-index call passes the body's last dword and the body itself
p = R + "network_game_client_decode_player_slot_chunk.c"
t = open(p, encoding="utf-8").read()
i = t.index("result = network_session_player_table_index_apply(client, 0);")
j = t.index("if (result != 0)", i)
t = t[:i] + ("result = network_session_player_table_index_apply(client, (int32_t)decoded_body[8], (const uint8_t *)decoded_body);\n"
             "                // 0x4dc367: stack (client, body +0x20), EBX = the body\n            ") + t[j:]
open(p, "w", encoding="utf-8", newline="\n").write(t)
print("ok")
