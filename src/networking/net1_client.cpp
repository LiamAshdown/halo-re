#include "halo/networking/net1_client.hpp"
#include "halo/networking/net_state.hpp"
#include "halo/networking/net1_dispatch.hpp"
#include <string.h>
#include <wchar.h>
#include <stdint.h>
#include <stdio.h>
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/cseries/api.hpp"
#include "halo/main/api.hpp"

extern "C" {
extern network_client_globals *network_client;
extern uint8_t network_host_handoff_requested;
extern int16_t network_game_mode;
extern uint8_t profile_globals_block[0x1ffc];
extern network_client_globals *network_session_create(void);
extern void network_debug_fill_canary_buffer(void);
extern int8_t chimera__on_connect(const uint32_t *target_address, network_client_globals *client, const uint32_t *session_info);
extern uint32_t chat_close(void);
extern datum_index machine_to_player[16];
extern data_array *player_data;
extern uint8_t network_stats_enabled_gate;
extern int64_t main_globals_data;
extern int32_t network_connect_timeout_ms;
extern int32_t message_delta_decode_begin(message_delta_decode_state *state, bit_stream *stream);
extern int32_t message_delta_decode_array_field(void **context);
extern void network_game_client_apply_received_update(network_machine *machine, uint32_t server, void **message);
extern void chat_server_relay_incoming_message(void **context, network_machine *machine);
extern void game_engine_update_lead_change_state(void **envelope, uint8_t *message);
extern uint32_t network_game_message_handle_ping_timestamp(int32_t **message, network_server_globals *server);
extern void network_server_handle_rcon_request(network_player_entry *client, void *message);
extern uint8_t network_session_active;
extern void message_delta_parameters_protocol_dump_to_config_file(void);
extern void player_update_history_destroy(void *update_history);
extern void network_channel_delete(network_channel *channel);
extern void network_stats_summary_log_write(void);
extern char network_log_path_format[];
extern uint8_t message_delta_decode_compound_field(void *decode_context, void *destination);
extern void message_delta_decode_compound_field_staged(void *decode_context);
extern void *global_white_argb;
extern void chimera__console_out(ColorARGB *color, char *format, ...);
extern int32_t network_channel_service_close_if_disconnected(network_channel *channel);
extern network_server_globals *network_server;
extern player_globals *local_player_globals;
extern void network_client_globals_dispose(void);
extern void network_client_globals_create(void);
extern uint8_t network_channel_table_default_flag;
extern void network_client_begin_connect(const wchar_t *name);
extern uint8_t network_client_vehicle_ack_enabled;
extern network_id_table *machine_table;
extern uint8_t player_unit_has_parent(datum_index player_handle);
extern int32_t build_local_player_position_update(uint8_t *out_changed, player *plr);
extern int32_t build_local_player_vehicle_update(uint8_t *out_changed, player *plr);
extern uint8_t network_session_send_to_machine(int32_t machine_id, void *data, int32_t bits, int32_t reliable, int32_t unknown_a, int32_t unknown_b, int32_t priority);
extern char network_join_handshake_tick(network_client_globals *client);
extern char network_join_connect_retry_tick(network_client_globals *client);
extern char network_host_lobby_tick(network_client_globals *client);
extern int8_t network_game_client_update(network_client_globals *client);
extern char network_host_channel_service_tick(network_client_globals *client);
extern void network_channel_remote_address_or_default(network_channel *channel, network_resolved_address *out_address);
extern uint8_t network_server_host_valid;
extern uint32_t split_screen_quit_prompt_string;
extern uint32_t network_join_error_reason;
extern void network_game_server_host_dispose(network_server_globals *host);
extern char network_client_state_dispatch(void);
extern int32_t network_client_connect_progress_percent(void);
extern uint8_t network_disconnect_timeout_flag;
extern void ui_network_wait_timeout_start(void);
extern char network_channel_service_light(int32_t flag);
extern void network_channel_record_timestamp(network_channel *channel);
extern int32_t network_game_process_incoming_messages(network_client_globals *client);
extern int8_t network_channel_service_retransmit_only(void);
extern int16_t network_join_error_code;
extern void network_connection_send_keepalive(network_client_globals *client);
extern uint8_t network_ping_debug_log_enabled;
extern uint32_t network_ping_debug_last_sample;
extern void console_print_error_va(uint8_t clear_first, const char *format, ...);
extern int32_t message_delta_sample_ring_buffer_average(message_delta_sample_ring_buffer *ring);
extern data_packet_group network_game_messages_group;
extern uint16_t network_challenge_packet_block[];
extern uint16_t *network_message_block_build(uint16_t *buffer, uint32_t *source, uint8_t flags, uint32_t length);
extern char network_channel_stream_flush(network_channel_stream *stream, network_channel *channel, char mode);
extern char network_player_entry_validate(network_player_entry *entry);
extern char network_player_entry_add(network_player_entry *entry, network_game_session *session);
extern int32_t network_channel_key_open(network_player_entry *entry);
extern int32_t player_data_iterator_advance(int16_t step_count);
extern void game_set_local_player(datum_index player_handle, int16_t local_player_index);
extern void update_server_queue_create_entry(datum_index requested_handle);
extern data_array *update_client_queues;
extern network_client_globals network_client_storage;
extern void message_delta_protocol_initialize(void);
extern network_channel *network_channel_new(uint32_t flags);
extern void network_session_destroy(network_client_globals *client);
extern void network_game_session_reset(network_game_session *session);
extern void network_stats_summary_log_open(void);
extern uint16_t *network_prepare_challenge_packet(int32_t message_type, void *payload);
extern int32_t network_game_socket_port;
extern char *network_address_to_string(s_network_address *addr);
extern int16_t network_channel_attempt_connect(int32_t a, int32_t b);
extern int32_t network_connection_endpoint_set(const uint32_t *source, network_client_globals *connection);
extern void message_delta_sample_record_and_append(int32_t a, int32_t c, int32_t b, message_delta_sample_ring_buffer *ring);
extern void network_channel_reliable_pool_store(network_channel *channel, void *message, uint8_t *out_flag, int32_t priority);
extern char network_channel_service(network_channel *channel, int32_t timeout_ms, network_channel **out_new_child);
extern char network_client_identity_tick(network_client_globals *client);
extern void network_host_presence_broadcast_tick(network_client_globals *client);
extern char network_build_string[];
extern int32_t network_signal_quality_glyph(void);
extern void network_receive_queue_close_socket(void);
extern void console_printf_verbose(const char *text);
extern int32_t interface_loading_screen_progress;
extern int32_t join_ui_state;
extern int32_t interface_loading_screen_request_id;
extern void network_join_status_text_update(int32_t mode, network_client_globals *client);
extern uint32_t network_local_address;
extern int32_t network_connection_initiate(network_client_globals *connection, const uint32_t *target, const uint32_t *session_info);
extern void *server_browser_join_target;
extern void *master_server_query_engine;
extern uint16_t network_join_target_address[128];
extern uint8_t server_browser_join_target_has_password;
extern int32_t network_game_socket;
extern uint8_t network_channels_open_ok;
extern int32_t interface_loading_screen_address_a;
extern int32_t interface_loading_screen_address_b;
extern int32_t progress_screen_text;
extern int32_t progress_screen_subtext;
extern uint32_t SBServerGetPublicQueryPort(int32_t handle);
extern char *SBServerGetPublicAddress(int32_t handle);
extern uint32_t SBServerGetPrivateQueryPort(int32_t handle);
extern char *SBServerGetPrivateAddress(int32_t handle);
extern int32_t SBServerHasPrivateAddress(int32_t handle);
extern int32_t SBServerDirectConnect(int32_t handle);
extern uint32_t gamespy_array_length(int32_t object);
extern char *ServerBrowserGetMyPublicIP(void *handle);
extern uint32_t ServerBrowserGetMyPublicIPAddr(void *handle);
extern void network_channels_open(void);
extern int32_t network_random_offset(int32_t base);
extern void ServerBrowserSendNatNegotiateCookieToServer(void *handle, char *hostname, uint32_t port, int32_t request_id);
extern void network_join_hostname_resolved_callback(int32_t resolve_failed, uint32_t unused, uint8_t *hostent);
extern int32_t NNBeginNegotiationWithSocket(int32_t hostname, int32_t request_id, int32_t one, void (*progress_callback)(void), void (*complete_callback)(int32_t, uint32_t, uint8_t *), int32_t zero);
extern uint32_t network_game_client_connect_to_address(char *address_string, uint16_t *target_string);
}

/**
 * Calls halo::cache::cache_file_request_map with the argument list this file was reversed with; the function itself takes a
 * different list, so the call reads whatever the original left in the registers it takes the rest in.
 * Unresolved until the callers are reversed.
 */
static char cache_file_request_map_unresolved(int32_t unknown)
{
    using call_t = char (*)(int32_t unknown);
    return reinterpret_cast<call_t>(&halo::cache::cache_file_request_map)(unknown);
}

/**
 * Calls halo::memory::datum_get with the argument list this file was reversed with; the function itself takes a
 * different list, so the call reads whatever the original left in the registers it takes the rest in.
 * Unresolved until the callers are reversed.
 */
static void * datum_get_unresolved(datum_index index)
{
    using call_t = void * (*)(datum_index index);
    return reinterpret_cast<call_t>(&halo::memory::datum_get)(index);
}

/**
 * Calls halo::memory::data_packet_group_encode_packet with the argument list this file was reversed with; the function itself takes a
 * different list, so the call reads whatever the original left in the registers it takes the rest in.
 * Unresolved until the callers are reversed.
 */
static int32_t data_packet_group_encode_packet_unresolved(uint8_t *buffer, data_packet_group *group, void *payload, int32_t *capacity, int32_t message_type, int32_t flag)
{
    using call_t = int32_t (*)(uint8_t *buffer, data_packet_group *group, void *payload, int32_t *capacity, int32_t message_type, int32_t flag);
    return reinterpret_cast<call_t>(&halo::memory::data_packet_group_encode_packet)(buffer, group, payload, capacity, message_type, flag);
}

namespace halo::networking {

/**
 * out/phase4/networking_functions.md: "Begins a new outgoing connection attempt:
 * allocates/reuses the client connection object, hands off to chimera__on_connect with the
 * requested name/options, and marks the network session active on success." Confirmed via
 * objdump against network_game_client_connect_to_address.c's call site (`lea ecx,[esp+0xc]`
 * right before `call 0x4dc8d0`) that the elided ECX argument is the caller's local
 * s_network_address; this function's own body reads it as `in_ECX->ipv4 != 0` and
 * `in_ECX->port != 0` before proceeding, which is exactly chimera__on_connect's own
 *
 * @address 0x4dc8d0
 */
uint32_t ClientView::begin_connect(wchar_t *player_name, s_network_address *target_address)
{
    network_client_begin_connect_scratch scratch;
    uint32_t result;

    result = 0;
    if (network_client == 0) {
        network_client = network_session_create();
        if (network_client != 0) {
            network_host_handoff_requested = 0;
        }
    }
    memcpy(scratch.config_template, profile_globals_block,
        0x7ff * 4);
    *(uint32_t *)((uint8_t *)network_client + 0xf4c) = 0;
    if (network_client->state == 0 && target_address->ipv4 != 0 && target_address->port != 0) {
        wcsncpy((wchar_t *)scratch.name, (const wchar_t *)player_name, 8);
        scratch.name_terminator = 0;
        network_debug_fill_canary_buffer();
        if (chimera__on_connect((const uint32_t *)target_address, network_client,
                (const uint32_t *)&scratch) != 0) {
            network_game_mode = 1;
            return 1;
        }
        network_host_handoff_requested = 1;
        result = chat_close() & 0xffffff00;
    }
    return result;
}

/**
 * Resolves machine_index to a live player via machine_to_player/player_data, then -- while the
 * stats gate is enabled -- maintains a rolling packet-loss ratio and per-sample latency in
 * that player's unknown_108..unknown_118 block, returning false once loss exceeds 36:1 over a
 * 5000ms window or latency exceeds 39.9ms for more than 5 consecutive samples.
 *
 * @address 0x4e0080
 */
uint32_t ClientView::check_connection_quality(uint32_t machine_index, uint8_t units)
{
    datum_index resolved;
    int16_t player_index;
    player *plr;
    int16_t salt;

    resolved = machine_to_player[machine_index & 0xffff];
    if (resolved == (datum_index)0xffffffff) {
        return 0;
    }
    player_index = (int16_t)resolved;
    if (player_index < 0 || player_index >= player_data->maximum_count) {
        return 0;
    }
    plr = (player *)((uint8_t *)player_data->data + player_data->size * player_index);
    if (plr->identifier == 0) {
        return 0;
    }
    salt = (int16_t)(resolved >> 16);
    if (salt != 0 && plr->identifier != salt) {
        return 0;
    }

    if (network_stats_enabled_gate == 1) {
        int32_t now_ms;
        uint32_t added;
        uint32_t sample_count;
        float loss_ratio;
        float latency;

        now_ms = (int32_t)((main_globals_data * 1000) / halo::cseries::globals().performance_frequency);
        added = units;

        if (plr->connection_quality_started == 0) {
            plr->loss_window_start_ms = now_ms;
            plr->loss_window_units = added;
            plr->latency_last_sample_ms = now_ms;
            plr->latency_bad_sample_count = 0;
            plr->connection_quality_started = 1;
        } else {
            plr->loss_window_units = plr->loss_window_units + added;
            sample_count = (uint32_t)(now_ms - plr->loss_window_start_ms);

            if (sample_count == 0) {
                loss_ratio = 0.0f;
            } else {
                loss_ratio = (float)(uint32_t)plr->loss_window_units / ((float)sample_count * 0.001f);
            }
            if (sample_count > 10000) {
                plr->loss_window_start_ms = now_ms;
                plr->loss_window_units = added;
                sample_count = 0;
            }

            {
                uint32_t latency_window;

                latency_window = (uint32_t)(now_ms - plr->latency_last_sample_ms);
                if (latency_window == 0) {
                    latency = 0.0f;
                } else {
                    latency = (float)added / ((float)latency_window * 0.001f);
                }
                plr->latency_last_sample_ms = now_ms;

                if (loss_ratio > 36.0f && sample_count > 5000) {
                    return 0;
                }
                if (latency != 0.0f) {
                    if (latency <= 39.9f) {
                        plr->latency_bad_sample_count = 0;
                        return 1;
                    }
                    plr->latency_bad_sample_count = plr->latency_bad_sample_count + 1;
                    if (plr->latency_bad_sample_count > 5) {
                        return 0;
                    }
                }
            }
        }
    }
    return 1;
}

/**
 * out/phase4/networking_functions.md summary ("Computes the connection-attempt
 * progress as a percentage of the configured connect timeout, for display while in the
 * 'connecting' state"); the sole caller (network_client_update_dispatch) assigns its result straight through,
 * consistent with the mode check against client+0xeda == 1 (the "waiting to join" state per
 * src/networking/network_client_state_dispatch.c case 1).
 *
 * @address 0x4d8c10
 */
int16_t ClientView::connect_progress_percent(int16_t *out_percent)
{
    network_client_globals *client = self;
    int32_t now;

    if (out_percent != 0) {
        *out_percent = 0;
        if (client->state == k_network_client_state_connecting) {
            now = halo::cseries::time_query_performance_counter_ms();
            *out_percent = (int16_t)(((uint32_t)(now - client->connect_attempt.started_ms) * 100)
                                      / network_connect_timeout_ms);
        }
    }
    return client->state;
}

/**
 * the name src/networking/network_channel_dispatch_bitstream_unit.c's own extern already chose
 * for this address)
 * address 0x4e1f40, size 288 bytes
 * name confidence: 0.4   rewrite confidence: 0.85 (REWRITTEN; was 0.25)
 * out/phase4/networking_functions.md: "Drains the client's queued network-game
 * update messages, applying each one according to its message-type tag."
 *
 * @address 0x4e1f40
 */
char ClientView::drain_queued_updates(network_server_globals *server, network_machine *machine, bit_stream *stream)
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

/**
 * out/phase4/networking_functions.md: "Allocates the primary network-game globals
 * structure (network_client) via network_session_create and, if successful, clears the network_host_handoff_requested flag."
 * network_client (0x0071c2d8), network_session_create (0x4d8a80, already rewritten) and
 * network_host_handoff_requested (0x0071c2de) all match established names in this module.
 *
 * @address 0x4dde50
 */
int32_t ClientView::globals_create()
{
    network_client = network_session_create();
    if (network_client != 0) {
        network_host_handoff_requested = 0;
    }
    return network_client != 0;
}

/**
 * out/phase4/networking_functions.md: "Tears down the network-game globals
 * (network_client): releases its connection sub-allocation, resets related counters/flags and
 * frees the globals themselves." client->update_history (+0xf48) and client->channel (+0xadc)
 * match types/networking.h's network_client_globals exactly; network_session_active
 * (0x0071c2c2) matches the header's own documented name for that address.
 *
 * @address 0x4dde70
 */
void ClientView::globals_dispose()
{
    if (network_client != 0) {
        message_delta_parameters_protocol_dump_to_config_file();
        player_update_history_destroy(network_client->update_history);
        network_client->update_history = 0;
        if (network_client->channel != 0) {
            network_channel_delete(network_client->channel);
        }
        network_session_active = 0;
        network_stats_summary_log_write();
        network_client = 0;
    }
    network_host_handoff_requested = 0;
}

/**
 * Decodes an incoming broadcast text-message packet and prints its text to the console, unless
 * the message's leading dword marks it as a "meaningless" (already-handled) duplicate.
 *
 * @address 0x4e5140
 */
void ClientView::handle_server_text_message(void *message)
{
    uint8_t decode_buf[80];

    if (*(int32_t *)*(int32_t *)message == 0) {
        memset(decode_buf, 0, sizeof(decode_buf));
        if (message_delta_decode_compound_field(message, decode_buf) != 0) {
            int32_t text_len = strlen((char *)decode_buf);
            if (text_len != 0) {
                chimera__console_out((ColorARGB *)global_white_argb, network_log_path_format, decode_buf, text_len);
            }
        }
    } else {
        message_delta_decode_compound_field_staged(message);
    }
}

/**
 * out/phase4/networking_functions.md summary ("Periodic bookkeeping for the local
 * client's machine/session identity: expires a stale pending-close flag, then either refreshes
 * or re-establishes the client's connection identity depending on whether [acting as host]").
 * client+0xee4/+0xee8/+0xeec/+0xef0/+0xef4 match network_client_globals::timer
 * (a network_client_timer_record, folded into types/networking.h by the review pass), and
 * client+0xef8..+0xf0c is network_client_globals::server_address.
 *
 * @address 0x4db310
 */
int32_t ClientView::identity_tick()
{
    network_client_globals *client = self;
    large_integer counter;
    uint32_t now_ms;
    wchar_t name[10];
    int32_t timer_tail[6];
    int32_t local_player_id;
    int32_t i;

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    now_ms = (uint32_t)((counter.quad_part * 1000) / halo::cseries::globals().performance_frequency);

    if (client->timer.active != 0 && (uint32_t)client->timer.deadline_ms <= now_ms) {
        int32_t result = network_channel_service_close_if_disconnected(client->channel);
        client->timer.active = 0;
        client->timer.deadline_ms = 0;
        client->timer.triggered = 1;
        client->timer.retrigger_ms = client->timer.context + (int32_t)now_ms;
        return result;
    }

    if (client->timer.triggered != 0 && (uint32_t)client->timer.retrigger_ms <= now_ms &&
        (client->channel->endpoint->flags & 0x40) != 0) {
        for (i = 0; i < 6; i = i + 1) {
            timer_tail[i] = ((int32_t *)&client->server_address)[i];
        }
        local_player_id = -1;

        wcsncpy(name, (const wchar_t *)((uint8_t *)client + 0xaf0), 8);

        if (network_server == 0) {
            if (*(int32_t *)local_player_globals->local_players != -1) {
                void *player = datum_get_unresolved(*(datum_index *)local_player_globals->local_players);
                if (player != 0) {
                    local_player_id = ((struct player *)player)->team;
                }
            }
        } else {
            local_player_id = client->team_index;
        }

        network_client_globals_dispose();
        if (network_server != 0) {
            network_client_globals_create();
            network_client->team_index = local_player_id;
            return 1;
        }

        network_channel_table_default_flag = 1;
        network_client_begin_connect(name);

        for (i = 0; i < 5; i = i + 1) {
            ((int32_t *)&client->timer)[i] = 0;
        }
        for (i = 0; i < 6; i = i + 1) {
            ((int32_t *)&client->server_address)[i] = 0;
        }
        client->team_index = 0;
        client->team_index = -1;
        network_client->team_index = local_player_id;
        network_channel_table_default_flag = 0;
    }
    return 1;
}

/**
 * Per-tick client routine: for every local player with a live unit, builds either a vehicle
 * transform ack (if the player's unit has a parent and vehicle acks are enabled) or a plain
 * position ack, and sends the result to machine 1 if anything was encoded.
 *
 * @address 0x4e77e0
 */
void ClientView::send_local_player_updates()
{
    data_iterator iter;
    player *candidate;
    uint8_t out_changed;
    int32_t encoded_size;

    iter.data = player_data;
    iter.next_index = 0;
    iter.index = k_datum_index_none;
    iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;
    candidate = (player *)halo::memory::data_iterator_next(&iter);
    while (candidate != 0) {
        if (candidate->local_player_index == -1 && candidate->unit != (datum_index)-1) {
            if (player_unit_has_parent(iter.index) == 0 || network_client_vehicle_ack_enabled == 0) {
                encoded_size = build_local_player_position_update(&out_changed, candidate);
            } else {
                encoded_size = build_local_player_vehicle_update(&out_changed, candidate);
            }
            if (0 < encoded_size) {
                network_session_send_to_machine(1, 0, encoded_size, 0, 0, 0, 0);
            }
        }
        candidate = (player *)halo::memory::data_iterator_next(&iter);
    }
}

/**
 * out/phase4/networking_functions.md summary ("Dispatches per-frame processing to the
 * handler matching the connection's current mode/type field"); the sole caller (network_client_update_dispatch,
 * out/halo_decompiled.c around line 143483) loads network_client (network_client, per
 * types/networking.h) into EAX before the call and reads network_client+0xedc right after it
 * returns, confirming in_EAX is network_client here.
 *
 * @address 0x4d8bb0
 */
int8_t ClientView::state_dispatch()
{
    network_client_globals *client = self;
    const ClientStateHandler *state = ClientStateMachine::handler_for(client->state);

    if (state == nullptr) {
        return 0;
    }
    return state->tick(client);
}

/**
 * out/phase4/networking_functions.md summary ("Schedules a delayed network event/timer
 * to fire after a given number of milliseconds"). The five fields written (byte, dword, byte,
 * dword, dword at client+0xee4/0xee8/0xeec/0xef0/0xef4) are the first five elements of
 * network_client_globals::timer, a network_client_timer_record that the review pass folded
 * into types/networking.h (it occupies the first five dwords of the zeroed run at +0xee4).
 *
 * @address 0x4d9ed0
 */
void ClientView::timer_schedule(int32_t delay_ms, int32_t context)
{
    network_client_globals *client = self;
    network_client_timer_record *timer;
    large_integer counter;
    int32_t now_ms;

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    now_ms = (int32_t)((counter.quad_part * 1000) / halo::cseries::globals().performance_frequency);

    timer = &client->timer;
    timer->deadline_ms = now_ms + delay_ms;
    timer->context = context;
    timer->active = 1;
    timer->triggered = 0;
    timer->retrigger_ms = 0;

    network_channel_remote_address_or_default(client->channel, &client->server_address);
}

/**
 * out/phase4/networking_functions.md: "Either performs the same host shutdown
 * sequence as network_host_shutdown_or_defer (map-state reset and host dispose) when network_host_handoff_requested is set, or
 * attempts to join/prepare via network_client_state_dispatch/network_client_connect_progress_percent otherwise." network_client_state_dispatch and network_client_connect_progress_percent
 * are already named (network_client_state_dispatch, network_client_connect_progress_percent) by
 * an earlier batch covering 0x4d8a80..0x4d9050. See network_host_shutdown_or_defer.c for the
 * shared shutdown sequence's field evidence.
 *
 * @address 0x4dded0
 */
char ClientView::update_dispatch()
{
    network_game_session *session;
    char result;
    char dispatch_result;

    result = 1;
    if (network_host_handoff_requested == 1) {
        network_game_mode = 0;
        halo::main::main_menu_music_stop();
        if (network_server != 0) {
            session = &network_server->session;
        } else if (network_client != 0) {
            session = &network_client->session;
        } else {
            session = 0;
        }
        if (session->map_loaded != 0) {
            halo::main::chimera__load_ui_map(1);
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
    } else {
        dispatch_result = network_client_state_dispatch();
        result = 0;
        if (dispatch_result != 0) {
            if (network_client->disconnect_reason == 0) {
                halo::networking::net_state::last_connect_progress_percent = network_client_connect_progress_percent();
                return dispatch_result;
            }
            return 0;
        }
    }
    return result;
}

/**
 * out/phase4/networking_functions.md summary ("Main per-tick network update for an
 * actively-connected client: services the channel, processes incoming messages, flushes
 * outgoing data, and (when a debug flag is set) periodically logs ping/latency/timing"). Reuses
 * the network_client_globals::connection fields already named in
 * network_connection_send_keepalive.c (message_count@0xad0, retry_count@0xad2, unknown_20@0xad4,
 * control_block@0xad8) and the channel-flags bit layout confirmed in network_host_update_tick.c.
 *
 * @address 0x4daf80
 */
int8_t ClientView::client_update()
{
    network_client_globals *client = self;
    network_channel *channel;
    uint32_t flags;
    char service_ok;
    int8_t result;
    network_connection_endpoint *endpoint;

    channel = client->channel;
    flags = channel->flags;
    if ((~(uint8_t)(flags >> 4) & 1) != 0 && (flags & 6) != 0 &&
        channel->endpoint != 0 && (channel->endpoint->flags & 1) != 0) {
        if (network_server == 0 || network_disconnect_timeout_flag != 0) {
            if ((flags >> 5 & 1) != 0) {
                ui_network_wait_timeout_start();
            }
            client->connection_stalled = (uint8_t)(flags >> 5) & 1;
        }
        service_ok = network_channel_service_light(0);
        if (network_game_mode == 2) {
            network_channel_record_timestamp(channel);
        }
        if (service_ok != 0) {
            result = 0;
            if (network_game_process_incoming_messages(client)) {
                result = network_channel_service_retransmit_only();
            }
            goto tail;
        }
        channel = client->channel;
        if ((~(uint8_t)(channel->flags >> 4) & 1) != 0 && (channel->flags & 6) != 0 &&
            channel->endpoint != 0 && (channel->endpoint->flags & 1) != 0) {
            result = 0;
            goto tail;
        }
    }
    if (network_join_error_code == -1) {
        network_join_error_code = 4;
    }
    result = 0;
tail:
    network_connection_send_keepalive(client);

    endpoint = &client->connection;
    if (network_ping_debug_log_enabled == 1 && (uint32_t)(uint16_t)endpoint->message_count != network_ping_debug_last_sample &&
        (uint16_t)endpoint->message_count % 10 == 0) {
        int32_t server_base_time;
        int32_t challenge_time;
        int32_t now2;

        network_ping_debug_last_sample = (uint16_t)endpoint->message_count;
        console_print_error_va(0, "current ping time[%d]  samples received[%d]  samples sent[%d]\n",
            endpoint->current_ping_ms, endpoint->retry_count, (uint16_t)endpoint->message_count);
        server_base_time = *(int32_t *)endpoint->control_block;

        challenge_time = message_delta_sample_ring_buffer_average((message_delta_sample_ring_buffer *)endpoint->control_block);
        now2 = halo::cseries::time_query_performance_counter_ms();
        console_print_error_va(0, "current time delta[%d]  latency[%d]  server time[%d]\n",
            server_base_time, challenge_time, now2 + server_base_time);
    }
    return result;
}

/**
 * out/phase4/networking_functions.md summary ("Encodes a large (up to 0x600-byte)
 * outgoing network-game message (type id 0x12) from an 8-dword source record and queues it for
 * transmission"). This function has zero callers anywhere in the binary (out/functions.json
 * callers=0); rewritten anyway per the task instructions, since it is not listed as
 * misattributed/library code in out/phase4/networking_types_notes.md.
 *
 * @address 0x4da130
 */
int32_t ClientView::record_message_send(const uint32_t *source)
{
    network_client_globals *client = self;
    uint8_t record_copy[32];
    uint8_t encoded[0x600];
    uint32_t *dst;
    int32_t i;
    int32_t capacity;
    uint16_t *record;
    network_channel *channel;
    int32_t bits_to_send;
    int32_t total_bits;
    uint32_t item_flag;

    dst = (uint32_t *)record_copy;
    for (i = 8; i != 0; i = i - 1) {
        *dst = *source;
        source = source + 1;
        dst = dst + 1;
    }
    if (record_copy[30] == 0xff) {
        record_copy[30] = 0;
    }

    capacity = 0x600;
    if ((char)data_packet_group_encode_packet_unresolved(encoded, &network_game_messages_group, record_copy, &capacity, 0x12, 1) == 0) {
        return 0;
    }

    record = network_message_block_build(network_challenge_packet_block, (uint32_t *)encoded, 3,
                                        (uint32_t)capacity);
    if (record == 0) {
        return 0;
    }

    channel = client->channel;
    bits_to_send = (int32_t)(*record >> 4) * 8;
    total_bits = bits_to_send + 1;
    if ((channel->flags & 1) == 0) {
        if ((int32_t)(channel->outgoing.stream.last_bit - channel->outgoing.stream.byte_cursor * 8 -
                      channel->outgoing.stream.bit_cursor) + 1 < total_bits) {
            if (network_channel_stream_flush(&channel->outgoing, channel, 1) == 0) {
                return 0;
            }
        }
        channel->send_budget = channel->send_budget + total_bits;
        item_flag = 0;
        halo::memory::bit_stream_write_bits_chunked(&channel->outgoing.stream, &item_flag, 1);
        channel->outgoing.empty = 0;
        halo::memory::bit_stream_write_bits_chunked(&channel->outgoing.stream, (const uint32_t *)record, bits_to_send);
        channel->outgoing.empty = 0;
    }
    return 1;
}

/**
 * out/phase4/networking_functions.md summary ("Creates the game-object datum for a
 * player once their connection has fully joined, marking it as the local player when
 * applicable"). `unaff_EDI` is a word-indexed client pointer (word 0x58a / byte 0xb14 is
 * &client->session, matching the other word-indexed functions in this cluster); word 0x76d
 * (byte 0xeda) is state; `in_EAX+0x1f` matches network_player_entry::slot_index.
 *
 * @address 0x4d9e30
 */
char ClientView::join_finalize(network_player_entry *entry)
{
    network_client_globals *client = self;
    char ok;
    network_player_entry *row;
    datum_index player_handle;

    ok = network_player_entry_validate(entry);
    if (ok == 0) {
        return 0;
    }

    ok = network_player_entry_add(entry, &client->session);
    if (ok != 0 && client->state == 3) {
        row = &client->session.players[(int8_t)entry->slot_index];
        ok = (char)network_channel_key_open(row);
        if (ok == 0) {
            return 0;
        }

        player_handle = (datum_index)player_data_iterator_advance((int16_t)row->slot_index);

        if ((int32_t)row->machine_index == (int32_t)*(uint16_t *)client) {
            game_set_local_player(player_handle, (int16_t)row->machine_player_index);
        }
        halo::memory::datum_new_at_index_with_salt(player_handle, update_client_queues);
        if (network_server != 0) {
            update_server_queue_create_entry(player_handle);
        }
    }
    return ok;
}

/**
 * types/networking.h "network_client_globals (0x4d8a80 network_session_create,
 * 0x4d8b70 destroy)" -- every DAT_ global in this function is network_client_storage
 * (0x00872de0) plus a fixed delta, matched field by field against that struct; the
 * out/phase4/networking_types_notes.md "network_client_globals (0xf4c)" paragraph confirms the
 * same deltas and confirms +0xb14 is the session network_game_session_reset receives.
 *
 * @address 0x4d8a80
 */
network_client_globals * ClientView::create()
{
    network_client_globals *client;
    player_update_history *history;
    int32_t *run;
    int32_t i;

    client = &network_client_storage;
    network_session_active = 1;
    message_delta_protocol_initialize();

    history = (player_update_history *)GlobalAlloc(0, 0x2c);
    client->update_history = history;
    history->next_update_id = 0;
    history->head = 0;
    history->tail = 0;
    for (i = 0; i < 8; i = i + 1) {
        history->statistics[i] = 0;
    }

    client->channel = network_channel_new(2);
    if (client->channel == 0) {
        network_session_destroy(client);
        client = 0;
    } else {
        network_game_session_reset(&client->session);
        client->flags = client->flags & 0xfff9;
        client->machine_index = 0xffff;
        client->state = 0;
        client->disconnect_reason = 0;
        client->unknown_ec8 = 0;
        client->last_update_id = 0;
        client->last_update_received_ms = 0;
        client->connection_stalled = 0;
        client->game_start_countdown_seconds = 0xffff;
        client->network_error_displayed = 0;

        run = (int32_t *)&client->timer;
        for (i = 12; i != 0; i = i - 1) {
            *run = 0;
            run = run + 1;
        }
        client->team_index = -1;
    }

    run = (int32_t *)((uint8_t *)client + 0xf14);
    for (i = 13; i != 0; i = i - 1) {
        *run = 0;
        run = run + 1;
    }
    network_stats_summary_log_open();
    return client;
}

/**
 * out/phase4/networking_functions.md summary ("Encodes and queues a session-info
 * packet for the connection, when its connection-mode field indicates that one is needed").
 *
 * @address 0x4d9050
 */
char ClientView::info_packet_send(const uint32_t *source)
{
    network_client_globals *client = self;
    char result;
    uint32_t local_buffer[8];
    uint16_t *challenge;
    network_channel *channel;
    int32_t bits_to_send;
    int32_t message_type;
    int32_t i;
    uint32_t item_flag;

    switch (client->state) {
    case 0:
    case 1:
        return 0;
    case 2:
        message_type = 0x10;
        break;
    case 3:
        message_type = 0x1d;
        break;
    case 4:
        message_type = 0x23;
        break;
    default:
        return 1;
    }

    for (i = 0; i < 8; i = i + 1) {
        local_buffer[i] = source[i];
    }
    challenge = network_prepare_challenge_packet(message_type, local_buffer);
    if (challenge == 0) {
        return 0;
    }

    channel = client->channel;
    bits_to_send = (int32_t)(*challenge >> 4) * 8;
    result = 1;
    if ((channel->flags & 1) == 0) {
        if (bits_to_send + 1 >
            (int32_t)(channel->outgoing.stream.last_bit - channel->outgoing.stream.byte_cursor * 8 -
                      channel->outgoing.stream.bit_cursor) + 1) {
            result = network_channel_stream_flush(&channel->outgoing, channel, 1);
            if (result == 0) {
                return 0;
            }
        }
        channel->send_budget = channel->send_budget + bits_to_send + 1;
        item_flag = 0;
        halo::memory::bit_stream_write_bits_chunked(&channel->outgoing.stream, &item_flag, 1);
        channel->outgoing.empty = 0;
        halo::memory::bit_stream_write_bits_chunked(&channel->outgoing.stream, (const uint32_t *)challenge, bits_to_send);
        channel->outgoing.empty = 0;
    }
    return result;
}

/**
 * out/phase4/networking_functions.md summary ("Records a newly-joined player's index
 * into the active session state and queues a notification packet announcing the join"). Shares
 * the challenge/send idiom (channel+0xa8c/+0x24/+0x1c/+0x20/+0xa80/+0x2c) with the other
 * functions in this cluster. `in_EAX+0x76d` (word index, byte 0xeda) is client->state;
 * `in_EAX+0x56e` (byte 0xadc) is client->channel; server+0x3ac and client+0xeb8 both land at
 * session-relative offset 0x3a4 (2 bytes into types/networking.h's opaque
 * network_game_session::unknown_3a2[10]), confirming both containers embed the same session.
 *
 * @address 0x4d9700
 */
void ClientView::player_join_notify(const uint32_t *source)
{
    network_client_globals *client = self;
    int16_t player_index;
    uint32_t group_value;
    int32_t *challenge;
    uint32_t challenge_payload[4];

    network_channel *channel;
    int32_t bits_to_send;
    int32_t total_bits;
    char retransmit_ok;

    player_index = *(int16_t *)(source + 1);
    if (player_index < 0 || player_index >= 0x10) {
        return;
    }
    client->machine_index = player_index;
    group_value = *source;
    client->state = 2;

    if (network_server != 0) {
        *(uint32_t *)&network_server->session.unknown_3a2[2] = group_value;
    }
    if (network_client != 0) {
        *(uint32_t *)&network_client->session.unknown_3a2[2] = group_value;
    }

    challenge = (int32_t *)network_prepare_challenge_packet(0x11, challenge_payload);
    if (challenge != 0) {
        channel = client->channel;
        bits_to_send = (uint32_t)(*(uint16_t *)challenge >> 4) * 8;
        total_bits = bits_to_send + 1;
        if ((channel->flags & 1) == 0) {
            if (total_bits <= ((*(int32_t *)&((network_channel *)channel)->outgoing.stream.last_bit +
                                 *(int32_t *)&((network_channel *)channel)->outgoing.stream.byte_cursor * -8) -
                                *(int32_t *)&((network_channel *)channel)->outgoing.stream.bit_cursor) + 1 ||
                (retransmit_ok = network_channel_stream_flush(&channel->outgoing, channel, 1), retransmit_ok != 0)) {

                channel->send_budget = channel->send_budget + total_bits;
                { uint32_t item_flag = 0; halo::memory::bit_stream_write_bits_chunked((bit_stream *)((uint8_t *)channel + 0x10), &item_flag, 1); }
                *((uint8_t *)channel + 0x2c) = 0;
                halo::memory::bit_stream_write_bits_chunked((bit_stream *)((uint8_t *)channel + 0x10), (const uint32_t *)(challenge), bits_to_send);
                *((uint8_t *)channel + 0x2c) = 0;
            }
        }
    }
}

/**
 * FIXED (objdump): every ret sets only AL; the upper bits of EAX are left as they were
 *
 * @address 0x4d9190
 */
uint8_t ClientView::player_table_index_apply(int32_t table_index, const uint8_t *candidate)
{
    network_client_globals *client = self;
    int32_t i;
    network_player_entry *entry;
    int32_t player_slot;
    uint8_t *player_base;

    i = 0;
    entry = &client->session.players[0];
    while (1) {
        if (network_player_entry_validate(entry) != 0 && entry->machine_index == (int8_t)candidate[0x1c] &&
            entry->machine_player_index == (int8_t)candidate[0x1d]) {
            break;
        }
        i = i + 1;
        entry = &client->session.players[i];
        if (i > 15) {
            return 0;
        }
    }

    player_slot = player_data_iterator_advance((int8_t)client->session.players[i].slot_index);
    if (client->session.map_loaded != 0 && player_slot != 0 && (uint32_t)player_slot != 0xffffffff &&
        table_index != -1) {
        player_base = *(uint8_t **)((uint8_t *)player_data + 0x34);
        *(int32_t *)(player_base + ((uint32_t)player_slot & 0xffff) * 0x200 + 0xd0) = table_index;
    }
    return 1;
}

/**
 * out/phase4/networking_functions.md summary ("Commits a previously-staged outgoing
 * message to the send queue while the connection is in state 2; otherwise a no-op"). This
 * function has zero callers anywhere in the binary (out/functions.json callers=0); rewritten
 * anyway per the task instructions. Shares the challenge/send idiom with the rest of this
 * cluster (network_session_info_packet_send.c etc).
 *
 * @address 0x4da250
 */
int32_t ClientView::staged_message_commit(uint16_t message_value)
{
    network_client_globals *client = self;
    uint16_t *challenge;
    uint32_t payload;
    network_channel *channel;
    int32_t bits_to_send;
    uint32_t item_flag;

    if (client->state != 2) {
        return 1;
    }
    *(uint16_t *)&payload = message_value;
    challenge = network_prepare_challenge_packet(0x13, &payload);
    if (challenge != 0) {
        channel = client->channel;
        bits_to_send = (int32_t)(*challenge >> 4) * 8;
        if ((channel->flags & 1) == 0) {
            if ((int32_t)(channel->outgoing.stream.last_bit - channel->outgoing.stream.byte_cursor * 8 -
                          channel->outgoing.stream.bit_cursor) + 1 < bits_to_send + 1) {
                if (network_channel_stream_flush(&channel->outgoing, channel, 1) == 0) {
                    return 1;
                }
            }
            channel->send_budget = channel->send_budget + bits_to_send + 1;
            item_flag = 0;
            halo::memory::bit_stream_write_bits_chunked(&channel->outgoing.stream, &item_flag, 1);
            channel->outgoing.empty = 0;
            halo::memory::bit_stream_write_bits_chunked(&channel->outgoing.stream, (const uint32_t *)challenge, bits_to_send);
            channel->outgoing.empty = 0;
        }
    }
    return 1;
}

/**
 * out/phase4/networking_functions.md summary ("Sets a connection object's remote
 * endpoint address fields and (re)allocates its per-endpoint control block").
 *
 * @address 0x4d8c50
 */
int32_t ConnectionView::endpoint_set(const uint32_t *source)
{
    network_client_globals *connection = self;
    network_connection_endpoint *endpoint;
    void *control_block;
    uint32_t *raw;
    int32_t i;

    endpoint = &connection->connection;
    raw = (uint32_t *)endpoint;
    endpoint->ready = 0;
    if (endpoint->control_block != 0) {
        GlobalFree(endpoint->control_block);
        endpoint->control_block = 0;
    }
    for (i = 0; i < 6; i = i + 1) {
        raw[i] = 0;
    }
    endpoint->last_send_ms = 0;
    endpoint->message_count = 0;
    endpoint->retry_count = 0;
    endpoint->current_ping_ms = 0;
    endpoint->ready = 0;
    endpoint->unknown_23 = 0;
    endpoint->control_block = 0;
    for (i = 0; i < 6; i = i + 1) {
        raw[i] = source[i];
    }
    endpoint->ready = 1;
    control_block = GlobalAlloc(0, 0x264);
    ((uint32_t *)control_block)[1] = 0;
    ((uint32_t *)control_block)[0] = 0;
    endpoint->control_block = control_block;
    return 1;
}

/**
 * out/phase4/networking_functions.md summary ("Begins establishing a connection
 * object to a given address/session, substituting the loopback address 127.0.0.1 when the
 * target turns out to be the local machine"); connection+0xec4 matches types/networking.h's
 * network_client_globals::unknown_ec4 exactly; connection+0xadc matches ::channel; the writes
 * to connection+0xac4/0xac6 as two 16-bit halves of a dword whose low half is set to the literal
 * 4 (k_network_address_size_ipv4) confirm connection+0xab4 is an s_network_address, which
 * upgrades the network_connection_endpoint struct first introduced in
 *
 * @address 0x4d8cf0
 */
int32_t ConnectionView::initiate(const uint32_t *target, const uint32_t *session_info)
{
    network_client_globals *connection = self;
    network_connection_attempt_state *attempt;
    network_connection_endpoint *endpoint;
    large_integer counter;
    int32_t started_ms;
    int32_t is_local_connection;
    int16_t registration_result;
    uint32_t loopback_a, loopback_b, loopback_c, loopback_d;
    uint16_t port;
    int32_t i;

    connection->unknown_ec4 = 1;
    attempt = &connection->connect_attempt;
    attempt->unknown_00 = 0;

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    started_ms = (int32_t)((counter.quad_part * 1000) / halo::cseries::globals().performance_frequency);

    attempt->elapsed_counter = 0;
    attempt->loading_started = 0;
    attempt->started_ms = started_ms;
    for (i = 0; i < 9; i = i + 1) {
        attempt->session_info[i] = session_info[i];
    }

    endpoint = &connection->connection;
    is_local_connection = connection->channel->endpoint != 0;
    if (is_local_connection && connection->channel->endpoint != 0) {
        network_address_to_string(&endpoint->address);
        registration_result = network_channel_attempt_connect(0x96640, 1);
        if (registration_result == 0) {
            goto local_machine_check_done;
        }
        is_local_connection = 0;
    } else {
local_machine_check_done:
        if (is_local_connection) {
            connection->state = 1;
            goto rebuild_endpoint;
        }
    }
    if (network_join_error_code == -1) {
        network_join_error_code = 7;
    }
rebuild_endpoint:
    endpoint->address.ipv4 = 0;
    endpoint->address.ipv6_1 = 0;
    endpoint->address.ipv6_2 = 0;
    endpoint->address.ipv6_3 = 0;
    *(uint32_t *)&endpoint->address.size = 0;
    endpoint->unknown_14 = 0;
    endpoint->last_send_ms = 0;
    endpoint->message_count = 0;
    endpoint->retry_count = 0;
    endpoint->current_ping_ms = 0;
    endpoint->ready = 0;
    endpoint->unknown_23 = 0;
    endpoint->control_block = 0;

    if ((int16_t)target[4] == 0) {

        loopback_a = inet_addr("127.0.0.1");
        loopback_b = inet_addr("127.0.0.1");
        loopback_c = inet_addr("127.0.0.1");
        loopback_d = inet_addr("127.0.0.1");
        port = (uint16_t)network_game_socket_port;
        endpoint->address.ipv4 = (loopback_a & 0xff0000 | loopback_b >> 0x10) >> 8 |
                                 (loopback_c << 0x10 | loopback_d & 0xff00) << 8;
        endpoint->address.size = k_network_address_size_ipv4;
        endpoint->address.port = port;
        network_connection_endpoint_set((const uint32_t *)endpoint, connection);
        return is_local_connection;
    }

    endpoint->address.ipv4 = target[0];
    endpoint->address.ipv6_1 = target[1];
    endpoint->address.ipv6_2 = target[2];
    endpoint->address.ipv6_3 = target[3];
    *(uint32_t *)&endpoint->address.size = target[4];
    endpoint->unknown_14 = target[5];
    network_connection_endpoint_set(target, connection);
    return is_local_connection;
}

/**
 * out/phase4/networking_functions.md summary ("Checks whether a connection's last
 * outgoing data has gone unacknowledged past its deadline and, if so, resends it and bumps the
 * retry counter"). client+0xad6/+0xab4/+0xad8 match network_client_globals::connection
 * (a network_connection_endpoint in types/networking.h since the review pass)
 * (ready flag, address.ipv4, control_block); this function additionally shows +0xad2 as a live
 * int16 retry counter (inside what those files call the raw `unknown_1c` dword, at its upper
 * half) and +0xad4 as a live int16 (their `unknown_20[2]`, matching that field's size exactly).
 *
 * @address 0x4d93b0
 */
void ConnectionView::retransmit_if_overdue(const uint32_t *sender_address, uint32_t deadline_ms, int32_t remote_time)
{
    network_client_globals *client = self;
    network_connection_endpoint *endpoint;
    uint32_t now;
    int16_t doubled;

    endpoint = &client->connection;
    if (endpoint->ready != 0 && endpoint->address.ipv4 == *sender_address) {
        now = halo::cseries::time_query_performance_counter_ms();
        if (deadline_ms <= now) {
            endpoint->retry_count = endpoint->retry_count + 1;

            message_delta_sample_record_and_append((int32_t)deadline_ms, (int32_t)now, remote_time,
                                                   (message_delta_sample_ring_buffer *)endpoint->control_block);
            doubled = (int16_t)message_delta_sample_ring_buffer_average(
                (message_delta_sample_ring_buffer *)endpoint->control_block);
            endpoint->current_ping_ms = doubled << 1;
        }
    }
}

/**
 * out/phase4/networking_functions.md summary ("Periodically sends a keepalive packet
 * on an idle connection once more than three seconds have elapsed since the last send").
 * Confirms and refines network_client_globals::connection, shared with
 * network_connection_endpoint_set.c / network_connection_initiate.c /
 * network_connection_retransmit_if_overdue.c: what those files call the raw `unknown_18` dword
 * (0xacc) is a live millisecond timestamp here, and what network_connection_retransmit_if_overdue.c
 * calls `unknown_1c_lo` (0xad0) is a live int16 counter this function increments.
 *
 * @address 0x4d9400
 */
void ConnectionView::send_keepalive()
{
    network_client_globals *client = self;
    large_integer counter;
    uint32_t now_ms;
    network_connection_endpoint *endpoint;
    int32_t *challenge;
    uint32_t challenge_payload[4];

    uint8_t out_flag;

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    now_ms = (uint32_t)((counter.quad_part * 1000) / halo::cseries::globals().performance_frequency);

    endpoint = &client->connection;
    if (endpoint->ready == 1 && 3000 < (int32_t)(now_ms - endpoint->last_send_ms)) {

        challenge = (int32_t *)network_prepare_challenge_packet(1, challenge_payload);
        if (challenge != 0) {
            out_flag = 0;
            if ((client->channel->flags & 1) == 0) {
                network_channel_reliable_pool_store(client->channel, challenge, &out_flag, 0);
            }
            endpoint->message_count = endpoint->message_count + 1;
            endpoint->last_send_ms = now_ms;
        }
    }
}

/**
 * out/phase4/networking_functions.md summary ("Per-tick network update used while
 * acting as host/server: services the channel, drains incoming messages, updates per-machine
 * state, and sends a periodic ping"). Shares the channel-flags bit4 ("not dead") test with
 * network_host_update_tick.c.
 *
 * @address 0x4db100
 */
char HostClientView::channel_service_tick()
{
    network_client_globals *client = self;
    char ok;

    ok = network_channel_service(client->channel, 15000, 0);
    if (ok != 0) {
        if (network_game_process_incoming_messages(client)) {
            ok = network_client_identity_tick(client);
            if (ok != 0) {
                network_connection_send_keepalive(client);
                return ok;
            }
        }
    }
    if ((~(uint8_t)(client->channel->flags >> 4) & 1) != 0) {
        return 0;
    }
    if (network_join_error_code == -1) {
        network_join_error_code = 4;
    }
    return 0;
}

/**
 * out/phase4/networking_functions.md: "Per-tick update for a hosted game: broadcasts
 * presence, services the network channel, and drains incoming messages while the host's socket
 * is healthy." It is case 2 of network_client_state_dispatch (0x4d8bb0), which sits between the
 * two join-side states (0, 1) and the in-game states (3 = network_game_client_update, 4 =
 * network_host_channel_service_tick), and it is the only caller of
 * network_host_presence_broadcast_tick (0x4dadb0, the once-a-second LAN announcement), so this
 * is the state a host runs in while sitting in the pre-game lobby.
 *
 * @address 0x4daef0
 */
char HostClientView::lobby_tick()
{
    network_client_globals *client = self;
    network_channel *channel;

    channel = client->channel;
    if ((~(uint8_t)(channel->flags >> 4) & 1) != 0 &&
        (channel->flags & (k_network_channel_client | k_network_channel_transmit_pending)) != 0 &&
        channel->endpoint != 0 &&
        (channel->endpoint->flags & 1) != 0) {
        network_host_presence_broadcast_tick(client);
        if (network_channel_service(client->channel, 15000, 0) != 0 &&
            network_game_process_incoming_messages(client) != 0) {
            return 1;
        }
    }

    if ((~(uint8_t)(client->channel->flags >> 4) & 1) == 0 && network_join_error_code == -1) {
        network_join_error_code = 4;
    }
    return 0;
}

/**
 * out/phase4/networking_functions.md summary ("Periodically (every second) builds and
 * queues a short broadcast-style message carrying a global status string, consistent with a
 * hosted-game LAN presence announcement"). client+0xed4 matches types/networking.h's
 * network_client_globals::unknown_ed4 exactly.
 *
 * @address 0x4dadb0
 */
void HostClientView::presence_broadcast_tick()
{
    network_client_globals *client = self;
    large_integer counter;
    int32_t now_ms;
    uint8_t buffer[256];
    int32_t *challenge;
    uint8_t *channel;
    int32_t bits_to_send;
    char retransmit_ok;

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    now_ms = (int32_t)((counter.quad_part * 1000) / halo::cseries::globals().performance_frequency);

    if (client->last_presence_broadcast_ms + 1000 < now_ms) {
        client->last_presence_broadcast_ms = now_ms;
        if (cache_file_request_map_unresolved(1) != 0) {
            memset(buffer, 0, sizeof(buffer));
            strncpy((char *)buffer, network_build_string, 0x100);

            challenge = (int32_t *)network_prepare_challenge_packet(0x15, buffer);
            if (challenge != 0) {
                channel = (uint8_t *)client->channel;
                bits_to_send = (uint32_t)(*(uint16_t *)challenge >> 4) * 8;
                if ((*(uint8_t *)&((network_channel *)channel)->flags & 1) == 0) {
                    if ((((*(int32_t *)&((network_channel *)channel)->outgoing.stream.last_bit + *(int32_t *)&((network_channel *)channel)->outgoing.stream.byte_cursor * -8) -
                          *(int32_t *)&((network_channel *)channel)->outgoing.stream.bit_cursor) + 1 < bits_to_send + 1) &&
                        (retransmit_ok = network_channel_stream_flush((network_channel_stream *)(channel + 0x10), (network_channel *)channel, 1), retransmit_ok == 0)) {
                        return;
                    }
                    {

                        ((network_channel *)channel)->send_budget = ((network_channel *)channel)->send_budget + bits_to_send + 1;
                        { uint32_t item_flag = 0; halo::memory::bit_stream_write_bits_chunked((bit_stream *)((uint8_t *)channel + 0x10), &item_flag, 1); }
                        ((network_channel *)channel)->outgoing.empty = 0;
                        halo::memory::bit_stream_write_bits_chunked((bit_stream *)((uint8_t *)channel + 0x10), (const uint32_t *)(challenge), bits_to_send);
                        ((network_channel *)channel)->outgoing.empty = 0;
                    }
                }
            }
        }
    }
}

/**
 * "Connecting..." case), matching this call site's own "still waiting, tick the dots" context.
 *
 * @address 0x4dab80
 */
int32_t JoinView::connect_retry_tick()
{
    network_client_globals *client = self;
    large_integer counter;
    int32_t now_ms;
    network_channel *channel;
    network_receive_queue *endpoint;
    network_connection_attempt_state *attempt;
    int32_t ok;

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    now_ms = (int32_t)((counter.quad_part * 1000) / halo::cseries::globals().performance_frequency);

    channel = client->channel;
    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    channel->last_activity_ms = (int32_t)((counter.quad_part * 1000) / halo::cseries::globals().performance_frequency);

    endpoint = channel->endpoint;
    attempt = &client->connect_attempt;

    if ((channel->flags & 6) == 0 || endpoint == 0 || (endpoint->flags & 1) == 0) {
        if (endpoint != 0 && endpoint->last_error == -0x18) {
            if (network_join_error_code == -1) {
                network_join_error_code = network_signal_quality_glyph();
                return 0;
            }
        } else if (endpoint != 0 && -1 < (int8_t)endpoint->flags) {
            if (attempt->unknown_00 == 0) {
                if ((client->flags & 4) == 0) {
                    if (3000 < (uint32_t)((now_ms + (int32_t)attempt->elapsed_counter * -3000) - attempt->started_ms)) {
                        network_join_status_text_update(1, client);
                    }
                    goto service_channel;
                }
            } else {
                if ((uint32_t)(now_ms - attempt->started_ms) <= (uint32_t)network_connect_timeout_ms) {
                    goto service_channel;
                }
                attempt->unknown_00 = 0;
            }
            network_receive_queue_close_socket();
        }
        if (network_join_error_code == -1) {
            network_join_error_code = 3;
        }
        return 0;
    }

    attempt->unknown_00 = 0;
    if (attempt->loading_started == 0) {
        attempt->elapsed_counter = 0;
        console_printf_verbose("Loading");
        interface_loading_screen_progress = 0;
        if (network_game_mode == 2) {
            if (join_ui_state != 1) {
                if (join_ui_state != 2 && join_ui_state == 4) {
                    interface_loading_screen_request_id = -1;
                }
                join_ui_state = 8;
                attempt->loading_started = 1;
                goto service_channel;
            }
        } else if (join_ui_state != 1 && join_ui_state != 2) {
            if (join_ui_state == 4) {
                interface_loading_screen_request_id = -1;
                attempt->loading_started = 1;
                goto service_channel;
            }
            join_ui_state = 7;
        }
        attempt->loading_started = 1;
    }
service_channel:

    ok = network_channel_service(client->channel, 5000, 0);
    if (ok != 0) {
        return network_game_process_incoming_messages(client);
    }
    return 0;
}

/**
 * Original `network_join_handshake_tick`, moved unchanged; recovered notes are in docs/original/networking/net1_client.md.
 *
 * @address 0x4daa20
 */
uint32_t JoinView::handshake_tick()
{
    network_client_globals *client = self;

    uint8_t frame[400];
    network_channel *channel;
    large_integer counter;
    int32_t now_ms;
    uint32_t result;
    uint32_t *fill;
    int32_t i;
    int32_t loopback_ip;

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    channel = client->channel;
    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    now_ms = (int32_t)((counter.quad_part * 1000) / halo::cseries::globals().performance_frequency);
    channel->last_activity_ms = now_ms;

    result = 1;
    if (network_server == 0) {

        result = (uint32_t)network_channel_service(client->channel, 5000, 0);
        if ((char)result == 0) {
            goto retry_limit_check;
        }
        result = network_game_process_incoming_messages(client);
        if (result) {
            network_connection_send_keepalive(client);
        }
    } else if (network_server->state == 1) {
        *(int32_t *)(frame + 72) = 0;
        *(int32_t *)(frame + 76) = 0;
        *(int32_t *)(frame + 80) = 0;
        *(int32_t *)(frame + 84) = 0;
        fill = (uint32_t *)(frame + 88);
        for (i = 0x48; i != 0; i = i - 1) {
            *fill = 0;
            fill = fill + 1;
        }

        loopback_ip = network_local_address;
        if (network_local_address == 0) {
            loopback_ip = 0x7f000001;
        }
        *(int32_t *)(frame + 12) = loopback_ip;
        *(int16_t *)(frame + 28) = 4;
        *(int16_t *)(frame + 30) = (int16_t)network_game_socket_port;
        *(int16_t *)(frame + 370) = 1;
        wcsncpy((wchar_t *)(frame + 38), (const wchar_t *)network_server->password, 8);
        *(int16_t *)(frame + 54) = 0;

        network_debug_fill_canary_buffer();

        result = (uint32_t)network_connection_initiate(client, (const uint32_t *)(frame + 72),
                                                         (const uint32_t *)(frame + 36));
        if ((char)result == 0) {
            goto retry_limit_check;
        }
    }
    return result;

retry_limit_check:
    if (network_join_error_code != -1) {
        return result;
    }
    network_join_error_code = 7;
    return result;
}

/**
 * out/phase2/results/networking_01.json / symbols/review_queue.txt)
 * address 0x4ba320, size 819 bytes
 * name confidence: 0.45   rewrite confidence: 0.35
 * out/phase4/networking_functions.md summary ("Resolves the host/IP for a pending
 * 'join server' request and either connects immediately or kicks off an asynchronous hostname
 * resolution"); out/phase2/results/networking_01.json evidence ("reads a pending connect
 * request (server_browser_join_target), retrieves hostname/IP fields via SBServerGetPublicAddress/00617640/00617650,
 *
 * @address 0x4ba320
 */
uint32_t JoinView::request_resolve_host()
{
    char dead_scratch[28];
    char host_buffer[0x18 + 1];
    char connect_string[64];
    uint8_t needs_async_resolve = 0;
    char *string_result;
    uint32_t port = 0;

    string_result = SBServerGetPublicAddress((int32_t)(uintptr_t)server_browser_join_target);
    if (string_result != 0) {
        strcpy(dead_scratch, string_result);
    }
    SBServerGetPrivateQueryPort((int32_t)(uintptr_t)server_browser_join_target);
    string_result = SBServerGetPrivateAddress((int32_t)(uintptr_t)server_browser_join_target);
    if (string_result != 0) {
        strcpy(dead_scratch, string_result);
    }
    string_result = ServerBrowserGetMyPublicIP(master_server_query_engine);
    if (string_result != 0) {
        strcpy(dead_scratch, string_result);
    }

    {
        uint32_t byte_a; int32_t byte_b; uint32_t byte_c;
        int32_t byte_d; uint32_t byte_e; uint32_t byte_f;
        int32_t has_resolved_address;

        port = SBServerGetPublicQueryPort((int32_t)(uintptr_t)server_browser_join_target);
        byte_a = gamespy_array_length((int32_t)(uintptr_t)server_browser_join_target);
        gamespy_array_length((int32_t)(uintptr_t)server_browser_join_target);
        byte_b = gamespy_array_length((int32_t)(uintptr_t)server_browser_join_target);
        byte_c = gamespy_array_length((int32_t)(uintptr_t)server_browser_join_target);
        ServerBrowserGetMyPublicIPAddr(master_server_query_engine);
        byte_d = ServerBrowserGetMyPublicIPAddr(master_server_query_engine);
        byte_e = ServerBrowserGetMyPublicIPAddr(master_server_query_engine);
        byte_f = ServerBrowserGetMyPublicIPAddr(master_server_query_engine);
        has_resolved_address = SBServerHasPrivateAddress((int32_t)(uintptr_t)server_browser_join_target);

        if (has_resolved_address == 0 ||
            (((byte_a & 0xff0000) >> 8 | (uint32_t)((byte_b << 0x10) | (byte_c & 0xff00)) << 8) !=
             ((uint32_t)((byte_d << 0x10) | (byte_e & 0xff00)) << 8 | ((byte_f >> 8) & 0xff00)))) {
            if (SBServerDirectConnect((int32_t)(uintptr_t)server_browser_join_target) == 0) {
                needs_async_resolve = 1;
            } else {
                strncpy(host_buffer, SBServerGetPublicAddress((int32_t)(uintptr_t)server_browser_join_target), 0x18);
                host_buffer[0x18] = 0;
            }
        } else {
            strncpy(host_buffer, SBServerGetPrivateAddress((int32_t)(uintptr_t)server_browser_join_target), 0x18);
            host_buffer[0x18] = 0;
            port = SBServerGetPrivateQueryPort((int32_t)(uintptr_t)server_browser_join_target);
        }
    }

    network_channels_open();
    if (network_channels_open_ok == 0) {
        server_browser_join_target = 0;
        return 0;
    }

    if (!needs_async_resolve) {
        interface_loading_screen_address_a = -1;
        interface_loading_screen_address_b = -1;
        interface_loading_screen_request_id = -1;
        join_ui_state = 0;
        interface_loading_screen_progress = 0;
        progress_screen_text = 0;
        progress_screen_subtext = 0;

        sprintf(connect_string, "%s:%d", host_buffer, port & 0xffff);
        {
            uint32_t result = network_game_client_connect_to_address(connect_string,
                                                                      network_join_target_address);
            network_join_target_address[0] = 0;
            server_browser_join_target_has_password = 0;
            server_browser_join_target = 0;
            return result;
        }
    }

    {
        uint32_t resolve_handle = gamespy_array_length(network_game_socket);

        char *hostname = SBServerGetPublicAddress((int32_t)(uintptr_t)server_browser_join_target);
        uint32_t resolve_port = SBServerGetPublicQueryPort((int32_t)(uintptr_t)server_browser_join_target);
        int32_t request_id = network_random_offset(10000);
        uint32_t result;

        ServerBrowserSendNatNegotiateCookieToServer(master_server_query_engine, hostname, resolve_port, request_id);
        result = NNBeginNegotiationWithSocket((int32_t)resolve_handle, request_id, 1, halo::cseries::function_do_nothing,
            network_join_hostname_resolved_callback, 0);
        server_browser_join_target = 0;
        if (result == 0) {
            interface_loading_screen_request_id = request_id;
            interface_loading_screen_address_a = -1;
            interface_loading_screen_address_b = -1;
            interface_loading_screen_progress = 0;
            progress_screen_text = 0;
            progress_screen_subtext = 0;
            join_ui_state = 4;
            return 0xffffff01;
        }
        return result & 0xffffff00;
    }
}

}
