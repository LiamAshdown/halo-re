#include "halo/networking/net1_decode.hpp"
#include "halo/networking/message_decode.hpp"
#include "halo/networking/channel_queue.hpp"
#include "halo/core/cstring.hpp"
#include "halo/saved_games/api.hpp"
#include "halo/networking/game_mode.hpp"
#include "halo/networking/delta_message_types.hpp"
#include "halo/projectiles/api.hpp"
#include "halo/networking/net1_dispatch.hpp"
#include <string.h>
#include <wchar.h>
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/items/api.hpp"
#include "halo/effects/api.hpp"
#include "halo/cseries/api.hpp"
#include "halo/main/api.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/interface/api.hpp"
#include "interface.h"
#include "saved_games.h"
#include "halo/core/link.hpp"
#include "halo/ai/vars.hpp"
#include "halo/interface/vars.hpp"
#include "halo/networking/vars.hpp"
#include "halo/objects/vars.hpp"
#include "../gamespy/gamespy_calls.hpp"
#include "halo/ai/api.hpp"

static auto &network_game_mode = halo::link::ref<int16_t>(halo::networking::vars().network_game_mode);
static auto &network_action_apply_active = halo::link::ref<uint8_t>(halo::objects::vars().network_action_apply_active);
static auto &network_incoming_message_scratch = halo::link::ref<uint8_t [0x510]>(halo::networking::vars().network_incoming_message_scratch);
typedef struct network_item_stream {
    bit_stream stream;
    uint32_t bit_count;
} network_item_stream;
static auto &join_ui_state = halo::link::ref<int32_t>(halo::networking::vars().join_ui_state);
static auto &interface_loading_screen_request_id = halo::link::ref<int32_t>(halo::networking::vars().interface_loading_screen_request_id);
static auto &shell_product_id = halo::link::ref<void *>(halo::networking::vars().shell_product_id);
static auto &profile_globals_block = halo::link::ref<uint8_t [0x1ffc]>(halo::ui::vars().profile_globals_block);
static auto &network_server = halo::link::ref<network_server_globals *>(halo::networking::vars().network_server);
static auto &game_time = halo::link::ref<game_time_globals *>(halo::ai::vars().game_time);

namespace {

int32_t decode_game_message(const uint8_t *buffer, int32_t length, void *decoded_body, int16_t expected_class)
{
    return halo::networking::decode_message_body(buffer + sizeof(halo::networking::network_message_header), length, decoded_body, expected_class);
}

}
typedef int32_t (*network_game_message_handler_proc)(network_client_globals *client, const void *record, int32_t record_length, const uint32_t *sender);
static auto &network_host_handoff_requested = halo::link::ref<uint8_t>(halo::networking::vars().network_host_handoff_requested);

namespace halo::networking {

namespace {

static void network_game_action_apply_shared(void **context, network_client_globals *client, int32_t type_id)
{
    switch (static_cast<halo::networking::delta_message>(type_id)) {
    case halo::networking::delta_message::hud_item_message: halo::interface::hud_receive_item_message(context); break;
    case halo::networking::delta_message::player_damage_direction: halo::effects::player_effect_mark_damage_direction_dispatch(context); break;
    case halo::networking::delta_message::chat: halo::interface::chat_dispatch_incoming(context); break;
    case halo::networking::delta_message::team_allegiance: halo::game::game_engine_client_apply_team_assignment(context); break;
    case halo::networking::delta_message::player_set_changed: halo::networking::network_channel_key_send_state(client, halo::networking::delta_context(context)); break;
    case halo::networking::delta_message::parameters_update:
        halo::networking::message_delta_parameters_protocol_receive_update(context);
        halo::networking::message_delta_definitions_invoke_field_bindings();
        break;
    case halo::networking::delta_message::map_cycle_list: halo::networking::network_player_ping_field_update_and_report(context); break;
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

    if (network_game_mode != halo::networking::k_game_mode_client && network_game_mode != halo::networking::k_game_mode_host) {
        return;
    }
    network_action_apply_active = 1;
    type_id = halo::networking::delta_context(context)->state->message_type;
    if (network_game_mode == halo::networking::k_game_mode_host) {
        network_game_action_apply_shared(context, client, type_id);
        network_action_apply_active = 0;
        return;
    }
    switch (static_cast<halo::networking::delta_message>(type_id)) {
    case halo::networking::delta_message::object_delete: halo::objects::object_delete_by_pooled_node_id((int32_t **)context); break;
    case halo::networking::delta_message::object_release_node_1:
    case halo::networking::delta_message::object_release_node_2:
    case halo::networking::delta_message::object_release_node_3:
    case halo::networking::delta_message::object_release_node_4:
    case halo::networking::delta_message::object_release_node_5: halo::objects::object_type_override_call_0x70_release_node((int32_t *)context, (uint32_t)client); break;
    case halo::networking::delta_message::object_value_event: halo::game::game_engine_apply_player_join_message(context); break;
    case halo::networking::delta_message::unit_weapon_loadout: halo::game::game_engine_apply_player_spawn_loadout_message(context); break;
    case halo::networking::delta_message::unit_seat_exit: halo::units::unit_dispatch_seat_exit_message(halo::networking::delta_context(context)); break;
    case halo::networking::delta_message::player_interaction: halo::game::game_engine_apply_player_interaction_message(context); break;
    case halo::networking::delta_message::unit_control_update: halo::units::unit_apply_network_control_update((unit_network_control_packet *)context); break;
    case halo::networking::delta_message::kill_streak_update: halo::game::game_engine_apply_kill_streak_message((int32_t **)context); break;
    case halo::networking::delta_message::slayer_profiles_updated:
    case halo::networking::delta_message::ctf_profiles_updated:
    case halo::networking::delta_message::koth_team_scores:
    case halo::networking::delta_message::koth_hill_times:
    case halo::networking::delta_message::ctf_state: halo::game::game_engine_invoke_profile_post_update_callback((uint32_t)client, (uint32_t)(uintptr_t)context); break;
    case halo::networking::delta_message::player_profile_update: halo::game::game_engine_apply_player_profile_entry(context); break;
    case halo::networking::delta_message::end_game: halo::game::game_engine_dispatch_end_game_notification(context); break;
    case halo::networking::delta_message::round_reset: halo::game::game_engine_apply_partial_round_reset_message(context); break;
    case halo::networking::delta_message::kill_event: halo::game::game_engine_handle_kill_feed_network_event((int32_t **)context); break;
    case halo::networking::delta_message::status_sound: halo::game::game_engine_handle_sound_status_event(context); break;
    case halo::networking::delta_message::unit_weapon_script: halo::units::unit_scripting_set_or_drop_weapon(halo::networking::delta_context(context)); break;
    case halo::networking::delta_message::unit_spawn_starting_weapons: halo::units::unit_spawn_with_starting_weapons(context); break;
    case halo::networking::delta_message::unit_create_update: halo::units::unit_network_create_update_apply(context); break;
    case halo::networking::delta_message::projectile_create: halo::projectiles::projectile_create_from_network(context); break;
    case halo::networking::delta_message::equipment_create: halo::items::equipment_create_from_creation_message(context); break;
    case halo::networking::delta_message::weapon_create: halo::items::weapon_create_from_creation_message(context); break;
    case halo::networking::delta_message::local_player_update: halo::networking::player_update_client_local_player_update_from_network(halo::networking::delta_context(context)); break;
    case halo::networking::delta_message::local_player_vehicle_update: halo::networking::player_update_client_local_player_vehicle_update_from_network(halo::networking::delta_context(context)); break;
    case halo::networking::delta_message::remote_player_action_update: halo::networking::player_update_client_remote_player_action_update_from_network(halo::networking::delta_context(context)); break;
    case halo::networking::delta_message::remote_player_action_apply: halo::networking::player_update_remote_player_action_update_apply((int32_t **)context); break;
    case halo::networking::delta_message::remote_player_position_delta: halo::networking::player_update_client_remote_player_position_delta_from_network(halo::networking::delta_context(context)); break;
    case halo::networking::delta_message::remote_player_vehicle_position_delta: halo::networking::player_update_client_remote_player_vehicle_position_delta_from_network(halo::networking::delta_context(context)); break;
    case halo::networking::delta_message::remote_player_biped_update: halo::networking::player_update_client_remote_player_total_biped_update_from_network(halo::networking::delta_context(context)); break;
    case halo::networking::delta_message::remote_player_vehicle_update: halo::networking::player_update_client_remote_player_total_vehicle_update_from_network(halo::networking::delta_context(context)); break;
    case halo::networking::delta_message::weapon_predict_ammo: halo::items::weapon_predict_ammo(context); break;
    case halo::networking::delta_message::weapon_add_ammunition: halo::items::weapon_add_ammunition(context); break;
    case halo::networking::delta_message::weapon_ammo_correction: halo::items::weapon_apply_ammo_correction(context); break;
    case halo::networking::delta_message::weapon_ammo_correction_resync: halo::items::weapon_apply_ammo_correction_and_resync(context); break;
    case halo::networking::delta_message::netgame_equipment_spawn: halo::game::game_engine_spawn_or_replay_netgame_equipment((int32_t *)context); break;
    case halo::networking::delta_message::projectile_detonation: halo::projectiles::projectile_detonation_message_apply(context); break;
    case halo::networking::delta_message::object_linked_impulse: halo::objects::object_apply_linked_impulse(context); break;
    case halo::networking::delta_message::object_shield_charge: halo::objects::object_apply_shield_charge_and_notify(context); break;
    case halo::networking::delta_message::projectile_attach: halo::projectiles::projectile_attach_apply(context); break;
    case halo::networking::delta_message::server_text: halo::networking::network_client_handle_server_text_message(context); break;
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
    message_delta_decode_state state;
    uint8_t record[0x80];
    message_delta_context storage;
    void **context = halo::networking::raw_context(&storage);
    char result = 0;

    halo::networking::network_channel_remote_address_or_default(client->channel, &remote);
    if (remote.address.ipv4 == *sender && (char)halo::networking::message_delta_decode_begin(&state, stream) != 0) {
        memset(record, 0, sizeof(record));
        memset(storage.changed, 0, sizeof(storage.changed));
        storage.state = &state;
        storage.target = record;
        state.changed = 0;
        state.more_items = 0;
        for (;;) {
            message_delta_decode_state *current;

            if ((char)halo::networking::message_delta_decode_array_field(context) == 0) {
                result = 0;
                break;
            }
            halo::networking::network_game_action_apply(context, client);
            current = storage.state;
            result = current->more_items == 1 && current->changed == 1;
            ++current->processed_count;
            memset(storage.changed, 0, sizeof(storage.changed));
            current->more_items = 0;
            current->changed = 0;
            if (result != 1) {
                break;
            }
            if (current->processed_count > current->item_count) {
                return result;
            }
        }
    }
    halo::networking::network_disconnect_notify_dropped_machines(client);
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
        result = (char)halo::networking::network_channel_incoming_read_item(channel, network_incoming_message_scratch, &bit_offset,
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
                result = halo::networking::network_incoming_item_dispatch(client, item_flag, &s.stream, sender);
            } while (result == 1);
        }
        result = result != 0;
    }
}

#pragma pack(push, 1)
/** The part of the decoded join challenge message the client reads: the machine id and the server name. */
struct join_challenge_message {
    uint8_t unknown_00[0x0c];    // 0x00
    uint16_t machine_index;      // 0x0c
    uint8_t unknown_0e[6];       // 0x0e
    char server_name[64];        // 0x14
};
static_assert(offsetof(join_challenge_message, server_name) == 0x14);

/** The challenge payload (message 0x0e) the client answers a join challenge with: the join request and its profile. */
struct join_request_frame {
    network_join_request header; // 0x00
    saved_player_profile profile; // 0x90 profile_globals_block
    uint8_t unused[0x2100 - 0x208c];
};
static_assert(offsetof(join_request_frame, profile) == 0x90);
static_assert(sizeof(join_request_frame) == 0x2100);

/** The decoded game state update message: ids, the simulation tick and one 8-dword player_action per player. */
struct game_state_update_message {
    uint32_t update_id;          // 0x00
    uint32_t random_seed;        // 0x04
    uint32_t game_time;          // 0x08
    uint8_t unknown_0c[2];       // 0x0c
    int16_t player_count;        // 0x0e
    uint32_t action_dwords[16][8]; // 0x10
};
static_assert(offsetof(game_state_update_message, action_dwords) == 0x10);
#pragma pack(pop)

/**
 * be recovered from this function's own decompiled body, so 0 is passed (row 0, matching
 * network_game_settings_packet_send.c's own unindexed use of the same template table).
 *
 * @address 0x4d9800
 */
int32_t GameClientView::settings_packet_receive(const uint32_t *request)
{
    network_client_globals *client = self;
    const network_game_session *incoming = (const network_game_session *)request;
    int32_t cmp;

    if (incoming->player_count < 0 || incoming->player_count > 0x10) {
        return 0;
    }

    cmp = strcmp(incoming->server_name, client->session.server_name);
    if (cmp != 0) {
        halo::main::main_queue_map_change_by_name_or_clear(const_cast<char *>(incoming->server_name));
        if (join_ui_state != 1) {
            if (join_ui_state != 2 && join_ui_state == 4) {
                interface_loading_screen_request_id = -1;
            }
            join_ui_state = 8;
        }
    }

    memcpy(&client->session, incoming, offsetof(network_game_session, map_loaded));

    if (client->settings_ack_sent == 0) {
        halo::networking::network_game_settings_ack_send(client, 0);
        client->settings_ack_sent = 1;
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
 * 0xee2) is settings_ack_sent, word 0x788 (byte 0xf10) is unknown_f10.
 *
 * @address 0x4d94c0
 */
void GameClientView::settings_packet_send(const uint8_t *request)
{
    network_client_globals *client = self;
    const join_challenge_message *challenge_request = (const join_challenge_message *)request;

    join_request_frame frame;
    int32_t cmp;
    uint16_t *challenge;

    if ((client->flags & 2) != 0) {
        return;
    }
    client->state = 2;
    client->machine_index = challenge_request->machine_index;

    cmp = strcmp(challenge_request->server_name, client->session.server_name);
    if (cmp != 0) {
        halo::main::main_queue_map_change_by_name_or_clear(const_cast<char *>(challenge_request->server_name));
        if (join_ui_state != 1) {
            if (join_ui_state != 2 && join_ui_state == 4) {
                interface_loading_screen_request_id = -1;
            }
            join_ui_state = 8;
        }
    }

    memset(&frame, 0, offsetof(network_join_request, unknown_8e));

    memcpy(frame.header.session_key, &client->connect_attempt.session_info[5], sizeof(frame.header.session_key));
    client->settings_ack_sent = 0;
    wcsncpy((wchar_t *)frame.header.password, (const wchar_t *)((const uint8_t *)client->connect_attempt.session_info + 2), 8);
    frame.header.rate_index = (uint8_t)client->connection_rate_index;
    frame.header.password_terminator = 0;
    gcd_compute_response(shell_product_id, (void *)request, frame.header.cd_key_response);

    memcpy(&frame.profile, profile_globals_block, sizeof(frame.profile));

    frame.header.player.machine_index = (uint8_t)challenge_request->machine_index;
    frame.header.player.machine_player_index = 0;
    wcsncpy((wchar_t *)frame.header.player.name, (const wchar_t *)frame.profile.name, 0xb);
    frame.header.player.team_index = (uint8_t)client->team_index;
    frame.header.player.name[11] = 0;
    frame.header.player.color_index = (uint16_t)frame.profile.player_color;
    frame.header.player.icon_index = (int16_t)0xffff;
    frame.header.player.slot_index = (int8_t)0xff;
    client->settings_ack_sent = 1;

    challenge = halo::networking::network_prepare_challenge_packet(0x0e, &frame);
    if (challenge != 0) {
        if (!halo::networking::channel_queue_packet(client->channel, challenge)) {
            return;
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
int32_t GameClientView::state_update_receive(uint8_t *record_bytes)
{
    game_state_update_message *record = (game_state_update_message *)record_bytes;
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

    current_capacity = record->player_count;
    target_capacity = client->session.player_count;

    if (current_capacity < target_capacity) {
        fill = record->action_dwords[0] + (uint32_t)current_capacity * 8;
        for (i = ((int32_t)target_capacity - (int32_t)current_capacity & 0x7ffffff) << 3; i != 0; i = i - 1) {
            *fill = 0;
            fill = fill + 1;
        }
        record->player_count = target_capacity;
    }

    if (record->update_id <= (uint32_t)client->last_update_id ||
        (network_server == 0 && (uint32_t)halo::game::globals().game_time->game_time == record->game_time &&
         record->random_seed != halo::math::globals().random_seed_global)) {
        halo::networking::network_disconnect_notify_dropped_machines(client);
    }

    copy_units = record->player_count;
    src = record->action_dwords[0];
    *(int16_t *)local_buffer = copy_units;
    dst = local_buffer + 1;
    for (i = (uint32_t)(uint16_t)copy_units << 3; i != 0; i = i - 1) {
        *dst = *src;
        src = src + 1;
        dst = dst + 1;
    }

    halo::game::update_client_advance_read_cursor((int32_t)record->update_id, local_buffer);
    client->last_update_id = record->update_id;

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    now_ms = (int32_t)((counter.quad_part * 1000) / halo::cseries::globals().performance_frequency);
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
        return halo::networking::network_game_action_queue_drain(client, stream, sender);
    }
    if (item_flag == 0) {
        uint16_t *record = halo::networking::network_message_read_sized_buffer(buffer, 0xfff, stream);

        if (record != 0) {
            return halo::networking::network_game_message_decode_dispatch(client, record, *record >> 4, sender);
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

    halo::networking::network_channel_remote_address_or_default(client->channel, &sender);
    if (sender.address.ipv4 == *sender_address && client->state == 4) {
        decode_game_message(buffer, length, decoded_body, 6);
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

    halo::networking::network_channel_remote_address_or_default(client->channel, &sender);
    if (sender.address.ipv4 == *sender_address && client->state == 2) {
        decode_game_message(buffer, length, decoded_body, 2);
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

    if (client->state != 0) {
        return 1;
    }
    if (decode_game_message(buffer, length, decoded_body, 1) != 0) {
        halo::networking::network_game_search_results_add_or_update(client->search_entries,
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

    halo::networking::network_channel_remote_address_or_default(client->channel, &sender);
    if (sender.address.ipv4 != *sender_address || client->state != 1) {
        return 0;
    }
    if (decode_game_message(buffer, length, decoded_body, 2) == 0) {
        return 0;
    }
    halo::networking::network_session_player_join_notify(client, decoded_body);
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

    halo::networking::network_channel_remote_address_or_default(client->channel, &sender);
    if (sender.address.ipv4 != *sender_address || client->state == 0 || client->state == 4) {
        return 0;
    }
    if (decode_game_message(buffer, length, decoded_body, 2) == 0) {
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

    halo::networking::network_channel_remote_address_or_default(client->channel, &sender);
    if (sender.address.ipv4 == *sender_address && client->state == 2) {
        if (decode_game_message(buffer, length, decoded_body, 2) != 0) {
            halo::networking::network_client_timer_default_or_disconnect(client);
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

    halo::networking::network_channel_remote_address_or_default(client->channel, &sender);
    if (sender.address.ipv4 != *sender_address) {
        return 1;
    }
    if (client->state != k_network_client_state_joining) {
        return 0;
    }
    if (decode_game_message(buffer, length, decoded_body, 2) == 0) {
        return 0;
    }
    return (uint8_t)halo::networking::network_connection_finalize_join(client);
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

    halo::networking::network_channel_remote_address_or_default(client->channel, &sender);
    if (sender.address.ipv4 == *sender_address && client->state == 2) {
        if (decode_game_message(buffer, length, &decoded_value, 2) != 0) {
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
    uint32_t decoded_body[8];

    result = 0;
    halo::networking::network_channel_remote_address_or_default(client->channel, &sender);
    if (sender.address.ipv4 != *param_3) {
        return 1;
    }
    if (client->state == 3) {
        if (decode_game_message(param_1, param_2, decoded_body, 4) != 0) {
            result = halo::networking::network_player_join_finalize(client, (network_player_entry *)decoded_body);
            if (result != 0) {
                return result;
            }
        }
    } else if (client->state == 4) {
        return 1;
    }
    halo::networking::network_disconnect_notify_dropped_machines(client);
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
    uint32_t decoded_body[9];

    result = 0;
    halo::networking::network_channel_remote_address_or_default(client->channel, &sender);
    if (sender.address.ipv4 != *param_3) {
        return 1;
    }
    state = client->state;
    if (state == 3 || state == 4 || state == 2) {
        if (decode_game_message(param_1, param_2, decoded_body, 4) != 0) {
            result = halo::networking::network_session_player_table_index_apply(client, (int32_t)decoded_body[8], (const uint8_t *)decoded_body);

            if (result != 0) {
                return result;
            }
        }
    } else if (state == 4) {
        return 1;
    }
    halo::networking::network_disconnect_notify_dropped_machines(client);
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

    if (client->state != 0 && client->state != 3) {
        return 1;
    }
    if (decode_game_message(buffer, length, decoded_body, 1) != 0) {
        halo::networking::network_connection_retransmit_if_overdue(sender_address, client, decoded_body[0], (int32_t)decoded_body[1]);
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
    uint8_t decoded_body[528];

    result = 0;
    halo::networking::network_channel_remote_address_or_default(client->channel, &sender);
    if ((sender.address.ipv4 != *param_3) || (client->state != 3)) {
        return 1;
    }
    if (decode_game_message(param_1, param_2, decoded_body, 4) != 0) {
        result = halo::networking::network_game_state_update_receive(client, decoded_body);
        if (result != 0) {
            return result;
        }
    }
    halo::networking::network_disconnect_notify_dropped_machines(client);
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

    halo::networking::network_channel_remote_address_or_default(client->channel, &sender);
    if (sender.address.ipv4 == *sender_address && client->state == 3) {
        decode_game_message(buffer, length, decoded_body, 4);
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
    int32_t decoded = 0;

    halo::networking::network_channel_remote_address_or_default(client->channel, &sender);
    if (sender.address.ipv4 != *sender_address) {
        return 1;
    }
    if (client->state != 4) {
        return 0;
    }
    if (decode_game_message(buffer, length, decoded_body, 6) != 0) {
        decoded = 1;
    }
    if (client->disconnect_reason == 0) {
        client->disconnect_reason = 8;
    }
    if (network_server == 0 || ((network_server->flags >> 2) & 1) == 0) {
        network_host_handoff_requested = 1;
        halo::interface::chat_close();
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
    uint32_t decoded_body[2];

    halo::networking::network_channel_remote_address_or_default(client->channel, &sender);
    if (sender.address.ipv4 != *param_3) {
        return 1;
    }
    if (client->state == 4 &&
        decode_game_message(param_1, param_2, decoded_body, 6) != 0) {
        if (network_game_mode != halo::networking::k_game_mode_client) {
            return 1;
        }
        halo::networking::network_client_timer_schedule((int32_t)decoded_body[0], (int32_t)decoded_body[1], client);
        return 1;
    }
    return 0;
}

}
