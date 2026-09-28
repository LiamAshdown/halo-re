R = "C:\\Users\\Liam-\\halo-re\\src\\networking\\"
INCLUDES = '''#include "tags.h"
#include "memory.h"
#include <string.h>
#include "math.h"
#include "game.h"
#include "networking.h"
'''


def rewrite(name, note, body):
    p = R + name + ".c"
    t = open(p, encoding="utf-8").read()
    inc = t.index("#include")
    cut = t.find("\n#if 0")
    tail = t[cut:] if cut >= 0 else "\n"
    head = t[:inc].rstrip("\n") + "\n" + note.rstrip("\n") + "\n\n"
    head = head.replace("rewrite confidence: ", "rewrite confidence: 0.85 (REWRITTEN; was ", 1)
    first = head.split("\n", 3)
    open(p, "w", encoding="utf-8", newline="\n").write(head + INCLUDES + body + tail)


rewrite("network_game_process_incoming_messages", '''// REWRITTEN 2026-09-28 (networking call audit) from the disassembly (0x4db180..0x4db304): while the client
// channel's incoming queue (+0xc) holds data, each item is read (network_channel_incoming_read_item, max 0x80000
// bits in EAX) into the scratch buffer 0x861de0 with its bit offset, bit count and sender address, and walked as a
// local bit stream {1, buffer, first bit, byte/bit cursor, last bit, bit count}: while at least 8 bits remain, one
// bit is read (inlined) and network_incoming_item_dispatch gets it with the stream (ECX) and the sender (ESI).
// The previous C flattened the stream into loose integers, so the dispatch (and everything below it) had no
// stream to decode from, and it passed read_item five of its six arguments. Returns the last result (1 when the
// queue was empty from the start).''', '''
extern uint8_t network_incoming_message_scratch[0x510]; // 0x00861de0, UNSURE size
extern int32_t network_channel_incoming_read_item(network_channel *channel, uint8_t *destination,
    int32_t *out_bit_offset, int32_t *out_remaining_bits, s_network_address *out_address,
    int32_t max_item_bits); // 0x4dcf10, stack x5, EAX max bits
extern char network_incoming_item_dispatch(network_client_globals *client, uint32_t item_flag, bit_stream *stream,
    const uint32_t *sender); // 0x4db630, stack, stack, ECX, ESI

typedef struct network_item_stream {
    bit_stream stream;             // 0x00
    uint32_t bit_count;            // 0x18
} network_item_stream;

int32_t network_game_process_incoming_messages(network_client_globals *client)
{
    char result = 1;

    for (;;) {
        network_channel *channel = client->channel;
        circular_buffer *incoming = channel->incoming;
        int32_t available;
        int32_t bit_offset = 0;
        int32_t bit_count = 0;
        uint32_t sender[6];
        network_item_stream s;

        if (incoming == 0) {
            return result;
        }
        available = incoming->write_cursor - incoming->read_cursor;
        if (available < 0) {
            available += incoming->capacity;
        }
        if (available == 0) {
            return result;
        }
        result = (char)network_channel_incoming_read_item(channel, network_incoming_message_scratch, &bit_offset,
                                                          &bit_count, (s_network_address *)sender, 0x80000);
        if (result == 0) {
            continue;
        }
        s.stream.unknown_00 = 1;
        s.stream.data = network_incoming_message_scratch;
        s.stream.first_bit = (uint32_t)bit_offset;
        s.stream.byte_cursor = (uint32_t)bit_offset >> 3;
        s.stream.bit_cursor = (uint32_t)bit_offset & 7;
        s.stream.last_bit = (uint32_t)(bit_count + bit_offset - 1);
        s.bit_count = (uint32_t)bit_count;
        if (result == 1) {
            do {
                uint32_t position = s.stream.byte_cursor * 8 + s.stream.bit_cursor;
                uint32_t next;
                uint8_t item_flag;

                if (s.stream.first_bit - s.stream.byte_cursor * 8 - s.stream.bit_cursor + (uint32_t)bit_count < 8) {
                    break;
                }
                if (position < s.stream.first_bit || position > s.stream.last_bit) {
                    result = 0;
                    break;
                }
                item_flag = (uint8_t)((s.stream.data[s.stream.byte_cursor] >> s.stream.bit_cursor) & 1);
                next = position + 1;
                if ((next >= s.stream.first_bit && next <= s.stream.last_bit) || next == s.stream.last_bit + 1) {
                    s.stream.byte_cursor = next >> 3;
                    s.stream.bit_cursor = next & 7;
                }
                result = network_incoming_item_dispatch(client, item_flag, &s.stream, sender);
            } while (result == 1);
        }
        result = result != 0;
    }
}
''')

rewrite("network_incoming_item_dispatch", '''// REWRITTEN 2026-09-28 (networking call audit) from the disassembly (0x4db630..0x4db6a9): the item's stream
// arrives in ECX and the sender address in ESI. A game-action item (flag 1) goes to
// network_game_action_queue_drain(client, stream, sender); a message item (flag 0) is read into a local 0x1000-byte
// buffer (network_message_read_sized_buffer: EDI buffer, EBX stream, capacity 0xfff) and handed to
// network_game_message_decode_dispatch with the client (EAX), the record (EDX), its length (the first word >> 4,
// EDI) and the sender (stack).''', '''
extern char network_game_action_queue_drain(network_client_globals *client, bit_stream *stream,
    const uint32_t *sender); // 0x4db870, stack
extern uint16_t *network_message_read_sized_buffer(uint16_t *buffer, int32_t capacity, bit_stream *stream); // 0x4de420, EDI, stack, EBX
extern char network_game_message_decode_dispatch(network_client_globals *client, uint16_t *record,
    int32_t record_length, const uint32_t *sender); // 0x4db6b0, EAX, EDX, EDI, stack

char network_incoming_item_dispatch(network_client_globals *client, uint32_t item_flag, bit_stream *stream,
    const uint32_t *sender)
{
    uint16_t buffer[0x800];

    if (item_flag == 1) {
        return network_game_action_queue_drain(client, stream, sender);
    }
    if (item_flag == 0) {
        uint16_t *record = network_message_read_sized_buffer(buffer, 0xfff, stream);

        if (record != 0) {
            return network_game_message_decode_dispatch(client, record, *record >> 4, sender);
        }
    }
    return 0;
}
''')

rewrite("network_game_action_queue_drain", '''// REWRITTEN 2026-09-28 (networking call audit) from the disassembly (0x4db870..0x4db99f): the arguments are
// the client, the item's bit stream and the sender address (not a bound and a sequence). The channel's remote
// address (client +0xadc, 0x4dd390) must match the sender; then a 0x34-byte decode state is begun on the stream
// (message_delta_decode_begin: EAX state, EDI stream) and the decode context is built: context[0] = the state,
// context[1..16] zero, context[0x11] = a zeroed 0x80-byte action record. Each message_delta_decode_array_field
// (EAX context) that succeeds is applied (network_game_action_apply: EAX context, ECX client); the state's +0x18
// counts them; the run continues only while both state flags +0x1c/+0x1d came back 1, and ends with 1 once the
// count passes the state's +0x08. Any other end notifies dropped machines (EBX client) and returns 0.''', '''
extern void network_channel_remote_address_or_default(network_channel *channel, network_resolved_address *out_address); // 0x4dd390, EAX, ECX
extern int32_t message_delta_decode_begin(message_delta_decode_state *state, bit_stream *stream); // 0x4ec490, EAX, EDI
extern int32_t message_delta_decode_array_field(void **context); // 0x4ec510, EAX
extern void network_game_action_apply(void **context, network_client_globals *client); // 0x4da320, EAX, ECX
extern void network_disconnect_notify_dropped_machines(network_client_globals *client); // 0x4d9340, EBX

char network_game_action_queue_drain(network_client_globals *client, bit_stream *stream, const uint32_t *sender)
{
    network_resolved_address remote;
    union {
        message_delta_decode_state state;
        uint8_t bytes[0x34];
    } state;
    uint8_t record[0x80];
    void *context[0x12];
    char result = 0;

    network_channel_remote_address_or_default(client->channel, &remote);
    if (*(uint32_t *)&remote == *sender && (char)message_delta_decode_begin(&state.state, stream) != 0) {
        memset(record, 0, sizeof(record));
        memset(&context[1], 0, 0x40);
        context[0] = &state;
        context[0x11] = record;
        state.bytes[0x1d] = 0;
        state.bytes[0x1c] = 0;
        for (;;) {
            uint8_t *current;

            if ((char)message_delta_decode_array_field(context) == 0) {
                result = 0;
                break;
            }
            network_game_action_apply(context, client);
            current = (uint8_t *)context[0];
            result = current[0x1c] == 1 && current[0x1d] == 1;
            ++*(int32_t *)(current + 0x18);
            memset(&context[1], 0, 0x40);
            current[0x1c] = 0;
            current[0x1d] = 0;
            if (result != 1) {
                break;
            }
            if (*(int32_t *)(current + 0x18) > *(int32_t *)(current + 0x08)) {
                return result;
            }
        }
    }
    network_disconnect_notify_dropped_machines(client);
    return result;
}
''')

rewrite("network_game_message_decode_dispatch", '''// REWRITTEN 2026-09-28 (networking call audit) from the disassembly (0x4db6b0..0x4db800, jump tables
// 0x4db84c / 0x4db800): the client arrives in EAX, the record in EDX, its length in EDI and the sender address on
// the stack. A record whose first word has low bits 0 and bits 2-3 == 3 is dispatched on its last byte; every
// handler gets (client, record, length, sender) -- the binary passes the client in EAX only to those that read it,
// and the record in EDX to the first two -- and its result is returned (1 for anything else). The previous C
// called all eighteen handlers with no arguments.''', '''
typedef int32_t (*network_game_message_handler_proc)(network_client_globals *client, const void *record,
    int32_t record_length, const uint32_t *sender);

extern int32_t network_game_client_decode_beacon_reply(); // 0x4db9a0, EAX client, EDX record, stack length
extern int32_t network_game_client_decode_pong_reply(); // 0x4dba20, EAX client, EDX record, stack length, sender
extern int32_t network_game_decode_settings_request(); // 0x4dbc00
extern int32_t network_game_client_decode_join_accepted(); // 0x4dbcc0
extern int32_t network_game_client_decode_connect_rejected(); // 0x4dbd40, EAX client
extern int32_t network_game_client_decode_join_complete(); // 0x4dbdc0
extern int32_t network_game_client_decode_settings_or_ack(); // 0x4dbe50
extern int32_t network_game_client_decode_player_config_value(); // 0x4dbf30
extern int32_t network_game_client_decode_and_discard_join_message(); // 0x4dbfb0
extern int32_t network_game_client_decode_and_discard_ingame_message(); // 0x4dc020
extern int32_t network_game_client_decode_join_finalize_message(); // 0x4dc090
extern int32_t network_game_client_decode_join_finalize_ack(); // 0x4dc120
extern int32_t network_game_client_decode_state_update_chunk(); // 0x4dc190, EAX client
extern int32_t network_game_client_decode_player_join_chunk(); // 0x4dc240, EAX client
extern int32_t network_game_client_decode_player_slot_chunk(); // 0x4dc2e0
extern int32_t network_game_client_decode_sync_complete(); // 0x4dc3a0
extern int32_t network_game_message_decode_replicated_command(); // 0x4dc410, EAX client
extern int32_t network_game_message_decode_ingame_notification(); // 0x4dc4b0

char network_game_message_decode_dispatch(network_client_globals *client, uint16_t *record, int32_t record_length,
    const uint32_t *sender)
{
    network_game_message_handler_proc handler;

    if ((*record & 3) != 0 || ((*record >> 2) & 3) != 3) {
        return 1;
    }
    switch (*((uint8_t *)record + record_length - 1)) {
    case 0x02: handler = (network_game_message_handler_proc)network_game_client_decode_beacon_reply; break;
    case 0x03: handler = (network_game_message_handler_proc)network_game_client_decode_pong_reply; break;
    case 0x04: handler = (network_game_message_handler_proc)network_game_decode_settings_request; break;
    case 0x05: handler = (network_game_message_handler_proc)network_game_client_decode_join_accepted; break;
    case 0x06: handler = (network_game_message_handler_proc)network_game_client_decode_connect_rejected; break;
    case 0x07: handler = (network_game_message_handler_proc)network_game_client_decode_join_complete; break;
    case 0x08: handler = (network_game_message_handler_proc)network_game_client_decode_settings_or_ack; break;
    case 0x09: handler = (network_game_message_handler_proc)network_game_client_decode_player_config_value; break;
    case 0x0a: handler = (network_game_message_handler_proc)network_game_client_decode_join_finalize_message; break;
    case 0x0b: handler = (network_game_message_handler_proc)network_game_client_decode_join_finalize_ack; break;
    case 0x0c: handler = (network_game_message_handler_proc)network_game_client_decode_and_discard_join_message; break;
    case 0x0d: handler = (network_game_message_handler_proc)network_game_client_decode_and_discard_ingame_message; break;
    case 0x16: handler = (network_game_message_handler_proc)network_game_client_decode_state_update_chunk; break;
    case 0x17: handler = (network_game_message_handler_proc)network_game_client_decode_player_join_chunk; break;
    case 0x18: handler = (network_game_message_handler_proc)network_game_client_decode_player_slot_chunk; break;
    case 0x19: handler = (network_game_message_handler_proc)network_game_client_decode_sync_complete; break;
    case 0x21: handler = (network_game_message_handler_proc)network_game_message_decode_replicated_command; break;
    case 0x22: handler = (network_game_message_handler_proc)network_game_message_decode_ingame_notification; break;
    default: return 1;
    }
    return (char)handler(client, record, record_length, sender);
}
''')
print("ok")
