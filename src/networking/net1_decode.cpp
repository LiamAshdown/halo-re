#include "halo/networking/net1_decode.hpp"
#include "halo/networking/net1_dispatch.hpp"
#include <string.h>
#include <wchar.h>
#include "halo/items/api.hpp"

extern "C" {
extern int16_t network_game_mode;
extern uint8_t network_action_apply_active;
extern void object_delete_by_pooled_node_id(void **context);
extern void object_type_override_call_0x70_release_node(void **context, network_client_globals *client);
extern void hud_receive_item_message(void **context);
extern void game_engine_apply_player_join_message(void **context);
extern void game_engine_apply_player_spawn_loadout_message(void **context);
extern void unit_dispatch_seat_exit_message(void **context);
extern uint8_t game_engine_apply_player_interaction_message(void **context);
extern void player_effect_mark_damage_direction_dispatch(void **context);
extern void unit_apply_network_control_update(void **context);
extern uint8_t game_engine_apply_kill_streak_message(void **context);
extern void chat_dispatch_incoming(void **context);
extern void game_engine_invoke_profile_post_update_callback(network_client_globals *client, void **context);
extern void game_engine_apply_player_profile_entry(void **context);
extern void game_engine_dispatch_end_game_notification(void **context);
extern void game_engine_apply_partial_round_reset_message(void **context);
extern void game_engine_handle_kill_feed_network_event(void **context);
extern void game_engine_handle_sound_status_event(void **context);
extern void game_engine_client_apply_team_assignment(void **context);
extern void unit_scripting_set_or_drop_weapon(void **context);
extern void unit_spawn_with_starting_weapons(void **context);
extern void unit_network_create_update_apply(void **context);
extern void projectile_create_from_network(void **context);
extern int32_t network_channel_key_send_state(network_client_globals *client, void **context);
extern void message_delta_parameters_protocol_receive_update(void **context);
extern void message_delta_definitions_invoke_field_bindings(void);
extern void player_update_client_local_player_update_from_network(void **context);
extern void player_update_client_local_player_vehicle_update_from_network(void **context);
extern void player_update_client_remote_player_action_update_from_network(void **context);
extern void player_update_remote_player_action_update_apply(void **context);
extern void player_update_client_remote_player_position_delta_from_network(void **context);
extern void player_update_client_remote_player_vehicle_position_delta_from_network(void **context);
extern void player_update_client_remote_player_total_biped_update_from_network(void **context);
extern void player_update_client_remote_player_total_vehicle_update_from_network(void **context);
extern void game_engine_spawn_or_replay_netgame_equipment(void **context);
extern void projectile_detonation_message_apply(void **context);
extern void object_apply_linked_impulse(void **context);
extern void object_apply_shield_charge_and_notify(void **context);
extern void projectile_attach_apply(void **context);
extern void network_player_ping_field_update_and_report(void **context);
extern void network_client_handle_server_text_message(void **context);
extern void network_channel_remote_address_or_default(network_channel *channel, network_resolved_address *out_address);
extern int32_t message_delta_decode_begin(message_delta_decode_state *state, bit_stream *stream);
extern int32_t message_delta_decode_array_field(void **context);
extern void network_game_action_apply(void **context, network_client_globals *client);
extern void network_disconnect_notify_dropped_machines(network_client_globals *client);
extern uint8_t network_incoming_message_scratch[0x510];
extern int32_t network_channel_incoming_read_item(network_channel *channel, uint8_t *destination, int32_t *out_bit_offset, int32_t *out_remaining_bits, s_network_address *out_address, int32_t max_item_bits);
extern char network_incoming_item_dispatch(network_client_globals *client, uint32_t item_flag, bit_stream *stream, const uint32_t *sender);
typedef struct network_item_stream {
    bit_stream stream;
    uint32_t bit_count;
} network_item_stream;
extern void main_queue_map_change_by_name_or_clear(void);
extern int32_t join_ui_state;
extern int32_t interface_loading_screen_request_id;
extern char network_game_settings_ack_send(uint8_t *client, int16_t template_row);
extern void *shell_product_id;
extern uint8_t profile_globals_block[0x1ffc];
extern void gcd_compute_response(void *a, void *request, uint8_t *out);
extern uint16_t *network_prepare_challenge_packet(int32_t message_type, void *payload);
extern char network_channel_stream_flush(network_channel_stream *stream, network_channel *channel, char mode);
extern int32_t bit_stream_write_bits_chunked(bit_stream *stream, const uint32_t *values, int32_t total_bit_count);
extern network_server_globals *network_server;
extern game_time_globals *game_time;
extern random_seed random_seed_global;
extern int64_t performance_frequency;
extern void update_client_advance_read_cursor(void *payload);
extern char network_game_action_queue_drain(network_client_globals *client, bit_stream *stream, const uint32_t *sender);
extern uint16_t *network_message_read_sized_buffer(uint16_t *buffer, int32_t capacity, bit_stream *stream);
extern char network_game_message_decode_dispatch(network_client_globals *client, uint16_t *record, int32_t record_length, const uint32_t *sender);
extern int32_t data_packet_group_decode_packet(int16_t *remaining_length, data_packet_group *group, void *decoded_body, uint8_t *buffer, int16_t *out_type, uint16_t *out_version_used, int16_t expected_class);
extern data_packet_group network_game_messages_group;
extern int32_t network_game_search_results_add_or_update(network_game_search_entry *results, const uint8_t *announcement);
extern void network_session_player_join_notify(network_client_globals *client, const uint32_t *source);
extern void network_client_timer_default_or_disconnect(network_client_globals *client);
extern int32_t network_connection_finalize_join(uint16_t *connection);
extern char network_player_join_finalize(network_client_globals *client, void *entry);
extern uint8_t network_session_player_table_index_apply(network_client_globals *client, int32_t table_index, const uint8_t *candidate);
extern void network_connection_retransmit_if_overdue(const uint32_t *sender_address, network_client_globals *client, uint32_t deadline_ms, int32_t remote_time);
extern int8_t network_game_state_update_receive(network_client_globals *client, void *decoded_body);
typedef int32_t (*network_game_message_handler_proc)(network_client_globals *client, const void *record, int32_t record_length, const uint32_t *sender);
extern int32_t network_game_client_decode_beacon_reply();
extern int32_t network_game_client_decode_pong_reply();
extern int32_t network_game_decode_settings_request();
extern int32_t network_game_client_decode_join_accepted();
extern int32_t network_game_client_decode_connect_rejected();
extern int32_t network_game_client_decode_join_complete();
extern int32_t network_game_client_decode_settings_or_ack();
extern int32_t network_game_client_decode_player_config_value();
extern int32_t network_game_client_decode_and_discard_join_message();
extern int32_t network_game_client_decode_and_discard_ingame_message();
extern int32_t network_game_client_decode_join_finalize_message();
extern int32_t network_game_client_decode_join_finalize_ack();
extern int32_t network_game_client_decode_state_update_chunk();
extern int32_t network_game_client_decode_player_join_chunk();
extern int32_t network_game_client_decode_player_slot_chunk();
extern int32_t network_game_client_decode_sync_complete();
extern int32_t network_game_message_decode_replicated_command();
extern int32_t network_game_message_decode_ingame_notification();
extern uint8_t network_host_handoff_requested;
extern void chat_close(void);
extern void network_client_timer_schedule(int32_t delay_ms, int32_t context, network_client_globals *client);
}

namespace halo::networking {

namespace {

static void network_game_action_apply_shared(void **context, network_client_globals *client, int32_t type_id)
{
    switch (type_id) {
    case 0x06: hud_receive_item_message(context); break;
    case 0x0b: player_effect_mark_damage_direction_dispatch(context); break;
    case 0x0f: chat_dispatch_incoming(context); break;
    case 0x1a: game_engine_client_apply_team_assignment(context); break;
    case 0x21: network_channel_key_send_state(client, context); break;
    case 0x22:
        message_delta_parameters_protocol_receive_update(context);
        message_delta_definitions_invoke_field_bindings();
        break;
    case 0x35: network_player_ping_field_update_and_report(context); break;
    }
}

}

/**
 * out/phase4/networking_functions.md summary ("Applies a single queued network-game
 * action by type id, calling the specific per-type handler (ammo pickups, player/vehicle
 * network updates, etc.)").
 *
 * @address 0x4da320
 */
void GameClientView::action_apply(void **context)
{
    network_client_globals *client = self;
    int32_t type_id;

    if (network_game_mode != 1 && network_game_mode != 2) {
        return;
    }
    network_action_apply_active = 1;
    type_id = ((int32_t *)context[0])[1];
    if (network_game_mode == 2) {
        network_game_action_apply_shared(context, client, type_id);
        network_action_apply_active = 0;
        return;
    }
    switch (type_id) {
    case 0x00: object_delete_by_pooled_node_id(context); break;
    case 0x01:
    case 0x02:
    case 0x03:
    case 0x04:
    case 0x05: object_type_override_call_0x70_release_node(context, client); break;
    case 0x07: game_engine_apply_player_join_message(context); break;
    case 0x08: game_engine_apply_player_spawn_loadout_message(context); break;
    case 0x09: unit_dispatch_seat_exit_message(context); break;
    case 0x0a: game_engine_apply_player_interaction_message(context); break;
    case 0x0c: unit_apply_network_control_update(context); break;
    case 0x0e: game_engine_apply_kill_streak_message(context); break;
    case 0x10:
    case 0x11:
    case 0x12:
    case 0x13:
    case 0x14: game_engine_invoke_profile_post_update_callback(client, context); break;
    case 0x15: game_engine_apply_player_profile_entry(context); break;
    case 0x16: game_engine_dispatch_end_game_notification(context); break;
    case 0x17: game_engine_apply_partial_round_reset_message(context); break;
    case 0x18: game_engine_handle_kill_feed_network_event(context); break;
    case 0x19: game_engine_handle_sound_status_event(context); break;
    case 0x1b: unit_scripting_set_or_drop_weapon(context); break;
    case 0x1c: unit_spawn_with_starting_weapons(context); break;
    case 0x1d: unit_network_create_update_apply(context); break;
    case 0x1e: projectile_create_from_network(context); break;
    case 0x1f: halo::items::equipment_create_from_creation_message(context); break;
    case 0x20: halo::items::weapon_create_from_creation_message(context); break;
    case 0x23: player_update_client_local_player_update_from_network(context); break;
    case 0x24: player_update_client_local_player_vehicle_update_from_network(context); break;
    case 0x25: player_update_client_remote_player_action_update_from_network(context); break;
    case 0x26: player_update_remote_player_action_update_apply(context); break;
    case 0x27: player_update_client_remote_player_position_delta_from_network(context); break;
    case 0x28: player_update_client_remote_player_vehicle_position_delta_from_network(context); break;
    case 0x29: player_update_client_remote_player_total_biped_update_from_network(context); break;
    case 0x2a: player_update_client_remote_player_total_vehicle_update_from_network(context); break;
    case 0x2b: halo::items::weapon_predict_ammo(context); break;
    case 0x2c: halo::items::weapon_add_ammunition(context); break;
    case 0x2d: halo::items::weapon_apply_ammo_correction(context); break;
    case 0x2e: halo::items::weapon_apply_ammo_correction_and_resync(context); break;
    case 0x2f: game_engine_spawn_or_replay_netgame_equipment(context); break;
    case 0x30: projectile_detonation_message_apply(context); break;
    case 0x31: object_apply_linked_impulse(context); break;
    case 0x32: object_apply_shield_charge_and_notify(context); break;
    case 0x33: projectile_attach_apply(context); break;
    case 0x37: network_client_handle_server_text_message(context); break;
    default: network_game_action_apply_shared(context, client, type_id); break;
    }
    network_action_apply_active = 0;
}

/**
 * Original `network_game_action_queue_drain`, moved unchanged; recovered notes are in docs/original/networking/net1_decode.md.
 *
 * @address 0x4db870
 */
char GameClientView::action_queue_drain(bit_stream *stream, const uint32_t *sender)
{
    network_client_globals *client = self;
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

/**
 * already named)
 * address 0x4db180, size 388 bytes
 * name confidence: 0.6   rewrite confidence: 0.85 (REWRITTEN; was 0.4)
 * out/phase4/networking_types_notes.md/header comment: "Drains the channel's
 * incoming ring buffer, extracting queued items bit-by-bit and dispatching each to the
 * network-message/action processor." client->channel->incoming matches
 * types/networking.h's network_channel::incoming (a types/memory.h circular_buffer).
 *
 * @address 0x4db180
 */
int32_t GameClientView::process_incoming_messages()
{
    network_client_globals *client = self;
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

/**
 * be recovered from this function's own decompiled body, so 0 is passed (row 0, matching
 * network_game_settings_packet_send.c's own unindexed use of the same template table).
 *
 * @address 0x4d9800
 */
int32_t GameClientView::settings_packet_receive(const uint32_t *request)
{
    network_client_globals *client = self;
    const uint8_t *pa, *pb;
    uint8_t a, b;
    int32_t cmp;
    uint32_t saved_last_dword;
    int32_t i;

    if (*(int16_t *)(request + 0x68) < 0 || *(int16_t *)(request + 0x68) > 0x10) {
        return 0;
    }

    pa = (const uint8_t *)(request + 0x21);
    pb = (const uint8_t *)client->session.server_name;
    while (1) {
        a = *pa;
        b = *pb;
        if (a != b) {
            cmp = (1 - (uint32_t)(a < b)) - (uint32_t)((a < b) != 0);
            goto compare_done;
        }
        if (a == 0) {
            cmp = 0;
            break;
        }
        a = pa[1];
        b = pb[1];
        if (a != b) {
            cmp = (1 - (uint32_t)(a < b)) - (uint32_t)((a < b) != 0);
            goto compare_done;
        }
        pa = pa + 2;
        pb = pb + 2;
        if (a == 0) {
            cmp = 0;
            break;
        }
    }
compare_done:
    if (cmp != 0) {
        main_queue_map_change_by_name_or_clear();
        if (join_ui_state != 1) {
            if (join_ui_state != 2 && join_ui_state == 4) {
                interface_loading_screen_request_id = -1;
            }
            join_ui_state = 8;
        }
    }

    saved_last_dword = ((const uint32_t *)&client->session)[235];
    for (i = 0; i < 236; i = i + 1) {
        ((uint32_t *)&client->session)[i] = request[i];
    }
    *(uint32_t *)&client->session.map_loaded = saved_last_dword;

    if (*(uint8_t *)&client->pad_ee2 == 0) {
        network_game_settings_ack_send((uint8_t *)client, 0);
        *(uint8_t *)&client->pad_ee2 = 1;
        return 1;
    }
    return 1;
}

/**
 * out/phase4/networking_functions.md summary ("Builds and sends a large
 * game-settings/map-data packet (including the player's name) to a newly-joining client whose
 * session id doesn't yet match ours"). Word-indexed offsets on `unaff_EBX` (an `undefined2 *`)
 * resolve cleanly against types/networking.h's network_client_globals when doubled: word 0x76d
 * (byte 0xeda) is state, word 0x76f (byte 0xede) is unknown_ede, word 0x5cc (byte 0xb98) is
 * exactly &client->session.server_name, word 0x56e (byte 0xadc) is channel, word 0x771 (byte
 * 0xee2) is pad_ee2, word 0x788 (byte 0xf10) is unknown_f10.
 *
 * @address 0x4d94c0
 */
void GameClientView::settings_packet_send(const uint8_t *request)
{
    network_client_globals *client = self;

    uint8_t frame[0x2100];
    uint8_t *pa, *pb;
    uint8_t a, b;
    int32_t cmp;
    uint32_t *zero_fill;
    int32_t i;
    int32_t *challenge;
    network_channel *channel;
    int32_t bits_to_send;
    char retransmit_ok;

    if ((client->flags & 2) != 0) {
        return;
    }
    client->state = 2;
    client->machine_index = *(uint16_t *)(request + 0xc);

    pa = (uint8_t *)(request + 0x14);
    pb = (uint8_t *)client->session.server_name;
    while (1) {
        a = *pa;
        b = *pb;
        if (a != b) {
            cmp = (1 - (uint32_t)(a < b)) - (uint32_t)((a < b) != 0);
            goto compare_done;
        }
        if (a == 0) {
            cmp = 0;
            break;
        }
        a = pa[1];
        b = pb[1];
        if (a != b) {
            cmp = (1 - (uint32_t)(a < b)) - (uint32_t)((a < b) != 0);
            goto compare_done;
        }
        pa = pa + 2;
        pb = pb + 2;
        if (a == 0) {
            cmp = 0;
            break;
        }
    }
compare_done:
    if (cmp != 0) {
        main_queue_map_change_by_name_or_clear();
        if (join_ui_state != 1) {
            if (join_ui_state != 2 && join_ui_state == 4) {
                interface_loading_screen_request_id = -1;
            }
            join_ui_state = 8;
        }
    }

    zero_fill = (uint32_t *)frame;
    for (i = 0x23; i != 0; i = i - 1) {
        *zero_fill = 0;
        zero_fill = zero_fill + 1;
    }
    *(uint16_t *)zero_fill = 0;

    *(uint32_t *)(frame + 0x00) = *(uint32_t *)((uint8_t *)client + 0xb02);
    *(uint32_t *)(frame + 0x04) = *(uint32_t *)((uint8_t *)client + 0xb06);
    *(uint32_t *)(frame + 0x08) = *(uint32_t *)((uint8_t *)client + 0xb0a);
    *(uint32_t *)(frame + 0x0c) = *(uint32_t *)((uint8_t *)client + 0xb0e);
    *(uint8_t *)&client->pad_ee2 = 0;
    wcsncpy((wchar_t *)(frame + 0x10), (const wchar_t *)((uint8_t *)client + 0xaf0), 8);
    frame[0x6b] = *((uint8_t *)client + 0xf4c);
    *(uint16_t *)(frame + 0x20) = 0;
    gcd_compute_response(shell_product_id, (void *)request, frame + 0x22);

    for (i = 0; i < 0x1ffc; i = i + 1) {
        frame[0x90 + i] = profile_globals_block[i];
    }

    *(uint8_t *)(frame + 0x8a) = request[0xc];
    frame[0x8b] = 0;
    wcsncpy((wchar_t *)(frame + 0x6e), (const wchar_t *)(frame + 0x92), 0xb);
    frame[0x8c] = *(uint8_t *)&client->team_index;
    *(uint16_t *)(frame + 0x84) = 0;
    *(uint16_t *)(frame + 0x86) = *(uint16_t *)(frame + 0x1aa);
    *(uint16_t *)(frame + 0x88) = 0xffff;
    frame[0x8d] = 0xff;
    *(uint8_t *)&client->pad_ee2 = 1;

    challenge = (int32_t *)network_prepare_challenge_packet(0x0e, frame);
    if (challenge != 0) {
        channel = client->channel;
        bits_to_send = (uint32_t)(*(uint16_t *)challenge >> 4) * 8;
        if ((channel->flags & 1) == 0) {
            if ((((*(int32_t *)&((network_channel *)channel)->outgoing.stream.last_bit +
                   *(int32_t *)&((network_channel *)channel)->outgoing.stream.byte_cursor * -8) -
                  *(int32_t *)&((network_channel *)channel)->outgoing.stream.bit_cursor) + 1 < bits_to_send + 1) &&
                (retransmit_ok = network_channel_stream_flush(&channel->outgoing, channel, 1), retransmit_ok == 0)) {
                return;
            }
            {

                channel->send_budget = channel->send_budget + bits_to_send + 1;
                { uint32_t item_flag = 0; bit_stream_write_bits_chunked((bit_stream *)((uint8_t *)channel + 0x10), &item_flag, 1); }
                *((uint8_t *)channel + 0x2c) = 0;
                bit_stream_write_bits_chunked((bit_stream *)((uint8_t *)channel + 0x10), (const uint32_t *)(challenge), bits_to_send);
                *((uint8_t *)channel + 0x2c) = 0;
            }
        }
        client->flags = client->flags | 2;
    }
}

/**
 * out/phase4/networking_functions.md summary ("Processes an incoming sequenced
 * game-state update packet, growing the per-connection reassembly buffer as needed and handing
 * the payload off for application"). client+0xecc/+0xed0 match types/networking.h's
 * network_client_globals::unknown_ecc/unknown_ed0 exactly.
 *
 * @address 0x4d9d20
 */
int32_t GameClientView::state_update_receive(uint8_t *record)
{
    network_client_globals *client = self;
    int16_t current_capacity;
    int16_t target_capacity;
    uint32_t *fill;
    int32_t i;
    uint32_t local_buffer[194];
    uint32_t *src, *dst;
    int16_t copy_units;
    large_integer counter;
    int32_t now_ms;

    current_capacity = *(int16_t *)(record + 0xe);
    target_capacity = client->session.player_count;

    if (current_capacity < target_capacity) {
        fill = (uint32_t *)(record + 0x10) + (uint32_t)current_capacity * 8;
        for (i = ((int32_t)target_capacity - (int32_t)current_capacity & 0x7ffffff) << 3; i != 0; i = i - 1) {
            *fill = 0;
            fill = fill + 1;
        }
        for (i = 0; i != 0; i = i - 1) {
            *(uint8_t *)fill = 0;
            fill = (uint32_t *)((uint8_t *)fill + 1);
        }
        *(int16_t *)(record + 0xe) = target_capacity;
    }

    if (*(uint32_t *)record <= (uint32_t)client->last_update_id ||
        (network_server == 0 && (uint32_t)game_time->game_time == *(uint32_t *)(record + 8) &&
         *(uint32_t *)(record + 4) != random_seed_global)) {
        network_disconnect_notify_dropped_machines(client);
    }

    copy_units = *(int16_t *)(record + 0xe);
    src = (uint32_t *)(record + 0x10);
    dst = local_buffer;
    for (i = (uint32_t)(uint16_t)copy_units << 3; i != 0; i = i - 1) {
        *dst = *src;
        src = src + 1;
        dst = dst + 1;
    }
    for (i = 0; i != 0; i = i - 1) {
        *(uint8_t *)dst = (uint8_t)*src;
        src = (uint32_t *)((uint8_t *)src + 1);
        dst = (uint32_t *)((uint8_t *)dst + 1);
    }

    update_client_advance_read_cursor(local_buffer);
    client->last_update_id = *(uint32_t *)record;

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    now_ms = (int32_t)((counter.quad_part * 1000) / performance_frequency);
    client->last_update_received_ms = now_ms;
    return 1;
}

/**
 * out/phase4/networking_functions.md summary ("Dispatches a single incoming queue
 * entry either to the queued-game-action applier or to the network-message decode switch,
 * depending on an entry-type flag").
 *
 * @address 0x4db630
 */
char GameClientView::incoming_item_dispatch(uint32_t item_flag, bit_stream *stream, const uint32_t *sender)
{
    network_client_globals *client = self;
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

/**
 * FIXED (step 1, objdump -d 0x4dc020..0x4dc082): stack (buffer, length, sender address); the decoder gets &length after
 * the 2-byte header; the (class 6) message is decoded only to be dropped. Always returns 1.
 *
 * @address 0x4dc020
 */
int32_t ClientMessageDecoder::and_discard_ingame_message(const uint8_t *buffer, int32_t length, const uint32_t *sender_address)
{
    network_client_globals *client = self;
    network_resolved_address sender;
    uint8_t decoded_body[32];
    int16_t out_type;
    uint16_t out_version;

    network_channel_remote_address_or_default(client->channel, &sender);
    if (sender.address.ipv4 == *sender_address && client->state == 4) {
        length = length - 2;
        data_packet_group_decode_packet((int16_t *)&length, &network_game_messages_group, decoded_body,
                                        (uint8_t *)buffer + 2, &out_type, &out_version, 6);
    }
    return 1;
}

/**
 * FIXED (step 1, objdump -d 0x4dbfb0..0x4dc014): stack (buffer, length, sender address); the decoder gets &length after
 * the 2-byte header; the (class 2) message is decoded only to be dropped. Always returns 1.
 *
 * @address 0x4dbfb0
 */
int32_t ClientMessageDecoder::and_discard_join_message(const uint8_t *buffer, int32_t length, const uint32_t *sender_address)
{
    network_client_globals *client = self;
    network_resolved_address sender;
    uint8_t decoded_body[32];
    int16_t out_type;
    uint16_t out_version;

    network_channel_remote_address_or_default(client->channel, &sender);
    if (sender.address.ipv4 == *sender_address && client->state == 2) {
        length = length - 2;
        data_packet_group_decode_packet((int16_t *)&length, &network_game_messages_group, decoded_body,
                                        (uint8_t *)buffer + 2, &out_type, &out_version, 2);
    }
    return 1;
}

/**
 * FIXED (step 1, objdump -d 0x4db9a0..0x4dba0f): the message length is the stack argument; the decoder gets
 * EAX = &length after dropping the 2-byte message header, the body is decoded from buffer + 2, and the search list is
 * updated from the DECODED announcement (EBX = &decoded_body), not the raw buffer.
 *
 * @address 0x4db9a0
 */
int32_t ClientMessageDecoder::beacon_reply(const uint8_t *buffer, int32_t length)
{
    network_client_globals *client = self;
    uint8_t decoded_body[368];
    int16_t out_type;
    uint16_t out_version;

    if (client->state != 0) {
        return 1;
    }
    length = length - 2;
    if (data_packet_group_decode_packet((int16_t *)&length, &network_game_messages_group, decoded_body,
                                        (uint8_t *)buffer + 2, &out_type, &out_version, 1) != 0) {
        network_game_search_results_add_or_update((network_game_search_entry *)((uint8_t *)client + 4),
                                                   decoded_body);
    }
    return 1;
}

/**
 * FIXED (step 1, objdump -d 0x4dbcc0..0x4dbd3e): stack (buffer, length, sender address), the sender dereferenced once,
 * &length after the header for the decoder; a decoded (class 2) acceptance goes to
 * network_session_player_join_notify(EAX client, ECX &decoded). Returns 1 only then.
 *
 * @address 0x4dbcc0
 */
int32_t ClientMessageDecoder::join_accepted(const uint8_t *buffer, int32_t length, const uint32_t *sender_address)
{
    network_client_globals *client = self;
    network_resolved_address sender;
    uint32_t decoded_body[8];
    int16_t out_type;
    uint16_t out_version;

    network_channel_remote_address_or_default(client->channel, &sender);
    if (sender.address.ipv4 != *sender_address || client->state != 1) {
        return 0;
    }
    length = length - 2;
    if (data_packet_group_decode_packet((int16_t *)&length, &network_game_messages_group, decoded_body,
                                        (uint8_t *)buffer + 2, &out_type, &out_version, 2) == 0) {
        return 0;
    }
    network_session_player_join_notify(client, decoded_body);
    return 1;
}

/**
 * FIXED (step 1, objdump -d 0x4dbdc0..0x4dbe4c): stack (buffer, length, sender address); the sender address is
 * dereferenced once; the decoder gets &length after the 2-byte header. Returns 0 unless the join completed.
 *
 * @address 0x4dbdc0
 */
int32_t ClientMessageDecoder::join_complete(const uint8_t *buffer, int32_t length, const uint32_t *sender_address)
{
    network_client_globals *client = self;
    network_resolved_address sender;
    uint8_t decoded_body[16];
    int16_t out_type;
    uint16_t out_version;

    network_channel_remote_address_or_default(client->channel, &sender);
    if (sender.address.ipv4 != *sender_address || client->state == 0 || client->state == 4) {
        return 0;
    }
    length = length - 2;
    if (data_packet_group_decode_packet((int16_t *)&length, &network_game_messages_group, decoded_body,
                                        (uint8_t *)buffer + 2, &out_type, &out_version, 2) == 0) {
        return 0;
    }
    join_ui_state = 9;
    client->state = 4;
    return 1;
}

/**
 * FIXED (step 1, objdump -d 0x4dc120..0x4dc18f): stack (buffer, length, sender address); the decoder gets &length after
 * the 2-byte header; a decoded (class 2) ack runs network_client_timer_default_or_disconnect. Always returns 1.
 *
 * @address 0x4dc120
 */
int32_t ClientMessageDecoder::join_finalize_ack(const uint8_t *buffer, int32_t length, const uint32_t *sender_address)
{
    network_client_globals *client = self;
    network_resolved_address sender;
    uint8_t decoded_body[32];
    int16_t out_type;
    uint16_t out_version;

    network_channel_remote_address_or_default(client->channel, &sender);
    if (sender.address.ipv4 == *sender_address && client->state == 2) {
        length = length - 2;
        if (data_packet_group_decode_packet((int16_t *)&length, &network_game_messages_group, decoded_body,
                                        (uint8_t *)buffer + 2, &out_type, &out_version, 2) != 0) {
            network_client_timer_default_or_disconnect(client);
        }
    }
    return 1;
}

/**
 * FIXED (step 1, objdump -d 0x4dc090..0x4dc111): stack (buffer, length, sender address); another sender -> 1; not
 * joining, or the (class 2) message does not decode -> 0; else network_connection_finalize_join's result.
 *
 * @address 0x4dc090
 */
int32_t ClientMessageDecoder::join_finalize_message(const uint8_t *buffer, int32_t length, const uint32_t *sender_address)
{
    network_client_globals *client = self;
    network_resolved_address sender;
    uint8_t decoded_body[32];
    int16_t out_type;
    uint16_t out_version;

    network_channel_remote_address_or_default(client->channel, &sender);
    if (sender.address.ipv4 != *sender_address) {
        return 1;
    }
    if (client->state != k_network_client_state_joining) {
        return 0;
    }
    length = length - 2;
    if (data_packet_group_decode_packet((int16_t *)&length, &network_game_messages_group, decoded_body,
                                        (uint8_t *)buffer + 2, &out_type, &out_version, 2) == 0) {
        return 0;
    }
    return (uint8_t)network_connection_finalize_join((uint16_t *)client);
}

/**
 * FIXED (step 1, objdump -d 0x4dbf30..0x4dbfa4): stack (buffer, length, sender address); the decoded (class 2)
 * message is one 16-bit value, stored at client+0xed8 (the original decodes it into the dead sender-address slot).
 *
 * @address 0x4dbf30
 */
int32_t ClientMessageDecoder::player_config_value(const uint8_t *buffer, int32_t length, const uint32_t *sender_address)
{
    network_client_globals *client = self;
    network_resolved_address sender;
    uint32_t decoded_value = 0;
    int16_t out_type;
    uint16_t out_version;

    network_channel_remote_address_or_default(client->channel, &sender);
    if (sender.address.ipv4 == *sender_address && client->state == 2) {
        length = length - 2;
        if (data_packet_group_decode_packet((int16_t *)&length, &network_game_messages_group, &decoded_value,
                                            (uint8_t *)buffer + 2, &out_type, &out_version, 2) != 0) {
            client->game_start_countdown_seconds = (uint16_t)decoded_value;
        }
    }
    return 1;
}

/**
 * out/phase4/networking_functions.md: "Decodes another synchronization-phase data
 * chunk during the join handshake, aborting via the shared cleanup routine on failure." On
 * success it calls network_player_join_finalize (0x4d9e30, already named, "Creates the
 * game-object datum for a player once their connection has fully joined"), which gives this
 * handler its name. Accepted only while client->state == 3; when it is 4 the message is
 * silently treated as consumed with no decode attempt.
 * register/parameter convention: see network_game_client_decode_state_update_chunk.c for the
 *
 * @address 0x4dc240
 */
char ClientMessageDecoder::player_join_chunk(uint8_t *param_1, int32_t param_2, int32_t *param_3)
{
    network_client_globals *client = self;
    char result;
    network_resolved_address sender;
    int16_t out_type;
    byte_stream input;
    uint32_t decoded_body[8];

    result = 0;
    network_channel_remote_address_or_default(client->channel, &sender);
    if (sender.address.ipv4 != *param_3) {
        return 1;
    }
    if (client->state == 3) {
        if (data_packet_group_decode_packet((param_2 -= 2, (int16_t *)&param_2), &network_game_messages_group,
                decoded_body, param_1 + 2, &out_type, (uint16_t *)&input, 4) != 0) {
            result = network_player_join_finalize(client, decoded_body);
            if (result != 0) {
                return result;
            }
        }
    } else if (client->state == 4) {
        return 1;
    }
    network_disconnect_notify_dropped_machines(client);
    return result;
}

/**
 * out/phase4/networking_functions.md: "Decodes a third variant of synchronization-phase
 * data chunk, accepted across a wider range of connection states." On success it calls
 * network_session_player_table_index_apply, whose own summary ("Finds the player slot matching a given machine id and
 * records its assigned table index (e.g. team or score-table slot) for that player") gives this
 * handler its name. Accepted while client->state is 2, 3 or 4.
 * register/parameter convention: see network_game_client_decode_state_update_chunk.c for the
 * shared EAX/ESI->client, param_2->remaining_length reconstruction, and
 *
 * @address 0x4dc2e0
 */
char ClientMessageDecoder::player_slot_chunk(uint8_t *param_1, int32_t param_2, int32_t *param_3)
{
    network_client_globals *client = self;
    char result;
    network_resolved_address sender;
    int16_t state;
    int16_t out_type;
    byte_stream input;
    uint32_t decoded_body[9];

    result = 0;
    network_channel_remote_address_or_default(client->channel, &sender);
    if (sender.address.ipv4 != *param_3) {
        return 1;
    }
    state = client->state;
    if (state == 3 || state == 4 || state == 2) {
        if (data_packet_group_decode_packet((param_2 -= 2, (int16_t *)&param_2), &network_game_messages_group,
                decoded_body, param_1 + 2, &out_type, (uint16_t *)&input, 4) != 0) {
            result = network_session_player_table_index_apply(client, (int32_t)decoded_body[8], (const uint8_t *)decoded_body);

            if (result != 0) {
                return result;
            }
        }
    } else if (state == 4) {
        return 1;
    }
    network_disconnect_notify_dropped_machines(client);
    return result;
}

/**
 * FIXED (step 1, objdump -d 0x4dba20..0x4dba91): the length and the sender's address are stack arguments; the decoder
 * gets &length after the 2-byte header; the decoded pong is (send time, remote time) and feeds the RTT sampler.
 *
 * @address 0x4dba20
 */
int32_t ClientMessageDecoder::pong_reply(const uint8_t *buffer, int32_t length, const uint32_t *sender_address)
{
    network_client_globals *client = self;
    uint32_t decoded_body[2];
    int16_t out_type;
    uint16_t out_version;

    if (client->state != 0 && client->state != 3) {
        return 1;
    }
    length = length - 2;
    if (data_packet_group_decode_packet((int16_t *)&length, &network_game_messages_group, decoded_body,
                                        (uint8_t *)buffer + 2, &out_type, &out_version, 1) != 0) {
        network_connection_retransmit_if_overdue(sender_address, client, decoded_body[0], (int32_t)decoded_body[1]);
    }
    return 1;
}

/**
 * Rejects the message unless the caller's expected sequence matches the guard's local
 * decode-result record and the connection is currently in state 3; otherwise reports the
 * message as consumed (1) without decoding it. On a successful decode it hands the payload to
 * network_game_state_update_receive; if that fails, or if the message was rejected up front by class/type, it runs
 * the shared disconnect-notification cleanup (network_disconnect_notify_dropped_machines).
 *
 * @address 0x4dc190
 */
char ClientMessageDecoder::state_update_chunk(uint8_t *param_1, int32_t param_2, int32_t *param_3)
{
    network_client_globals *client = self;
    char result;
    network_resolved_address sender;
    int16_t out_type;
    byte_stream input;
    uint8_t decoded_body[528];

    result = 0;
    network_channel_remote_address_or_default(client->channel, &sender);
    if ((sender.address.ipv4 != *param_3) || (client->state != 3)) {
        return 1;
    }
    if (data_packet_group_decode_packet((param_2 -= 2, (int16_t *)&param_2), &network_game_messages_group,
            decoded_body, param_1 + 2, &out_type, (uint16_t *)&input, 4) != 0) {
        result = network_game_state_update_receive(client, decoded_body);
        if (result != 0) {
            return result;
        }
    }
    network_disconnect_notify_dropped_machines(client);
    return result;
}

/**
 *           out_version_used, expected_class
 * FIXED (step 1, objdump -d 0x4dc3a0..0x4dc40b): stack (buffer, length, sender address); the state moves to 4 whether
 * or not the (class 4) message decodes. Always returns 1.
 *
 * @address 0x4dc3a0
 */
int32_t ClientMessageDecoder::sync_complete(const uint8_t *buffer, int32_t length, const uint32_t *sender_address)
{
    network_client_globals *client = self;
    network_resolved_address sender;
    uint8_t decoded_body[16];
    int16_t out_type;
    uint16_t out_version;

    network_channel_remote_address_or_default(client->channel, &sender);
    if (sender.address.ipv4 == *sender_address && client->state == 3) {
        length = length - 2;
        data_packet_group_decode_packet((int16_t *)&length, &network_game_messages_group, decoded_body,
                                        (uint8_t *)buffer + 2, &out_type, &out_version, 4);
        client->state = 4;
    }
    return 1;
}

/**
 * named)
 * address 0x4db6b0, size 336 bytes
 * name confidence: 0.5   rewrite confidence: 0.85 (REWRITTEN; was 0.3)
 * out/phase4/networking_functions.md summary ("The central switch that dispatches a
 * decoded incoming network-game message to the correct per-type handler based on a type byte in
 * the packet").
 *
 * @address 0x4db6b0
 */
char ClientMessageDecoder::dispatch(uint16_t *record, int32_t record_length, const uint32_t *sender)
{
    network_client_globals *client = self;
    const ClientMessageHandler *handler;

    if ((*record & 3) != 0 || ((*record >> 2) & 3) != 3) {
        return 1;
    }
    handler = ClientMessageRegistry::find(*((uint8_t *)record + record_length - 1));
    if (handler == nullptr) {
        return 1;
    }
    return (char)handler->handle(client, record, record_length, sender);
}

/**
 * FIXED (step 1, objdump -d 0x4dc4b0..0x4dc555): stack (buffer, length, sender address); the decoder gets &length after
 * the 2-byte header (the draft passed the length as a pointer plus two extra arguments). Another sender -> 1; not in
 * game (state 4) -> 0; otherwise the (class 6) decode result, after defaulting +0xedc to 8 and, unless this machine is
 * hosting with bit 2, raising the handoff flag and closing chat.
 *
 * @address 0x4dc4b0
 */
int32_t ClientMessageDecoder::ingame_notification(const uint8_t *buffer, int32_t length, const uint32_t *sender_address)
{
    network_client_globals *client = self;
    network_resolved_address sender;
    uint8_t decoded_body[32];
    int16_t out_type;
    uint16_t out_version;
    int32_t decoded = 0;

    network_channel_remote_address_or_default(client->channel, &sender);
    if (sender.address.ipv4 != *sender_address) {
        return 1;
    }
    if (client->state != 4) {
        return 0;
    }
    length = length - 2;
    if (data_packet_group_decode_packet((int16_t *)&length, &network_game_messages_group, decoded_body,
                                        (uint8_t *)buffer + 2, &out_type, &out_version, 6) != 0) {
        decoded = 1;
    }
    if (client->disconnect_reason == 0) {
        client->disconnect_reason = 8;
    }
    if (network_server == 0 || ((network_server->flags >> 2) & 1) == 0) {
        network_host_handoff_requested = 1;
        chat_close();
    }
    return decoded;
}

/**
 * out/phase4/networking_functions.md: "Decodes an in-game message and, only when acting
 * as host, forwards its two payload values to a follow-up handler -- consistent with the host
 * applying a command replicated by a client." That description does not match the code as
 * decoded: the follow-up call only runs when network_game_mode == 1, and types/networking.h
 * documents mode 1 as "client", not host (0 local, 1 client, 2 host, 3 replay). Kept neutral in
 * this rewrite's name pending resolution; the exact code below is preserved either way. On
 * success it decodes an 8-byte payload (packet class 6, not 4 like this cluster's other
 *
 * @address 0x4dc410
 */
int32_t ClientMessageDecoder::replicated_command(uint8_t *param_1, int32_t param_2, int32_t *param_3)
{
    network_client_globals *client = self;
    network_resolved_address sender;
    int16_t out_type;
    byte_stream input;
    uint32_t decoded_body[2];

    network_channel_remote_address_or_default(client->channel, &sender);
    if (sender.address.ipv4 != *param_3) {
        return 1;
    }
    if (client->state == 4 &&
        data_packet_group_decode_packet((param_2 -= 2, (int16_t *)&param_2), &network_game_messages_group,
            decoded_body, param_1 + 2, &out_type, (uint16_t *)&input, 6) != 0) {
        if (network_game_mode != 1) {
            return 1;
        }
        network_client_timer_schedule((int32_t)decoded_body[0], (int32_t)decoded_body[1], client);
        return 1;
    }
    return 0;
}

}
