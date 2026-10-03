#include "halo/networking/net1_client.hpp"
#include <stdlib.h>
#include <string.h>

extern "C" {
extern uint8_t network_disconnect_timeout_flag;
extern void network_timer_advance(network_timer_pair *timer);
extern void network_timer_increment_clamped(network_timer_pair *timer, int32_t upper_bound, int32_t increment);
extern void network_timer_decrement_floored(network_timer_pair *timer, int32_t decrement);
extern void network_timer_start(network_timer_pair *timer, int32_t duration_ms);
extern char network_game_all_machines_have_player(network_server_globals *server);
extern char network_game_any_team_empty(network_server_globals *server);
extern void time_query_performance_counter_ms(void);
extern char network_channel_short_disconnect_timeout(void);
extern int32_t network_server_count_connected_machines(network_server_globals *server);
extern network_client_globals *network_client;
extern uint8_t network_host_handoff_requested;
extern char network_player_entry_validate(network_player_entry *entry);
extern void network_session_info_packet_send(network_client_globals *client);
extern uint32_t network_send_join_request_packet(network_client_globals *client);
extern void chat_close(void);
extern network_server_globals *network_server;
extern uint8_t network_disconnect_notice_shown;
extern uint8_t local_player_globals[8];
extern void display_error(int32_t code, int32_t player_index, uint8_t flag_a, uint8_t flag_b);
extern uint32_t network_game_socket_port;
extern int32_t progress_screen_text;
extern wchar_t *string_convert_ascii_to_unicode(wchar_t *dest, int32_t dest_bytes, const char *source);
extern uint32_t network_client_begin_connect(wchar_t *player_name, s_network_address *target_address);
extern void message_delta_parameters_protocol_dump_to_config_file(void);
extern void player_update_history_destroy(player_update_history *history);
extern void network_channel_delete(network_channel *channel);
extern void network_stats_summary_log_write(void);
extern uint8_t network_session_active;
extern int32_t data_packet_group_encode_packet(uint8_t *buffer, data_packet_group *group, void *payload, int32_t *capacity, int32_t message_type, int32_t flag);
extern data_packet_group network_game_messages_group;
extern uint16_t network_challenge_packet_block[];
extern uint16_t *network_message_block_build(uint16_t *buffer, uint32_t *source, uint8_t flags, uint32_t length);
extern char network_channel_stream_flush(network_channel_stream *stream, network_channel *channel, char mode);
extern int32_t bit_stream_write_bits_chunked(bit_stream *stream, const uint32_t *values, int32_t total_bit_count);
extern uint8_t server_browser_join_target_has_password;
extern uint16_t network_join_target_address[128];
extern uint16_t empty_string;
extern int16_t network_join_error_code;
extern int32_t network_join_error_reason;
extern uint8_t split_screen_quit_prompt_string[4];
extern uint32_t network_game_client_connect_to_address(char *address_string, uint16_t *target_string);
extern uint32_t gt2NetworkToHostShort(int16_t value);
extern void gt2AddressToString(uint32_t address, uint16_t port, void *out_address);
extern void console_printf_verbose(const char *format, ...);
extern int32_t interface_loading_screen_progress;
extern int32_t join_ui_state;
extern int16_t network_game_mode;
extern int32_t interface_loading_screen_request_id;
extern const char network_ellipsis_dots[];
}

namespace halo::networking {

/**
 * out/phase4/networking_functions.md: "State machine that drives the client
 * connection handshake/timeout: while connected, updates the disconnect-timeout timer per
 * incoming message id, and while not yet connected, (re)starts the timeout ti[mer]." The four
 * FUN_004debXX callees are types/networking.h's network_timer_pair helpers, already rewritten
 * in this repo (network_timer_advance/_start/_increment_clamped/_decrement_floored).
 *
 * @address 0x4e0590
 */
void ClientView::connection_handshake_tick(int16_t state, network_server_globals *owner)
{
    network_timer_pair *timer;
    uint8_t *base;

    base = (uint8_t *)owner;
    timer = (network_timer_pair *)(base + 0x9c8);

    if (*(uint8_t *)(base + 0x9d5) != 0) {
        return;
    }
    if (!((network_game_all_machines_have_player(owner) != 0 &&
           network_game_any_team_empty(owner) == 0) ||
          state == 2)) {
        return;
    }

    if (*(uint8_t *)(base + 0x9d4) == 1) {
        if (*(uint8_t *)(base + 0x9d6) != 0) {
            return;
        }
        switch (state) {
        case 0:
            *(uint8_t *)(base + 0x9d6) = 1;
            network_timer_increment_clamped(timer, 0, 0);
            return;
        case 1:
            *(uint8_t *)(base + 0x9d6) = 1;
            network_timer_advance(timer);
            if (timer->remaining_ms > 999) {
                network_timer_decrement_floored(timer, 0);
                network_timer_advance(timer);
                if (timer->remaining_ms < 999) {
                    network_timer_start(timer, 999);
                    return;
                }
            }
            break;
        case 2:
            *(uint8_t *)(base + 0x9d6) = 1;
            *(uint8_t *)(base + 0x9d4) = 0;
            return;
        case 3:
            *(uint8_t *)(base + 0x9d6) = 1;
            network_timer_start(timer, 0);
            return;
        default:
            break;
        }
    } else {
        char ready;
        int16_t connected_count;

        time_query_performance_counter_ms();
        if (state == 3) {
            network_timer_start(timer, 0);
            *(uint8_t *)(base + 0x9d4) = 1;
            *(uint8_t *)(base + 0x9d6) = 0;
            return;
        }
        if (network_disconnect_timeout_flag == 0 ||
            (connected_count = network_server_count_connected_machines(owner), connected_count > 0)) {
            ready = network_channel_short_disconnect_timeout();
            *(uint8_t *)(base + 0x9d4) = 1;
            network_timer_start(timer, ready != 0 ? 10999 : 30999);
            *(int32_t *)(base + 0x9d0) = 0;
            *(uint8_t *)(base + 0x9d6) = 0;
        }
    }
}

/**
 * out/phase4/networking_functions.md: "Finds the channel-key entry matching the
 * caller's key and parameter and, unless a follow-up check succeeds, flags DAT_0071c2de and
 * calls FUN_004aa900 (likely to force a host handoff or disconnect)." The scanned array (client
 * treated as short*, +0x669 shorts == byte +0xcd2) matches network_client->session.players[]'s
 * machine_index/machine_player_index fields exactly (same evidence as
 * network_game_session_reset.c and network_player_entry_add.c). network_session_info_packet_send and
 * network_send_join_request_packet are already named by the batch covering 0x4d8a80..0x4d9340.
 *
 * @address 0x4de390
 */
void ClientView::rejoin_check(int8_t machine_player_index)
{
    int16_t own_id;
    int32_t i;
    network_player_entry *player;
    uint32_t send_result;

    if (network_client != 0) {
        own_id = *(int16_t *)network_client;
        if (own_id != -1) {
            for (i = 0; i < 0x10; i++) {
                player = &network_client->session.players[i];
                if (network_player_entry_validate(player) != 0 && (int32_t)player->machine_index == (int32_t)own_id &&
                    player->machine_player_index == machine_player_index) {
                    if (player == 0) {
                        return;
                    }
                    network_session_info_packet_send(network_client);
                    send_result = network_send_join_request_packet(network_client);
                    if ((char)send_result != 0) {
                        return;
                    }
                    network_host_handoff_requested = 1;
                    chat_close();
                    return;
                }
            }
        }
    }
}

/**
 * out/phase4/networking_functions.md summary ("Either leaves an already-flagged
 * session alone or triggers a network disconnect/cleanup after defaulting the retry-limit
 * field"). client+0xedc matches types/networking.h's network_client_globals::unknown_edc
 * exactly; network_server+6 bit2 matches network_server_globals::flags's documented
 * "bit2 stats logging".
 *
 * @address 0x4d9ce0
 */
void ClientView::timer_default_or_disconnect()
{
    network_client_globals *client = self;
    if (client->disconnect_reason == 0) {
        client->disconnect_reason = 8;
    }
    if (network_server != 0 && (network_server->flags >> 2 & 1) != 0) {
        return;
    }
    network_host_handoff_requested = 1;
    chat_close();
}

/**
 * out/phase4/networking_functions.md summary ("Displays a disconnect-notification
 * error message for each machine that dropped from the session"). client+0xee0 matches
 * types/networking.h's network_client_globals::unknown_ee0 exactly.
 *
 * @address 0x4d9340
 */
void ClientView::disconnect_notify_dropped_machines()
{
    network_client_globals *client = self;
    int16_t player_index_16;
    int32_t player_index;
    int32_t next;

    if (network_disconnect_notice_shown != 0) {
        return;
    }
    if (client->network_error_displayed == 0) {
        player_index = -1;
        if (*(int32_t *)&local_player_globals[4] != -1) {
            player_index = 0;
        }
        player_index_16 = (int16_t)player_index;
        while (player_index_16 != -1) {
            display_error(8, player_index, 1, 0);
            next = -1;
            if (*(int32_t *)&local_player_globals[4] != -1 && (int16_t)player_index < 0) {
                next = 0;
            }
            player_index = next;
            player_index_16 = (int16_t)next;
        }
    }
    client->network_error_displayed = 1;
}

/**
 * already named)
 * address 0x4dc790, size 314 bytes
 * name confidence: 0.5   rewrite confidence: 0.45
 * out/phase4/networking_functions.md: "Parses a user-entered address[:port] string,
 * resolves it to a numeric address, and kicks off the connect handshake." Confirmed against
 * objdump -d -M intel bin/halo.exe at 0x4dc790..0x4dc8c9: the address-string argument is EAX
 * (saved into EBX at entry, in_EAX in Ghidra's decompile); the four inet_addr() calls per branch
 *
 * @address 0x4dc790
 */
uint32_t ClientView::client_connect_to_address(wchar_t *player_name, char *address_string)
{
    char host_part[256];
    char *colon;
    int i;
    s_network_address target;
    uint32_t byte0, byte1, byte2, byte3;

    for (i = 0; ; i++) {
        host_part[i] = address_string[i];
        if (address_string[i] == '\0') {
            break;
        }
    }
    colon = strchr(host_part, ':');
    if (colon == 0) {
        byte0 = inet_addr(address_string);
        byte1 = inet_addr(address_string);
        byte2 = inet_addr(address_string);
        byte3 = inet_addr(address_string);
        target.ipv4 = ((byte0 & 0xff0000 | byte1 >> 0x10) >> 8) |
                      ((byte2 << 0x10 | byte3 & 0xff00) << 8);
        target.size = k_network_address_size_ipv4;
        target.port = (uint16_t)network_game_socket_port;
    } else {
        *colon = '\0';
        byte0 = inet_addr(host_part);
        byte1 = inet_addr(host_part);
        byte2 = inet_addr(host_part);
        byte3 = inet_addr(host_part);
        target.ipv4 = ((byte0 & 0xff0000 | byte1 >> 0x10) >> 8) |
                      ((byte2 << 0x10 | byte3 & 0xff00) << 8);
        target.size = k_network_address_size_ipv4;
        target.port = (uint16_t)atol(colon + 1);
    }
    if (address_string == 0) {

        progress_screen_text = 0;
    } else {
        string_convert_ascii_to_unicode(0, 0, address_string);

    }
    return network_client_begin_connect(player_name, &target);
}

/**
 * types/networking.h "network_client_globals (0x4d8a80 network_session_create,
 * 0x4d8b70 destroy)"; the two offsets this function writes (+0xf48, +0xadc) are exactly
 * update_history and channel, matching out/phase4/networking_types_notes.md's
 * "network_session_destroy (0x4d8b70) frees the +0xf48 list and deletes the +0xadc channel".
 *
 * @address 0x4d8b70
 */
void ClientView::destroy()
{
    network_client_globals *client = self;
    message_delta_parameters_protocol_dump_to_config_file();
    if (client != 0) {
        player_update_history_destroy((player_update_history *)client->update_history);
        client->update_history = 0;
        if (client->channel != 0) {
            network_channel_delete(client->channel);
        }
        network_session_active = 0;
    }
    network_stats_summary_log_write();
}

/**
 * out/phase4/networking_functions.md summary ("Encodes and queues an outgoing
 * join-request packet (packet type 0x1e) on the given connection"); shares the channel+0xa8c/
 * +0x24/+0x1c/+0x20/+0xa80/+0x2c raw-offset idiom already used (and left unresolved) in
 * network_session_info_packet_send.c and src/game/game_engine_send_team_allegiance_message.c.
 *
 * @address 0x4d9220
 */
int32_t ConnectionView::send_join_request_packet()
{
    network_client_globals *connection = self;
    uint8_t encoded[0x600];
    uint32_t payload;
    int32_t capacity;
    uint16_t *record;
    network_channel *channel;
    int32_t bits_to_send;
    int32_t total_bits;
    char result;
    uint32_t item_flag;

    if (network_server == 0 || ((*(uint8_t *)((uint8_t *)network_server + 6) >> 2 & 1) == 0)) {
        network_host_handoff_requested = 1;
        chat_close();
    }

    capacity = 0x600;
    if ((char)data_packet_group_encode_packet(encoded, &network_game_messages_group, &payload, &capacity, 0x1e, 1) == 0) {
        return 0;
    }

    record = network_message_block_build(network_challenge_packet_block, (uint32_t *)encoded, 3,
                                        (uint32_t)capacity);
    if (record == 0) {
        return 0;
    }

    channel = connection->channel;
    bits_to_send = (int32_t)(*record >> 4) * 8;
    total_bits = bits_to_send + 1;
    result = 1;
    if ((channel->flags & 1) == 0) {
        if ((int32_t)(channel->outgoing.stream.last_bit - channel->outgoing.stream.byte_cursor * 8 -
                      channel->outgoing.stream.bit_cursor) + 1 < total_bits) {
            result = network_channel_stream_flush(&channel->outgoing, channel, 1);
            if (result == 0) {
                return 0;
            }
        }
        channel->send_budget = channel->send_budget + total_bits;
        item_flag = 0;
        bit_stream_write_bits_chunked(&channel->outgoing.stream, &item_flag, 1);
        channel->outgoing.empty = 0;
        bit_stream_write_bits_chunked(&channel->outgoing.stream, (const uint32_t *)record, bits_to_send);
        channel->outgoing.empty = 0;
    }
    return result;
}

/**
 * 0x16-byte address record (stride confirmed by the imul esi,esi,0x16 at 0x6148c8)  // outside this batch
 *
 * @address 0x4ba270
 */
void JoinView::hostname_resolved_callback(int32_t resolve_failed, uint32_t unused, uint8_t *hostent)
{
    char resolved_address[22];

    (void)unused;

    if (hostent != 0) {
        uint32_t address = *(uint32_t *)(hostent + 4);
        uint32_t port = gt2NetworkToHostShort(*(int16_t *)(hostent + 2));

        gt2AddressToString(address, port, resolved_address);
    }

    if (resolve_failed == 0) {
        uint16_t *address_string = server_browser_join_target_has_password == 0
            ? &empty_string
            : network_join_target_address;

        network_game_client_connect_to_address(resolved_address, address_string);
        network_join_target_address[0] = 0;
        server_browser_join_target_has_password = 0;
        return;
    }

    if (network_join_error_code == -1) {
        network_join_error_code = 0x2b;
    }
    network_join_error_reason = 0;
    *(uint16_t *)split_screen_quit_prompt_string = 0xffff;
    split_screen_quit_prompt_string[3] = 1;
}

/**
 * out/phase4/networking_functions.md summary ("Updates the on-screen join/connect
 * status text ('Loading', 'Connecting', or an animated 'Connecting...') and the associated UI
 * state machine based on an integer mode selector").
 *
 * @address 0x4db4c0
 */
void JoinView::status_text_update(int32_t mode)
{
    network_client_globals *client = self;
    network_connection_attempt_state *attempt = &client->connect_attempt;
    char dots[17];
    int32_t count;

    if (mode == 0) {
        attempt->elapsed_counter = 0;
        console_printf_verbose("Connecting");
        interface_loading_screen_progress = 0;
        join_ui_state = 5;
    } else if (mode == 1) {
        memset(dots, 0, sizeof(dots));
        count = attempt->elapsed_counter + 1;
        attempt->elapsed_counter = count;
        if (count < 0) {
            count = 0;
        } else if (count > 0x10) {
            count = 0x10;
        }
        strncpy(dots, network_ellipsis_dots, count);
        console_printf_verbose("Connecting%s", dots);
        interface_loading_screen_progress = attempt->elapsed_counter;
        if (join_ui_state != 1 && join_ui_state != 2) {
            if (join_ui_state == 4) {
                interface_loading_screen_request_id = -1;
                return;
            }
            join_ui_state = 6;
            return;
        }
    } else {
        attempt->elapsed_counter = 0;
        console_printf_verbose("Loading");
        interface_loading_screen_progress = 0;
        if (network_game_mode == 2) {
            if (join_ui_state != 1) {
                if (join_ui_state != 2 && join_ui_state == 4) {
                    interface_loading_screen_request_id = -1;
                }
                join_ui_state = 8;
                return;
            }
        } else if (join_ui_state != 1 && join_ui_state != 2) {
            if (join_ui_state == 4) {
                interface_loading_screen_request_id = -1;
                interface_loading_screen_progress = 0;
                return;
            }
            join_ui_state = 7;
            return;
        }
    }
}

}
