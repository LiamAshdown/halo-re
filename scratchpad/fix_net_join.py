"""Rewrites of the host's join path around the CD key check, each from its disassembly (the earlier versions were
0.25..0.3 confidence and called their callees with missing or invented arguments)."""
import os, textwrap
os.chdir(r'C:\Users\Liam-\halo-re')


def replace_head(path, header, body):
    s = open(path, encoding='utf-8').read()
    i = s.find('#if 0')
    tail = s[i:] if i >= 0 else ''
    open(path, 'w', encoding='utf-8').write(header + body.lstrip('\n') + ('\n' + tail if tail else ''))


def hdr(name, addr, size, note, cc, conf='0.85', ghidra=None):
    lines = textwrap.wrap('REWRITTEN 2026-09-28 from objdump 0x%x..0x%x: %s' % (addr, addr + size - 1, note), 113)
    wr = ''.join(('// ' if k == 0 else '//   ') + l + '\n' for k, l in enumerate(lines))
    first = '// %s  (%s)\n' % (name, ghidra or 'Ghidra: FUN_%08x' % addr)
    return (first + '// address 0x%x, size %d bytes\n// name confidence: 0.5   rewrite confidence: %s\n%s// blam-cc: %s\n\n'
            '#include "tags.h"\n#include "memory.h"\n#include "math.h"\n#include "game.h"\n#include "networking.h"\n#include <string.h>\n#include <wchar.h>\n\n'
            % (addr, size, conf, wr, cc))


COMMON = '''extern uint16_t *network_prepare_challenge_packet(int32_t message_type, void *payload); // 0x4deaf0, blam-cc: EAX type, EDX payload
extern uint8_t network_session_send_to_machine(int32_t machine_id, network_server_globals *server, uint32_t param_1, void *data,
    uint32_t bits, uint32_t reliable, uint32_t unknown_a, char force, uint32_t priority); // 0x4e1930, blam-cc: EAX machine_id, ESI server
'''

# ---------------- 0x4e0af0
replace_head('src/networking/network_server_notify_or_resend_challenge.c',
    hdr('network_server_notify_or_resend_challenge', 0x4e0af0, 150,
        'CX reason, EDI machine, stack server. For a machine whose channel is connected (+0xa98): the chat close deadline (0x00718fa4, when unset) becomes reason + 0x2b, the host hand-off flag is set, chat closes; returns 1. Otherwise a type 6 packet carrying the reason goes to the machine (reliable, 3) and the machine timer restarts for 1000 ms; returns whether the send worked (0 when no packet was built). The server argument was missing.',
        'CX -> reason, EDI -> machine, stack -> server', ghidra='Ghidra: FUN_004e0af0'),
    COMMON + '''extern int16_t network_chat_close_deadline; // 0x00718fa4
extern uint8_t network_host_handoff_requested; // 0x0071c2de
extern void chat_close(void); // 0x4aa900
extern void network_machine_timer_start(network_machine *machine, int32_t duration_ms); // 0x4df090, blam-cc: ESI machine

uint8_t network_server_notify_or_resend_challenge(int16_t reason, network_machine *machine, network_server_globals *server)
{
    int32_t payload = reason;
    uint16_t *packet;
    uint8_t ok = 1;

    if (machine != 0 && machine->channel != 0 && machine->channel->connected != 0) {
        if (network_chat_close_deadline == -1) {
            network_chat_close_deadline = (int16_t)(reason + 0x2b);
        }
        network_host_handoff_requested = 1;
        chat_close();
        return 1;
    }
    packet = network_prepare_challenge_packet(6, &payload);
    if (packet == 0 || network_session_send_to_machine(machine->machine_id, server, 0, packet, (uint32_t)(*packet >> 4) << 3,
                                                        1, 1, 0, 3) == 0) {
        ok = 0;
    }
    network_machine_timer_start(machine, 1000);
    return ok;
}
''')

# ---------------- 0x5760a0 (new)
open('src/networking/network_session_host_cd_key_callback.c', 'w', encoding='utf-8').write(
    hdr('network_session_host_cd_key_callback', 0x5760a0, 92,
        'the gcd_authenticate_user callback (game id, local id, authenticated, message, instance): a rejected key sends reason 4 to the machine with that CD key local id (+0x5c of the 0x60 byte machines at server +0x3b8; NULL when none).',
        'cdecl (a gcdkey callback)', ghidra='not a Ghidra function; no C existed').replace('REWRITTEN', 'WRITTEN') + '''extern network_server_globals *network_server; // 0x0071c2d4
extern uint8_t network_server_notify_or_resend_challenge(int16_t reason, network_machine *machine,
    network_server_globals *server); // 0x4e0af0, blam-cc: CX reason, EDI machine

void network_session_host_cd_key_callback(int32_t game_id, int32_t local_id, int32_t authenticated, const char *message,
    void *instance)
{
    network_server_globals *server = network_server;
    network_machine *machine = 0;
    int32_t i;

    (void)game_id;
    (void)message;
    (void)instance;
    if (authenticated != 0) {
        return;
    }
    for (i = 0; i < 0x10; i++) {
        if (*(int32_t *)((uint8_t *)server + 0x414 + i * 0x60) == local_id) {
            machine = (network_machine *)((uint8_t *)server + 0x3b8 + i * 0x60);
            break;
        }
    }
    network_server_notify_or_resend_challenge(4, machine, server);
}
''')

# ---------------- 0x575ff0
replace_head('src/networking/network_session_host_reject_or_cleanup_client.c',
    hdr('network_session_host_reject_or_cleanup_client', 0x575ff0, 162,
        'the host\'s CD key check for a joining machine: gcd_authenticate_user(game id 0x0069fdfc, local id, ip, challenge, response, network_session_host_cd_key_callback, 0), then the ban list check on the key hash (gcd_getkeyhash, EDI). Not banned: 1. Banned: reason 6 to the machine with that local id (or NULL), the key is disconnected from gcd (every key for local id -1), 0. (Name kept; it authenticates.)',
        'EAX -> response, ECX -> challenge, EDX -> ip, ESI -> local_id', ghidra='Ghidra: FUN_00575ff0'),
    '''extern network_server_globals *network_server; // 0x0071c2d4
extern int32_t network_cd_key_game_id; // 0x0069fdfc
extern void FUN_0061b110(int32_t game_id, int32_t local_id, uint32_t ip, const char *challenge, const char *response,
    void *callback, void *instance); // 0x61b110 gcd_authenticate_user
extern const char *FUN_0061aa50(int32_t game_id, int32_t local_id); // 0x61aa50 gcd_getkeyhash
extern void FUN_0061b350(int32_t game_id, int32_t local_id); // 0x61b350 gcd_disconnect_user
extern void FUN_0061b3f0(int32_t game_id); // 0x61b3f0 gcd_disconnect_all
extern uint8_t ban_list_check_and_reject_player(char *key); // 0x4e3820, blam-cc: EDI key
extern uint8_t network_server_notify_or_resend_challenge(int16_t reason, network_machine *machine,
    network_server_globals *server); // 0x4e0af0
extern void network_session_host_cd_key_callback(int32_t game_id, int32_t local_id, int32_t authenticated,
    const char *message, void *instance); // 0x5760a0

uint8_t network_session_host_reject_or_cleanup_client(const char *response, const char *challenge, uint32_t ip, int32_t local_id)
{
    network_server_globals *server;
    network_machine *machine = 0;
    int32_t i;

    FUN_0061b110(network_cd_key_game_id, local_id, ip, challenge, response, (void *)network_session_host_cd_key_callback, 0);
    if (ban_list_check_and_reject_player((char *)FUN_0061aa50(network_cd_key_game_id, local_id)) == 0) {
        return 1;
    }
    server = network_server;
    for (i = 0; i < 0x10; i++) {
        if (*(int32_t *)((uint8_t *)server + 0x414 + i * 0x60) == local_id) {
            machine = (network_machine *)((uint8_t *)server + 0x3b8 + i * 0x60);
            break;
        }
    }
    network_server_notify_or_resend_challenge(6, machine, server);
    if (local_id == -1) {
        FUN_0061b3f0(network_cd_key_game_id);
    } else {
        FUN_0061b350(network_cd_key_game_id, local_id);
    }
    return 0;
}
''')

# ---------------- 0x4e0ab0
replace_head('src/networking/network_join_request_reset_state.c',
    hdr('network_join_request_reset_state', 0x4e0ab0, 63,
        'EAX machine, stack response: the CD key check of a joining machine with its remote ip (the local address 0x006869b0 for loopback 127.0.0.1), its challenge (+0x52) and CD key local id (+0x5c). (Name kept.)',
        'EAX -> machine, stack -> response', ghidra='Ghidra: FUN_004e0ab0'),
    '''extern void network_channel_remote_address_or_default(network_channel *channel, network_resolved_address *out_address); // 0x4dd390, blam-cc: EAX channel, ECX out
extern uint32_t network_local_address; // 0x006869b0
extern uint8_t network_session_host_reject_or_cleanup_client(const char *response, const char *challenge, uint32_t ip,
    int32_t local_id); // 0x575ff0, blam-cc: EAX response, ECX challenge, EDX ip, ESI local_id

uint8_t network_join_request_reset_state(network_machine *machine, const char *response)
{
    network_resolved_address address;
    uint32_t ip;

    network_channel_remote_address_or_default(machine != 0 ? machine->channel : 0, &address);
    ip = *(uint32_t *)&address;
    if (ip == 0x7f000001) {
        ip = network_local_address;
    }
    return network_session_host_reject_or_cleanup_client(response, (const char *)machine + 0x52, ip,
        *(int32_t *)((uint8_t *)machine + 0x5c));
}
''')

DECODE = '''extern data_packet_group network_game_messages_group; // 0x006994f8
extern int32_t data_packet_group_decode_packet(int16_t *remaining_length, data_packet_group *group, void *decoded_body,
    uint8_t *buffer, int16_t *out_type, uint16_t *out_version_used, int16_t expected_class); // 0x4d09d0, blam-cc: EAX remaining_length
extern uint32_t network_game_session_finalize_and_add_player(network_player_entry *entry, network_server_globals *server,
    network_machine *machine); // 0x4df840, blam-cc: EAX entry, ECX server, EDX machine
extern uint32_t network_game_broadcast_player_set_changed(network_server_globals *server, uint8_t *param_1); // 0x4e1bf0 (reads its stack server)
extern uint8_t network_server_notify_or_resend_challenge(int16_t reason, network_machine *machine,
    network_server_globals *server); // 0x4e0af0
'''

# ---------------- 0x4e21d0
replace_head('src/networking/network_game_server_handle_join_password.c',
    hdr('network_game_server_handle_join_password', 0x4e21d0, 546,
        'the host\'s join request handler (message 0xe; EBX machine, stack server, buffer, length). While the host is not in a game (+4 0 or 1) and the machine is not already joining (+0xe bit 1): a machine without a connected channel after the game ended gets the full game info. Otherwise the request (a 0x84 byte body) is decoded; unless the server accepts joins (+6 bit 0, state 0 or 1) it is refused (0); the CD key response at body +0x22 must pass the host check (else 6); the first 16 bytes must match the version canary (else 1); the 8-character password at body +0x10 must match a set password (else 2); the machine is reset and its player (body +0x6e) added (else 3), the player set is broadcast, the channel rate (+0xa88) becomes 4 or body +0x6b by the hint byte 0x006894a2, and a type 0xa accept packet is sent. Refusals send the reason; every path returns 1.',
        'EBX -> machine, stack -> server, buffer, length', ghidra='Ghidra: FUN_004e21d0'),
    COMMON + DECODE + '''extern uint8_t network_pending_join_password_hint; // 0x006894a2
extern char network_server_build_full_game_info_packet(network_machine *machine); // 0x4e0bd0
extern void network_channel_remote_address_or_default(network_channel *channel, network_resolved_address *out_address); // 0x4dd390
extern uint8_t network_join_request_reset_state(network_machine *machine, const char *response); // 0x4e0ab0, blam-cc: EAX machine
extern void network_debug_fill_canary_buffer(uint32_t *buffer); // 0x4e0790, blam-cc: EAX buffer
extern void network_server_password_get(network_server_globals *server, wchar_t *dest); // 0x4e0930, blam-cc: EAX server, ESI dest
extern int32_t network_server_password_is_set(network_server_globals *server); // 0x4e08e0, blam-cc: EAX server
extern int32_t network_machine_reset(network_machine *machine); // 0x4df690, blam-cc: ESI machine

char network_game_server_handle_join_password(network_machine *machine, network_server_globals *server, uint8_t *buffer,
    int32_t length)
{
    int16_t out_type;
    uint16_t out_version;
    uint32_t scratch[6];
    uint8_t body[0x90];
    int16_t remaining;
    int16_t reason;

    if (server->unknown_004 != 0 && server->unknown_004 != 1) {
        return 1;
    }
    remaining = (int16_t)(length - 2);
    if ((machine->flags & 0x02) != 0) {
        return 1;
    }
    if ((machine->channel == 0 || machine->channel->connected == 0) && server->game_over != 0) {
        char result = network_server_build_full_game_info_packet(machine);

        return result != 0 ? result : 1;
    }
    if (data_packet_group_decode_packet(&remaining, &network_game_messages_group, body, buffer + 2, &out_type, &out_version,
                                         3) == 0) {
        return 1;
    }
    network_channel_remote_address_or_default(machine->channel, (network_resolved_address *)scratch);
    if ((server->flags & 1) == 0 || (server->unknown_004 != 0 && server->unknown_004 != 1)) {
        reason = 0;
    } else if (network_join_request_reset_state(machine, (const char *)body + 0x22) == 0) {
        reason = 6;
    } else if ((network_debug_fill_canary_buffer(scratch), memcmp(body, scratch, 0x10)) != 0) {
        reason = 1;
    } else {
        network_server_password_get(server, (wchar_t *)scratch);
        *(uint16_t *)(body + 0x20) = 0;
        if (wcsncmp((const wchar_t *)(body + 0x10), (const wchar_t *)scratch, 8) != 0 && network_server_password_is_set(server) != 0) {
            reason = 2;
        } else if (network_machine_reset(machine) == 0 ||
                   network_game_session_finalize_and_add_player((network_player_entry *)(body + 0x6e), server, machine) == 0) {
            reason = 3;
        } else {
            uint32_t payload = 0;
            uint16_t *packet;

            network_game_broadcast_player_set_changed(server, (uint8_t *)server);
            *(int32_t *)((uint8_t *)machine->channel + 0xa88) = network_pending_join_password_hint == 0 ? 4 : body[0x6b];
            packet = network_prepare_challenge_packet(0xa, &payload);
            if (packet != 0 && machine->machine_id != -1) {
                network_session_send_to_machine(machine->machine_id, server, 0, packet, (uint32_t)(*packet >> 4) << 3, 1, 0, 1, 3);
            }
            return 1;
        }
    }
    network_server_notify_or_resend_challenge(reason, machine, server);
    return 1;
}
''')

# ---------------- 0x4e2400
replace_head('src/networking/network_game_server_handle_join_confirm.c',
    hdr('network_game_server_handle_join_confirm', 0x4e2400, 200,
        'EAX machine, ECX server, EDX buffer, stack length: while the host is not in a game, the decoded body is the player to add; failure sends reason 3, success broadcasts the player set and sends a type 0xa accept. Returns 1.',
        'EAX -> machine, ECX -> server, EDX -> buffer, stack -> length', ghidra='Ghidra: FUN_004e2400'),
    COMMON + DECODE + '''
char network_game_server_handle_join_confirm(network_machine *machine, network_server_globals *server, uint8_t *buffer,
    int32_t length)
{
    uint16_t out_version;
    int16_t out_type;
    uint8_t body[0x20];
    int16_t remaining;

    if (server->unknown_004 != 0 && server->unknown_004 != 1) {
        return 1;
    }
    remaining = (int16_t)(length - 2);
    if (data_packet_group_decode_packet(&remaining, &network_game_messages_group, body, buffer + 2, &out_type, &out_version, 3) == 0) {
        return 1;
    }
    if (network_game_session_finalize_and_add_player((network_player_entry *)body, server, machine) == 0) {
        network_server_notify_or_resend_challenge(3, machine, server);
        return 1;
    }
    if (network_game_broadcast_player_set_changed(server, (uint8_t *)server) != 0) {
        uint32_t payload = 0;
        uint16_t *packet = network_prepare_challenge_packet(0xa, &payload);

        if (packet != 0 && machine->machine_id != -1) {
            network_session_send_to_machine(machine->machine_id, server, 0, packet, (uint32_t)(*packet >> 4) << 3, 1, 0, 1, 3);
        }
    }
    return 1;
}
''')
print('ok')
