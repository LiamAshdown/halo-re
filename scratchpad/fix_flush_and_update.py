import re
R = "C:\\Users\\Liam-\\halo-re\\src\\"
DECL = ("extern char network_channel_stream_flush(network_channel_stream *stream, network_channel *channel, char mode);"
        " // 0x4ddb60, ESI stream (channel +0x10), stack channel, mode")
for rel in ("game\\game_engine_send_team_allegiance_message.c", "interface\\chat_queue_team_message.c",
            "interface\\chimera__chat_out.c", "networking\\rcon_send_request.c"):
    p = R + rel
    t = open(p, encoding="utf-8").read()
    cut = t.find("\n#if 0")
    h, tail = (t, "") if cut < 0 else (t[:cut], t[cut:])
    h, n = re.subn(r"extern [^;(]*\bnetwork_channel_stream_flush\s*\([^;]*\);[^\n]*", lambda m: DECL, h)
    assert n == 1, (rel, n)
    h, n = re.subn(r"network_channel_stream_flush\((\w+), 1\)",
                   r"network_channel_stream_flush((network_channel_stream *)((uint8_t *)\1 + 0x10), (network_channel *)\1, 1)", h)
    assert n == 1, (rel, "call", n)
    h = h.replace("// FIXED 2026-09-28 (send-path audit, from the disassembly): the two bit_stream_write_bits_chunked",
                  "// FIXED 2026-09-28 (send-path audit, from the disassembly): network_channel_stream_flush takes the channel's\n"
                  "// stream (ESI, channel +0x10), the channel and the mode (the C passed the channel as the stream). The two\n"
                  "// bit_stream_write_bits_chunked", 1)
    open(p, "w", encoding="utf-8", newline="\n").write(h + tail)
    print("flush fixed", rel)

p = R + "networking\\update_server_send_update.c"
t = open(p, encoding="utf-8").read()
old_check = '''                max_bits = (channel->retransmit.stream.last_bit - channel->retransmit.stream.byte_cursor * 8) -
                           channel->retransmit.stream.bit_cursor + 1;'''
assert old_check in t
t = t.replace(old_check, '''                max_bits = (channel->outgoing.stream.last_bit - channel->outgoing.stream.byte_cursor * 8) -
                           channel->outgoing.stream.bit_cursor + 1;''')
old_w = '''                bit_stream_write_bits_chunked(1, 0, &channel->retransmit.stream); // UNSURE: elided args
                channel->retransmit.empty = 0;
                bit_stream_write_bits_chunked(1, (uint32_t)(uintptr_t)encoded, &channel->retransmit.stream); // UNSURE
                channel->retransmit.empty = 0;'''
assert old_w in t
t = t.replace(old_w, '''                {
                    uint32_t item_flag = 1;

                    bit_stream_write_bits_chunked(&channel->outgoing.stream, &item_flag, 1);
                    channel->outgoing.empty = 0;
                    bit_stream_write_bits_chunked(&channel->outgoing.stream, (const uint32_t *)network_message_scratch,
                                                  (int32_t)(uintptr_t)encoded);
                    channel->outgoing.empty = 0;
                }''')
t, n = re.subn(r"extern [^;(]*\bbit_stream_write_bits_chunked\s*\([^;]*\);[^\n]*",
               lambda m: ("extern int32_t bit_stream_write_bits_chunked(bit_stream *stream, const uint32_t *values, "
                          "int32_t total_bit_count); // 0x4cf8f0, EAX stream, ECX values, stack bits"), t, count=1)
assert n == 1
i = t.index("#include")
t = t[:i] + ('''// FIXED 2026-09-28 (send-path audit, from the disassembly 0x4de1ed..0x4de26e): the room check and both writes use
// the channel's outgoing stream (+0x10), not the retransmit stream (+0x544): the 1-bit item flag (1, a game action)
// and then the encoded bits from the network scratch 0x871de0; the C wrote placeholders into the wrong stream.
''') + "\n" + t[i:]
if "network_message_scratch" not in t[:t.index("#if 0") if "#if 0" in t else len(t)].split("FIXED 2026-09-28")[0] and \
        "extern uint8_t network_message_scratch" not in t:
    j = t.index("extern int32_t bit_stream_write_bits_chunked")
    t = t[:j] + "extern uint8_t network_message_scratch[0x7ff8]; // 0x00871de0\n" + t[j:]
open(p, "w", encoding="utf-8", newline="\n").write(t)
print("update_server fixed")
