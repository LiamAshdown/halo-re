#include "halo/networking/net1_session.hpp"
#include "halo/core/network_constants.hpp"
#include "halo/game/constants.hpp"
#include "halo/networking/delta_message_types.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/lcg.hpp"
#include "halo/text/api.hpp"
#include "halo/scenario/api.hpp"
#include <string.h>
#include <wchar.h>
#include <stdint.h>
#include "units.h"
#include "items.h"
#include <stdio.h>
#include <stdarg.h>
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/core/tag_groups.hpp"
#include "halo/effects/api.hpp"
#include "halo/cseries/api.hpp"
#include "halo/main/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/interface/api.hpp"
extern "C" { extern data_packet_group network_game_messages_group; }

extern "C" {
extern void *object_type_definitions[12];
extern uint8_t network_message_scratch[halo::k_network_message_scratch_size];
extern datum_index machine_to_player[16];
extern data_array *player_data;
extern void update_server_queue_push_history(int16_t machine_index, int32_t tick_count, uint32_t *source, uint32_t extra);
extern wchar_t empty_string;
extern network_client_globals *network_client;
extern network_server_globals *network_server;
extern uint32_t profile_globals_block[];
extern game_time_globals *game_time;
extern uint16_t network_challenge_packet_block[];
extern uint8_t network_server_host_valid;
extern uint8_t network_game_info_packet_flag;
extern uint8_t network_session_host_flags_byte;
extern uint8_t network_channels_open_ok;
extern int32_t message_delta_vector3d_mode;
extern uint8_t network_host_handoff_requested;
extern int32_t interface_loading_screen_address_a;
extern int32_t interface_loading_screen_address_b;
extern int32_t join_ui_state;
extern int32_t interface_loading_screen_progress;
extern int16_t progress_screen_text;
extern int16_t progress_screen_subtext;
extern int32_t interface_loading_screen_request_id;
extern int16_t network_game_mode;
extern uint8_t network_disconnect_timeout_flag;
extern int32_t sv_maxplayers_value;
extern uint8_t game_engine_ensure_variant_history_has_entry(void);
extern void game_engine_apply_current_custom_variant(void);
extern void game_engine_sync_variant_defaults(void);
extern void widget_close_all(void);
extern int16_t network_join_error_code;
extern void chat_close(void);
extern uint8_t *main_game_globals;
extern void cache_file_switch_map_by_path(char *path, uint8_t apply_state);
extern void game_unload_map(void);
extern void game_start_new_map(void);
extern void game_stop_current_map(void);
extern void game_engine_reset_all_players(void);
extern void game_engine_apply_variant(const game_variant *variant);
extern void game_engine_init_tick_record_for_mode(void);
extern uint8_t network_channel_table_default_flag;
extern int32_t game_engine_notify_object_value_event(int32_t team);
extern int32_t game_engine_player_profile_cache_find(void);
extern void game_engine_capture_player_profile(int32_t value);
extern uint8_t network_player_update_log_enabled;
extern char *network_player_update_history_log_path;
extern char player_update_log_file_mode_string[];
extern void *network_session_host_object;
extern int32_t network_session_host_state;
extern int32_t network_console_connection_id;
extern void gcd_shutdown(void);
extern void qr2_shutdown(void *object);
extern int32_t network_game_socket;
extern int32_t NNBeginNegotiationWithSocket(uint32_t socket, int32_t cookie, int32_t client_index, void *progress_callback, void *completed_callback, void *user_data);
extern uint16_t gt2NetworkToHostShort(uint16_t value);
extern char *gt2AddressToString(uint32_t ip, uint16_t port, char *string);
typedef struct ColorARGB ColorARGB;
extern void *console_message_default_color;
extern void *current_game_engine;
extern void qr2_buffer_add(void *buffer, const char *value);
extern void qr2_buffer_add_int(void *buffer, int32_t value);
extern void qr2_keybuffer_add(void *keybuffer, int32_t key_id);
extern void gcd_authenticate_user(int32_t game_id, int32_t local_id, uint32_t ip, const char *challenge, const char *response, void *callback, void *instance);
extern const char *gcd_getkeyhash(int32_t game_id, int32_t local_id);
extern void gcd_disconnect_user(int32_t game_id, int32_t local_id);
extern void gcd_disconnect_all(int32_t game_id);
extern int32_t network_session_start_game_type;
extern char network_session_start_host_name[];
extern char network_session_start_map_name[];
extern int32_t qr2_init_socketA(void **qrec_out, uint32_t socket, int32_t port, const char *gamename, const char *secret_key, int32_t ispublic, int32_t natnegotiate, void *server_key, void *player_key, void *team_key, void *key_list, void *count, void *adderror, void *userdata);
extern void qr2_register_natneg_callback(void *qrec, void *callback);
extern void gcd_init_qr2(void *qrec, int32_t game_id, int32_t use_network);
extern char network_session_start_variant_name[];
extern uint8_t network_session_host_closing;
extern int32_t network_session_host_last_tick;
extern void qr2_send_statechanged(void *object);
extern void qr2_think(void *object);
}

namespace halo::networking {

/**
 * Encodes and broadcasts an object-update packet for every live object whose network_role is
 * zero and whose type-table team slot is populated, accumulating the total encoded size into
 * *bytes_sent and the object count into *object_count.
 *
 * @address 0x4df950
 */
void GameRuntime::broadcast_team_object_updates(int32_t *object_count, uint32_t param_1, int32_t *bytes_sent)
{
    object_iterator iterator;
    object *obj;
    int32_t encoded_bits;

    (void)param_1;
    iterator.type_mask = halo::k_dword_none;
    iterator.flags_mask = 0;
    iterator.index = 0;
    iterator.handle = halo::k_dword_none;

    obj = halo::objects::object_iterator_next(&iterator);
    while (obj != 0) {
        if (obj->network_role == 0 &&
            *(int32_t *)((uint8_t *)object_type_definitions[obj->type] + 0x10) != -1) {
            encoded_bits = halo::objects::object_type_override_get_0x64(iterator.handle, network_message_scratch, halo::k_network_message_scratch_size);
            if (encoded_bits > 0) {
                *bytes_sent = *bytes_sent + encoded_bits;
                *object_count = *object_count + 1;
                halo::networking::network_session_send_to_machine((int32_t)param_1, network_server, 1, network_message_scratch, encoded_bits, 1, 0, 0, 3);
            }
        }
        obj = halo::objects::object_iterator_next(&iterator);
    }
}

/**
 * Queues the control record carried by `packet` for the machine's player on the host. The packet is
 * dropped when its update id (low 31 bits) is older than the last one accepted from the machine, when
 * it claims more than one action, or when the machine has no player with a live unit. On success the
 * action goes to update_server_queue_push_history, the accepted id is stored in the machine, and
 * the player's pending interaction mask (unknown_11c) takes the action's control flags & 0x4d0.
 *
 * @address 0x4dff70
 */
void GameRuntime::client_apply_position_update(network_machine *machine, const client_position_packet *packet,
    int32_t tick_count, uint32_t history_byte)
{
    uint32_t action[8] = {};
    datum_index player_datum;
    player *plr;

    if (machine->last_update_id > (packet->update_id & 0x7fffffff)) {
        return;
    }
    if (packet->action_count < 0 || packet->action_count > 1) {
        return;
    }
    if (packet->action_count > 0) {
        memcpy(action, packet->action, sizeof(action));
    }

    player_datum = machine_to_player[(uint16_t)machine->machine_id];
    if (player_datum == (datum_index)halo::k_dword_none) {
        return;
    }
    plr = (player *)halo::memory::datum_get(player_datum, halo::game::globals().player_data);
    if (plr == 0 || plr->unit == (datum_index)halo::k_dword_none) {
        return;
    }

    halo::game::update_server_queue_push_history(machine->machine_id, tick_count, action, history_byte);
    machine->last_update_id = packet->update_id & 0x7fffffff;
    plr = (player *)halo::memory::datum_get(player_datum, player_data);
    if (plr != 0) {
        plr->unknown_11c = action[0] & 0x4d0;
    }
}

/**
 * Handles a message 0x0d (client update) decoded from a machine's queued updates. The record is decoded
 * against the machine's previous one (either by merging the changed sub-fields or from scratch) and stored
 * back; when it covers at least one tick and the connection-quality check passes, the control it carries is
 * turned into a one-action position packet and applied through client_apply_position_update, together with
 * the history byte that came in front of the record. Non-host machines get an "update received" log line.
 *
 * @address 0x4e0280
 */
void GameRuntime::client_apply_received_update(network_machine *machine, uint32_t server, void **message)
{
    client_update_record update = machine->last_update;
    message_delta_decode_state *state = (message_delta_decode_state *)message[0];
    uint8_t history_byte;

    (void)server;
    if (state->incremental == 1) {
        int32_t delta_bits = halo::networking::message_delta_read_changed_subfields(state,
            (uint8_t *)(message + 1), (int32_t)&machine->last_update, (int32_t)&update);

        state->bits_read = state->bits_read + delta_bits;
        state->changed = 1;
    } else {
        halo::networking::message_delta_decode_compound_field(message, &update);
    }
    machine->last_update = update;
    if (update.tick_count == 0) {
        return;
    }
    if (halo::networking::network_client_check_connection_quality(machine->machine_id, update) != 1) {
        return;
    }

    {
        client_position_packet packet = {};
        player_action *action = (player_action *)packet.action;

        history_byte = *(uint8_t *)message[0x11];
        packet.action_count = 1;
        action->control_flags = update.control_flags;
        action->desired_yaw = update.yaw;
        action->desired_pitch = update.pitch;
        action->throttle_x = update.throttle_x;
        action->throttle_y = update.throttle_y;
        action->primary_trigger = update.primary_trigger;
        action->weapon_index = update.weapon_index;
        action->grenade_index = update.grenade_index;
        action->zoom_level = update.zoom_level;
        halo::networking::network_game_client_apply_position_update(machine, &packet, update.tick_count, history_byte);
    }
    if (machine->machine_id != 0) {
        halo::networking::network_player_update_history_log_write("[%d]: [%d]:\t Received update [%d] for [%d] ticks.\n",
            GetTickCount(), game_time->game_time, (int32_t)history_byte, (int32_t)update.tick_count);
    }
}

/**
 * named)
 * address 0x4dea80, size 112 bytes
 * name confidence: 0.7   rewrite confidence: 0.45
 * out/phase4/networking_functions.md: "Looks up the 'ui\random_player_names' tag and,
 * if it has entries, returns a randomly selected default player name; otherwise returns the
 * built-in fallback name string." tag_instances (0x0087bc14) and its `(index*0x20+0x14)`
 * definition-pointer idiom match the same pattern already established throughout src/ai (e.g.
 *
 * @address 0x4dea80
 */
wchar_t * GameRuntime::get_random_player_name()
{
    uint32_t tag_id;
    void *definition;

    tag_id = halo::cache::tag_lookup(halo::groups::unicode_string_list, (char *)"ui\\random_player_names");
    if (tag_id != halo::k_dword_none) {
        definition = *(void **)((uint8_t *)halo::cache::globals().tag_instances + (tag_id & halo::k_datum_slot_mask) * 0x20 + 0x14);
        if (definition != 0 && *(int32_t *)definition != 0) {
            halo::math::globals().effect_random_seed = halo::advance_random_seed(halo::math::globals().effect_random_seed);
            return reinterpret_cast<wchar_t *>(halo::text::text_string_list_get_string((int32_t)tag_id, 0));
        }
    }
    return &empty_string;
}

/**
 * out/phase4/networking_functions.md: "Returns whether a network game is currently
 * active by checking that either the network-game globals or the host globals pointer is
 * non-null." network_client (0x0071c2d8) and network_server (0x0071c2d4) match
 * types/networking.h exactly.
 *
 * @address 0x4ddca0
 */
int32_t GameRuntime::is_active()
{
    if (network_client == 0 && network_server == 0) {
        return 0;
    }
    return 1;
}

/**
 * network_game_settings_packet_send.c)
 * out/phase4/networking_functions.md summary ("Builds and queues a small
 * acknowledgement message while the local connection is in state 2 or 3 (used right after
 * receiving the game-settings/map message); a no-op otherwise"); called from
 * network_game_settings_packet_receive.c (0x4d9800, same task batch) the first time a
 * game-settings packet is applied.
 *
 * @address 0x4d9f50
 */
char GameRuntime::settings_ack_send(uint8_t *client, int16_t template_row)
{

    uint8_t frame[0x2070];
    uint32_t *dst, *src;
    int32_t i;
    int32_t mode;
    int32_t *challenge;
    uint8_t *channel;
    int32_t bits_to_send;
    int32_t total_bits;
    int32_t free_bits;
    char result;

    src = &profile_globals_block[(uint32_t)template_row * 0x801];
    dst = (uint32_t *)(frame + 0x46);
    for (i = 0x7ff; i != 0; i = i - 1) {
        *dst = *src;
        src = src + 1;
        dst = dst + 1;
    }
    *(uint8_t *)(frame + 0x23) = (uint8_t)template_row;
    *(uint8_t *)(frame + 0x22) = *client;
    wcsncpy((wchar_t *)(frame + 0x6), (const wchar_t *)(frame + 0x48), 0xb);
    *(uint16_t *)(frame + 0x1e) = *(uint16_t *)(frame + 0x160);
    *(uint16_t *)(frame + 0x20) = 0xffff;
    *(uint8_t *)(frame + 0x24) = 0xff;
    *(uint8_t *)(frame + 0x25) = 0xff;
    *(uint16_t *)(frame + 0x1c) = 0;

    mode = *(int16_t *)(client + 0xeda);
    switch (mode) {
    case 0:
    case 1:
    case 4:
        return 0;
    case 2:
    case 3:
        for (i = 0; i < 8; i = i + 1) {
            ((uint32_t *)(frame + 0x26))[i] = ((uint32_t *)(frame + 0x6))[i];
        }

        challenge = (int32_t *)halo::networking::network_prepare_challenge_packet(0x0f, frame + 0x26);
        if (challenge == 0) {
            return 1;
        }
        channel = *(uint8_t **)(client + 0xadc);
        bits_to_send = (uint32_t)(*(uint16_t *)challenge >> 4) * 8;
        total_bits = bits_to_send + 1;
        if ((*(uint8_t *)&((network_channel *)channel)->flags & 1) != 0) {
            return 1;
        }
        free_bits = ((*(int32_t *)&((network_channel *)channel)->outgoing.stream.last_bit + *(int32_t *)&((network_channel *)channel)->outgoing.stream.byte_cursor * -8) -
                     *(int32_t *)&((network_channel *)channel)->outgoing.stream.bit_cursor) + 1;
        break;
    default:
        return 1;
    }

    result = 1;
    if (total_bits <= free_bits || (result = halo::networking::network_channel_stream_flush((network_channel_stream *)(channel + 0x10), (network_channel *)channel, 1), result != 0)) {

        ((network_channel *)channel)->send_budget = ((network_channel *)channel)->send_budget + bits_to_send + 1;
        { uint32_t item_flag = 0; halo::memory::bit_stream_write_bits_chunked((bit_stream *)((uint8_t *)channel + 0x10), &item_flag, 1); }
        ((network_channel *)channel)->outgoing.empty = 0;
        halo::memory::bit_stream_write_bits_chunked((bit_stream *)((uint8_t *)channel + 0x10), (const uint32_t *)(challenge), bits_to_send);
        ((network_channel *)channel)->outgoing.empty = 0;
    }
    return result;
}

/**
 * Broadcasts `entry` (a session player row, with the tick the row is removed at appended) as message 0x18 to
 * every machine and stamps the row's player with that removal tick through network_object_record_last_sender.
 * Returns 1 when the packet was built and sent, 0 otherwise.
 *
 * @address 0x4df0e0
 */
uint32_t GameRuntime::settings_broadcast_send(network_server_globals *server, const network_player_entry *entry)
{
    uint8_t buffer[0x600];
    int16_t capacity = 0x600;
    uint32_t payload[9];
    uint32_t quit_tick = (uint32_t)(game_time->game_time + 0x21);

    memcpy(payload, entry, 8 * sizeof(uint32_t));
    payload[8] = quit_tick;
    if (halo::memory::data_packet_group_encode_packet(&network_game_messages_group, buffer, payload, &capacity, 0x18, 1) != 0) {
        uint16_t *message = halo::networking::network_message_block_build(network_challenge_packet_block,
            (uint32_t *)buffer, 3, (uint32_t)capacity);

        if (message != 0) {
            halo::networking::network_session_broadcast_to_all(server, 0, message, 1, 0, 0, 3);
            halo::networking::network_object_record_last_sender(entry->slot_index, (int32_t)quit_tick, server);
            return 1;
        }
    }
    return 0;
}

/**
 * Tears down any existing hosted session, opens the network channels, creates the server host
 * and (if not already present) the shared client/session globals, applies default UI/engine
 * state, switches network_game_mode to host (2), and writes name/password into the new server.
 * Returns 1 on success, 0 on any failure (each of which also tears the partial state back down).
 *
 * @address 0x4e4150
 */
uint8_t GameRuntime::start_new_server_with_name_and_password(uint32_t unused, uint16_t *name, uint16_t *password)
{
    uint8_t ok;

    if (network_server != 0) {
        halo::networking::network_game_server_host_dispose(network_server);
        network_server = 0;
        network_server_host_valid = 0;
    }
    halo::networking::network_client_globals_dispose();
    if (*name == 0) {
        static const uint16_t default_name[] = { 'H', 'a', 'l', 'o', 0 };
        name = (uint16_t *)default_name;
    }
    network_session_host_flags_byte = network_game_info_packet_flag;
    halo::networking::network_channels_open();
    if (network_channels_open_ok == 0) {
        goto fail;
    }
    message_delta_vector3d_mode = (network_game_info_packet_flag == 1);
    ok = halo::networking::network_game_server_host_create();
    if (ok == 1) {
        if (((network_server->flags >> 2) & 1) == 0) {
            network_client = halo::networking::network_session_create();
            ok = 0;
            if (network_client == 0) {
                goto fail_or_dispose;
            }
            network_host_handoff_requested = 0;
            *(int32_t *)((uint8_t *)network_client + 0xf4c) = 4;
        }
        interface_loading_screen_address_a = -1;
        interface_loading_screen_address_b = -1;
        join_ui_state = 0;
        interface_loading_screen_progress = 0;
        progress_screen_text = 0;
        progress_screen_subtext = 0;
        interface_loading_screen_request_id = -1;
        ok = halo::game::game_engine_ensure_variant_history_has_entry();
        if (ok == 0) {
        fail:
            if (network_server != 0) {
                halo::networking::network_game_server_host_dispose(network_server);
                network_server = 0;
                network_server_host_valid = 0;
            }
            halo::networking::network_client_globals_dispose();
            return 0;
        }
        halo::game::globals().variant_history_current = -1;
        halo::game::game_engine_apply_current_custom_variant();
        halo::game::game_engine_sync_variant_defaults();
        network_game_mode = 2;
        halo::networking::network_host_round_reset(network_server);
        network_disconnect_timeout_flag = 1;
    } else {
    fail_or_dispose:
        if (ok == 0) {
            goto fail;
        }
    }
    if (network_server == 0) {
        halo::networking::network_client_globals_dispose();
        return 0;
    }
    {
        network_channel *listen_channel = network_server->listen_channel;
        network_server->flags = network_server->flags | 1;
        listen_channel->listening = 1;

        *((uint8_t *)network_server + 0x9d5) = 0;

        wcsncpy((wchar_t *)((uint8_t *)network_server + 8), (const wchar_t *)name, 0x3f);
        network_server->session.unknown_07e = 0;
        wcsncpy((wchar_t *)network_server->password, (const wchar_t *)password, 8);
        network_server->password[8] = 0;
        {
            int32_t max_players = sv_maxplayers_value;
            if (max_players < 0) {
                max_players = 0;
                sv_maxplayers_value = 0;
            } else if (0x10 < max_players) {
                max_players = 0x10;
                sv_maxplayers_value = 0x10;
            }
            network_server->session.maximum_players = (uint8_t)max_players;
        }
        halo::interface::widget_close_all();
        network_server->new_server_pending = 1;
        if (((network_server->flags >> 2) & 1) == 0) {
            join_ui_state = 2;
        }
        return 1;
    }
}

/**
 * this module, 0x4e19c0
 *
 * @address 0x4deec0
 */
void GameRuntime::map_cycle_list_broadcast()
{
    void *entries[16];
    uint8_t encoded[1024];
    network_map_cycle_entry scratch[16];
    int32_t count;
    data_iterator iterator;
    void *item;

    for (count = 0; count < 16; count++) {
        entries[count] = 0;
    }
    count = 0;
    iterator.data = 0;
    iterator.next_index = 0;
    iterator.index = 0;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
    item = halo::memory::data_iterator_next(&iterator);
    if (item != 0) {
        do {
            scratch[count].unknown_00 = *(uint8_t *)((uint8_t *)item + 0x67);
            scratch[count].unknown_04 = *(uint32_t *)&((struct item_object *)item)->base.maximum_shield_vitality;
            entries[count] = &scratch[count];
            count = count + 1;
            item = halo::memory::data_iterator_next(&iterator);
        } while (item != 0 && count < 16);
        if (count > 0) {
            halo::networking::message_delta_encode_message((int32_t)encoded, 0x2000, 0, halo::networking::message_id(halo::networking::delta_message::map_cycle_list), 0, entries, 0, count, 0);
            halo::networking::network_session_broadcast_to_all(network_server, 1, encoded, 0, 0, 0, 3);
        }
    }
}

/**
 * out/phase4/networking_functions.md summary ("Records an error code and triggers a
 * network-session disconnect/cleanup").
 *
 * @address 0x4d97e0
 */
void GameRuntime::disconnect_with_error(int16_t error_code)
{
    if (network_join_error_code == -1) {
        network_join_error_code = error_code + 0x2b;
    }
    network_host_handoff_requested = 1;
    halo::interface::chat_close();
}

/**
 * already named)
 * address 0x4df730, size 96 bytes
 * name confidence: 0.55   rewrite confidence: 0.45
 * out/phase4/networking_functions.md: "Picks a random default player name via
 * network_game_get_random_player_name(), retrying until it doesn't collide with any currently
 * active player, and copies the result into the output buffer." Same session scan base
 * (container + 0x1aa == session.players[0].name, stride 0x10 wchars == 0x20 bytes) as
 *
 * @address 0x4df730
 */
void GameSessionView::generate_unique_random_name(wchar_t *out_name)
{
    network_game_session *session = self;
    wchar_t *candidate;
    int32_t collisions;
    int32_t i;

    do {
        candidate = halo::networking::network_game_get_random_player_name();
        collisions = 0;
        for (i = 0; i < 0x10; i++) {
            if (halo::networking::network_player_entry_validate(&session->players[i]) != 0 &&
                wcscmp((wchar_t *)session->players[i].name, candidate) == 0) {
                collisions = collisions + 1;
            }
        }
    } while (collisions != 0);
    wcsncpy(out_name, candidate, 0xb);
    out_name[0xb] = L'\0';
}

/**
 * named)
 * address 0x4de6d0, size 405 bytes
 * name confidence: 0.55   rewrite confidence: 0.25
 * out/phase4/networking_functions.md: "Prepares and issues a halo::scenario::scenario_load() request
 * for the network game using the requested map name and seed, then, when hosting, opens a
 * channel for every connected machine and returns whether the map is now loaded." session at
 * param_1: server_name (+0x84), unknown_19e (+0x19e) and unknown_3ac (+0x3ac, the map-loaded
 *
 * @address 0x4de6d0
 */
char GameSessionView::scenario_load_request()
{
    network_game_session *session = self;
    network_scenario_load_request request;
    int32_t i;
    char loaded;
    network_game_session *shared_session;

    memset(&request, 0, sizeof(request));
    request.difficulty = 1;
    request.salt = 0xdeadbeef;
    strncpy(request.map_name, session->server_name, 0x7f);
    request.difficulty = session->difficulty;

    if (network_game_mode > 0) {
        if (network_game_mode < 3) {
            if (network_server != 0) {
                shared_session = &network_server->session;
            } else if (network_client != 0) {
                shared_session = &network_client->session;
            } else {
                shared_session = 0;
            }
            if (shared_session != 0) {
                request.salt = *(uint32_t *)((uint8_t *)shared_session + 0x3a4);
            }
        } else if (network_game_mode == 3) {
            request.salt = *(uint32_t *)((uint8_t *)session + 0x3a4);
        }
    }
    halo::game::cache_file_switch_map_by_path(request.map_name, 1);
    if (halo::game::globals().game_time->initialized != 0 && (halo::game::globals().game_time->active != 0 || halo::game::globals().game_time->paused != 0)) {
        halo::game::game_stop_current_map();
        halo::game::game_unload_map();
    }
    halo::main::main_menu_music_stop();
    if (session->variant.game_engine_index != 0) {
        halo::game::game_engine_apply_variant(&session->variant);
    }
    halo::game::cache_file_switch_map_by_path(request.map_name, 1);
    memcpy(main_game_globals + 8, &request, sizeof(request));
    loaded = halo::scenario::scenario_load(request.map_name);
    if (loaded == 0) {
        if (*main_game_globals == 0) {
            return session->map_loaded;
        }
    } else {
        *main_game_globals = 1;
    }
    session->map_loaded = 1;
    halo::game::game_start_new_map();
    if (network_game_mode == 2) {
        for (i = 0; i < 0x10; i++) {
            if (halo::networking::network_player_entry_validate(&session->players[i]) == 0) {
                break;
            }
            if (halo::networking::network_channel_key_open(&session->players[i]) == 0) {
                session->map_loaded = 0;
                break;
            }
        }
        if (((*(uint8_t *)((uint8_t *)network_server + 6) >> 2) & 1) != 0) {
            halo::game::game_engine_init_tick_record_for_mode();
            halo::game::game_engine_reset_all_players();
        }
    }
    return session->map_loaded;
}

/**
 * session->unknown_3ac by this function, matching the header's own note on that field
 * Zeroes the whole session, then explicitly resets maximum_players to 16, player_count to 0,
 * every player row to its documented empty state, and unknown_3ac from the global flag.
 *
 * @address 0x4de470
 */
void GameSessionView::session_reset()
{
    network_game_session *session = self;
    int32_t i;
    network_player_entry *player;

    memset(session, 0, sizeof(network_game_session));
    session->player_count = 0;
    for (i = 0; i < 16; i++) {
        player = &session->players[i];
        player->name[0] = 0;
        player->color_index = -1;
        player->icon_index = -1;
        player->machine_index = -1;
        player->machine_player_index = -1;
        player->team_index = -1;
        player->slot_index = -1;
    }
    session->maximum_players = 0x10;
    session->map_loaded = network_channel_table_default_flag != 0;
}

/**
 * types/networking.h cites this address directly: "network_player_entry (... 0x4df790
 * colour assignment)". out/phase4/networking_functions.md: "Randomly assigns a player colour
 * index that isn't already in use by another active player, widening the candidate range after
 * 10 failed attempts, and stores it at param_1+0x18." param_1+0x18 matches
 * network_player_entry.color_index; the scanned array (container+0x1c2 == session.players[0]+
 * 0x20 == session.players[1].color_index, stride 0x10 shorts) matches every active player's own
 * color_index field.
 *
 * @address 0x4df790
 */
void GameSessionView::assign_random_color(network_player_entry *entry)
{
    network_game_session *session = self;
    int32_t attempt;
    uint32_t seed;
    int16_t candidate;
    int32_t in_use;
    int32_t i;

    attempt = 0;
    seed = halo::math::globals().effect_random_seed;
    for (;;) {
        seed = halo::advance_random_seed(seed);
        if (attempt < 10) {
            candidate = (int16_t)((int32_t)(seed >> 0x10) * 3 >> 0x10);
        } else {
            candidate = (int16_t)((int32_t)(seed >> 0x10) * 0x11 >> 0x10);
        }
        in_use = 0;
        halo::math::globals().effect_random_seed = seed;
        for (i = 0; i < 0x10; i++) {
            if (halo::networking::network_player_entry_validate(&session->players[i]) != 0 &&
                session->players[i].color_index == candidate) {
                in_use = 1;
                break;
            }
        }
        attempt = attempt + 1;
        if (!in_use) {
            entry->color_index = candidate;
            return;
        }
    }
}

/**
 * Looks up an existing player row by (machine_index, machine_player_index); if none matches,
 * validates the incoming record (network_player_entry_validate), finds a free row (preferring the incoming
 * record's own slot_index if it names an empty row), copies the 32-byte record in, and bumps
 * player_count. Returns 1 on success, 0 on failure (only AL is defined; the upper bytes of EAX
 * are leftovers, not a packed slot index).
 *
 * @address 0x4de4e0
 */
uint32_t GameSessionView::add(network_player_entry *incoming)
{
    network_game_session *session = self;
    int8_t machine_index;
    int8_t machine_player_index;
    int32_t i;
    int32_t free_index;
    int8_t incoming_slot;
    uint32_t *src;
    uint32_t *dst;
    int32_t k;

    if (session->player_count >= session->maximum_players) {
        return 0;
    }
    machine_index = incoming->machine_index;
    machine_player_index = incoming->machine_player_index;
    if (machine_index < 0 || machine_index >= 0x10 ||
        machine_player_index < 0 || machine_player_index >= 1) {
        return 0;
    }

    for (i = 0; i < 0x10; i++) {
        if (session->players[i].machine_index == machine_index &&
            session->players[i].machine_player_index == machine_player_index) {
            break;
        }
    }
    if (i == 0x10 && halo::networking::network_player_entry_validate(incoming) != 0) {
        free_index = -1;
        for (k = 0; k < 0x10; k++) {
            if (session->players[k].slot_index == -1) {
                free_index = k;
                break;
            }
        }
        incoming_slot = incoming->slot_index;
        if (incoming_slot != -1 && free_index != incoming_slot) {
            free_index = incoming_slot;
        }
        if (free_index != -1) {
            incoming->slot_index = (int8_t)free_index;
            dst = (uint32_t *)&session->players[free_index];
            src = (uint32_t *)incoming;
            for (k = 0; k < 8; k++) {
                dst[k] = src[k];
            }
            session->player_count = session->player_count + 1;
            return 1;
        }
    }
    return 0;
}

/**
 * types/networking.h cites this address directly under network_player_entry: "0x4de900
 * find". Validates the key record via network_player_entry_validate, then scans session->players[] for a
 * machine_index/machine_player_index match. Both callers in this batch (network_player_entry_
 * update.c, network_player_entry_remove.c) only ever test the low byte of this function's
 * return (truthy/falsy); only AL is defined in the original, so a plain 0/1 return is equivalent.
 *
 * @address 0x4de900
 */
char GameSessionView::find(network_player_entry *key)
{
    network_game_session *session = self;
    int32_t i;

    if (halo::networking::network_player_entry_validate(key) == 0) {
        return 0;
    }
    for (i = 0; i < 0x10; i++) {
        if (session->players[i].machine_index == key->machine_index &&
            session->players[i].machine_player_index == key->machine_player_index) {
            return 1;
        }
    }
    return 0;
}

/**
 * types/networking.h cites this address directly under network_player_entry: "0x4de640
 * remove". After the network_player_entry_find pre-check, re-scans for the same
 * (machine_index, machine_player_index) key and resets that row to its documented empty state
 * (matching network_game_session_reset.c's field-by-field evidence), decrementing player_count.
 *
 * @address 0x4de640
 */
uint32_t GameSessionView::remove(network_player_entry *key)
{
    network_game_session *session = self;
    int32_t i;
    network_player_entry *slot;

    if (halo::networking::network_player_entry_find(session, key) == 0) {
        return 0;
    }
    for (i = 0; ; i++) {
        if (halo::networking::network_player_entry_validate(key) != 0 && session->players[i].machine_index == key->machine_index &&
            session->players[i].machine_player_index == key->machine_player_index) {
            break;
        }
        if (i > 0xf) {
            return 0;
        }
    }
    slot = &session->players[i];
    slot->team_index = -1;
    slot->machine_player_index = -1;
    slot->icon_index = -1;
    slot->slot_index = -1;
    slot->name[0] = 0;
    slot->color_index = -1;
    slot->machine_index = -1;
    session->player_count = session->player_count - 1;
    return ((uint32_t)i << 8) | 1;
}

/**
 * types/networking.h cites this address directly under network_player_entry: "0x4de5f0
 * update". Finds the row via network_player_entry_find (0x4de900) then overwrites it with the
 * incoming 32-byte record from `in_EAX`, after confirming the found slot's own
 * machine_player_index/color_index still match (a stale-slot guard).
 *
 * @address 0x4de5f0
 */
uint8_t GameSessionView::update_(network_player_entry *incoming)
{
    network_game_session *session = self;
    int32_t slot_index;
    network_player_entry *slot;
    uint32_t *src;
    uint32_t *dst;
    int32_t i;

    if (halo::networking::network_player_entry_find(session, incoming) == 0) {
        return 0;
    }
    slot_index = incoming->slot_index;
    slot = &session->players[slot_index];
    if (slot->machine_player_index == incoming->machine_player_index &&
        slot->machine_index == incoming->machine_index) {
        dst = (uint32_t *)slot;
        src = (uint32_t *)incoming;
        for (i = 0; i < 8; i++) {
            dst[i] = src[i];
        }
        return ((uint32_t)slot_index << 8) | 1;
    }
    return 0;
}

/**
 * out/phase4/networking_functions.md: "Checks whether the wide-character name pointed
 * to by unaff_EBX already matches any active player's name in the 16-slot table, returning 0 on
 * a collision." The scan base (container + 0x1aa) is exactly container->session.players[0].name
 * when container is a network_client_globals/network_server_globals (session embedded at +8,
 * players[] at session+0x1a2, name the first field of each 0x20-byte entry) -- 0x008 + 0x1a2 =
 * 0x1aa.
 *
 * @address 0x4df6f0
 */
uint8_t GameSessionView::name_collision_check(uint16_t *candidate_name)
{
    network_game_session *session = self;
    int32_t i;

    for (i = 0; i < 0x10; i++) {
        if (halo::networking::network_player_entry_validate(&session->players[i]) != 0) {
            if (wcscmp((wchar_t *)session->players[i].name, (wchar_t *)candidate_name) == 0) {
                return 0;
            }
        }
    }
    return 1;
}

/**
 * out/phase4/networking_functions.md summary ("Tests whether a single entry in the
 * 9-slot game-search/pending-connection record table is still within its ~6 second freshness
 * window"); entry+0x12d and entry+0x18 match types/networking.h's
 * network_game_search_entry::in_use and ::received_ms exactly, and 0x1771 (6001 ms) matches
 * k_network_game_search_expiry_ms (6000) plus one.
 *
 * @address 0x4da770
 */
uint8_t SearchEntryView::entry_is_fresh()
{
    network_game_search_entry *entry = self;
    large_integer counter;
    int32_t now_ms;
    int32_t elapsed_ms;

    if (entry->in_use == 0) {
        return 0;
    }
    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    now_ms = (int32_t)((counter.quad_part * 1000) / halo::cseries::globals().performance_frequency);
    elapsed_ms = now_ms - entry->received_ms;
    if (elapsed_ms < 0x1771) {
        return 1;
    }
    return 0;
}

/**
 * already named)
 * address 0x4da7d0, size 582 bytes
 * name confidence: 0.5   rewrite confidence: 0.45
 * out/phase4/networking_types_notes.md "network_game_search_entry (0x130)" fully
 * documents this function's field writes; every offset below is taken directly from that
 * section and from types/networking.h's network_game_search_entry struct.
 *
 * @address 0x4da7d0
 */
int32_t SearchEntryView::results_add_or_update(const uint8_t *announcement)
{
    network_game_search_entry *results = self;
    large_integer counter;
    int32_t now_ms;
    int32_t i;
    int32_t slot;
    char joinable;
    network_game_search_entry *entry;
    const wchar_t *name_source;

    joinable = 1;
    if ((*(announcement + 0x15e) & 2) == 0 || *(const int16_t *)(announcement + 0x156) > 0xf) {
        joinable = 0;
    }

    for (i = 0; i < 9; i = i + 1) {
        entry = &results[i];
        if (entry->in_use == 0) {
            memset(entry, 0, sizeof(network_game_search_entry));
            continue;
        }
        QueryPerformanceCounter((LARGE_INTEGER *)&counter);
        now_ms = (int32_t)((counter.quad_part * 1000) / halo::cseries::globals().performance_frequency);
        if (6000 < now_ms - entry->received_ms) {
            memset(entry, 0, sizeof(network_game_search_entry));
        }
    }

    slot = -1;
    for (i = 0; i < 9; i = i + 1) {
        if (*(const uint32_t *)announcement == results[i].identity[0]) {
            slot = i;
            break;
        }
    }

    if (slot == -1) {
        for (i = 0; i < 9; i = i + 1) {
            if (results[i].in_use == 0) {
                slot = i;
                break;
            }
        }
    }

    if (slot == -1) {
        if (!joinable) {
            return 0;
        }
        for (i = 0; i < 9; i = i + 1) {
            if (results[i].joinable == 0) {
                memset(&results[i], 0, sizeof(network_game_search_entry));
                slot = i;
                break;
            }
        }
        if (slot == -1) {
            return 0;
        }
    }

    entry = &results[slot];
    entry->in_use = 1;
    entry->identity[0] = *(const uint32_t *)(announcement + 0x00);
    entry->identity[1] = *(const uint32_t *)(announcement + 0x04);
    entry->identity[2] = *(const uint32_t *)(announcement + 0x08);
    entry->identity[3] = *(const uint32_t *)(announcement + 0x0c);
    entry->identity[4] = *(const uint32_t *)(announcement + 0x10);
    entry->identity[5] = *(const uint32_t *)(announcement + 0x14);

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    now_ms = (int32_t)((counter.quad_part * 1000) / halo::cseries::globals().performance_frequency);
    entry->received_ms = now_ms;

    entry->unknown_12a = *(const int16_t *)(announcement + 0x1c);
    name_source = (const wchar_t *)(announcement + 0x1e);
    if (*name_source == L'\0') {
        name_source = L"???";
    }
    wcsncpy((wchar_t *)entry->name, name_source, 0x3f);
    entry->name[63] = 0;

    entry->game_engine_index = *(const int16_t *)(announcement + 0x154);
    for (i = 0; i < 0x21; i = i + 1) {
        entry->info[i] = *(const uint32_t *)(announcement + 0xd0 + i * 4);
    }
    entry->player_count = *(const int16_t *)(announcement + 0x156);
    entry->unknown_124 = *(const int16_t *)(announcement + 0x158);
    entry->unknown_126 = *(const int16_t *)(announcement + 0x15a);
    entry->unknown_128 = *(const int16_t *)(announcement + 0x15c);
    entry->joinable = joinable;
    entry->stats_logging = (*(announcement + 0x15e) >> 2) & 1;

    if (entry->game_engine_index == 3 && (*(announcement + 0x15e) & 8) != 0) {
        entry->unknown_12f = 1;
        return 1;
    }
    entry->unknown_12f = 0;
    return 1;
}

/**
 * Returns the desired team index of the player currently occupying object->unknown_00c's
 * machine slot, or -1 if the slot is empty, out of range, or the player datum is not live.
 *
 * @address 0x4e0cf0
 */
int32_t ObjectOwnership::owner_team_index_desired(object *obj)
{
    uint16_t slot;
    datum_index resolved;
    player *plr;

    slot = *(uint16_t *)&((struct object *)obj)->network_update_tick;
    if (slot != 0xffff && &machine_to_player[slot] != 0 && machine_to_player[slot] != (datum_index)halo::k_dword_none) {
        resolved = machine_to_player[slot];
        plr = (player *)halo::memory::datum_get(resolved, halo::game::globals().player_data);
        if (plr != 0) {
            return (int32_t)plr->team_index_desired;
        }
    }
    return -1;
}

/**
 * Resolves `slot_index` to a player datum; if it is live, notifies game_engine_notify_object_value_event of the
 * player's team and, if a local ownership claim is active, clears it.
 *
 * @address 0x4dfc10
 */
void ObjectOwnership::release_ownership_claim(uint8_t slot_index)
{
    uint32_t datum;
    int16_t player_index;
    int16_t salt;
    player *plr;

    datum = halo::networking::player_data_iterator_advance(slot_index);
    if (datum == halo::k_dword_none) {
        return;
    }
    player_index = (int16_t)datum;
    if (player_index < 0 || player_index >= halo::game::globals().player_data->maximum_count) {
        return;
    }
    plr = (player *)((uint8_t *)halo::game::globals().player_data->data + halo::game::globals().player_data->size * player_index);
    if (plr->identifier == 0) {
        return;
    }
    salt = (int16_t)(datum >> 16);
    if (salt != 0 && plr->identifier != salt) {
        return;
    }
    halo::game::game_engine_notify_object_value_event(slot_index, (int32_t)datum, -1, (void *)(uintptr_t)plr->team);
    {
        int32_t profile_slot = halo::game::game_engine_player_profile_cache_find((datum_index)datum);
        if (profile_slot != -1) {
            halo::game::game_engine_capture_player_profile(profile_slot, 0);
        }
    }
}

/**
 * True when entry is non-NULL, its machine_player_index is 0, its machine_index is 0..15, and
 * its name field contains a NUL within its first 12 UTF-16 code units.
 *
 * @address 0x4de9f0
 */
char PlayerEntryView::validate()
{
    network_player_entry *entry = self;
    int32_t i;

    if (entry != 0 && entry->machine_player_index >= 0 && entry->machine_player_index < 1 &&
        entry->machine_index >= 0 && entry->machine_index < 0x10) {
        for (i = 0; i < 12; i++) {
            if (entry->name[i] == 0) {
                return 1;
            }
        }
    }
    return 0;
}

/**
 * fopen: <stdio.h>, resolved to the game CRT at 0x624186 // 0x624186, fopen-shaped CRT wrapper
 * Formats format (with its trailing varargs) into a scratch buffer, then appends it to
 * ServerPlayerUpdateHistory.log when server player-update-history logging is enabled.
 *
 * @address 0x4e7f90
 */
void PlayerReports::update_history_log_write_v(const char *format, va_list args)
{
    char buffer[0x400];
    FILE *file;

    vsprintf(buffer, format, args);
    if (network_player_update_log_enabled == 1) {
        file = (FILE *)fopen(network_player_update_history_log_path,
            player_update_log_file_mode_string);
        if (file != 0) {
            fprintf(file, buffer);
            fclose(file);
        }
    }
}

/**
 * the gcd_authenticate_user callback (game id, local id,
 * authenticated, message, instance): a rejected key sends reason 4 to the machine with that CD key local id (+0x5c
 * of the 0x60 byte machines at server +0x3b8; NULL when none).
 *
 * @address 0x5760a0
 */
void HostSession::cd_key_callback(int32_t game_id, int32_t local_id, int32_t authenticated, const char *message, void *instance)
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
    halo::networking::network_server_notify_or_resend_challenge(4, machine, server);
}

/**
 * Tears down the network channel/session object created by network_session_host_start, if one
 * exists.
 *
 * @address 0x5778f0
 */
void HostSession::dispose()
{
    if (network_session_host_object != 0) {
        if (network_session_host_state != 2) {
            network_session_host_state = 2;
        }
        halo::networking::network_session_host_update();
        network_console_connection_id = -1;
        gcd_shutdown();
        qr2_shutdown(network_session_host_object);
        network_session_host_object = 0;
    }
}

/**
 * the query/report NAT negotiation callback (cookie): starts
 * NNBeginNegotiationWithSocket on the game socket's SOCKET (read through 0x6175f0, folded with ArrayLength) with
 * that cookie, client index 0, function_do_nothing as the progress callback and
 * network_session_host_natneg_completed.
 *
 * @address 0x578160
 */
void HostSession::natneg_callback(int32_t cookie)
{
    NNBeginNegotiationWithSocket(*(uint32_t *)network_game_socket, cookie, 0, (void *)halo::cseries::function_do_nothing,
        (void *)halo::networking::network_session_host_natneg_completed, 0);
}

/**
 * the NAT negotiation completion callback (result, socket,
 * remote sockaddr_in, user data): on success it formats the remote address into a stack buffer with
 * gt2AddressToString and does nothing with it (a leftover).
 *
 * @address 0x578120
 */
void HostSession::natneg_completed(int32_t result, uint32_t socket, const uint8_t *remote_address, void *user_data)
{
    char text[0x16];

    (void)socket;
    (void)user_data;
    if (result == 0) {
        gt2AddressToString(*(const uint32_t *)(remote_address + 4), gt2NetworkToHostShort(*(const uint16_t *)(remote_address + 2)), text);
    }
}

/**
 * the qr2 add-error callback (error, message, user data):
 * "qr2_adderror_callback - %s" to the console in its standard color.
 *
 * @address 0x578100
 */
void HostSession::qr2_add_error(int32_t error, char *message, void *user_data)
{
    (void)error;
    (void)user_data;
    halo::interface::console_printf_verbose((ColorARGB *)console_message_default_color, (char *)"qr2_adderror_callback - %s", message);
}

/**
 * the player / team count callback (key type, user data): 0
 * outside a game; the game engine's +0xa8 hook when it has one; otherwise players -> the active player count, teams
 * -> 2 with teams (else 0), anything else 0.
 *
 * @address 0x5780c0
 */
int32_t HostSession::qr2_count(int32_t key_type, void *user_data)
{
    int32_t (*hook)(int32_t);

    (void)user_data;
    if (current_game_engine == 0) {
        return 0;
    }
    hook = *(int32_t (**)(int32_t))((uint8_t *)current_game_engine + 0xa8);
    if (hook != 0) {
        return hook(key_type);
    }
    if (key_type == 1) {
        return halo::game::players_active_count();
    }
    if (key_type == 2 && halo::game::globals().teams_enabled != 0) {
        return 2;
    }
    return 0;
}

/**
 * the team key callback (key, index, buffer, user data): the
 * game engine's +0xa4 hook answers first; key 0x1c is the team name ("Red" for 0, "Blue" for 1 -- as the binary has
 * it, index 1 is "Blue"); anything else is empty.
 *
 * @address 0x577f40
 */
void HostSession::qr2_team_key(int32_t key_id, int32_t index, void *buffer, void *user_data)
{
    (void)user_data;
    if (current_game_engine != 0) {
        uint8_t (*hook)(int32_t, int32_t, void *) = *(uint8_t (**)(int32_t, int32_t, void *))((uint8_t *)current_game_engine + 0xa4);

        if (hook != 0 && hook(key_id, index, buffer) != 0) {
            return;
        }
    }
    if (key_id == 0x1c) {
        qr2_buffer_add(buffer, index == 1 ? "Blue" : "Red");
        return;
    }
    qr2_buffer_add(buffer, "");
}

/**
 * the host's CD key check for a joining machine:
 * gcd_authenticate_user(game id 0x0069fdfc, local id, ip, challenge, response,
 * network_session_host_cd_key_callback, 0), then the ban list check on the key hash (gcd_getkeyhash, EDI). Not
 * banned: 1. Banned: reason 6 to the machine with that local id (or NULL), the key is disconnected from gcd (every
 * key for local id -1), 0. (Name kept; it authenticates.)
 *
 * @address 0x575ff0
 */
uint8_t HostSession::reject_or_cleanup_client(const char *response, const char *challenge, uint32_t ip, int32_t local_id)
{
    network_server_globals *server;
    network_machine *machine = 0;
    int32_t i;

    gcd_authenticate_user(network_console_connection_id, local_id, ip, challenge, response, (void *)halo::networking::network_session_host_cd_key_callback, 0);
    if (halo::networking::ban_list_check_and_reject_player((char *)gcd_getkeyhash(network_console_connection_id, local_id)) == 0) {
        return 1;
    }
    server = network_server;
    for (i = 0; i < 0x10; i++) {
        if (*(int32_t *)((uint8_t *)server + 0x414 + i * 0x60) == local_id) {
            machine = (network_machine *)((uint8_t *)server + 0x3b8 + i * 0x60);
            break;
        }
    }
    halo::networking::network_server_notify_or_resend_challenge(6, machine, server);
    if (local_id == -1) {
        gcd_disconnect_all(network_console_connection_id);
    } else {
        gcd_disconnect_user(network_console_connection_id, local_id);
    }
    return 0;
}

/**
 * disposes the old session, opens the channels and starts
 * query/report on the game socket's SOCKET (0x6175f0) with the port, game name and secret key strings, the public
 * flag byte, natneg on, the six host callbacks (the earlier version passed NULL for five of them and the player key
 * callback in the wrong slot) and the argument as user data; registers the natneg callback, and initializes the CD
 * key server with game id 0x319 on the same record. Returns qr2_init_socketA's result.
 *
 * @address 0x577850
 */
int32_t HostSession::start(void *user_data)
{
    int32_t result;

    halo::networking::network_session_host_dispose();
    halo::networking::network_channels_open();
    result = qr2_init_socketA(&network_session_host_object, *(uint32_t *)network_game_socket, network_session_start_game_type,
        network_session_start_host_name, network_session_start_map_name, network_session_host_flags_byte, 1,
        (void *)halo::networking::network_session_host_qr2_server_key, (void *)halo::networking::network_session_host_dispatch_message,
        (void *)halo::networking::network_session_host_qr2_team_key, (void *)halo::networking::network_session_host_qr2_key_list,
        (void *)halo::networking::network_session_host_qr2_count, (void *)halo::networking::network_session_host_qr2_add_error, user_data);
    qr2_register_natneg_callback(network_session_host_object, (void *)halo::networking::network_session_host_natneg_callback);
    network_console_connection_id = 0x319;
    gcd_init_qr2(network_session_host_object, 0x319, network_session_host_flags_byte);
    return result;
}

/**
 * Stashes the host, map and (optional) variant names plus a game-type value into the globals a
 * new network session is started from, then registers the well-known hs script globals
 * (dedicated, player_flags, game_flags, game_classic) a dedicated server exposes.
 *
 * @address 0x576100
 */
void HostSession::start_info_set(char *host_name, char *map_name, char *variant_name, int32_t game_type)
{
    char *dest;
    char *src;

    dest = network_session_start_host_name;
    src = host_name;
    do {
        *dest++ = *src;
    } while (*src++ != 0);

    dest = network_session_start_map_name;
    src = map_name;
    do {
        *dest++ = *src;
    } while (*src++ != 0);

    if (variant_name != 0) {
        dest = network_session_start_variant_name;
        src = variant_name;
        do {
            *dest++ = *src;
        } while (*src++ != 0);
    }

    network_session_start_game_type = game_type;
    halo::networking::qr2_register_key(0x33, "dedicated");
    halo::networking::qr2_register_key(0x34, "player_flags");
    halo::networking::qr2_register_key(0x35, "game_flags");
    halo::networking::qr2_register_key(0x36, "game_classic");
}

/**
 * Periodic per-frame update for the network channel/session object: flushes it on timeout (1000+
 * ticks since the last flush) or on a pending close request, then pumps it either way.
 *
 * @address 0x577940
 */
void HostSession::update_()
{
    if (network_session_host_object != 0) {
        if (network_session_host_state != 0) {
            int32_t now = halo::cseries::time_query_performance_counter_ms();
            if (network_session_host_state == 2 || (uint32_t)(now - network_session_host_last_tick) > 999) {
                network_session_host_closing = (network_session_host_state == 2);
                qr2_send_statechanged(network_session_host_object);
                network_session_host_state = 0;
                network_session_host_last_tick = now;
            }
        }
        qr2_think(network_session_host_object);
        network_session_host_closing = 0;
    }
}

}
