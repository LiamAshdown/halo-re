R = "C:\\Users\\Liam-\\halo-re\\src\\networking\\"
OLD_DECL = '''extern int32_t data_packet_group_decode_packet(data_packet_group *group, void *decoded_body,
    const uint8_t *buffer, int16_t *out_a, int16_t *out_b, int32_t expected_class); // 0x4d09d0'''
NEW_DECL = '''extern int32_t data_packet_group_decode_packet(int16_t *remaining_length, data_packet_group *group,
    void *decoded_body, const uint8_t *buffer, int16_t *out_type, uint16_t *out_version_used,
    int16_t expected_class); // 0x4d09d0, EAX remaining, stack x6'''
NOTE = '''// FIXED 2026-09-28 (networking call audit, from the disassembly): the third argument is the record length (an int);
// the original reduces it by 2 in its own slot and passes its address (EAX) as data_packet_group_decode_packet's
// remaining length -- that argument was missing from the declaration, so every other argument was shifted by one.
'''


def fix(name, pairs):
    p = R + name + ".c"
    t = open(p, encoding="utf-8").read()
    cut = t.index("\n#if 0")
    h, tail = t[:cut], t[cut:]
    pairs = [(OLD_DECL, NEW_DECL)] + pairs
    for old, new in pairs:
        assert h.count(old) == 1, (name, old[:60])
        h = h.replace(old, new)
    i = h.index("#include")
    h = h[:i] + NOTE + "\n" + h[i:]
    open(p, "w", encoding="utf-8", newline="\n").write(h + tail)
    print("fixed", name)


fix("network_game_decode_settings_request", [
    ("void *capacity, const int32_t *expected_sequence)", "int32_t length, const int32_t *expected_sequence)"),
    ('''    uint8_t decoded_body[8];
    int16_t out_a, out_b;
    char engine_match_byte; // Ghidra's local_8c, an extra byte decode_packet writes past decoded_body
''', '''    uint8_t decoded_body[0x94];   // the engine-version byte is +0x08 (0x4dbc7c)
    int16_t out_a, out_b;
'''),
    ('''        if (data_packet_group_decode_packet(&network_game_messages_group, decoded_body, buffer + 2,
                                             &out_a, &out_b, 2) != 0) {
            network_engine_version_match_flag = (engine_match_byte == 1);''',
     '''        if (data_packet_group_decode_packet((length -= 2, (int16_t *)&length), &network_game_messages_group,
                                             decoded_body, buffer + 2, &out_a, (uint16_t *)&out_b, 2) != 0) {
            network_engine_version_match_flag = (decoded_body[8] == 1);'''),
    ("network_game_settings_packet_send(client, decoded_body); // UNSURE argument",
     "network_game_settings_packet_send(client, decoded_body); // 0x4dbc87: stack body, EBX client"),
])
fix("network_game_client_decode_connect_rejected", [
    ("void *capacity, const uint32_t **expected_sequence)", "int32_t length, const uint32_t *expected_sequence)"),
    ("sender.address.ipv4 == **expected_sequence", "sender.address.ipv4 == *expected_sequence"),
    ('''        if (data_packet_group_decode_packet(&network_game_messages_group, decoded_body, buffer + 2,
                                             &out_a, (int16_t *)expected_sequence, 2) != 0) {
            network_session_disconnect_with_error(*(int16_t *)decoded_body); // UNSURE argument''',
     '''        uint16_t version_used;
        if (data_packet_group_decode_packet((length -= 2, (int16_t *)&length), &network_game_messages_group,
                                             decoded_body, buffer + 2, &out_a, &version_used, 2) != 0) {
            network_session_disconnect_with_error((int16_t)decoded_body[0]); // 0x4dbdac: EAX = body[0]'''),
])
fix("network_game_client_decode_settings_or_ack", [
    ("void *capacity, const int32_t *expected_sequence)", "int32_t length, const int32_t *expected_sequence)"),
    ('''            if (data_packet_group_decode_packet(&network_game_messages_group, decoded_body, buffer + 2,
                                                 &out_a, &out_b, 2) != 0) {''',
     '''            if (data_packet_group_decode_packet((length -= 2, (int16_t *)&length), &network_game_messages_group,
                                                 decoded_body, buffer + 2, &out_a, (uint16_t *)&out_b, 2) != 0) {'''),
])
print("ok")
