#include "halo/networking/net1_server.hpp"
#include <string.h>
#include <stdint.h>
#include <wchar.h>
#include "units.h"
#include "halo/math/api.hpp"

extern "C" {
extern uint16_t *network_message_read_sized_buffer(uint16_t *buffer, int32_t capacity, bit_stream *stream);
extern uint32_t network_game_process_incoming_message(int32_t length, network_machine *machine, uint16_t *record, network_server_globals *server);
extern char network_client_drain_queued_updates(network_server_globals *server, network_machine *machine, bit_stream *stream);
extern uint8_t network_incoming_message_scratch[0x510];
extern int32_t network_channel_incoming_read_item(network_channel *channel, uint8_t *destination, int32_t *out_bit_offset, int32_t *out_remaining_bits, s_network_address *out_address, int32_t max_item_bits);
extern char network_channel_dispatch_bitstream_unit(network_server_globals *server, uint32_t unit, bit_stream *stream, network_machine *machine);
typedef struct network_item_stream {
    bit_stream stream;
    uint32_t bit_count;
} network_item_stream;
extern char network_player_entry_validate(network_player_entry *entry);
extern uint8_t network_message_scratch[0x7ff8];
extern void message_delta_parameters_protocol_send_update(void);
extern int32_t message_delta_encode_message(int32_t extra_eax, int32_t extra_edx, int32_t flag, int32_t message_type, int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed);
extern char network_session_broadcast_to_all(network_server_globals *server, int32_t param_1, void *data, int32_t param_3, int32_t param_4, char force, int32_t param_6);
extern network_server_globals *network_server;
extern uint16_t network_challenge_packet_block[];
extern int32_t data_packet_group_encode_packet(uint8_t *buffer, int32_t *capacity, int32_t packet_type, int32_t version);
extern uint16_t *network_message_block_build(uint16_t *dest, uint32_t *buffer, uint8_t flags, uint32_t length);
extern network_client_globals *network_client;
extern int64_t performance_frequency;
extern player_profile player_profile_cache[16];
extern int32_t player_profile_cache_count;
extern char network_player_join_finalize(void);
extern char network_channel_key_open(void);
extern uint32_t player_data_iterator_advance(uint8_t slot_index);
extern void datum_new_at_index_with_salt(void);
extern void player_update_queue_create(void);
extern void game_engine_player_new_life(uint32_t player_datum);
extern int32_t game_engine_player_profile_cache_find(void);
extern void network_game_server_handoff_object_ownership(int32_t *object_count_passthrough, network_server_globals *server, network_machine *machine);
extern void network_object_release_ownership_claim(uint8_t slot_index);
extern data_packet_group network_game_messages_group;
extern int32_t data_packet_group_decode_packet(int16_t *remaining_length, data_packet_group *group, void *decoded_body, const uint8_t *buffer, int16_t *out_type, uint16_t *out_version_used, int16_t expected_class);
extern void network_game_server_handle_client_join(int32_t *object_count_passthrough, network_server_globals *server, network_machine *machine, uint8_t bl_passthrough);
extern char network_server_build_full_game_info_packet(network_machine *machine);
extern data_array *player_data;
extern data_array *object_data;
extern game_engine_definition *current_game_engine;
extern void network_game_broadcast_team_object_updates(int32_t *object_count, uint32_t param_1, int32_t *bytes_sent);
extern int32_t game_engine_notify_object_value_event(int32_t team);
extern void build_player_full_resync_update(int32_t machine_id);
extern void game_engine_capture_player_profile(int32_t value);
extern void game_engine_send_unit_weapon_loadout(void *machine, int32_t team, int32_t machine_id);
typedef void (*network_join_complete_callback)(int32_t unused, int32_t machine_id);
extern network_server_globals *network_game_server_host_new(void);
extern int32_t network_console_connection_id;
extern uint8_t network_session_active2;
extern void *network_session_host_object;
extern int32_t network_session_host_state;
extern void gcd_disconnect_all(int32_t connection_id);
extern void message_delta_parameters_protocol_dump_to_config_file(void);
extern void network_stats_summary_log_write(void);
extern uint16_t *network_prepare_challenge_packet(int32_t message_type, void *payload);
extern char network_channel_service(network_channel *channel, int32_t timeout_ms, network_channel **out_new_child);
extern void network_channel_delete(network_channel *channel);
extern void network_session_host_update(void);
extern void gcd_shutdown(void);
extern void qr2_shutdown(void *object);
extern int32_t network_session_reject_pending_connection_callback(void *unused, int32_t reject_code);
extern network_server_globals network_server_storage;
extern int32_t network_scenario_round_counter_a;
extern uint8_t network_scenario_round_counter_b;
extern uint8_t unknown_00861d4e;
extern uint8_t unknown_00861d4f;
extern int16_t pending_difficulty;
extern void network_channels_open(void);
extern network_channel *network_channel_new(uint32_t flags);
extern void network_game_session_reset(network_game_session *session);
extern void network_game_server_host_dispose(network_server_globals *host);
extern char network_game_session_reset_defaults(void);
extern void * network_session_host_start(int32_t user_data);
extern void message_delta_protocol_initialize(void);
extern void network_stats_summary_log_open(void);
extern void update_server_push_player_tick_history(void);
extern void game_engine_tick(void);
extern char network_game_session_finalize_and_add_player(network_player_entry *entry, network_server_globals *server, network_machine *machine);
extern uint32_t network_game_broadcast_state_snapshot(const uint32_t *record, network_server_globals *server);
extern uint8_t game_engine_team_is_leading(uint32_t requested_team);
extern void network_game_generate_unique_random_name(void);
extern char network_player_name_collision_check(void);
extern void network_player_assign_random_color(void);
extern uint32_t network_player_entry_add(network_player_entry *entry, network_game_session *session);
extern game_variant game_engine_pending_variant;
extern char variant_defaults_source[];
extern uint8_t DAT_00000050;
extern int16_t network_channel_get_remote_address(s_network_address *address, network_receive_queue *queue);
typedef struct rcon_request_decode {
    char password[20];
    char command[64];
} rcon_request_decode;
extern char sv_rcon_password_value[9];
extern uint8_t message_delta_decode_compound_field(void *decode_context, void *destination);
extern void message_delta_decode_compound_field_staged(void *decode_context);
extern uint8_t console_process_rcon_command(char *command);
extern void chimera__rcon_out(char *text, int32_t machine_id);
extern void *global_white_argb;
extern void chimera__console_out(ColorARGB *color, char *format, ...);
extern uint8_t network_session_send_to_machine(int32_t machine_id, network_server_globals *server, uint32_t status_bit, void *data, uint32_t bits, uint32_t reliable, uint32_t unknown_a, char force, uint32_t priority);
extern int16_t network_join_error_code;
extern uint8_t network_host_handoff_requested;
extern void chat_close(void);
extern void network_machine_timer_start(network_machine *machine, int32_t duration_ms);
extern int32_t network_server_status_last_print_ms;
extern void sv_status(void);
extern int32_t network_pending_connection_count;
extern network_pending_connection network_pending_connections[30];
extern char network_channel_queue_message(void *data, void *out_status, int32_t one, uint32_t param_3, uint32_t param_4, uint32_t param_6);
extern uint32_t network_game_settings_broadcast_send(uint32_t round, uint32_t *record);
extern void network_machine_check_build_version(const char *remote_version, network_machine *machine);
extern void network_client_connection_handshake_tick(int16_t state, network_server_globals *owner);
extern void network_channel_reliable_pool_store(network_channel *channel, uint16_t *packet, uint8_t *reliable_flag, int32_t priority);
extern void *datum_get(void);
extern int32_t time_query_performance_counter_ms(void);
extern uint32_t network_game_broadcast_player_set_changed(network_server_globals *session);
extern uint8_t network_player_entry_update(network_player_entry *incoming, network_game_session *session);
extern void *message_delta_definition_table;
extern int16_t network_game_mode;
extern uint8_t network_server_host_valid;
extern uint32_t split_screen_quit_prompt_string;
extern uint32_t network_join_error_reason;
extern void main_menu_music_stop(void);
extern void chimera__load_ui_map(char reset);
extern void network_client_globals_dispose(void);
extern char network_host_update_tick(network_server_globals *host);
extern int32_t network_channel_remove_child(network_channel *parent, network_channel *child);
extern char network_server_count_machines_and_resolve_address(network_server_globals *host, network_channel *new_child);
extern void network_map_cycle_list_broadcast(void);
extern char network_server_service_machines_tick(network_server_globals *host);
extern char network_server_heartbeat_tick(network_server_globals *host);
extern char network_server_status_periodic_print(network_server_globals *host);
extern char network_server_resend_challenge_periodic(network_server_globals *host);
extern void network_channel_remote_address_or_default(network_channel *channel, network_resolved_address *out_address);
extern uint32_t network_local_address;
extern uint8_t network_session_host_reject_or_cleanup_client(const char *response, const char *challenge, uint32_t ip, int32_t local_id);
extern char network_build_string[];
}

namespace halo::networking {

/**
 * out/phase4/networking_functions.md: "Small dispatcher used while draining a
 * channel's bitstream, routing each unit to either the queued-message processor or the
 * incoming-packet decoder." network_game_process_incoming_message is already named and in
 * this batch.
 *
 * @address 0x4e18b0
 */
char ServerView::dispatch_bitstream_unit(uint32_t unit, bit_stream *stream, network_machine *machine)
{
    network_server_globals *server = self;
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

/**
 * out/phase4/networking_functions.md: "Drains a shared bitstream buffer one bit at a
 * time, dispatching each bit through FUN_004e18b0 as part of connecting a new machine."
 * *param_2 (machine->channel) and channel->incoming (channel+0xc) match
 * network_channel::endpoint/incoming... wait, channel+0xc is actually ::incoming per
 * types/networking.h; channel+0x8/+0xc/+0x10 on the *circular_buffer* itself match
 * read_cursor/write_cursor/capacity. FUN_004dcf10 is functions.md's
 * "network_channel_incoming_read_item" (already named there, conf 0.4, not renamed here since
 *
 * @address 0x4e1290
 */
char ServerView::drain_bitstream(network_machine *machine)
{
    network_server_globals *server = self;
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

/**
 * network_server_check_machine_timeout (0x4e0f80 `mov eax,esi` / 0x4e102b
 * `lea eax,[esp+0x20]`), both immediately before the call.
 * For every one of the 16 machine slots with a connected (0..15) machine_id, requires at least
 * one valid player-table entry whose machine_index matches it. Returns 0 as soon as a
 * connected machine has no matching player; returns 1 once all 16 slots have been checked.
 *
 * @address 0x4e04f0
 */
uint32_t ServerView::all_machines_have_player()
{
    network_server_globals *server = self;
    int32_t machine_index;

    for (machine_index = 0; machine_index < 16; machine_index = machine_index + 1) {
        int16_t machine_id;

        machine_id = server->machines[machine_index].machine_id;
        if (machine_id >= 0 && machine_id < 16) {
            char found;
            network_player_entry *entry;
            int32_t i;

            found = 0;
            entry = server->session.players;
            for (i = 0; i < 16; i = i + 1) {
                char valid;

                valid = network_player_entry_validate(entry);
                if (valid != 0 && entry->machine_index == (int8_t)machine_id) {
                    found = 1;
                }
                entry = entry + 1;
            }
            if (!found) {
                return 0;
            }
        }
    }
    return 1;
}

/**
 * network_server_check_machine_timeout (0x4e0f80 `mov eax,esi` / 0x4e102b
 * `lea eax,[esp+0x20]`), both immediately before the call.
 * Returns 1 if the game is in team mode and either team 0 or team 1 currently has zero valid,
 * active players; returns 0 if both teams are populated, or if the game is not in team mode.
 *
 * @address 0x4e0480
 */
uint32_t ServerView::any_team_empty()
{
    network_server_globals *server = self;
    int16_t counts[2];
    network_player_entry *entry;
    int32_t i;

    if (server->session.variant.teams == 0) {
        return 0;
    }
    counts[0] = 0;
    counts[1] = 0;
    entry = server->session.players;
    for (i = 0; i < 16; i = i + 1) {
        char valid;
        int8_t team;

        valid = network_player_entry_validate(entry);
        if (valid != 0) {
            team = entry->team_index;
            if (team >= 0 && team < 2) {
                counts[(int32_t)team] = counts[(int32_t)team] + 1;
            }
        }
        entry = entry + 1;
    }
    for (i = 0; i < 2; i = i + 1) {
        if (counts[i] == 0) {
            return 1;
        }
    }
    return 0;
}

/**
 * out/phase4/networking_functions.md: "Posts a type-0x21 game-engine event and, if
 * accepted, broadcasts an associated update packet to the whole session -- used whenever the
 * connected-player set changes." 0x00871de0 is types/networking.h's shared encode scratch
 * buffer, already named network_message_scratch by
 * src/networking/network_server_check_machine_timeout.c (same address, "shared with
 * network_game_broadcast_team_object_updates.c").
 *
 * @address 0x4e1bf0
 */
uint32_t ServerView::broadcast_player_set_changed(uint8_t *param_1)
{
    int32_t encoded_bits;
    void *record;

    message_delta_parameters_protocol_send_update();
    record = param_1 + 8;
    encoded_bits = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 0x21, 0, &record, 0, 1, 0);
    if (0 < encoded_bits) {
        network_session_broadcast_to_all(network_server, 1, network_message_scratch, 1, 0, 1, 3);
    }
    return 0 < encoded_bits;
}

/**
 * this module, 0x4e19c0
 * Copies an 8-dword game-state record into a scratch buffer, encodes it as message type 0x17,
 * and broadcasts the encoded block to every established machine in the session.
 * FIXED (register inputs, objdump): the original never reads ESI as an input (it overwrites or only saves it); those parameters arrive on the stack (1 stack argument(s) read).
 *
 * @address 0x4e1b50
 */
uint32_t ServerView::broadcast_state_snapshot(const uint32_t *record)
{
    network_server_globals *server = self;
    uint32_t buffer[8];
    int32_t capacity;
    int32_t i;
    uint16_t *encoded;

    for (i = 0; i < 8; i = i + 1) {
        buffer[i] = record[i];
    }
    capacity = 0x600;
    if (data_packet_group_encode_packet((uint8_t *)buffer, &capacity, 0x17, 1) != 0) {
        encoded = network_message_block_build(network_challenge_packet_block, buffer, 3, (uint32_t)capacity);
        if (encoded != 0) {
            return network_session_broadcast_to_all(server, 0, encoded, 1, 0, 1, 3);
        }
    }
    return 0;
}

/**
 * One-time per-round bookkeeping (first call this round records whether a listen-server
 * client exists and stamps a millisecond timestamp), then scans the 16 player-table slots
 * for the one whose key matches `machine`, finalizes that player's game object (via the
 * stats-logging-aware path selection), records it into player_profile_cache, and hands the
 * result to network_game_server_handoff_object_ownership /
 * network_object_release_ownership_claim.
 *
 * @address 0x4dfc90
 */
void ServerView::handle_client_join(int32_t *object_count_passthrough, network_machine *machine, uint8_t bl_passthrough)
{
    network_server_globals *server = self;
    network_player_entry *entry;
    network_player_entry *scan;
    char valid;
    char handled;
    char ok;
    int32_t i;
    int32_t remaining;
    int32_t *field_9c4;
    uint8_t *unknown_9bc_base;
    uint32_t player_datum;
    int32_t profile_index;
    player_profile *profile;
    int32_t j;

    machine->flags |= 0x04;
    unknown_9bc_base = (uint8_t *)server;
    field_9c4 = (int32_t *)(unknown_9bc_base + 0x9c4);

    if (server->state != 1) {
        int16_t *machine_id_ptr;
        char all_processed_or_invalid;

        all_processed_or_invalid = 1;
        machine_id_ptr = (int16_t *)((uint8_t *)server + 0x3c4);
        for (i = 4; i != 0; i = i - 1) {
            if (machine_id_ptr[0] >= 0 && machine_id_ptr[0] < 16 &&
                (((uint8_t *)machine_id_ptr)[2] & 0x04) == 0) {
                all_processed_or_invalid = 0;
            }
            if (machine_id_ptr[0x30] >= 0 && machine_id_ptr[0x30] < 16 &&
                (((uint8_t *)(machine_id_ptr + 0x30))[2] & 0x04) == 0) {
                all_processed_or_invalid = 0;
            }
            if (machine_id_ptr[0x60] >= 0 && machine_id_ptr[0x60] < 16 &&
                (((uint8_t *)(machine_id_ptr + 0x60))[2] & 0x04) == 0) {
                all_processed_or_invalid = 0;
            }
            if (machine_id_ptr[0x90] >= 0 && machine_id_ptr[0x90] < 16 &&
                (((uint8_t *)(machine_id_ptr + 0x90))[2] & 0x04) == 0) {
                all_processed_or_invalid = 0;
            }
            machine_id_ptr = machine_id_ptr + 0xc0;
        }
        if (all_processed_or_invalid) {
            char has_client;

            has_client = (network_client != 0);
            server->state = 1;
            *field_9c4 = 0;
            server->session.map_loaded = has_client ? *((uint8_t *)network_client + 0xec0) : 0;

        }
        if (*field_9c4 == 0) {
            large_integer counter;

            QueryPerformanceCounter((LARGE_INTEGER *)&counter);
            *field_9c4 = (int32_t)((counter.quad_part * 1000) / performance_frequency);
        }
    }

    entry = server->session.players;
    remaining = 16;
    for (;;) {
        valid = network_player_entry_validate(entry);
        handled = 0;
        if (valid != 0) {
            scan = server->session.players;
            i = 0;
            do {
                if (scan->machine_index == entry->machine_index &&
                    scan->machine_player_index == entry->machine_player_index) {
                    if ((int16_t)entry->machine_index == machine->machine_id) {
                        if ((server->flags >> 2 & 1) == 0) {
                            ok = network_player_join_finalize();
                        } else {
                            ok = 0;
                            if (network_player_entry_validate(entry) != 0 && server->state == 1) {

                                ok = network_channel_key_open();
                                if (ok != 0) {
                                    player_data_iterator_advance((uint8_t)entry->slot_index) ;
                                    datum_new_at_index_with_salt();
                                    datum_new_at_index_with_salt();
                                    player_update_queue_create();
                                }
                            }
                        }
                        if (ok != 0) {
                            machine->player_joined = 1;
                            player_datum = player_data_iterator_advance((uint8_t)entry->slot_index) ;
                            game_engine_player_new_life(player_datum);
                            if (game_engine_player_profile_cache_find() != -1) {
                                handled = 1;
                            } else {

                                for (j = 0; j < 16; j = j + 1) {
                                    profile = &player_profile_cache[j];
                                    if (profile->in_use == 0) {
                                        profile->in_use = 1;
                                        profile->player = (datum_index)player_datum;
                                        player_profile_cache_count = player_profile_cache_count + 1;
                                        break;
                                    }
                                }
                                handled = 1;
                            }
                        }
                    }
                    break;
                }
                i = i + 1;
                scan = scan + 1;
            } while (i < 16);
        }
        if (handled) {
            network_game_server_handoff_object_ownership(object_count_passthrough, server, machine);
            network_object_release_ownership_claim(bl_passthrough);
        }
        entry = entry + 1;
        remaining = remaining - 1;
        if (remaining == 0) {
            return;
        }
    }
}

/**
 * out/phase4/networking_functions.md: "Handles message type 0x1a, sending a full
 * server-info reply to not-yet-established machines or otherwise forwarding to FUN_004dfc90"
 * -- network_game_server_handle_client_join, already written. `*unaff_EDI + 0xa98` matches
 * network_channel::connected via network_machine::channel (offset 0), and
 * `unaff_ESI + 0xa0f` matches network_server_globals::game_over, exactly as in the type-0xe and
 * type-0xf handlers.
 *
 * @address 0x4e2700
 */
uint32_t ServerView::handle_info_request(network_machine *machine, uint8_t *record, int32_t length)
{
    network_server_globals *server = self;
    uint32_t body[1];
    int16_t out_type;
    uint16_t version_used;
    int16_t state = *(int16_t *)((uint8_t *)server + 4);
    uint8_t *connection;

    if ((state != 0 && state != 1) || data_packet_group_decode_packet((length -= 2, (int16_t *)&length), &network_game_messages_group, body, record + 2, &out_type, &version_used, 5) == 0) {
        return 0;
    }
    connection = machine != 0 ? *(uint8_t **)machine : 0;
    if ((connection != 0 && connection[0xa98] != 0) || *((uint8_t *)server + 0xa0f) == 0) {
        network_game_server_handle_client_join(0 , server, machine, 1);
        return 1;
    }
    return (uint8_t)network_server_build_full_game_info_packet(machine);
}

/**
 * For each of the 16 player-table slots, if the slot's (machine_index, machine_player_index)
 * key does not already belong to `machine`, locates the network_machine record that does own
 * it and, when that machine's channel is established and its player is a live, non-frozen
 * unit, transfers network ownership of that unit to `machine`. Invokes the completion
 * callback at current_game_engine+0x90 once all 16 slots are processed.
 *
 * @address 0x4dfa10
 */
void ServerView::handoff_object_ownership(int32_t *object_count_passthrough, network_machine *machine)
{
    network_server_globals *server = self;
    int32_t bytes_sent;
    int32_t machine_id;
    network_player_entry *entry;
    network_player_entry *scan;
    int32_t remaining;
    int32_t i;
    int8_t key_machine_index;
    network_machine *owner;
    uint32_t datum;
    int16_t player_index;
    int16_t salt;
    player *plr;
    int32_t team;
    int32_t unit;
    object_header *hdr;
    object *unit_obj;

    bytes_sent = 0;
    machine_id = (int32_t)machine->machine_id;
    network_game_broadcast_team_object_updates(object_count_passthrough, 0, &bytes_sent);
    network_game_broadcast_team_object_updates(object_count_passthrough, 0, &bytes_sent);

    entry = server->session.players;
    remaining = 16;
    for (;;) {
        int do_transfer;

        do_transfer = 0;
        owner = 0;
        if (network_player_entry_validate(entry) != 0) {
            key_machine_index = entry->machine_index;
            scan = server->session.players;
            i = 0;
            do {
                if (scan->machine_index == key_machine_index &&
                    scan->machine_player_index == entry->machine_player_index) {
                    if ((int16_t)key_machine_index != machine->machine_id) {
                        owner = 0;
                        for (i = 0; i <= 15; i = i + 1) {
                            if (server->machines[i].machine_id == (int16_t)key_machine_index) {
                                owner = &server->machines[i];
                                break;
                            }
                        }
                        do_transfer = 1;
                    }
                    break;
                }
                i = i + 1;
                scan = scan + 1;
            } while (i < 16);
        }

        if (do_transfer && owner != 0 && (owner->flags & 0x04) != 0) {

            datum = player_data_iterator_advance((uint8_t)entry->slot_index) ;
            if (datum != 0xffffffff) {
                player_index = (int16_t)datum;
                if (player_index >= 0 && player_index < player_data->maximum_count) {
                    plr = (player *)((uint8_t *)player_data->data + player_data->size * player_index);
                    if (plr->identifier != 0) {
                        salt = (int16_t)(datum >> 16);
                        if (salt == 0 || plr->identifier == salt) {
                            team = plr->team;
                            unit = plr->unit;
                            game_engine_notify_object_value_event(team);
                            build_player_full_resync_update(machine_id);
                            if (game_engine_player_profile_cache_find() != -1) {
                                game_engine_capture_player_profile(0);
                            }
                            if ((uint32_t)unit != 0xffffffff) {
                                hdr = &((object_header *)object_data->data)[unit & 0xffff];
                                unit_obj = hdr->data;
                                if ((unit_obj->vitality_flags & 0x04) == 0) {
                                    game_engine_send_unit_weapon_loadout(owner, team, machine_id);
                                }
                            }
                        }
                    }
                }
            }
        }

        entry = entry + 1;
        remaining = remaining - 1;
        if (remaining == 0) {
            if (current_game_engine != 0 &&
                *(void **)((uint8_t *)current_game_engine + 0x90) != 0) {
                ((network_join_complete_callback)(*(void **)((uint8_t *)current_game_engine + 0x90)))(0, machine_id);
            }
            return;
        }
    }
}

/**
 * out/phase4/networking_functions.md: "Allocates and installs the network host globals
 * (DAT_0071c2d4) via network_game_server_host_new and seeds its randomisation salt field from the global PRNG
 * state, mirroring the salt into the network-game globals when present." network_server
 * (0x0071c2d4) and network_client (0x0071c2d8) match types/networking.h.
 *
 * @address 0x4ddd40
 */
int32_t ServerView::host_create()
{
    network_server_globals *host;
    uint32_t salt;

    host = network_game_server_host_new();
    network_server = host;
    if (host != 0) {
        halo::math::globals().effect_random_seed = halo::math::globals().effect_random_seed * 0x19660d + 0x3c6ef35f;
        salt = halo::math::globals().effect_random_seed >> 0x10;
        *(uint32_t *)((uint8_t *)host + 0x3ac) = salt;
        if (network_client != 0) {
            *(uint32_t *)((uint8_t *)network_client + 0xeb8) = salt;
        }
    }
    return host != 0;
}

/**
 * out/phase4/networking_functions.md: "Tears down the network host globals block
 * passed in param_1: disposes each of its 16 team/player sub-entries, frees its allocation,
 * zeroes the structure, and shuts down the associated transport connection." host->flags bit2
 * (+6, stats logging), machines[16] at +0x3b8 stride 0x60 with channel/machine_id/flags, and the
 * final 0x284-dword (0xa10-byte) zero of the whole network_server_globals all match
 * types/networking.h exactly.
 *
 * @address 0x4deda0
 */
void ServerView::host_dispose()
{
    network_server_globals *host = self;
    int32_t i;
    network_machine *machine;
    int32_t challenge_packet;
    int32_t message_type;
    uint32_t challenge_payload[4];

    gcd_disconnect_all(network_console_connection_id);
    if (((*(uint8_t *)((uint8_t *)host + 6) >> 2) & 1) != 0) {
        message_delta_parameters_protocol_dump_to_config_file();
        network_stats_summary_log_write();
    }
    if ((host->state == 0 || host->state == 2)) {
        message_type = (host->state == 2) ? 0x22 : 0x0b;

        challenge_packet = (int32_t)network_prepare_challenge_packet(message_type, challenge_payload);
        if (challenge_packet != 0) {
            network_session_broadcast_to_all(network_server, 0, (void *)(uintptr_t)challenge_packet,
                1, 0, 1, 3);
        }
    }
    for (i = 0; i < 16; i++) {
        machine = &host->machines[i];
        if (machine->machine_id != -1 && (machine->channel->flags & k_network_channel_dead) == 0) {

            network_channel_service(machine->channel, 15000, 0);
        }
    }
    if (host->listen_channel != 0) {
        network_channel_delete(host->listen_channel);
    }
    memset(host, 0, sizeof(*host));
    network_session_active2 = 0;
    if (network_session_host_object != 0) {
        if (network_session_host_state != 2) {
            network_session_host_state = 2;
        }
        network_session_host_update();
        network_console_connection_id = -1;
        gcd_shutdown();
        qr2_shutdown(network_session_host_object);
        network_session_host_object = 0;
    }
}

/**
 * types/networking.h's own comment cites this address: "network_game_server_host_new
 * (0x4dec40) zeroes 0x284 dwords of 0x00861340, which is the size of network_server_globals."
 * Every field this function sets after the zero (flags bit1, session.message_callback,
 * session.unknown_19e, and the 16-entry machines[] init matching network_machine's
 * channel/unknown_04/unknown_08/machine_id/flags/unknown_50/unknown_51/unknown_52/unknown_56/
 * unknown_5c fields, including the header's own "unaligned in the original" note on
 * unknown_52/unknown_56) matches types/networking.h exactly.
 *
 * @address 0x4dec40
 */
void * ServerView::host_new()
{
    network_server_globals *host;
    int32_t i;
    network_machine *machine;

    memset(&network_server_storage, 0, sizeof(network_server_storage));
    host = &network_server_storage;
    network_session_active2 = 1;
    network_scenario_round_counter_a = 0;
    network_scenario_round_counter_b = 0;
    unknown_00861d4e = 0;
    unknown_00861d4f = 0;
    network_channels_open();
    host->listen_channel = network_channel_new(k_network_channel_listening);
    if (host->listen_channel != 0) {
        host->flags = host->flags | 2;
        host->state = 0;
        network_game_session_reset(&host->session);
        host->session.message_callback = (void *)network_session_reject_pending_connection_callback;
        host->session.difficulty = pending_difficulty;
        *(int32_t *)((uint8_t *)host + 0x3b0) = -1;
        for (i = 0; i < 16; i++) {
            machine = &host->machines[i];
            machine->channel = 0;
            machine->unknown_04 = 0;
            machine->unknown_08 = 0;
            machine->machine_id = -1;
            machine->flags = 0;
            machine->unknown_0f = 0;
            *(uint32_t *)((uint8_t *)machine + 0x52) = 0;
            *(uint32_t *)((uint8_t *)machine + 0x56) = 0;
            machine->gcd_user_id = -1;
            machine->player_joined = 0;
            machine->players_removed_broadcast = 0;
        }
        host->update_tick = 0;
        host->first_join_ms = 0;
        host->handshake_timer.remaining_ms = 0;
        host->handshake_timer.last_tick_ms = 0;
        host->last_challenge_sent_ms = 0;
        host->unknown_9d0 = 0;
        host->scenario_announced = 0;
        host->new_server_pending = 0;
        host->join_finalize_pending = 0;
        *(int32_t *)((uint8_t *)host + 0x3b0) = *(int32_t *)((uint8_t *)host + 0x3b0) + 1;
        *(uint32_t *)&host->handshake_state = 0;
        if (network_game_session_reset_defaults() != 0) {
            network_session_host_start(0);
            goto done;
        }
    }
    network_game_server_host_dispose(host);
    host = 0;
done:
    if (host != 0 && ((*((uint8_t *)host + 6) >> 2) & 1) != 0) {
        message_delta_protocol_initialize();
        network_stats_summary_log_open();
    }
    return host;
}

/**
 * While the server is in state 1 (client-processing), drains `update_count` queued update
 * packets and, if a deferred "process this machine's queued update" request is pending
 * (unknown_9f8), locates the matching machine by its saved id and finalizes/broadcasts its
 * join. In state 2, just ticks the game engine directly.
 *
 * @address 0x4e03c0
 */
void ServerView::per_frame_tick(int16_t update_count)
{
    network_server_globals *server = self;
    if (server->state == 1) {
        if (update_count > 0) {
            uint32_t remaining;
            large_integer counter;

            remaining = (uint32_t)update_count;
            do {
                server->update_tick = server->update_tick + 1;
                update_server_push_player_tick_history();
                QueryPerformanceCounter((LARGE_INTEGER *)&counter);
                remaining = remaining - 1;
            } while (remaining != 0);
        }
        if (server->join_finalize_pending != 0) {
            int32_t i;
            int8_t saved_machine_id;
            network_machine *machine;

            saved_machine_id = *((int8_t *)server + 0x9f4);
            i = 0;
            while (server->machines[i].machine_id != (int16_t)saved_machine_id) {
                i = i + 1;
                if (i > 0xf) {
                    server->join_finalize_pending = 0;
                    return;
                }
            }
            machine = &server->machines[i];
            if (machine != 0) {
                char ok;

                network_player_entry *entry = &server->pending_join_entry;

                ok = network_game_session_finalize_and_add_player(entry, server, machine);
                if (ok != 0) {
                    network_game_broadcast_state_snapshot((const uint32_t *)entry, server);
                }
            }
            server->join_finalize_pending = 0;
        }
    } else if (server->state == 2) {
        game_engine_tick();
    }
}

/**
 * Clears byte +0x1c of an opaque broadcast-message context, per the function's (unresolved)
 * name.
 *
 * @address 0x4e4ef0
 */
void ServerView::send_message_to_all_machines_ingame(uint8_t *context)
{
    context[0x1c] = 0;
}

/**
 * Rejects the '%' and '|' escape characters from a candidate player name, regenerates a
 * fresh random name on any collision or reserved character, and assigns a random colour to
 * a still-unassigned slot, before handing the finished entry to network_player_entry_add.
 * Only runs when the entry's machine_index already matches the caller's machine.
 *
 * @address 0x4df840
 */
uint32_t ServerView::session_finalize_and_add_player(network_player_entry *entry, network_machine *machine)
{
    network_server_globals *server = self;
    wchar_t reserved[4];
    wchar_t *hit;

    if (machine->machine_id != (int16_t)entry->machine_index) {
        return (uint32_t)entry & 0xffffff00;
    }
    reserved[0] = L'%';
    reserved[1] = L'\0';
    reserved[2] = L'|';
    reserved[3] = L'\0';
    if (entry->team_index == -1) {
        entry->team_index = (int8_t)game_engine_team_is_leading(0xffffffff);
    }
    if (entry->name[0] == L'\0') {
        network_game_generate_unique_random_name();
    }
    hit = wcsstr((wchar_t *)entry->name, reserved);
    if (hit != 0 || (hit = wcsstr((wchar_t *)entry->name, reserved + 2), hit != 0)) {
        network_game_generate_unique_random_name();
    }
    if (network_player_name_collision_check() == 0) {
        network_game_generate_unique_random_name();
    }
    if (entry->color_index == -1) {
        network_player_assign_random_color();
    }
    return network_player_entry_add(entry, &server->session);
}

/**
 * Resets `server`'s session to compiled-in defaults: copies the pending game variant, the
 * default server name, and clears the two fields between them, then marks the session and the
 * listen channel as initialized.
 * FIXED: every ret of the original is preceded by mov eax,1 and callers test it
 *
 * @address 0x4e1820
 */
int32_t ServerView::session_reset_defaults()
{
    network_server_globals *server = self;
    memcpy(&server->session.variant, &game_engine_pending_variant, sizeof(game_variant));
    strncpy(server->session.server_name, variant_defaults_source, 0x3f);
    server->session.server_name[0x3f] = 0;
    server->session.unknown_07e = 0;
    server->session.unknown_080 = 0;
    server->flags |= 1;
    server->listen_channel->listening = 1;
    return 1;
}

/**
 * Finds the machines[] slot whose machine_id equals machine_id and clears its unknown_50
 * byte, returning that slot's address (with its low byte masked off). If no slot matches,
 * writes a zero byte to absolute address 0x50 instead (see UNSURE note).
 *
 * @address 0x4e0b90
 */
uint32_t ServerView::clear_flag_by_id(int32_t machine_id)
{
    network_server_globals *server = self;
    int32_t i;

    for (i = 0; i < 16; i = i + 1) {
        if (server->machines[i].machine_id == machine_id) {
            server->machines[i].player_joined = 0;
            return ((uint32_t)&server->machines[i]) & 0xffffff00;
        }
    }
    DAT_00000050 = 0;
    return ((uint32_t)16) & 0xffffff00;
}

/**
 * Returns the machines[] slot whose machine_id equals `machine_id`, or NULL if none matches.
 *
 * @address 0x4e0810
 */
network_machine * ServerView::find_by_id(int32_t machine_id)
{
    network_server_globals *server = self;
    int32_t i;

    for (i = 0; i < 16; i = i + 1) {
        if (server->machines[i].machine_id == machine_id) {
            return &server->machines[i];
        }
    }
    return 0;
}

/**
 * this module, 0x4e19c0
 *
 * @address 0x4df290
 */
void ServerView::advance_connect_state()
{
    network_server_globals *server = self;
    int32_t challenge_packet;
    uint32_t challenge_payload[4];

    if (*(int16_t *)((uint8_t *)server + 4) == 1) {
        *(int16_t *)((uint8_t *)server + 4) = 2;

        challenge_packet = (int32_t)network_prepare_challenge_packet(0x19, challenge_payload);
        if (challenge_packet != 0) {
            network_session_broadcast_to_all(server, 0, (void *)(uintptr_t)challenge_packet,
                1, 0, 1, 3);
        }
    }
}

/**
 * Returns 1 if no machine slot is both connected (id 0..15) and free of
 * k_network_machine_version_mismatch; returns 0 as soon as one is found.
 * FIXED (objdump): every ret sets only AL; the upper bits of EAX are left as they were
 *
 * @address 0x4e14e0
 */
uint8_t ServerView::any_machine_awaiting_flag()
{
    network_server_globals *server = self;
    int32_t i;

    for (i = 0; i < 16; i = i + 1) {
        int16_t id;

        id = server->machines[i].machine_id;
        if (id >= 0 && id <= 15 && (server->machines[i].flags & k_network_machine_version_mismatch) == 0) {
            return 0;
        }
    }
    return 1;
}

/**
 * Counts machine-table slots that have both a live channel and a connected (non -1) id.
 *
 * @address 0x4e1880
 */
int32_t ServerView::count_connected_machines()
{
    network_server_globals *server = self;
    int32_t count;
    int32_t i;

    count = 0;
    for (i = 0; i < 16; i = i + 1) {
        if (server->machines[i].channel != 0 && server->machines[i].machine_id != -1) {
            count = count + 1;
        }
    }
    return count;
}

/**
 * out/phase4/networking_functions.md: "Counts occupied machine-table slots up to the
 * first free one and, for a new connection, fetches its remote address via
 * network_channel_get_remote_address." Ghidra's own decompilation removes eleven blocks as
 * "unreachable", so most of this 438-byte function's body is not available here; only the
 * surviving control flow is transcribed. param_1+6 matches network_server_globals::flags;
 * param_1+0x3c4 matches ::machines[0].machine_id.
 *
 * @address 0x4e0d30
 */
uint32_t ServerView::count_machines_and_resolve_address(uint32_t eax_passthrough, s_network_address *address_out, network_receive_queue **connection)
{
    network_server_globals *server = self;
    if ((server->flags & 1) != 0) {
        int32_t i;
        int16_t *machine_id_ptr;

        i = 0;
        machine_id_ptr = (int16_t *)((uint8_t *)server + 0x3c4);
        while (*machine_id_ptr != -1) {
            i = i + 1;
            machine_id_ptr = machine_id_ptr + 0x30;
            if (i > 15) {
                return ((uint32_t)machine_id_ptr) & 0xffffff00;
            }
        }
        if (*connection != 0) {
            network_channel_get_remote_address(address_out, *connection);
        }
        eax_passthrough = 0;
    }
    return eax_passthrough & 0xffffff00;
}

/**
 * Server-side handler for an incoming rcon-request message: decodes it, validates the password
 * against sv_rcon_password_value, executes the command if both the server has rcon enabled and
 * the password matches, and reports the outcome back to the requesting client via
 * chimera__rcon_out plus a server console log line.
 *
 * @address 0x4e4f00
 */
void ServerView::handle_rcon_request(network_player_entry *client, void *message)
{
    int16_t machine_id = client->machine_index;
    rcon_request_decode decode;

    if (*(int32_t *)*(int32_t *)message != 0) {
        message_delta_decode_compound_field_staged(message);
        chimera__console_out((ColorARGB *)global_white_argb, (char *)"Ignoring meaningless rcon_request message from client #%d", machine_id);
        return;
    }
    memset(&decode, 0, sizeof(decode));
    if (message_delta_decode_compound_field(message, &decode) == 0) {
        chimera__console_out((ColorARGB *)global_white_argb, (char *)"Could not decode rcon message from client #%d", machine_id);
        return;
    }
    if (sv_rcon_password_value[0] == 0) {
        chimera__rcon_out((char *)"rcon command ignored (rcon is disabled)", machine_id);
        chimera__console_out((ColorARGB *)global_white_argb, (char *)"Ignoring rcon request from client #%d (rcon is disabled)", machine_id);
        return;
    }
    if (strcmp(sv_rcon_password_value, decode.password) != 0) {
        chimera__rcon_out((char *)"rcon command ignored (bad password)", machine_id);
        chimera__console_out((ColorARGB *)global_white_argb, (char *)"Ignoring rcon request from client #%d (bad password)", machine_id);
        return;
    }
    if (decode.command[0] == 0) {
        chimera__rcon_out((char *)"rcon command ignored (empty)", machine_id);
        chimera__console_out((ColorARGB *)global_white_argb, (char *)"Ignoring rcon request from client #%d (empty command)", machine_id);
        return;
    }
    if (console_process_rcon_command(decode.command) != 0) {
        chimera__rcon_out((char *)"rcon command finished", machine_id);
        chimera__console_out((ColorARGB *)global_white_argb, (char *)"Successfully executed rcon command from client #%d.", machine_id + 1);
        return;
    }
    chimera__rcon_out((char *)"rcon command failed", machine_id);
    chimera__console_out((ColorARGB *)global_white_argb, (char *)"Failure executing rcon command from client #%d.", machine_id);
}

/**
 * CX reason, EDI machine, stack server. For a machine whose
 * channel is connected (+0xa98): the chat close deadline (0x00718fa4, when unset) becomes reason + 0x2b, the host
 * hand-off flag is set, chat closes; returns 1. Otherwise a type 6 packet carrying the reason goes to the machine
 * (reliable, 3) and the machine timer restarts for 1000 ms; returns whether the send worked (0 when no packet was
 * built). The server argument was missing.
 *
 * @address 0x4e0af0
 */
uint8_t ServerView::notify_or_resend_challenge(int16_t reason, network_machine *machine)
{
    network_server_globals *server = self;
    int32_t payload = reason;
    uint16_t *packet;
    uint8_t ok = 1;

    if (machine != 0 && machine->channel != 0 && machine->channel->connected != 0) {
        if (network_join_error_code == -1) {
            network_join_error_code = (int16_t)(reason + 0x2b);
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

/**
 * Copies `server`'s password (up to 8 wide characters) into `dest` and NUL-terminates it.
 *
 * @address 0x4e0930
 */
void ServerView::password_get(wchar_t *dest)
{
    network_server_globals *server = self;
    wcsncpy(dest, (wchar_t *)server->password, 8);
    dest[8] = 0;
}

/**
 * True if `server`'s join password is not the empty string.
 *
 * @address 0x4e08e0
 */
int32_t ServerView::password_is_set()
{
    network_server_globals *server = self;
    return wcsncmp((wchar_t *)server->password, L"", 8) != 0;
}

/**
 * Copies up to 8 wide characters from `source` into `server`'s password field and forces a
 * NUL terminator.
 *
 * @address 0x4e0910
 */
void ServerView::password_set(const wchar_t *source)
{
    network_server_globals *server = self;
    wcsncpy((wchar_t *)server->password, source, 8);
    server->password[8] = 0;
}

/**
 * While server's stats-logging flag is set, prints the dedicated server status roughly every
 * 15000ms.
 *
 * @address 0x4e1520
 */
uint32_t ServerView::status_periodic_print()
{
    network_server_globals *server = self;
    large_integer counter;

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    if ((server->flags >> 2 & 1) != 0) {
        int32_t now_ms;

        QueryPerformanceCounter((LARGE_INTEGER *)&counter);
        now_ms = (int32_t)((counter.quad_part * 1000) / performance_frequency);
        if ((uint32_t)(now_ms - network_server_status_last_print_ms) > 15000) {
            sv_status();
            network_server_status_last_print_ms = now_ms;
        }
    }
    return 1;
}

/**
 * Reads the build/version word from the most recently queued pending connection and checks it
 * against an accepted range, then -- if in range -- checks the session has room and is ready,
 * finally searching for a free machine slot.
 *
 * @address 0x4e0850
 */
int32_t ServerView::validate_join_request()
{
    network_server_globals *server = self;
    int32_t build;

    if (network_pending_connection_count > 0) {
        build = network_pending_connections[network_pending_connection_count - 1].first_payload_word;
    }

    if (build > 0x9663f) {
        if (build > 0x96640) {
            return 5;
        }
        if (server->session.player_count < (int16_t)(int8_t)server->session.maximum_players ) {
            int32_t i;

            if ((server->flags & 1) == 0) {
                return 7;
            }
            for (i = 0; i < 16; i = i + 1) {
                if (server->machines[i].machine_id == -1) {
                    return 0;
                }
            }
        }
        return 6;
    }
    return 4;
}

/**
 * Sends `data` through network_channel_queue_message to every machine slot whose flags bit 0x02 is set and
 * whose channel is alive (not k_network_channel_dead) and either connected or `force` is set.
 * Returns false if any qualifying send fails.
 *
 * @address 0x4e19c0
 */
char ServerView::broadcast_to_all(int32_t param_1, void *data, int32_t param_3, int32_t param_4, char force, int32_t param_6)
{
    network_server_globals *server = self;
    char ok;
    int32_t i;

    ok = 1;
    for (i = 0; i < 16; i = i + 1) {
        network_machine *machine;
        network_channel *channel;
        char connected;

        machine = &server->machines[i];
        channel = machine->channel;
        connected = (channel != 0) ? channel->connected : 0;

        if ((machine->flags & 0x02) != 0 &&
            (connected != 1 || force != 0) &&
            channel != 0 &&
            (channel->flags & 0x10) == 0) {
            uint8_t status;
            char sent;

            status = (uint8_t)(param_1 != 0);
            sent = network_channel_queue_message(data, &status, 1, param_3, param_4, param_6);
            if (sent == 0) {
                ok = 0;
            }
        }
    }
    return ok;
}

/**
 * out/phase4/networking_functions.md's summary claims message type 0x1b, but
 * network_game_process_incoming_message.c's own verified dispatch switch (this batch) shows
 * `case 0x1c: FUN_004e2790(param_1);` -- type 0x1b is instead handled inline inside the
 * dispatcher itself (decode + a direct call to network_game_client_apply_position_update). The
 * dispatcher's literal call table is trusted here instead of the low-confidence (0.3) summary.
 * This function latches an incoming 32-byte game/map data block into the client's pending-state
 * fields.
 *
 * @address 0x4e2790
 */
uint32_t ServerMessageHandlers::client_map_data(uint8_t *record, int32_t length)
{
    network_server_globals *server = self;
    uint32_t body[8];
    int16_t out_type;
    uint16_t version_used;
    uint8_t *s = (uint8_t *)server;

    if (*(int16_t *)(s + 4) == 1 && data_packet_group_decode_packet((length -= 2, (int16_t *)&length), &network_game_messages_group, body, record + 2, &out_type, &version_used, 5) != 0 && s[0x9f8] == 0 &&
        network_player_entry_validate((network_player_entry *)body) != 0) {
        memcpy(s + 0x9d8, body, sizeof(body));
        s[0x9f8] = 1;
    }
    return 1;
}

/**
 * out/phase4/networking_functions.md's summary claims message type 0x1d, but
 * network_game_process_incoming_message.c's own verified dispatch switch (this batch) shows
 * `case 0x1e: FUN_004e2870();` (case 0x1d instead reaches FUN_004e2810, the settings relay) --
 * the dispatcher's literal call table is trusted here instead of the low-confidence (0.3)
 * summary. Forwards to network_machine_timer_start; the client-side counterpart of the
 * FUN_004e2630 server handler, gated on role == 1 and decode class 5.
 *
 * @address 0x4e2870
 */
uint32_t ServerMessageHandlers::client_retry_schedule(network_machine *machine, uint8_t *record, int32_t length)
{
    network_server_globals *server = self;
    uint32_t body[1];
    int16_t out_type;
    uint16_t version_used;

    if (*(int16_t *)((uint8_t *)server + 4) == 1 && data_packet_group_decode_packet((length -= 2, (int16_t *)&length), &network_game_messages_group, body, record + 2, &out_type, &version_used, 5) != 0) {
        network_machine_timer_start(machine, 0);
    }
    return 1;
}

/**
 * out/phase4/networking_functions.md's summary claims message type 0x1c, but
 * network_game_process_incoming_message.c's own verified dispatch switch (this batch) shows
 * `case 0x1d: FUN_004e2810();` (case 0x1c instead reaches FUN_004e2790, the map-data latch) --
 * the dispatcher's literal call table is trusted here instead of the low-confidence (0.3)
 * summary. Forwards to network_game_settings_broadcast_send, the same forward the server-side
 * FUN_004e24d0 handler makes, but gated on role == 1 (client) and decode class 5 instead of 3.
 *
 * @address 0x4e2810
 */
uint32_t ServerMessageHandlers::client_settings_relay(uint8_t *record, int32_t length)
{
    network_server_globals *server = self;
    uint32_t body[8];
    int16_t out_type;
    uint16_t version_used;

    if (*(int16_t *)((uint8_t *)server + 4) == 1 && data_packet_group_decode_packet((length -= 2, (int16_t *)&length), &network_game_messages_group, body, record + 2, &out_type, &version_used, 5) != 0) {
        return network_game_settings_broadcast_send((uint32_t)server, body);
    }
    return 1;
}

/**
 * out/phase4/networking_functions.md's summary claims message types 0x14/0x25, but
 * network_game_process_incoming_message.c's own verified dispatch switch (this batch) shows
 * `case 0x15: FUN_004e2630();` -- the low-confidence (0.3) summary swapped this function's type
 * number with FUN_004e26a0's (which the same switch shows at `case 0x14: case 0x25:`); the
 * dispatcher's literal call table is trusted here instead. Forwards to
 * network_machine_check_build_version (FUN_004dff20, already written: EAX -> remote_version,
 * EDI -> machine).
 *
 * @address 0x4e2630
 */
uint32_t ServerMessageHandlers::build_version(network_machine *machine, uint8_t *record, int32_t length)
{
    network_server_globals *server = self;
    char body[0x100];
    int16_t out_type;
    uint16_t version_used;

    if (*(int16_t *)((uint8_t *)server + 4) == 0 && data_packet_group_decode_packet((length -= 2, (int16_t *)&length), &network_game_messages_group, body, record + 2, &out_type, &version_used, 3) != 0) {
        network_machine_check_build_version(body, machine);
    }
    return 1;
}

/**
 * out/phase4/networking_functions.md: "Handles message type 0x13 by decoding it and
 * forwarding to FUN_004e0590" -- network_client_connection_handshake_tick, already written in
 * an earlier batch.
 *
 * @address 0x4e25e0
 */
uint32_t ServerMessageHandlers::handshake_forward(uint8_t *record, int32_t length)
{
    network_server_globals *server = self;
    int32_t body[1];
    int16_t out_type;
    uint16_t version_used;

    if (*(int16_t *)((uint8_t *)server + 4) == 0 && data_packet_group_decode_packet((length -= 2, (int16_t *)&length), &network_game_messages_group, body, record + 2, &out_type, &version_used, 3) != 0) {
        network_client_connection_handshake_tick((int16_t)body[0], server);
    }
    return 1;
}

/**
 * rewrite)
 * address 0x4e2930, size 86 bytes
 * name confidence: 0.35   rewrite confidence: 0.85 (REWRITTEN; was 0.3)
 * out/phase4/networking_functions.md's summary claims message type 0x23, but
 * network_game_process_incoming_message.c's own verified dispatch switch (this batch) shows
 * `case 0x24: FUN_004e2930();` ('$' is 0x24, not 0x23) -- the dispatcher's literal call table
 * is trusted here instead of the low-confidence (0.3) summary. The cleared bit
 *
 * @address 0x4e2930
 */
uint32_t ServerMessageHandlers::join_finalize_ack_role2(network_machine *machine, uint8_t *record, int32_t length)
{
    network_server_globals *server = self;
    uint32_t body[1];
    int16_t out_type;
    uint16_t version_used;

    if (*(int16_t *)((uint8_t *)server + 4) == 2 && data_packet_group_decode_packet((length -= 2, (int16_t *)&length), &network_game_messages_group, body, record + 2, &out_type, &version_used, 7) != 0) {
        *((uint8_t *)machine + 0xe) &= 0xfb;
    }
    return 1;
}

/**
 * Builds a timestamped acknowledgement (echoing `record`'s first dword plus the current
 * millisecond clock) as message type 3 and, when `channel` is not itself in listening mode,
 * queues it for reliable send.
 *
 * @address 0x4e2110
 */
uint32_t ServerMessageHandlers::keepalive(network_channel **channel, int32_t *record)
{
    network_channel *chan;
    large_integer counter;
    struct {
        int32_t echoed_value;
        int32_t timestamp_ms;
    } payload;
    uint16_t *packet;

    if (channel == 0) {
        return 0;
    }
    chan = *channel;
    if (chan == 0) {
        return 0;
    }
    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    payload.echoed_value = *record;
    payload.timestamp_ms = (int32_t)((counter.quad_part * 1000) / performance_frequency);
    packet = network_prepare_challenge_packet(3, &payload);
    if (packet != 0) {
        uint8_t reliable_flag = 0;
        if ((chan->flags & 1) == 0) {
            network_channel_reliable_pool_store(chan, packet, &reliable_flag, 1);
        }
        return 1;
    }
    return 0;
}

/**
 * If the queued message's first dword is non-zero, skips it via FUN_004ec670. Otherwise, if
 * FUN_004ec590 reports true, resolves the local player's datum and stores the elapsed time
 * since server+0x9c0 into datum+0xdc.
 *
 * @address 0x4e20b0
 */
uint32_t ServerMessageHandlers::ping_timestamp(int32_t **message)
{
    network_server_globals *server = self;
    uint8_t *player;
    uint8_t decode_scratch[5];

    if (**message != 0) {
        message_delta_decode_compound_field_staged(message);
        return 1;
    }
    if (message_delta_decode_compound_field(message, decode_scratch) == 1) {
        player = (uint8_t *)datum_get();
        if (player != 0) {
            int32_t stored_time = *(int32_t *)((uint8_t *)server + 0x9c0);
            int32_t now = time_query_performance_counter_ms();
            ((struct player *)player)->ping = now - stored_time;
        }
    }
    return 1;
}

/**
 * out/phase4/networking_functions.md: "Handles message type 0x11 by decoding it and
 * triggering a session-wide player-count broadcast" -- network_game_broadcast_player_set_changed
 * (FUN_004e1bf0, this batch). Same decode shape as every sibling handler.
 *
 * @address 0x4e2530
 */
uint32_t ServerMessageHandlers::player_count_broadcast(uint8_t *record, int32_t length)
{
    network_server_globals *server = self;
    uint32_t body[1];
    int16_t out_type;
    uint16_t version_used;
    int16_t state = *(int16_t *)((uint8_t *)server + 4);

    if ((state == 0 || state == 1) && data_packet_group_decode_packet((length -= 2, (int16_t *)&length), &network_game_messages_group, body, record + 2, &out_type, &version_used, 3) != 0) {
        network_game_broadcast_player_set_changed(server);
    }
    return 1;
}

/**
 * out/phase4/networking_functions.md: "Handles message type 0x12, conditionally
 * triggering a player-count broadcast after an approval check" -- the approval check is
 * network_player_entry_update (FUN_004de5f0, already written), whose own signature is
 * (network_player_entry *incoming, network_game_session *session); on success this broadcasts
 * the player set change exactly like the type-0x11 handler.
 *
 * @address 0x4e2580
 */
uint32_t ServerMessageHandlers::player_entry_update(uint8_t *record, int32_t length)
{
    network_server_globals *server = self;
    uint32_t body[8];
    int16_t out_type;
    uint16_t version_used;

    if (*(int16_t *)((uint8_t *)server + 4) == 0 && data_packet_group_decode_packet((length -= 2, (int16_t *)&length), &network_game_messages_group, body, record + 2, &out_type, &version_used, 3) != 0 &&
        network_player_entry_update((network_player_entry *)body, (network_game_session *)((uint8_t *)server + 8)) != 0) {
        network_game_broadcast_player_set_changed(server);
    }
    return 1;
}

/**
 * out/phase4/networking_functions.md's summary claims message type 0x15, but
 * network_game_process_incoming_message.c's own verified dispatch switch (this batch) shows
 * `case 0x14: case 0x25: FUN_004e26a0();` -- the low-confidence (0.3) summary swapped this
 * function's type number with FUN_004e2630's; the dispatcher's literal call table is trusted
 * here instead. Forwards to network_machine_timer_start, already written (blam-cc: ESI ->
 * machine, stack -> duration_ms).
 *
 * @address 0x4e26a0
 */
uint32_t ServerMessageHandlers::retry_schedule(network_machine *machine, uint8_t *record, int32_t length)
{
    network_server_globals *server = self;
    uint32_t body[1];
    int16_t out_type;
    uint16_t version_used;

    if (*(int16_t *)((uint8_t *)server + 4) == 0 && data_packet_group_decode_packet((length -= 2, (int16_t *)&length), &network_game_messages_group, body, record + 2, &out_type, &version_used, 3) != 0) {
        network_machine_timer_start(machine, 0);
    }
    return 1;
}

/**
 * out/phase4/networking_functions.md: "Server-side handler for message type 0x10
 * that decodes its payload and passes it to FUN_004df0e0" -- the already-written
 * network_game_settings_broadcast_send. Follows the exact decode-then-forward shape shared by
 * every sibling handler in this cluster (see network_game_process_incoming_message.c).
 *
 * @address 0x4e24d0
 */
uint32_t ServerMessageHandlers::settings_relay(uint8_t *record, int32_t length)
{
    network_server_globals *server = self;
    uint32_t body[8];
    int16_t out_type;
    uint16_t version_used;

    if (*(int16_t *)((uint8_t *)server + 4) == 0 && data_packet_group_decode_packet((length -= 2, (int16_t *)&length), &network_game_messages_group, body, record + 2, &out_type, &version_used, 3) != 0) {
        return network_game_settings_broadcast_send((uint32_t)server, body);
    }
    return 1;
}

/**
 * out/phase4/networking_functions.md's summary claims message type 0x1e, but
 * network_game_process_incoming_message.c's own verified dispatch switch (this batch) shows
 * `case 0x23: FUN_004e28d0();` -- the dispatcher's literal call table is trusted here instead
 * of the low-confidence (0.3) summary. This is the third of three role-gated forwarders to
 * network_game_settings_broadcast_send (roles 0, 1 and 2 at message types 0x10, 0x1d and 0x23
 * respectively), gated on role == 2 and decode class 7.
 *
 * @address 0x4e28d0
 */
uint32_t ServerMessageHandlers::settings_relay_role2(uint8_t *record, int32_t length)
{
    network_server_globals *server = self;
    uint32_t body[8];
    int16_t out_type;
    uint16_t version_used;

    if (*(int16_t *)((uint8_t *)server + 4) == 2 && data_packet_group_decode_packet((length -= 2, (int16_t *)&length), &network_game_messages_group, body, record + 2, &out_type, &version_used, 7) != 0) {
        return network_game_settings_broadcast_send((uint32_t)server, body);
    }
    return 1;
}

/**
 * out/phase4/networking_functions.md: "Resets the per-round update counters and
 * completion flags on the object at in_EAX, increments its round counter, and calls network_game_session_reset_defaults
 * to continue setup." Same host offsets (+0x9b8, +0x9bc-region, +0x3b0) as
 * network_game_server_host_new.c; see that file's header for the field-matching evidence and the
 * same "+0x3b0 lands in session.unknown_3a2[10]" UNSURE note.
 *
 * @address 0x4df640
 */
void HostServerView::round_reset()
{
    network_server_globals *host = self;
    host->handshake_timer.remaining_ms = 0;
    host->handshake_timer.last_tick_ms = 0;
    host->unknown_9d0 = 0;
    *(uint32_t *)&host->handshake_state = 0;
    host->update_tick = 0;
    host->first_join_ms = 0;
    host->scenario_announced = 0;
    host->new_server_pending = 0;
    host->join_finalize_pending = 0;
    *(int32_t *)((uint8_t *)host + 0x3b0) = *(int32_t *)((uint8_t *)host + 0x3b0) + 1;
    network_game_session_reset_defaults();
}

/**
 * out/phase4/networking_functions.md: "Sends the one-time scenario/challenge
 * announcement packets (via FUN_004ec940/FUN_004e19c0 and network_prepare_challenge_packet) the first time it is
 * called for this game, then latches a done flag." host->unknown_9f9/unknown_9b8 match
 * network_game_server_host_new.c's established offsets on network_server_globals.
 *
 * @address 0x4df1c0
 */
int32_t HostServerView::send_scenario_announcement()
{
    network_server_globals *host = self;
    int32_t result;
    void *payload;
    int32_t encode_result;
    int32_t challenge_packet;
    uint32_t challenge_payload[4];

    result = 1;
    if (host->scenario_announced == 0) {
        message_delta_parameters_protocol_send_update();
        payload = (uint8_t *)host + 8;
        encode_result = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 0x21, 0, &payload, 0, 1, 0);
        if (encode_result > 0) {
            network_session_broadcast_to_all(network_server, 1, &message_delta_definition_table, 1, 0, 1, 3);
        }
        result = encode_result > 0;
        if (encode_result > 0) {

            challenge_packet = (int32_t)network_prepare_challenge_packet(0x0a, challenge_payload);
            if (challenge_packet != 0) {
                if (network_session_broadcast_to_all(network_server, 0, (void *)(uint32_t)challenge_packet, 1, 0, 1, 3) != 0) {
                    host->scenario_announced = 1;
                    result = 1;
                }
            }
        }
    }
    host->update_tick = 0;
    return result;
}

/**
 * out/phase4/networking_functions.md: "If a network host is active and not in the
 * special team-sync case, resets the host's map-load flag and history state and disposes the
 * host globals; otherwise defers to network_host_update_tick." session->unknown_3ac (the map-loaded flag)
 * matches types/networking.h's network_game_session exactly when reached through
 * network_server->session or network_client->session directly (unlike
 * network_game_server_host_create.c's host-relative offset, which lands elsewhere -- see that
 * file's UNSURE note).
 *
 * @address 0x4ddd90
 */
int32_t HostServerView::shutdown_or_defer()
{
    network_game_session *session;

    if (network_server != 0) {
        if (network_host_handoff_requested != 1 || ((network_server->flags >> 2) & 1) == 0) {

            return network_host_update_tick(network_server);
        }
        network_game_mode = 0;
        main_menu_music_stop();
        if (network_server != 0) {
            session = &network_server->session;
        } else if (network_client != 0) {
            session = &network_client->session;
        } else {
            session = 0;
        }
        if (session->map_loaded != 0) {
            chimera__load_ui_map(1);
        }
        session->map_loaded = 0;
        network_client_globals_dispose();
        if (network_server != 0) {
            network_game_server_host_dispose(network_server);
            network_server = 0;
            network_server_host_valid = 0;
        }
        *(uint16_t *)&split_screen_quit_prompt_string = 0xffff;
        network_join_error_reason = 0;
        *((uint8_t *)&split_screen_quit_prompt_string + 3) = 1;
    }
    return 1;
}

/**
 * out/phase4/networking_functions.md: "Per-tick client-side network update handler:
 * pulls the next queued packet, refreshes the map/variant cycle list periodically, and
 * dispatches processing to one of three state-specific handlers based on [state]." host->flags
 * bit1 (k_network_server_host, matches network_game_server_host_new.c) and host->unknown_004
 * (the state dispatched on 0/1/2, matching network_game_server_host_dispose.c's own 0/2 test)
 * match types/networking.h.
 *
 * @address 0x4def80
 */
char HostServerView::update_tick()
{
    network_server_globals *host = self;
    char service_result;
    char proceed;
    network_channel *new_child;
    network_resolved_address sender;
    uint32_t now_ms;

    service_result = 1;
    if ((host->flags >> 1) & 1) {
        new_child = 0;
        service_result = network_channel_service(host->listen_channel, 0, &new_child);
        if (service_result == 1) {
            proceed = 1;
            if (new_child != 0) {
                service_result = network_server_count_machines_and_resolve_address(host, new_child);
                if (service_result == 1) {
                    network_channel_remote_address_or_default(new_child, &sender);
                    proceed = 1;
                } else {
                    proceed = (char)network_channel_remove_child(host->listen_channel, new_child);
                }
            }
            service_result = 0;
            if (proceed != 0) {
                now_ms = time_query_performance_counter_ms();
                if ((uint32_t)((int32_t)host->last_stamp_ms + 3000) < now_ms) {
                    network_map_cycle_list_broadcast();
                    host->last_stamp_ms = now_ms;
                }
                service_result = network_server_service_machines_tick(host);
                if (service_result == 0) {
                    return 0;
                }
                if (host->state == 0) {
                    return network_server_heartbeat_tick(host);
                }
                if (host->state != 1) {
                    if (host->state != 2) {
                        return 0;
                    }
                    return network_server_resend_challenge_periodic(host);
                }
                return network_server_status_periodic_print(host);
            }
        }
    }
    return service_result;
}

/**
 * EAX machine, stack response: the CD key check of a joining
 * machine with its remote ip (the local address 0x006869b0 for loopback 127.0.0.1), its challenge (+0x52) and CD
 * key local id (+0x5c). (Name kept.)
 *
 * @address 0x4e0ab0
 */
uint8_t MachineView::reset_state(const char *response)
{
    network_machine *machine = self;
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

/**
 * Sets k_network_machine_version_mismatch on `machine` when `remote_version` matches this
 * build's version string exactly (see the polarity UNSURE note above).
 *
 * @address 0x4dff20
 */
void MachineView::check_build_version(const char *remote_version)
{
    network_machine *machine = self;
    const uint8_t *local;
    const uint8_t *remote;
    int32_t equal;

    local = (const uint8_t *)network_build_string;
    remote = (const uint8_t *)remote_version;
    equal = 0;
    while (*local == *remote) {
        if (*local == 0) {
            equal = 1;
            break;
        }
        local = local + 1;
        remote = remote + 1;
    }
    if (equal) {
        machine->flags |= k_network_machine_version_mismatch;
    }
}

/**
 * types/networking.h cites this address directly: "network_machine (0x4dec40 init,
 * 0x4df690 reset, ...)". flags |= k_network_machine_pending, timer_14/timer_18/unknown_50, and
 * the 0xd-dword (0x34-byte) zero of connect_state[0x34] at +0x1c all match exactly.
 * The leading network_channel_remote_address_or_default call takes EAX = machine->channel and
 * ECX = &local scratch; its result is unused (verified 0x4df6a8..0x4df6b9).
 *
 * @address 0x4df690
 */
int32_t MachineView::reset()
{
    network_machine *machine = self;
    network_resolved_address sender;

    network_channel_remote_address_or_default(machine->channel, &sender);
    machine->flags = machine->flags | k_network_machine_pending;
    machine->disconnect_timer_active = 0;
    machine->timer_14 = 0;
    machine->timer_18 = 0;
    machine->player_joined = 0;
    memset(machine->connect_state, 0, sizeof(machine->connect_state));
    return 1;
}

/**
 * out/phase4/networking_functions.md: "Starts a timer on the object at unaff_ESI:
 * records the current time, marks it active, and computes its expiry as now plus the given
 * duration." Fields +0x10/+0x14/+0x18 match network_machine's unknown_10/timer_14/timer_18
 * exactly (same object network_machine_reset.c clears).
 *
 * @address 0x4df090
 */
void MachineView::timer_start(int32_t duration_ms)
{
    network_machine *machine = self;
    large_integer counter;
    int32_t now_ms;

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    now_ms = (int32_t)((counter.quad_part * 1000) / performance_frequency);
    machine->timer_14 = now_ms;
    machine->disconnect_timer_active = 1;
    machine->timer_18 = now_ms + duration_ms;
}

}
