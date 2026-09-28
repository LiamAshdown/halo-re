"""the send path: every sender writes a 1-bit item flag (1 game action, 0 message record) and then the encoded bits
into the channel's outgoing bit stream (channel +0x10) -- bit_stream_write_bits_chunked(EAX stream, ECX values,
stack bits). The C passed placeholders ("unaff_" values, zeros) or dropped the arguments."""
import re
R = "C:\\Users\\Liam-\\halo-re\\src\\"
DECL = ("extern int32_t bit_stream_write_bits_chunked(bit_stream *stream, const uint32_t *values, int32_t total_bit_count);"
        " // 0x4cf8f0, EAX stream, ECX values, stack bits")
NOTE = ("// FIXED 2026-09-28 (send-path audit, from the disassembly): the two bit_stream_write_bits_chunked calls write into\n"
        "// the channel's outgoing bit stream (channel +0x10, EAX): first the 1-bit item flag (%d: %s) from a local, then\n"
        "// the encoded bits from %s; the C passed placeholders or dropped the arguments.\n")
# file, channel expression, flag, buffer expression, bits expression
SENDERS = [
    ("game\\game_engine_send_team_allegiance_message.c", "session", 1, "network_message_scratch", "encoded_bits"),
    ("interface\\chat_queue_team_message.c", "session", 1, "network_message_scratch", "encoded_bits"),
    ("interface\\chimera__chat_out.c", "session", 1, "network_message_scratch", "encoded_bits"),
    ("networking\\network_game_server_send_message_to_all_machines.c", "channel", 1, "network_message_scratch", "encoded_bits"),
    ("networking\\network_connection_finalize_join.c", "(uintptr_t)iVar6", 0, "&network_join_message_header", "iVar12"),
    ("networking\\network_game_record_message_send.c", "channel", 0, "record", "bits_to_send"),
    ("networking\\network_send_join_request_packet.c", "channel", 0, "record", "bits_to_send"),
    ("networking\\network_game_settings_ack_send.c", "channel", 0, "challenge", "bits_to_send"),
    ("networking\\network_game_settings_packet_send.c", "channel", 0, "challenge", "bits_to_send"),
    ("networking\\network_host_presence_broadcast_tick.c", "channel", 0, "challenge", "bits_to_send"),
    ("networking\\network_session_info_packet_send.c", "channel", 0, "challenge", "bits_to_send"),
    ("networking\\network_session_player_join_notify.c", "channel", 0, "challenge", "bits_to_send"),
    ("networking\\network_staged_message_commit.c", "channel", 0, "challenge", "bits_to_send"),
    ("networking\\network_server_build_full_game_info_packet.c", "channel", 0, "encoded_buffer", "bit_len"),
    ("networking\\network_server_build_game_info_packet.c", "channel", 0, "encoded_buffer", "bit_len"),
]
CALL = re.compile(r"bit_stream_write_bits_chunked\((?:[^()]|\([^()]*\))*\);(?:[ \t]*//[^\n]*)?")
for rel, ch, flag, buf, bits in SENDERS:
    p = R + rel
    t = open(p, encoding="utf-8").read()
    cut = t.find("\n#if 0")
    h, tail = (t, "") if cut < 0 else (t[:cut], t[cut:])
    h, n = re.subn(r"extern [^;(]*\bbit_stream_write_bits_chunked\s*\([^;]*\);[^\n]*", lambda m: DECL, h, flags=re.S)
    assert n == 1, (rel, "decl", n)
    stream = "(bit_stream *)((uint8_t *)%s + 0x10)" % ch
    calls = [m for m in CALL.finditer(h) if not h[max(0, m.start() - 7):m.start()].endswith("extern ") and
             "bit_stream *stream" not in m.group(0)]
    assert len(calls) == 2, (rel, len(calls), [c.group(0) for c in calls])
    first = ("{ uint32_t item_flag = %d; bit_stream_write_bits_chunked(%s, &item_flag, 1); }" % (flag, stream))
    second = "bit_stream_write_bits_chunked(%s, (const uint32_t *)(%s), %s);" % (stream, buf, bits)
    h = h[:calls[0].start()] + first + h[calls[0].end():calls[1].start()] + second + h[calls[1].end():]
    # the placeholder locals the old calls used
    h = re.sub(r"[ \t]*uint32_t unaff_write_value;\n", "", h)
    h = re.sub(r"[ \t]*bit_stream \*unaff_write_stream;\n", "", h)
    i = h.index("#include")
    what = "network_message_scratch 0x871de0" if buf == "network_message_scratch" else buf
    h = h[:i] + NOTE % (flag, "a game action" if flag else "a message record", what) + "\n" + h[i:]
    open(p, "w", encoding="utf-8", newline="\n").write(h + tail)
    print("fixed", rel)
