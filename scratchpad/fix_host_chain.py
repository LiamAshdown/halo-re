R = "C:\\Users\\Liam-\\halo-re\\src\\"
INCLUDES = '''#include "tags.h"
#include "memory.h"
#include <string.h>
#include "math.h"
#include "game.h"
#include "networking.h"
'''


def rewrite(rel, note, body, includes=INCLUDES):
    p = R + rel
    t = open(p, encoding="utf-8").read()
    inc = t.index("#include")
    cut = t.find("\n#if 0")
    tail = t[cut:] if cut >= 0 else "\n"
    head = t[:inc].rstrip("\n") + "\n" + note.rstrip("\n") + "\n\n"
    if "rewrite confidence: 0.85" not in head:
        head = head.replace("rewrite confidence: ", "rewrite confidence: 0.85 (REWRITTEN; was ", 1)
        lines = head.split("\n")
        for i, l in enumerate(lines):
            if "(REWRITTEN; was " in l:
                lines[i] = l + ")"
                break
        head = "\n".join(lines)
    open(p, "w", encoding="utf-8", newline="\n").write(head + includes + body + tail)
    print("rewrote", rel)


rewrite("networking\\network_channel_drain_bitstream.c", '''// REWRITTEN 2026-09-28 (networking call audit) from the disassembly (0x4e1290..0x4e1407): the host twin of
// network_game_process_incoming_messages. While the machine's channel (machine +0) has queued data, each item is
// read (network_channel_incoming_read_item, max 0x80000 bits in EAX) into 0x861de0 and walked as a local bit stream;
// each leading bit goes to network_channel_dispatch_bitstream_unit(server, bit) with the stream (ECX) and the
// machine (ESI). A failed read, or a dispatch returning 0, ends the drain with 0; an empty queue returns 1.''', '''
extern uint8_t network_incoming_message_scratch[0x510]; // 0x00861de0, UNSURE size
extern int32_t network_channel_incoming_read_item(network_channel *channel, uint8_t *destination,
    int32_t *out_bit_offset, int32_t *out_remaining_bits, s_network_address *out_address,
    int32_t max_item_bits); // 0x4dcf10, stack x5, EAX max bits
extern char network_channel_dispatch_bitstream_unit(network_server_globals *server, uint32_t unit, bit_stream *stream,
    network_machine *machine); // 0x4e18b0, stack, stack, ECX, ESI

typedef struct network_item_stream {
    bit_stream stream;             // 0x00
    uint32_t bit_count;            // 0x18
} network_item_stream;

char network_channel_drain_bitstream(network_server_globals *server, network_machine *machine)
{
    char result = 1;

    for (;;) {
        network_channel *channel = *(network_channel **)machine;
        circular_buffer *incoming = channel->incoming;
        int32_t available;
        int32_t bit_offset = 0;
        int32_t bit_count = 0;
        uint32_t sender[6];
        network_item_stream s;

        if (incoming == 0) {
            return 1;
        }
        available = incoming->write_cursor - incoming->read_cursor;
        if (available < 0) {
            available += incoming->capacity;
        }
        if (available == 0) {
            return 1;
        }
        result = (char)network_channel_incoming_read_item(channel, network_incoming_message_scratch, &bit_offset,
                                                          &bit_count, (s_network_address *)sender, 0x80000);
        if (result == 0) {
            return 0;
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
                result = network_channel_dispatch_bitstream_unit(server, item_flag, &s.stream, machine);
            } while (result == 1);
        }
        if (result == 0) {
            return 0;
        }
    }
}
''')

rewrite("networking\\network_channel_dispatch_bitstream_unit.c", '''// REWRITTEN 2026-09-28 (networking call audit) from the disassembly (0x4e18b0..0x4e1927): besides (server, unit)
// on the stack it takes the item's stream in ECX and the machine in ESI. A game-action item (1) drains through
// network_client_drain_queued_updates (ECX stream; stack server, machine); a message item (0) is read into a local
// 0x1000-byte buffer (EDI buffer, EBX stream, capacity 0xfff) and handed to network_game_process_incoming_message
// (EAX length = first word >> 4, ECX machine, EDX record, stack server). The previous C passed the unit flag as the
// server and no stream.''', '''
extern uint16_t *network_message_read_sized_buffer(uint16_t *buffer, int32_t capacity, bit_stream *stream); // 0x4de420, EDI, stack, EBX
extern uint32_t network_game_process_incoming_message(int32_t length, network_machine *machine,
    uint16_t *record, network_server_globals *server); // 0x4e1c60, EAX, ECX, EDX, stack
extern char network_client_drain_queued_updates(network_server_globals *server, network_machine *machine,
    bit_stream *stream); // 0x4e1f40, stack, stack, ECX

char network_channel_dispatch_bitstream_unit(network_server_globals *server, uint32_t unit, bit_stream *stream,
    network_machine *machine)
{
    uint16_t buffer[0x800];

    if (unit == 1) {
        return network_client_drain_queued_updates(server, machine, stream);
    }
    if (unit == 0) {
        uint16_t *record = network_message_read_sized_buffer(buffer, 0xfff, stream);

        if (record != 0) {
            return (char)network_game_process_incoming_message(*record >> 4, machine, record, server);
        }
    }
    return 0;
}
''')

rewrite("networking\\network_client_drain_queued_updates.c", '''// REWRITTEN 2026-09-28 (networking call audit) from the disassembly (0x4e1f40..0x4e2060, jump tables 0x4e2078 /
// 0x4e2060): the host twin of network_game_action_queue_drain. Arguments: the item's stream in ECX, then (server,
// machine) on the stack. A 0x34-byte decode state is begun on the stream (EAX state, EDI stream) and the decode
// context built (context[0] the state, [1..16] zero, [0x11] a 0x80-byte record); each decoded item of type 0xd
// (network_game_client_apply_received_update: EBX machine, stack server, context), 0xf (chat_server_relay_incoming_
// message: EAX context, stack machine), 0x1a (game_engine_update_lead_change_state: EAX context, stack machine),
// 0x34 (network_game_message_handle_ping_timestamp: EAX context, stack server) or 0x36
// (network_server_handle_rcon_request: EAX machine, EDX context) is applied; the run continues while both state
// flags +0x1c/+0x1d came back 1 and the count (+0x18) has not passed +0x08. Returns the last result.''', '''
extern int32_t message_delta_decode_begin(message_delta_decode_state *state, bit_stream *stream); // 0x4ec490, EAX, EDI
extern int32_t message_delta_decode_array_field(void **context); // 0x4ec510, EAX
extern void network_game_client_apply_received_update(network_machine *machine, uint32_t param_1, void **message); // 0x4e0280
extern void chat_server_relay_incoming_message(void **context, network_machine *machine); // 0x4aabd0
extern void game_engine_update_lead_change_state(void **envelope, uint8_t *message); // 0x470810
extern uint32_t network_game_message_handle_ping_timestamp(int32_t **message, network_server_globals *param_1); // 0x4e20b0
extern void network_server_handle_rcon_request(network_player_entry *client, void *message); // 0x4e4f00

char network_client_drain_queued_updates(network_server_globals *server, network_machine *machine, bit_stream *stream)
{
    union {
        message_delta_decode_state state;
        uint8_t bytes[0x34];
    } state;
    uint8_t record[0x80];
    void *context[0x12];
    char result;

    result = (char)message_delta_decode_begin(&state.state, stream);
    if (result != 1) {
        return result;
    }
    memset(&context[1], 0, 0x40);
    context[0] = &state;
    context[0x11] = record;
    state.bytes[0x1d] = 0;
    state.bytes[0x1c] = 0;
    for (;;) {
        uint8_t *current;

        if ((char)message_delta_decode_array_field(context) == 0) {
            return 0;
        }
        current = (uint8_t *)context[0];
        switch (*(int32_t *)(current + 4)) {
        case 0x0d: network_game_client_apply_received_update(machine, (uint32_t)server, context); break;
        case 0x0f: chat_server_relay_incoming_message(context, machine); break;
        case 0x1a: game_engine_update_lead_change_state(context, (uint8_t *)machine); break;
        case 0x34: network_game_message_handle_ping_timestamp((int32_t **)context, server); break;
        case 0x36: network_server_handle_rcon_request((network_player_entry *)machine, context); break;
        }
        result = current[0x1c] == 1 && current[0x1d] == 1;
        ++*(int32_t *)(current + 0x18);
        memset(&context[1], 0, 0x40);
        current[0x1c] = 0;
        current[0x1d] = 0;
        if (result != 1) {
            return result;
        }
        if (*(int32_t *)((uint8_t *)context[0] + 0x18) > *(int32_t *)((uint8_t *)context[0] + 0x08)) {
            return result;
        }
    }
}
''')

rewrite("interface\\chat_server_relay_incoming_message.c", '''// REWRITTEN 2026-09-28 (networking call audit) from the disassembly (0x4aabd0..0x4aadd0): EAX is the decode
// context, the stack holds the sending machine. The chat record {scope, sender byte, text pointer} decodes into
// locals (the text into a local 0x200-byte buffer); the sender's player index comes from
// network_object_owner_team_index_desired(machine) (0x4e0cf0, EAX) and a -1 drops the message. The record is
// re-encoded (message type 0xf) into 0x871de0 and sent through network_session (0x71c2d4): scope 0 to every flagged
// machine (0x4e1a80), scope 1 to every player on the sender's team, scope 2 to every player whose unit's vehicle
// (+0x11c) is the sender's (player_get_vehicle) -- each such player's machine (+0x64, not -1) gets
// network_session_send_to_machine (0x4e1930). The previous C decoded into nothing and sent with invented arguments.''', '''
extern network_server_globals *network_session;   // 0x0071c2d4
extern data_array *player_data;                     // 0x0087a480
extern uint8_t network_message_scratch[0x7ff8];     // 0x00871de0

extern uint8_t message_delta_decode_compound_field(void *decode_context, void *destination); // 0x4ec590, EAX context, ECX destination
extern uint8_t message_delta_decode_compound_field_staged(void *decode_context); // 0x4ec670, EAX context
extern int32_t network_object_owner_team_index_desired(void *obj); // 0x4e0cf0, EAX
extern int32_t message_delta_encode_message(int32_t buffer, int32_t bit_budget, int32_t flag, int32_t message_type,
    int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed); // 0x4ec940, EAX, EDX
extern uint8_t network_session_send_to_machine(int32_t machine_id, network_server_globals *server, uint32_t param_1,
    void *data, uint32_t param_3, uint32_t reliable, uint32_t unknown_a, char force, uint32_t priority); // 0x4e1930, EAX, ESI, stack
extern char network_session_broadcast_to_flagged(int32_t body_bit_count, network_server_globals *server, int32_t param_1,
    void *data, int32_t param_3, int32_t param_4, char force, int32_t param_6); // 0x4e1a80, EAX, ECX, stack
extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, EDI
extern void *datum_get(datum_index handle, data_array *array); // 0x4d0680, EDX, ESI
extern datum_index player_get_vehicle(datum_index player_index); // 0x4ab170, ECX
extern void *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, ECX, stack

typedef struct chat_relay_message {
    int32_t scope;                 // 0x00: 0 everyone, 1 team, 2 vehicle
    uint8_t sender;                // 0x04, player index
    uint8_t pad_05[3];
    void *text;                    // 0x08 -> the local text buffer
} chat_relay_message;

static void chat_relay_iterator_begin(data_iterator *iterator)
{
    iterator->data = player_data;
    iterator->next_index = 0;
    iterator->index = (datum_index)0xffffffff;
    iterator->signature = (uint32_t)(uintptr_t)player_data ^ k_data_iterator_signature;
}

void chat_server_relay_incoming_message(void **context, void *machine)
{
    chat_relay_message message;
    void *item;
    uint32_t zero_24;
    uint8_t text[0x200];
    int32_t sender;
    int32_t bits;
    data_iterator iterator;
    uint8_t *entry;

    if (*(int32_t *)context[0] != 0) {
        message_delta_decode_compound_field_staged(context);
        return;
    }
    message.scope = 0;
    message.sender = 0xff;
    message.text = text;
    if (message_delta_decode_compound_field(context, &message) == 0) {
        return;
    }
    sender = network_object_owner_team_index_desired(machine);
    if (sender == -1) {
        return;
    }
    message.sender = (uint8_t)sender;
    item = &message;
    zero_24 = 0;
    (void)zero_24;
    bits = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 0xf, 0, &item, 0, 1, 0);
    if (message.scope == 0) {
        network_session_broadcast_to_flagged(bits, network_session, 1, network_message_scratch, 1, 0, 1, 3);
    } else if (message.scope == 1) {
        uint8_t *sender_player = (uint8_t *)datum_get((datum_index)message.sender, player_data);

        if (sender_player == 0) {
            return;
        }
        chat_relay_iterator_begin(&iterator);
        while ((entry = (uint8_t *)data_iterator_next(&iterator)) != 0) {
            if (*(int32_t *)(entry + 0x20) == *(int32_t *)(sender_player + 0x20) && *(int8_t *)(entry + 0x64) != -1) {
                network_session_send_to_machine(*(int8_t *)(entry + 0x64), network_session, 1, network_message_scratch,
                                                (uint32_t)bits, 1, 0, 1, 3);
            }
        }
    } else if (message.scope == 2) {
        datum_index vehicle = player_get_vehicle((datum_index)message.sender);

        if (vehicle == (datum_index)0xffffffff) {
            return;
        }
        chat_relay_iterator_begin(&iterator);
        while ((entry = (uint8_t *)data_iterator_next(&iterator)) != 0) {
            uint8_t *unit = (uint8_t *)object_try_and_get(*(datum_index *)(entry + 0x34), 3);

            if (unit != 0 && *(datum_index *)(unit + 0x11c) == vehicle && *(int8_t *)(entry + 0x64) != -1) {
                network_session_send_to_machine(*(int8_t *)(entry + 0x64), network_session, 1, network_message_scratch,
                                                (uint32_t)bits, 1, 0, 1, 3);
            }
        }
    }
}
''', includes='''#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "objects.h"
#include "units.h"
#include <stdint.h>
''')
print("ok")
