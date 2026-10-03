#include "halo/networking/net1_server.hpp"
#include <string.h>
#include <wchar.h>
#include <stdio.h>
#include "halo/memory/api.hpp"
#include "halo/cseries/api.hpp"
#include "halo/game/api.hpp"
#include "halo/interface/api.hpp"

extern "C" {
extern uint16_t *network_prepare_challenge_packet(int32_t message_type, void *payload);
extern uint8_t network_session_send_to_machine(int32_t machine_id, network_server_globals *server, uint32_t status_bit, void *data, uint32_t bits, uint32_t reliable, uint32_t unknown_a, char force, uint32_t priority);
extern data_packet_group network_game_messages_group;
extern uint32_t network_game_session_finalize_and_add_player(network_player_entry *entry, network_server_globals *server, network_machine *machine);
extern uint32_t network_game_broadcast_player_set_changed(uint8_t *param_1);
extern uint8_t network_server_notify_or_resend_challenge(int16_t reason, network_machine *machine, network_server_globals *server);
extern uint8_t network_game_info_packet_flag;
extern char network_server_build_full_game_info_packet(network_machine *machine);
extern void network_channel_remote_address_or_default(network_channel *channel, network_resolved_address *out_address);
extern uint8_t network_join_request_reset_state(network_machine *machine, const char *response);
extern void network_debug_fill_canary_buffer(uint32_t *buffer);
extern void network_server_password_get(network_server_globals *server, wchar_t *dest);
extern int32_t network_server_password_is_set(network_server_globals *server);
extern int32_t network_machine_reset(network_machine *machine);
extern network_server_globals *network_server;
extern int32_t network_scenario_round_counter_a;
extern int32_t network_scenario_round_counter_b;
extern uint8_t network_statistics_logging_enabled;
extern FILE *network_summary_log_file;
extern char network_build_string[];
extern char network_game_scenario_load_request(network_game_session *session);
extern data_array *player_data;
extern uint32_t player_data_iterator_advance(int16_t step_count);
extern uint16_t *network_message_block_build(uint16_t *dest, uint32_t *buffer, uint8_t flags, uint32_t length);
extern uint16_t network_challenge_packet_block[];
extern char network_channel_stream_flush(network_channel_stream *stream, network_channel *channel, char mode);
extern char *autopatch_temp_name_generate(void);
typedef struct network_game_info_record {
    char short_name[7];
    uint8_t nul;
    uint8_t flag;
    uint8_t pad[3];
    int32_t machine_id;
    uint8_t snapshot[0x84];
    uint8_t scratch[0x600];
} network_game_info_record;

extern network_client_globals *network_client;
extern int32_t network_console_connection_id;
extern uint8_t network_message_scratch[0x7ff8];
extern char network_player_entry_validate(network_player_entry *entry);
extern uint32_t network_game_settings_broadcast_send(uint32_t round, uint32_t *record);
extern uint32_t network_player_entry_remove(network_player_entry *key, network_game_session *session);
extern void network_channel_remove_child(network_channel *channel);
extern void gcd_disconnect_user(int32_t id, int32_t value);
extern void gcd_disconnect_all(int32_t id);
extern void message_delta_parameters_protocol_send_update(void);
extern int32_t message_delta_encode_message(int32_t extra_eax, int32_t extra_edx, int32_t flag, int32_t message_type, int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed);
extern char network_session_broadcast_to_all(network_server_globals *server, int32_t param_1, void *data, int32_t param_3, int32_t param_4, char force, int32_t param_6);
extern char network_channel_service(int32_t mode);
extern void network_machine_timer_start(network_machine *machine, int32_t duration_ms);
extern char network_server_check_machine_timeout(network_server_globals *server, network_machine *machine);
extern char network_channel_drain_bitstream(network_server_globals *server, network_machine *machine);
extern char network_channel_queue_message(network_channel *channel, uint32_t header_value, uint32_t body_value, int32_t header_bit_count, char immediate, char flush_after, int32_t body_bit_count);
extern player_globals *local_player_globals;
extern void *variant_defaults_source;
extern uint8_t game_engine_pending_variant[0x98];
extern int32_t join_ui_state;
extern int32_t ui_root_widget;
extern int32_t ui_widget_history;
extern uint8_t ui_pause_depth;
extern int32_t controls_capture_row;
extern uint8_t controls_input_capture_flags;
extern uint8_t controls_input_capture_buffer[0x280];
extern void message_delta_parameters_protocol_dump_to_config_file(void);
extern void network_stats_summary_log_write(void);
extern void message_delta_protocol_initialize(void);
extern void network_stats_summary_log_open(void);
extern char network_game_server_load_scenario(void);
extern void network_host_full_state_broadcast(network_server_globals *host);
extern void network_client_timer_schedule(int32_t a, int32_t b);
}

namespace halo::networking {

/**
 * EAX machine, ECX server, EDX buffer, stack length: while
 * the host is not in a game, the decoded body is the player to add; failure sends reason 3, success broadcasts the
 * player set and sends a type 0xa accept. Returns 1.
 *
 * @address 0x4e2400
 */
char ServerView::handle_join_confirm(network_machine *machine, uint8_t *buffer, int32_t length)
{
    network_server_globals *server = self;
    uint16_t out_version;
    int16_t out_type;
    uint8_t body[0x20];
    int16_t remaining;

    if (server->state != 0 && server->state != 1) {
        return 1;
    }
    remaining = (int16_t)(length - 2);
    if (halo::memory::data_packet_group_decode_packet(&remaining, &network_game_messages_group, body, buffer + 2, &out_type, &out_version, 3) == 0) {
        return 1;
    }
    if (network_game_session_finalize_and_add_player((network_player_entry *)body, server, machine) == 0) {
        network_server_notify_or_resend_challenge(3, machine, server);
        return 1;
    }
    if (network_game_broadcast_player_set_changed((uint8_t *)server) != 0) {
        uint32_t payload = 0;
        uint16_t *packet = network_prepare_challenge_packet(0xa, &payload);

        if (packet != 0 && machine->machine_id != -1) {
            network_session_send_to_machine(machine->machine_id, server, 0, packet, (uint32_t)(*packet >> 4) << 3, 1, 0, 1, 3);
        }
    }
    return 1;
}

/**
 * the host's join request handler (message 0xe; EBX machine,
 * stack server, buffer, length). While the host is not in a game (+4 0 or 1) and the machine is not already joining
 * (+0xe bit 1): a machine without a connected channel after the game ended gets the full game info. Otherwise the
 * request (a 0x84 byte body) is decoded; unless the server accepts joins (+6 bit 0, state 0 or 1) it is refused
 * (0); the CD key response at body +0x22 must pass the host check (else 6); the first 16 bytes must match the
 * version canary (else 1); the 8-character password at body +0x10 must match a set password (else 2); the machine
 * is reset and its player (body +0x6e) added (else 3), the player set is broadcast, the channel rate (+0xa88)
 *
 * @address 0x4e21d0
 */
char ServerView::handle_join_password(network_machine *machine, uint8_t *buffer, int32_t length)
{
    network_server_globals *server = self;
    int16_t out_type;
    uint16_t out_version;
    uint32_t scratch[6];
    uint8_t body[0x90];
    int16_t remaining;
    int16_t reason;

    if (server->state != 0 && server->state != 1) {
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
    if (halo::memory::data_packet_group_decode_packet(&remaining, &network_game_messages_group, body, buffer + 2, &out_type, &out_version,
                                         3) == 0) {
        return 1;
    }
    network_channel_remote_address_or_default(machine->channel, (network_resolved_address *)scratch);
    if ((server->flags & 1) == 0 || (server->state != 0 && server->state != 1)) {
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

            network_game_broadcast_player_set_changed((uint8_t *)server);
            *(int32_t *)((uint8_t *)machine->channel + 0xa88) = network_game_info_packet_flag == 0 ? 4 : body[0x6b];
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

/**
 * Resets the two per-round counters and loads the pending scenario for `network_server`'s
 * session, optionally logging the build string to the summary log when verbose,
 * statistics-enabled logging is active.
 *
 * @address 0x4e0720
 */
char ServerView::load_scenario()
{
    network_server_globals *server;
    char ok;

    server = network_server;
    network_scenario_round_counter_a = 0;
    network_scenario_round_counter_b = 0;
    ok = network_game_scenario_load_request(&server->session);
    if ((server->flags >> 2 & 1) != 0 && halo::cseries::globals().debug_log_level > 2 &&
        network_statistics_logging_enabled != 0 && network_summary_log_file != 0) {
        fprintf(network_summary_log_file, "%s\t", network_build_string);
    }
    return ok;
}

/**
 * When the session flag at server->session.unknown_3ac is set, records `sender` as the last
 * object to update the resolved player's record (player->unknown_d0), provided the resolve
 * succeeded, the resolved index is non-zero, and sender is a valid handle.
 *
 * @address 0x4df900
 */
uint32_t ServerView::record_last_sender(int32_t sender, int16_t step_count)
{
    network_server_globals *server = self;
    uint32_t resolved;

    resolved = player_data_iterator_advance(step_count);
    if (resolved == 0xffffffff) {
        return 0;
    }
    if (server->session.map_loaded != 0 && resolved != 0 && sender != -1) {
        *(int32_t *)((uint8_t *)player_data->data + (resolved & 0xffff) * 0x200 + 0xd0) = sender;
    }
    return 1;
}

/**
 * Encodes a type-7 "full game info" message into a 0x600-byte scratch record, then -- unless
 * machine's channel already reports connected -- queues it onto that channel's outgoing
 * stream (flushing first if there isn't room, and again after queuing). Marks the machine's
 * flags bit 0x10 ("answered") on success.
 *
 * @address 0x4e0bd0
 */
char ServerView::build_full_game_info_packet(network_machine *machine)
{
    uint8_t record[0x600];
    uint32_t payload = 0;
    int16_t size;
    char encoded;
    uint16_t *encoded_buffer;
    int32_t bit_len;
    char ok;
    network_channel *channel;

    size = 0x600;
    encoded = halo::memory::data_packet_group_encode_packet(&network_game_messages_group, record, &payload, &size, 7, 1);
    if (encoded == 0) {
        return 0;
    }
    encoded_buffer = network_message_block_build(network_challenge_packet_block, (uint32_t *)record, 3, (uint32_t)size);
    if (encoded_buffer == 0) {
        return 0;
    }
    bit_len = (int32_t)(*encoded_buffer >> 4) * 8;
    ok = 0;

    channel = (machine != 0) ? machine->channel : 0;
    if (machine != 0 && channel != 0 && channel->connected != 1) {
        int32_t total_bits;

        total_bits = bit_len + 1;
        if ((channel->flags & 0x01) == 0) {
            int32_t free_bits;

            free_bits = (int32_t)(channel->outgoing.stream.last_bit -
                                  channel->outgoing.stream.byte_cursor * 8) -
                        (int32_t)channel->outgoing.stream.bit_cursor + 1;
            if (free_bits < total_bits) {
                char flushed;

                flushed = network_channel_stream_flush(&channel->outgoing, channel, 1);
                ok = 0;
                if (flushed == 0) {
                    goto done;
                }
            }
            channel->send_budget = channel->send_budget + total_bits;
            { uint32_t item_flag = 0; halo::memory::bit_stream_write_bits_chunked((bit_stream *)((uint8_t *)channel + 0x10), &item_flag, 1); }
            channel->outgoing.empty = 0;
            halo::memory::bit_stream_write_bits_chunked((bit_stream *)((uint8_t *)channel + 0x10), (const uint32_t *)(encoded_buffer), bit_len);
            channel->outgoing.empty = 0;
            ok = network_channel_stream_flush(&channel->outgoing, channel, 1);
        } else {
            ok = 1;
        }
    }
done:
    if (ok == 0) {
        return 0;
    }
    machine->flags |= 0x10;
    return ok;
}

/**
 * out/phase4/networking_functions.md: "Builds and queues a small 'game info' style
 * packet (short name plus a game-data snapshot) for the channel referenced by param_2, encoded
 * as message type 4." param_2+0x0/+0xc match network_machine::channel/machine_id;
 * param_1+0x88 (server + 0x88 == session + 0x80) copies exactly 0x84 bytes -- session's
 * unknown_080, server_name[64] and unknown_0c4[0x40] back to back -- into a scratch snapshot.
 * The free-space/flush sequence on the result matches network_channel::outgoing
 * (bit_stream last_bit/byte_cursor/bit_cursor at +0x24/+0x1c/+0x20) and
 *
 * @address 0x4e0950
 */
char ServerView::build_game_info_packet(network_machine *machine)
{
    network_server_globals *server = self;
    network_game_info_record record;
    int16_t size;
    char *source_name;
    char encoded;

    source_name = autopatch_temp_name_generate();
    strncpy((char *)machine + 0x52, source_name, 7);
    *((char *)machine + 0x59) = 0;

    strncpy(record.short_name, (char *)machine + 0x52, 7);
    record.nul = 0;
    memcpy(record.snapshot, (uint8_t *)server + 0x88, sizeof(record.snapshot));
    record.flag = network_game_info_packet_flag;
    record.machine_id = (int32_t)machine->machine_id;

    size = 0x600;
    encoded = halo::memory::data_packet_group_encode_packet(&network_game_messages_group, record.scratch, &record, &size, 4, 1);
    if (encoded != 0) {
        uint16_t *encoded_buffer;

        encoded_buffer = network_message_block_build(network_challenge_packet_block, (uint32_t *)record.scratch, 3, (uint32_t)size);
        if (encoded_buffer != 0) {
            network_channel *channel;

            channel = machine->channel;
            if (channel != 0) {
                int32_t bit_len;
                char result;

                bit_len = (int32_t)(*encoded_buffer >> 4) * 8;
                result = 1;
                if ((channel->flags & 0x01) == 0) {
                    int32_t free_bits;

                    free_bits = (int32_t)(channel->outgoing.stream.last_bit -
                                          channel->outgoing.stream.byte_cursor * 8) -
                                (int32_t)channel->outgoing.stream.bit_cursor + 1;
                    if (free_bits < bit_len + 1) {
                        result = network_channel_stream_flush(&channel->outgoing, channel, 1);
                        if (result == 0) {
                            return 0;
                        }
                    }
                    channel->send_budget = channel->send_budget + bit_len + 1;
                    { uint32_t item_flag = 0; halo::memory::bit_stream_write_bits_chunked((bit_stream *)((uint8_t *)channel + 0x10), &item_flag, 1); }
                    channel->outgoing.empty = 0;
                    halo::memory::bit_stream_write_bits_chunked((bit_stream *)((uint8_t *)channel + 0x10), (const uint32_t *)(encoded_buffer), bit_len);
                    channel->outgoing.empty = 0;
                }
                return result;
            }
        }
    }
    return 0;
}

/**
 * this module, 0x4e19c0
 *
 * @address 0x4e0ef0
 */
int32_t ServerView::check_machine_timeout(network_machine *machine)
{
    network_server_globals *server = self;
    large_integer counter;
    uint32_t now_ms;
    uint32_t timer_18;
    uint32_t timer_14;
    int32_t result;

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    now_ms = (uint32_t)((counter.quad_part * 1000) / halo::cseries::globals().performance_frequency);
    timer_18 = (uint32_t)machine->timer_18;
    timer_14 = (uint32_t)machine->timer_14;
    result = 0;

    if (timer_14 <= timer_18) {
        if (timer_18 <= now_ms || now_ms < timer_14) {
            goto not_timed_out;
        }
        if (timer_14 <= timer_18) {
            return 1;
        }
    }
    if (now_ms < timer_18 || timer_14 <= now_ms) {
        return 1;
    }

not_timed_out:
    if (machine->player_joined == 0) {
        int16_t machine_id;

        machine_id = machine->machine_id;
        if (machine_id != -1) {
            network_player_entry *entry;
            int32_t remaining;
            int32_t i;

            entry = server->session.players;
            remaining = 16;
            do {
                network_player_entry copy;

                memcpy(&copy, entry, sizeof(copy));
                if (network_player_entry_validate(&copy) != 0 &&
                    (int32_t)copy.machine_index == machine_id) {
                    if (network_game_settings_broadcast_send((uint32_t)server, (uint32_t *)&copy) != 0) {
                        network_player_entry_remove(&copy, &server->session);
                        if (network_client != 0 && (int32_t)network_client != -0xb14 &&
                            (machine->flags >> 2 & 1) != 0) {
                            network_player_entry_remove(&copy, &network_client->session);
                        }
                    }
                }
                entry = entry + 1;
                remaining = remaining - 1;
            } while (remaining != 0);

            for (i = 0; i < 16; i = i + 1) {
                if (&server->machines[i] == machine) {
                    if (machine->channel != 0) {
                        network_channel_remove_child(machine->channel);
                    }
                    machine->channel = 0;
                    machine->unknown_04 = 0;
                    machine->unknown_08 = 0;
                    machine->machine_id = -1;

                    machine->flags = 0;
                    machine->unknown_0f = 0;
                    *(int32_t *)((uint8_t *)machine + 0x52) = 0;
                    *(int32_t *)((uint8_t *)machine + 0x56) = 0;
                    if (machine->gcd_user_id == -1) {
                        gcd_disconnect_all(network_console_connection_id);
                    } else {
                        gcd_disconnect_user(network_console_connection_id, machine->gcd_user_id);
                    }
                    machine->player_joined = 0;
                    machine->players_removed_broadcast = 0;
                    machine->gcd_user_id = -1;

                    message_delta_parameters_protocol_send_update();
                    {
                        network_game_session *session_ptr;
                        int32_t encoded;

                        session_ptr = &server->session;
                        encoded = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 0x21, 0, (void **)&session_ptr, 0, 1, 0);
                        if (encoded > 0) {
                            network_session_broadcast_to_all(network_server, 1, network_message_scratch,
                                1, 0, 1, 3);
                        }
                    }
                    return 2;
                }
            }
        }
        return 0;
    }

    if (machine->players_removed_broadcast != 0) {
        return 1;
    }
    {
        char any_valid;
        network_player_entry *entry;
        int32_t i;
        int32_t local_result;

        any_valid = 0;
        entry = server->session.players;
        local_result = 0;
        for (i = 0; i < 16; i = i + 1) {
            if (network_player_entry_validate(entry) != 0 &&
                (int16_t)entry->machine_index == machine->machine_id) {
                if (network_game_settings_broadcast_send((uint32_t)server, (uint32_t *)entry) == 0) {
                    local_result = 0;
                    break;
                }
                any_valid = 1;
                local_result = 1;
            }
            entry = entry + 1;
        }
        if (!any_valid) {
            local_result = 1;
        }
        machine->players_removed_broadcast = 1;
        return local_result;
    }
}

/**
 * For each of the 16 machine slots with a connected id: if the machine is still "pending"
 * (unknown_10 == 0), either restarts its timeout timer (when its channel is dead, unservicable,
 * missing the client/transmit-pending flags, or lacks a connection-oriented endpoint) or drains
 * its incoming bitstream, restarting the timer again if that fails. Once the machine is no
 * longer pending, checks it for timeout. Returns false as soon as any machine's timeout check
 * fails.
 *
 * @address 0x4e11d0
 */
char ServerView::service_machines_tick()
{
    network_server_globals *server = self;
    int32_t i;
    char ok;
    char skip_timeout_check;

    ok = 1;
    for (i = 0; i < 16; i = i + 1) {
        network_machine *machine;

        machine = &server->machines[i];
        skip_timeout_check = 0;

        if (machine->machine_id != -1) {
            if (machine->disconnect_timer_active == 0) {
                network_channel *channel;
                char service_ok;
                char proceed;

                channel = machine->channel;
                proceed = 1;
                if ((channel->flags & 0x10) != 0) {
                    proceed = 0;
                } else {
                    service_ok = network_channel_service(0);
                    if (service_ok == 0) {
                        proceed = 0;
                    } else if ((channel->flags & 0x06) == 0) {
                        proceed = 0;
                    } else if (channel->endpoint == 0 || (channel->endpoint->flags & 0x01) == 0) {
                        proceed = 0;
                    }
                }

                if (!proceed) {
                    network_machine_timer_start(machine, 0);
                } else {
                    char drained;

                    drained = network_channel_drain_bitstream(server, machine);
                    if (drained == 0) {
                        network_machine_timer_start(machine, 0);
                    }
                    ok = 1;

                }

                if (machine->disconnect_timer_active == 0) {
                    skip_timeout_check = 1;
                }
            }

            if (!skip_timeout_check && network_server_check_machine_timeout(server, machine) == 0) {
                ok = 0;
            }
        }

        if (ok == 0) {
            return 0;
        }
    }
    return ok;
}

/**
 * Same qualifying test as network_session_broadcast_to_all, plus flags bit 0x04 (not one of
 * the enumerated network_machine_flags).
 * FIXED (objdump 0x4e1a8d, 0x4e1aef..0x4e1b13): the original also takes EAX -- the message length in bits, which it
 * forwards in EBX to network_channel_queue_message -- and queues on each qualifying machine's channel (EDI) with
 * stack (data, &status, 1, immediate, flush_after); the sixth push (unused) is not read by the callee.
 *
 * @address 0x4e1a80
 */
char ServerView::broadcast_to_flagged(int32_t body_bit_count, int32_t status_bit, void *data, int32_t immediate, int32_t flush_after, char force, int32_t unused)
{
    network_server_globals *server = self;
    char ok;
    int32_t i;

    ok = 1;
    for (i = 0; i < 16; i = i + 1) {
        network_machine *machine;
        network_channel *channel;
        char connected;
        uint8_t flags;

        machine = &server->machines[i];
        channel = machine->channel;
        connected = (channel != 0) ? channel->connected : 0;
        flags = machine->flags;

        if ((flags & 0x02) != 0 && (flags & 0x04) != 0 &&
            (connected != 1 || force != 0) &&
            channel != 0 &&
            (channel->flags & 0x10) == 0) {
            uint8_t status;
            char sent;

            status = (uint8_t)(status_bit != 0);
            sent = network_channel_queue_message(channel, (uint32_t)data, (uint32_t)&status, 1, (char)immediate,
                                                 (char)flush_after, body_bit_count);
            if (sent == 0) {
                ok = 0;
            }
        }
    }
    return ok;
}

/**
 * Finds the machine slot whose machine_id matches `machine_id` and, if it has a live channel
 * that is either connected or `force` is set, forwards the send through network_channel_queue_message.
 * FIXED (objdump 0x4e1930..0x4e19b8): the send is skipped only when the channel's flag at +0xa98 is 1 AND force is 0
 * (the draft had that inverted); the queue call is (EDI channel, EBX = body_bit_count bits, stack data, &status, 1, reliable,
 * unknown_a) with status = (status_bit != 0); the result is AL -- 0, or the queue's result when a message was queued.
 *
 * @address 0x4e1930
 */
uint8_t ServerView::send_to_machine(int32_t machine_id, uint32_t status_bit, void *data, uint32_t body_bit_count, uint32_t reliable, uint32_t unknown_a, char force, uint32_t priority)
{
    network_server_globals *server = self;
    int32_t i;

    (void)priority;
    for (i = 0; i < 16; i = i + 1) {
        if (server->machines[i].machine_id == machine_id) {
            network_channel *channel = server->machines[i].channel;
            uint8_t status;

            if (channel == 0 || (channel->connected == 1 && force == 0)) {
                return 0;
            }
            status = (uint8_t)(status_bit != 0);
            return (uint8_t)network_channel_queue_message(channel, (uint32_t)data, (uint32_t)&status, 1,
                                                          (char)reliable, (char)unknown_a, (int32_t)body_bit_count);
        }
    }
    return 0;
}

/**
 * already named)
 * address 0x4df2e0, size 541 bytes
 * name confidence: 0.6   rewrite confidence: 0.15
 * out/phase4/networking_functions.md: "Handles a game-settings update for a new
 * network round: resets channel and history tables, applies the current game variant/defaults,
 * loads the requested map (network_game_server_load_scenario), and kicks off either the host or client path
 * depending on host->flags bit2." Confirmed field matches: host->flags bit2 (+6), the
 *
 * @address 0x4df2e0
 */
uint32_t ServerMessageHandlers::client_game_settings_updated()
{
    network_server_globals *host = self;
    int32_t i;
    network_machine *machine;
    network_player_entry *player;
    uint32_t result;
    int is_host;

    memset((uint8_t *)host + 0x9c8, 0, 0x10);

    is_host = (host->flags >> 2) & 1;
    if (!is_host) {
        if (local_player_globals->local_players[0] != (datum_index)-1) {
            struct player *local_player = (struct player *)halo::memory::datum_get(local_player_globals->local_players[0], player_data);
            if (local_player != 0) {
                network_client->team_index = *(int32_t *)((uint8_t *)local_player + 0x20);
            }
        }
    } else {
        message_delta_parameters_protocol_dump_to_config_file();
        network_stats_summary_log_write();
        message_delta_protocol_initialize();
        network_stats_summary_log_open();
    }

    *(int32_t *)((uint8_t *)host + 0x3b0) = *(int32_t *)((uint8_t *)host + 0x3b0) + 1;
    *(int32_t *)((uint8_t *)host + 0x9b8) = 0;
    *(int32_t *)((uint8_t *)host + 0x9c4) = 0;
    host->scenario_announced = 0;
    host->new_server_pending = 0;
    *(uint8_t *)((uint8_t *)host + 0x9f8) = 0;

    for (i = 0; i < 16; i++) {
        machine = &host->machines[i];
        machine->flags = machine->flags & 0xfb;
        machine->unknown_04 = 0;
        machine->unknown_08 = 0;
        machine->player_joined = 0;
    }

    memset((uint8_t *)host + 0x88, 0, 0x21 * 4);
    memset((uint8_t *)host + 0x10c, 0, 0x26 * 4);
    *(uint16_t *)((uint8_t *)host + 0x1a8) = 0;

    for (i = 0; i < 16; i++) {
        player = &host->session.players[i];
        player->machine_index = -1;
        player->machine_player_index = -1;
        player->team_index = -1;
        player->slot_index = -1;
        player->name[0] = 0;
        player->color_index = -1;
        player->icon_index = -1;
    }
    host->update_tick = 0;
    host->state = 0;

    halo::game::game_engine_apply_current_custom_variant();
    halo::game::game_engine_sync_variant_defaults();
    if (!is_host) {
        join_ui_state = 2;
    }
    host->full_state_broadcast_pending = 1;
    memcpy((uint8_t *)host + 0x10c, game_engine_pending_variant, sizeof(game_engine_pending_variant));
    strncpy((char *)host + 0x8c, (char *)variant_defaults_source, 0x3f);
    *(uint8_t *)((uint8_t *)host + 0xcb) = 0;
    host->state = host->state | 1;
    *(uint16_t *)((uint8_t *)host + 0x86) = 0;
    *(int32_t *)((uint8_t *)host + 0x88) = 0;
    host->listen_channel->listening = 1;

    if (ui_root_widget != 0) {
        halo::interface::widget_close((widget_instance *)ui_root_widget);
    }
    if (ui_widget_history != 0) {
        halo::interface::widget_pool_list_free_all((widget_history_node **)&ui_widget_history);
    }
    ui_pause_depth = 0;
    if (controls_capture_row != -1) {
        controls_input_capture_flags = controls_input_capture_flags & 0xf7;
        memset(controls_input_capture_buffer, 0, sizeof(controls_input_capture_buffer));
        controls_capture_row = -1;
    }
    host->handshake_blocked = 0;

    result = network_game_server_load_scenario();
    if ((char)result == 1) {
        host->state = 1;
        if (is_host) {
            network_host_full_state_broadcast(host);
            result = 1;
            host->new_server_pending = 0;
            return result & 0xffffff00;
        }
        network_client_timer_schedule(0, 0);
    }
    host->new_server_pending = 0;
    return result & 0xffffff00;
}

}
