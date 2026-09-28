R = "C:\\Users\\Liam-\\halo-re\\src\\networking\\"
INCLUDES = '''#include "tags.h"
#include "memory.h"
#include <string.h>
#include "math.h"
#include "game.h"
#include "networking.h"

extern data_packet_group network_game_messages_group; // 0x006994f8
extern int32_t data_packet_group_decode_packet(int16_t *remaining_length, data_packet_group *group,
    void *decoded_body, const uint8_t *buffer, int16_t *out_type, uint16_t *out_version_used,
    int16_t expected_class); // 0x4d09d0, EAX remaining, stack x6
'''
COMMON = '''// REWRITTEN 2026-09-28 (networking call audit) from the disassembly: network_game_process_incoming_message
// (0x4e1c60) calls this with the registers noted at the signature; the record length arrives on the stack, is
// reduced by 2 in place and passed by address (EAX) as data_packet_group_decode_packet's remaining length, which
// takes 7 arguments (the previous C declared 6, shifting every argument, and passed no record or length at all).
'''


def rewrite(name, note, body):
    p = R + name + ".c"
    t = open(p, encoding="utf-8").read()
    inc = t.index("#include")
    cut = t.find("\n#if 0")
    tail = t[cut:] if cut >= 0 else "\n"
    head = t[:inc].rstrip("\n") + "\n" + COMMON + note.rstrip("\n") + "\n\n"
    if "rewrite confidence: 0.85" not in head:
        head = head.replace("rewrite confidence: ", "rewrite confidence: 0.85 (REWRITTEN; was ", 1)
        lines = head.split("\n")
        for i, l in enumerate(lines):
            if "(REWRITTEN; was " in l:
                lines[i] = l + ")"
                break
        head = "\n".join(lines)
    open(p, "w", encoding="utf-8", newline="\n").write(head + INCLUDES + body + tail)
    print("rewrote", name)


DECODE = "data_packet_group_decode_packet((length -= 2, (int16_t *)&length), &network_game_messages_group, %s, record + 2, &out_type, &version_used, %d)"

rewrite("network_game_message_handle_settings_relay", "// 0x4e24d0: ESI server, EDX record, stack length; state (+4) 0, class 3.", '''
extern uint32_t network_game_settings_broadcast_send(uint32_t round, uint32_t *record); // 0x4df0e0, stack (server, record)

uint32_t network_game_message_handle_settings_relay(network_server_globals *server, uint8_t *record, int32_t length)
{
    uint32_t body[8];
    int16_t out_type;
    uint16_t version_used;

    if (*(int16_t *)((uint8_t *)server + 4) == 0 && %s != 0) {
        return network_game_settings_broadcast_send((uint32_t)server, body);
    }
    return 1;
}
''' % (DECODE % ("body", 3)))

rewrite("network_game_message_handle_settings_relay_role2", "// 0x4e28d0: ESI server, EDX record, stack length; state (+4) 2, class 7.", '''
extern uint32_t network_game_settings_broadcast_send(uint32_t round, uint32_t *record); // 0x4df0e0, stack (server, record)

uint32_t network_game_message_handle_settings_relay_role2(network_server_globals *server, uint8_t *record, int32_t length)
{
    uint32_t body[8];
    int16_t out_type;
    uint16_t version_used;

    if (*(int16_t *)((uint8_t *)server + 4) == 2 && %s != 0) {
        return network_game_settings_broadcast_send((uint32_t)server, body);
    }
    return 1;
}
''' % (DECODE % ("body", 7)))

rewrite("network_game_client_handle_settings_relay", "// 0x4e2810: ESI server, EDX record, stack length; state (+4) 1, class 5.", '''
extern uint32_t network_game_settings_broadcast_send(uint32_t round, uint32_t *record); // 0x4df0e0, stack (server, record)

uint32_t network_game_client_handle_settings_relay(network_server_globals *server, uint8_t *record, int32_t length)
{
    uint32_t body[8];
    int16_t out_type;
    uint16_t version_used;

    if (*(int16_t *)((uint8_t *)server + 4) == 1 && %s != 0) {
        return network_game_settings_broadcast_send((uint32_t)server, body);
    }
    return 1;
}
''' % (DECODE % ("body", 5)))

rewrite("network_game_message_handle_player_count_broadcast", "// 0x4e2530: ESI server, EDX record, stack length; state (+4) 0 or 1, class 3.", '''
extern uint32_t network_game_broadcast_player_set_changed(network_server_globals *session); // 0x4e1bf0, stack

uint32_t network_game_message_handle_player_count_broadcast(network_server_globals *server, uint8_t *record, int32_t length)
{
    uint32_t body[1];
    int16_t out_type;
    uint16_t version_used;
    int16_t state = *(int16_t *)((uint8_t *)server + 4);

    if ((state == 0 || state == 1) && %s != 0) {
        network_game_broadcast_player_set_changed(server);
    }
    return 1;
}
''' % (DECODE % ("body", 3)))

rewrite("network_game_message_handle_player_entry_update", "// 0x4e2580: ESI server, EDX record, stack length; state (+4) 0, class 3; the entry goes to\n// network_player_entry_update (EAX entry, ECX server +8).", '''
extern uint8_t network_player_entry_update(network_player_entry *incoming, network_game_session *session); // 0x4de5f0, EAX, ECX
extern uint32_t network_game_broadcast_player_set_changed(network_server_globals *session); // 0x4e1bf0, stack

uint32_t network_game_message_handle_player_entry_update(network_server_globals *server, uint8_t *record, int32_t length)
{
    uint32_t body[8];
    int16_t out_type;
    uint16_t version_used;

    if (*(int16_t *)((uint8_t *)server + 4) == 0 && %s != 0 &&
        network_player_entry_update((network_player_entry *)body, (network_game_session *)((uint8_t *)server + 8)) != 0) {
        network_game_broadcast_player_set_changed(server);
    }
    return 1;
}
''' % (DECODE % ("body", 3)))

rewrite("network_game_message_handle_handshake_forward", "// 0x4e25e0: ESI server, EDX record, stack length; state (+4) 0, class 3; the decoded state goes to\n// network_client_connection_handshake_tick (EAX state, ECX server).", '''
extern void network_client_connection_handshake_tick(int16_t state, network_server_globals *owner); // 0x4e0590, EAX, ECX

uint32_t network_game_message_handle_handshake_forward(network_server_globals *server, uint8_t *record, int32_t length)
{
    int32_t body[1];
    int16_t out_type;
    uint16_t version_used;

    if (*(int16_t *)((uint8_t *)server + 4) == 0 && %s != 0) {
        network_client_connection_handshake_tick((int16_t)body[0], server);
    }
    return 1;
}
''' % (DECODE % ("body", 3)))

rewrite("network_game_message_handle_build_version", "// 0x4e2630: EAX server, EDI machine, stack (record, length); state (+4) 0, class 3; the decoded 0x100-byte\n// version string goes to network_machine_check_build_version (EAX string, EDI machine).", '''
extern void network_machine_check_build_version(const char *remote_version, network_machine *machine); // 0x4dff20, EAX, EDI

uint32_t network_game_message_handle_build_version(network_server_globals *server, network_machine *machine, uint8_t *record,
    int32_t length)
{
    char body[0x100];
    int16_t out_type;
    uint16_t version_used;

    if (*(int16_t *)((uint8_t *)server + 4) == 0 && %s != 0) {
        network_machine_check_build_version(body, machine);
    }
    return 1;
}
''' % (DECODE % ("body", 3)))

rewrite("network_game_message_handle_retry_schedule", "// 0x4e26a0: EAX server, ESI machine, stack (record, length); state (+4) 0, class 3; then the machine's\n// timer starts (network_machine_timer_start: ESI machine, stack 0).", '''
extern void network_machine_timer_start(network_machine *machine, int32_t duration_ms); // 0x4df090, ESI, stack

uint32_t network_game_message_handle_retry_schedule(network_server_globals *server, network_machine *machine, uint8_t *record,
    int32_t length)
{
    uint32_t body[1];
    int16_t out_type;
    uint16_t version_used;

    if (*(int16_t *)((uint8_t *)server + 4) == 0 && %s != 0) {
        network_machine_timer_start(machine, 0);
    }
    return 1;
}
''' % (DECODE % ("body", 3)))

rewrite("network_game_client_handle_retry_schedule", "// 0x4e2870: EAX server, ESI machine, stack (record, length); state (+4) 1, class 5; then the machine's\n// timer starts (network_machine_timer_start: ESI machine, stack 0).", '''
extern void network_machine_timer_start(network_machine *machine, int32_t duration_ms); // 0x4df090, ESI, stack

uint32_t network_game_client_handle_retry_schedule(network_server_globals *server, network_machine *machine, uint8_t *record,
    int32_t length)
{
    uint32_t body[1];
    int16_t out_type;
    uint16_t version_used;

    if (*(int16_t *)((uint8_t *)server + 4) == 1 && %s != 0) {
        network_machine_timer_start(machine, 0);
    }
    return 1;
}
''' % (DECODE % ("body", 5)))

rewrite("network_game_message_handle_join_finalize_ack_role2", "// 0x4e2930: ECX server, ESI machine, stack (record, length); state (+4) 2, class 7; then the machine's flag\n// byte +0x0e loses bit 4.", '''
uint32_t network_game_message_handle_join_finalize_ack_role2(network_server_globals *server, network_machine *machine,
    uint8_t *record, int32_t length)
{
    uint32_t body[1];
    int16_t out_type;
    uint16_t version_used;

    if (*(int16_t *)((uint8_t *)server + 4) == 2 && %s != 0) {
        *((uint8_t *)machine + 0xe) &= 0xfb;
    }
    return 1;
}
''' % (DECODE % ("body", 7)))

rewrite("network_game_server_handle_info_request", "// 0x4e2700: ESI server, EDI machine, EDX record, stack length; state (+4) 0 or 1, class 5. A machine whose\n// connection (+0) has +0xa98 set, or any machine while the server's +0xa0f is clear, gets\n// network_game_server_handle_client_join (stack server, machine) and 1; otherwise the full game info packet is\n// built for it (0x4e0bd0) and its result returned. Returns 0 when the state or the decode fails.", '''
extern void network_game_server_handle_client_join(int32_t *object_count_passthrough, network_server_globals *server,
    network_machine *machine, uint8_t bl_passthrough); // 0x4dfc90, stack (server, machine)
extern char network_server_build_full_game_info_packet(network_machine *machine); // 0x4e0bd0, stack

uint32_t network_game_server_handle_info_request(network_server_globals *server, network_machine *machine, uint8_t *record,
    int32_t length)
{
    uint32_t body[1];
    int16_t out_type;
    uint16_t version_used;
    int16_t state = *(int16_t *)((uint8_t *)server + 4);
    uint8_t *connection;

    if ((state != 0 && state != 1) || %s == 0) {
        return 0;
    }
    connection = machine != 0 ? *(uint8_t **)machine : 0;
    if ((connection != 0 && connection[0xa98] != 0) || *((uint8_t *)server + 0xa0f) == 0) {
        network_game_server_handle_client_join(0 /* UNSURE: a register pass-through */, server, machine, 1);
        return 1;
    }
    return (uint8_t)network_server_build_full_game_info_packet(machine);
}
''' % (DECODE % ("body", 5)))

rewrite("network_game_client_handle_map_data", "// 0x4e2790: stack (server, length), EDX record, ESI machine; state (+4) 1, class 5. A decoded 0x20-byte player\n// entry that validates (network_player_entry_validate, EAX) is stored at server +0x9d8 once (+0x9f8 set).", '''
extern char network_player_entry_validate(network_player_entry *entry); // 0x4de9f0, EAX

uint32_t network_game_client_handle_map_data(network_server_globals *server, uint8_t *record, int32_t length)
{
    uint32_t body[8];
    int16_t out_type;
    uint16_t version_used;
    uint8_t *s = (uint8_t *)server;

    if (*(int16_t *)(s + 4) == 1 && %s != 0 && s[0x9f8] == 0 &&
        network_player_entry_validate((network_player_entry *)body) != 0) {
        memcpy(s + 0x9d8, body, sizeof(body));
        s[0x9f8] = 1;
    }
    return 1;
}
''' % (DECODE % ("body", 5)))

rewrite("network_game_process_incoming_message", "// 0x4e1c60: EAX length, ECX machine, EDX record, stack server; jump tables 0x4e1f0c / 0x4e1ec8. The message\n// type is the record's last byte; a machine without flag bit 1 only gets type 0xe, or type 1 with bit 4. Every\n// handler now receives what the original passes it (the previous C passed nothing to eleven of them).", '''
extern uint8_t network_disconnect_timeout_flag; // 0x0071c2dc, UNSURE name
extern uint32_t network_game_message_handle_keepalive(network_channel **channel, int32_t *record); // 0x4e2110, EAX machine, stack
extern char network_game_server_handle_join_password(network_machine *machine, network_server_globals *server, uint8_t *buffer,
    int32_t length); // 0x4e21d0, EBX machine, stack (server, record, length)
extern char network_game_server_handle_join_confirm(network_machine *machine, network_server_globals *server, uint8_t *buffer,
    int32_t length); // 0x4e2400, EAX machine, ECX server, EDX record, stack length
extern uint32_t network_game_message_handle_settings_relay(network_server_globals *server, uint8_t *record, int32_t length); // 0x4e24d0
extern uint32_t network_game_message_handle_player_count_broadcast(network_server_globals *server, uint8_t *record, int32_t length); // 0x4e2530
extern uint32_t network_game_message_handle_player_entry_update(network_server_globals *server, uint8_t *record, int32_t length); // 0x4e2580
extern uint32_t network_game_message_handle_handshake_forward(network_server_globals *server, uint8_t *record, int32_t length); // 0x4e25e0
extern uint32_t network_game_message_handle_retry_schedule(network_server_globals *server, network_machine *machine,
    uint8_t *record, int32_t length); // 0x4e26a0
extern uint32_t network_game_message_handle_build_version(network_server_globals *server, network_machine *machine,
    uint8_t *record, int32_t length); // 0x4e2630
extern uint32_t network_game_server_handle_info_request(network_server_globals *server, network_machine *machine,
    uint8_t *record, int32_t length); // 0x4e2700
extern void network_game_client_apply_position_update(uint8_t *state, uint32_t *packet, void *param_3, void *object); // 0x4dff70, stack
extern uint32_t network_game_client_handle_map_data(network_server_globals *server, uint8_t *record, int32_t length); // 0x4e2790
extern uint32_t network_game_client_handle_settings_relay(network_server_globals *server, uint8_t *record, int32_t length); // 0x4e2810
extern uint32_t network_game_client_handle_retry_schedule(network_server_globals *server, network_machine *machine,
    uint8_t *record, int32_t length); // 0x4e2870
extern uint32_t network_game_message_handle_settings_relay_role2(network_server_globals *server, uint8_t *record, int32_t length); // 0x4e28d0
extern uint32_t network_game_message_handle_join_finalize_ack_role2(network_server_globals *server, network_machine *machine,
    uint8_t *record, int32_t length); // 0x4e2930

uint32_t network_game_process_incoming_message(int32_t length, network_machine *machine, uint16_t *record, network_server_globals *server)
{
    uint8_t *bytes = (uint8_t *)record;
    uint8_t type_byte;
    uint8_t machine_flags;

    if ((*record & 3) != 0 || ((*record >> 2) & 3) != 3) {
        return 1;
    }
    type_byte = bytes[(int16_t)length - 1];
    machine_flags = *((uint8_t *)machine + 0xe);
    if (((machine_flags >> 1) & 1) == 0 && type_byte != 0x0e && !(((machine_flags >> 4) & 1) != 0 && type_byte == 1)) {
        return 1;
    }
    switch (type_byte) {
    case 0x01:
        if (network_disconnect_timeout_flag != 0) {
            int32_t body[1];
            int16_t out_type;
            uint16_t version_used;

            if (data_packet_group_decode_packet((length -= 2, (int16_t *)&length), &network_game_messages_group, body,
                                                bytes + 2, &out_type, &version_used, 0) != 0) {
                network_game_message_handle_keepalive((network_channel **)machine, body);
            }
        }
        return 1;
    case 0x0e: return (uint8_t)network_game_server_handle_join_password(machine, server, bytes, length);
    case 0x0f: return (uint8_t)network_game_server_handle_join_confirm(machine, server, bytes, length);
    case 0x10: return (uint8_t)network_game_message_handle_settings_relay(server, bytes, length);
    case 0x11: return (uint8_t)network_game_message_handle_player_count_broadcast(server, bytes, length);
    case 0x12: return (uint8_t)network_game_message_handle_player_entry_update(server, bytes, length);
    case 0x13: return (uint8_t)network_game_message_handle_handshake_forward(server, bytes, length);
    case 0x14:
    case 0x25: return (uint8_t)network_game_message_handle_retry_schedule(server, machine, bytes, length);
    case 0x15: return (uint8_t)network_game_message_handle_build_version(server, machine, bytes, length);
    case 0x1a: return (uint8_t)network_game_server_handle_info_request(server, machine, bytes, length);
    case 0x1b:
        if (*(int16_t *)((uint8_t *)server + 4) == 1) {
            uint32_t body[8];
            int16_t out_type;
            uint16_t version_used;

            if (data_packet_group_decode_packet((length -= 2, (int16_t *)&length), &network_game_messages_group, body,
                                                bytes + 2, &out_type, &version_used, 5) != 0) {
                network_game_client_apply_position_update((uint8_t *)machine, body, (void *)-1, 0);
            }
        }
        return 1;
    case 0x1c: return (uint8_t)network_game_client_handle_map_data(server, bytes, length);
    case 0x1d: return (uint8_t)network_game_client_handle_settings_relay(server, bytes, length);
    case 0x1e: return (uint8_t)network_game_client_handle_retry_schedule(server, machine, bytes, length);
    case 0x23: return (uint8_t)network_game_message_handle_settings_relay_role2(server, bytes, length);
    case 0x24: return (uint8_t)network_game_message_handle_join_finalize_ack_role2(server, machine, bytes, length);
    }
    return 1;
}
''')

# network_game_broadcast_player_set_changed: one argument (the session), the broadcast goes through network_session
p = R + "network_game_broadcast_player_set_changed.c"
t = open(p, encoding="utf-8").read()
old = '''uint32_t network_game_broadcast_player_set_changed(network_server_globals *server, uint8_t *param_1)
{'''
assert old in t
t = t.replace(old, '''// FIXED 2026-09-28 (networking call audit, from the disassembly 0x4e1bf0..0x4e1c57): the only argument is the
// session on the stack (its +8 is the record to encode; the game/ callers already pass one argument); the
// broadcast's server (ECX) is the global network_session (0x71c2d4), not a parameter.
extern network_server_globals *network_session; // 0x0071c2d4
uint32_t network_game_broadcast_player_set_changed(uint8_t *param_1)
{''')
t = t.replace("network_session_broadcast_to_all(server, 1, network_object_update_scratch, 1, 0, 1, 3);",
              "network_session_broadcast_to_all(network_session, 1, network_object_update_scratch, 1, 0, 1, 3);")
open(p, "w", encoding="utf-8", newline="\n").write(t)
print("ok")
