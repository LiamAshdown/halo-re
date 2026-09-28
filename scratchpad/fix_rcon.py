R = "C:\\Users\\Liam-\\halo-re\\src\\networking\\"

p = R + "rcon_send_request.c"
t = open(p, encoding="utf-8").read()
start = t.index("extern int32_t message_delta_encode_message(int32_t a, int32_t message_type")
cut = t.index("\n#if 0")
t = t[:start] + '''extern int32_t message_delta_encode_message(int32_t buffer, int32_t bit_budget, int32_t flag, int32_t message_type,
    int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed); // 0x4ec940, EAX, EDX
extern uint8_t network_message_scratch[0x7ff8]; // 0x00871de0
extern uint8_t network_channel_stream_flush(network_channel *channel, int32_t mode); // 0x4ddb60
extern int32_t bit_stream_write_bits_chunked(bit_stream *stream, const uint32_t *values, int32_t total_bit_count); // 0x4cf8f0, EAX stream, ECX values, stack bits

// FIXED 2026-09-28 (send-path audit, from the disassembly 0x4e4dc0..0x4e4ef8 -- the function is 0x130 bytes; the
// "function" at 0x4e4e30 is the loop label of its first string copy): the password and the command are copied into
// ONE record {password[9], command[0x41]} which is encoded (message type 0x36, one item) into the network scratch
// 0x871de0, then written to the client channel's outgoing stream as a game-action item (flag 1). The C encoded two
// separate arrays through a malformed call and wrote placeholders.
typedef struct rcon_request_record {
    char password[9];              // 0x00
    char command[0x41];            // 0x09
} rcon_request_record;

void rcon_send_request(char *command, char *password) // blam-cc: EAX -> command, ECX -> password
{
    rcon_request_record record;
    void *items[2];
    int32_t encoded_bits;

    if (strlen(password) > 8) {
        chimera__console_out((ColorARGB *)console_color_006851fc, "ERROR: Maximum rcon password length is %d characters", 8);
        return;
    }
    if (strlen(command) > 0x40) {
        chimera__console_out((ColorARGB *)console_color_006851fc, "ERROR: Maximum rcon command length is %d characters", 0x40);
        return;
    }
    strcpy(record.password, password);
    strcpy(record.command, command);
    items[0] = &record;
    items[1] = 0;
    encoded_bits = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 0x36, 0, items, 0, 1, 0);
    if (encoded_bits > 0) {
        network_channel *channel = network_client->channel;
        bit_stream *stream = (bit_stream *)((uint8_t *)channel + 0x10);
        int32_t free_bits = channel->outgoing.stream.last_bit -
            channel->outgoing.stream.byte_cursor * 8 - channel->outgoing.stream.bit_cursor + 1;

        if ((channel->flags & 1) == 0 &&
            (encoded_bits + 1 <= free_bits || network_channel_stream_flush(channel, 1) != 0)) {
            uint32_t item_flag = 1;

            channel->send_budget = channel->send_budget + encoded_bits + 1;
            bit_stream_write_bits_chunked(stream, &item_flag, 1);
            channel->outgoing.empty = 0;
            bit_stream_write_bits_chunked(stream, (const uint32_t *)network_message_scratch, encoded_bits);
            channel->outgoing.empty = 0;
        }
    }
}
''' + t[cut:]
open(p, "w", encoding="utf-8", newline="\n").write(t)

p = R + "network_game_server_send_message_to_all_machines.c"
t = open(p, encoding="utf-8").read()
inc = t.index("#include")
cut = t.index("\n#if 0")
t = t[:inc].rstrip("\n").replace("rewrite confidence: 0.35", "rewrite confidence: n/a (FRAGMENT)") + '''
// FRAGMENT (2026-09-28, from the disassembly): 0x4e4e30 is not a function -- it is the loop label of the first string
// copy inside rcon_send_request (0x4e4dc0..0x4e4ef8; `jne 0x4e4e30` at 0x4e4e38), which is why nothing calls it.
// rcon_send_request.c covers the whole range; nothing is translated here, so the standalone link gives it no code
// entry.
''' + t[cut:]
open(p, "w", encoding="utf-8", newline="\n").write(t)
print("ok")
