#include "halo/game/lockstep.hpp"
#include "halo/networking/net1_client.hpp"
#include "halo/networking/delta_message_types.hpp"
#include "halo/game/records.hpp"
#include "halo/networking/channel_queue.hpp"
#include "halo/core/cstring.hpp"
#include "halo/networking/game_mode.hpp"
#include "halo/core/network_constants.hpp"
#include "halo/game/constants.hpp"
#include "halo/core/datum.hpp"
#include "interface.h"
#include "main.h"
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
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/interface/api.hpp"
#include "halo/core/link.hpp"
#include "halo/game/vars.hpp"
#include "halo/interface/vars.hpp"
#include "halo/main/vars.hpp"
#include "halo/networking/vars.hpp"
#include "../gamespy/gamespy_calls.hpp"
#include "saved_games.h"
#include "halo/saved_games/layout.hpp"
#include "halo/platform/time.hpp"
#include "halo/platform/memory.hpp"

static auto &network_message_scratch = halo::link::ref<uint8_t [0x7ff8]>(halo::game::vars().network_message_scratch);
static auto &network_client = halo::link::ref<network_client_globals *>(halo::networking::vars().network_client);
static auto &network_host_handoff_requested = halo::link::ref<uint8_t>(halo::networking::vars().network_host_handoff_requested);
static auto &network_game_mode = halo::link::ref<int16_t>(halo::networking::vars().network_game_mode);
static auto &profile_globals_block = halo::link::ref<uint8_t [0x1ffc]>(halo::ui::vars().profile_globals_block);
static auto &machine_to_player = halo::link::ref<datum_index [16]>(halo::game::vars().machine_to_player);
static auto &network_stats_enabled_gate = halo::link::ref<uint8_t>(halo::networking::vars().network_stats_enabled_gate);
static auto &main_globals_data = halo::link::ref<main_globals>(halo::main::vars().main_globals_data);
static auto &network_connect_timeout_ms = halo::link::ref<int32_t>(halo::networking::vars().network_connect_timeout_ms);
static auto &network_session_active = halo::link::ref<uint8_t>(halo::networking::vars().network_session_active);
static auto &network_log_path_format = halo::link::ref<char []>(halo::networking::vars().network_log_path_format);
static auto &global_white_argb = halo::link::ref<void *>(halo::networking::vars().global_white_argb);
static auto &network_server = halo::link::ref<network_server_globals *>(halo::networking::vars().network_server);
static auto &local_player_globals = halo::link::ref<player_globals *>(halo::game::vars().local_player_globals);
static auto &network_channel_table_default_flag = halo::link::ref<uint8_t>(halo::networking::vars().network_channel_table_default_flag);
static auto &network_client_vehicle_ack_enabled = halo::link::ref<uint8_t>(halo::networking::vars().network_client_vehicle_ack_enabled);
static auto &network_server_host_valid = halo::link::ref<uint8_t>(halo::networking::vars().network_server_host_valid);
static auto &split_screen_quit_prompt_string = halo::link::ref<uint32_t>(halo::ui::vars().split_screen_quit_prompt_string);
static auto &network_join_error_reason = halo::link::ref<uint32_t>(halo::networking::vars().network_join_error_reason);
static auto &network_disconnect_timeout_flag = halo::link::ref<uint8_t>(halo::networking::vars().network_disconnect_timeout_flag);
static auto &network_join_error_code = halo::link::ref<int16_t>(halo::networking::vars().network_join_error_code);
static auto &network_ping_debug_log_enabled = halo::link::ref<uint8_t>(halo::networking::vars().network_ping_debug_log_enabled);
static auto &network_ping_debug_last_sample = halo::link::ref<uint32_t>(halo::networking::vars().network_ping_debug_last_sample);
static auto &network_game_messages_group = halo::link::ref<data_packet_group>(halo::networking::vars().network_game_messages_group);
static auto &network_challenge_packet_block = halo::link::ref<uint16_t []>(halo::networking::vars().network_challenge_packet_block);
static auto &update_client_queues = halo::link::ref<data_array *>(halo::game::vars().update_client_queues);
static auto &network_client_storage = halo::link::ref<network_client_globals>(halo::networking::vars().network_client_storage);
static auto &network_game_socket_port = halo::link::ref<int32_t>(halo::networking::vars().network_game_socket_port);
static auto &network_build_string = halo::link::ref<char []>(halo::networking::vars().network_build_string);
static auto &interface_loading_screen_progress = halo::link::ref<int32_t>(halo::networking::vars().interface_loading_screen_progress);
static auto &join_ui_state = halo::link::ref<int32_t>(halo::networking::vars().join_ui_state);
static auto &interface_loading_screen_request_id = halo::link::ref<int32_t>(halo::networking::vars().interface_loading_screen_request_id);
static auto &network_local_address = halo::link::ref<uint32_t>(halo::networking::vars().network_local_address);
static auto &server_browser_join_target = halo::link::ref<void *>(halo::networking::vars().server_browser_join_target);
static auto &master_server_query_engine = halo::link::ref<void *>(halo::networking::vars().master_server_query_engine);
static auto &network_join_target_address = halo::link::ref<uint16_t [128]>(halo::networking::vars().network_join_target_address);
static auto &server_browser_join_target_has_password = halo::link::ref<uint8_t>(halo::networking::vars().server_browser_join_target_has_password);
static auto &network_game_socket = halo::link::ref<int32_t>(halo::networking::vars().network_game_socket);
static auto &network_channels_open_ok = halo::link::ref<uint8_t>(halo::networking::vars().network_channels_open_ok);
static auto &interface_loading_screen_address_a = halo::link::ref<int32_t>(halo::main::vars().interface_loading_screen_address_a);
static auto &interface_loading_screen_address_b = halo::link::ref<int32_t>(halo::main::vars().interface_loading_screen_address_b);
static auto &progress_screen_text = halo::link::ref<int32_t>(halo::main::vars().progress_screen_text);
static auto &progress_screen_subtext = halo::link::ref<int32_t>(halo::main::vars().progress_screen_subtext);

namespace {

/** The session info the loopback join hands to connection initiate: nine dwords holding the password and the canary text. */
struct handshake_session_info {
    uint16_t pad_00;
    uint16_t name[8];
    uint16_t name_terminator;
    uint32_t canary[4];
};
static_assert(sizeof(handshake_session_info) == 9 * sizeof(uint32_t), "session info is nine dwords");

/** The stack block join_handshake_tick builds for the loopback join: the host address, the session info and the target endpoint. */
struct handshake_frame {
    uint8_t unused_00[12];
    s_network_address connect_address;      // 0x0c
    uint8_t unused_20[4];                   // 0x20
    handshake_session_info session;         // 0x24
    s_network_address target;               // 0x48
    uint8_t target_tail[0x172 - 0x5c];      // 0x5c
    int16_t target_flag;                    // 0x172
    uint8_t target_tail2[0x178 - 0x174];    // 0x174
    uint8_t unused_178[400 - 0x178];        // 0x178
};
static_assert(offsetof(handshake_frame, connect_address) == 12 && offsetof(handshake_frame, session) == 36 &&
              offsetof(handshake_frame, session.canary) == 56 && offsetof(handshake_frame, target) == 72 &&
              offsetof(handshake_frame, target_flag) == 370 && sizeof(handshake_frame) == 400, "handshake frame layout");

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
        network_client = halo::networking::network_session_create();
        if (network_client != 0) {
            network_host_handoff_requested = 0;
        }
    }
    memcpy(scratch.config_template, profile_globals_block,
        0x7ff * 4);
    network_client->connection_rate_index = halo::saved_games::k_connection_type_locked
        ? halo::saved_games::k_connection_type_t1_lan
        : ((const saved_player_profile *)profile_globals_block)->connection_type;
    if (network_client->state == 0 && target_address->ipv4 != 0 && target_address->port != 0) {
        wcsncpy((wchar_t *)scratch.name, (const wchar_t *)player_name, 8);
        scratch.name_terminator = 0;
        halo::networking::network_debug_fill_canary_buffer(scratch.config_template);
        if (halo::networking::chimera__on_connect((const uint32_t *)target_address, network_client,
                (const uint32_t *)&scratch) != 0) {
            network_game_mode = halo::networking::k_game_mode_client;
            return 1;
        }
        network_host_handoff_requested = 1;
        halo::interface::chat_close();
        result = 0;
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
uint32_t ClientView::check_connection_quality(int16_t machine_id, client_update_record update)
{
    datum_index resolved;
    int16_t player_index;
    player *plr;
    int16_t salt;

    resolved = machine_to_player[(uint16_t)machine_id];
    if (resolved == (datum_index)halo::k_dword_none) {
        return 0;
    }
    player_index = (int16_t)resolved;
    if (player_index < 0 || player_index >= halo::game::globals().player_data->maximum_count) {
        return 0;
    }
    plr = halo::game::player_at(static_cast<uint32_t>(player_index));
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

        now_ms = (int32_t)(((int64_t)(((uint64_t)main_globals_data.frame_counter_high << 32) | main_globals_data.frame_counter_low) * 1000) / halo::cseries::globals().performance_frequency);
        added = update.tick_count;

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
    message_delta_decode_state state;
    uint8_t record[0x80];
    message_delta_context storage;
    void **context = halo::networking::raw_context(&storage);
    char result;

    result = (char)halo::networking::message_delta_decode_begin(&state, stream);
    if (result != 1) {
        return result;
    }
    memset(storage.changed, 0, sizeof(storage.changed));
    storage.state = &state;
    storage.target = record;
    state.changed = 0;
    state.more_items = 0;
    for (;;) {
        message_delta_decode_state *current;

        if ((char)halo::networking::message_delta_decode_array_field(context) == 0) {
            return 0;
        }
        current = storage.state;
        switch (current->message_type) {
        case 0x0d: halo::networking::network_game_client_apply_received_update(machine, (uint32_t)server, context); break;
        case 0x0f: halo::interface::chat_server_relay_incoming_message(context, machine); break;
        case 0x1a: halo::game::game_engine_update_lead_change_state(context, (uint8_t *)machine); break;
        case 0x34: halo::networking::network_game_message_handle_ping_timestamp((int32_t **)context, server); break;
        case 0x36: halo::networking::network_server_handle_rcon_request((network_player_entry *)machine, context); break;
        }
        result = current->more_items == 1 && current->changed == 1;
        ++current->processed_count;
        memset(storage.changed, 0, sizeof(storage.changed));
        current->more_items = 0;
        current->changed = 0;
        if (result != 1) {
            return result;
        }
        if (storage.state->processed_count > storage.state->item_count) {
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
    network_client = halo::networking::network_session_create();
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
        halo::networking::message_delta_parameters_protocol_dump_to_config_file();
        halo::networking::player_update_history_destroy(network_client->update_history);
        network_client->update_history = 0;
        if (network_client->channel != 0) {
            halo::networking::network_channel_delete(network_client->channel);
        }
        network_session_active = 0;
        halo::networking::network_stats_summary_log_write();
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
    void **context = static_cast<void **>(message);

    if (halo::networking::delta_context(context)->state->incremental == 0) {
        memset(decode_buf, 0, sizeof(decode_buf));
        if (halo::networking::message_delta_decode_compound_field(context, decode_buf) != 0) {
            int32_t text_len = strlen(reinterpret_cast<const char *>(decode_buf));
            if (text_len != 0) {
                halo::interface::chimera__console_out((ColorARGB *)global_white_argb, network_log_path_format, decode_buf, text_len);
            }
        }
    } else {
        halo::networking::message_delta_decode_compound_field_staged(context);
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

    halo::platform::read_performance_counter(&counter);
    now_ms = (uint32_t)((counter.quad_part * 1000) / halo::cseries::globals().performance_frequency);

    if (client->timer.active != 0 && (uint32_t)client->timer.deadline_ms <= now_ms) {
        int32_t result = halo::networking::network_channel_service_close_if_disconnected(client->channel);
        client->timer.active = 0;
        client->timer.deadline_ms = 0;
        client->timer.triggered = 1;
        client->timer.retrigger_ms = client->timer.context + (int32_t)now_ms;
        return result;
    }

    if (client->timer.triggered != 0 && (uint32_t)client->timer.retrigger_ms <= now_ms &&
        (client->channel->endpoint->flags & 0x40) != 0) {
        memcpy(timer_tail, &client->server_address, sizeof(client->server_address));
        local_player_id = -1;

        wcsncpy(name, (const wchar_t *)((const uint8_t *)client->connect_attempt.session_info + 2), 8);

        if (network_server == 0) {
            if (halo::game::globals().local_player_globals->local_players[0] != k_datum_index_none) {
                void *player = halo::memory::datum_get(halo::game::globals().local_player_globals->local_players[0], halo::game::globals().player_data);
                if (player != 0) {
                    local_player_id = ((struct player *)player)->team;
                }
            }
        } else {
            local_player_id = client->team_index;
        }

        halo::networking::network_client_globals_dispose();
        if (network_server != 0) {
            halo::networking::network_client_globals_create();
            network_client->team_index = local_player_id;
            return 1;
        }

        network_channel_table_default_flag = 1;
        halo::networking::network_client_begin_connect(name, (s_network_address *)timer_tail);

        memset(&client->timer, 0, sizeof(client->timer));
        memset(&client->server_address, 0, sizeof(client->server_address));
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

    iter.data = halo::game::globals().player_data;
    iter.next_index = 0;
    iter.index = k_datum_index_none;
    iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;
    candidate = (player *)halo::memory::data_iterator_next(&iter);
    while (candidate != 0) {
        if (candidate->local_player_index == -1 && candidate->unit != k_datum_index_none) {
            if (halo::game::player_unit_has_parent(iter.index) == 0 || network_client_vehicle_ack_enabled == 0) {
                encoded_size = halo::networking::build_local_player_position_update(&out_changed, candidate);
            } else {
                encoded_size = halo::networking::build_local_player_vehicle_update(&out_changed, candidate);
            }
#if defined(__EMSCRIPTEN__)
            {  // web diagnostic: the host's ack of a remote player's control updates
                static uint32_t calls, built;
                calls++;
                built += 0 < encoded_size;
                if (calls % 150 == 1) {
                    fprintf(stderr, "web: host ack player machine=%d f4=%d baseline=%d last=%d size=%d built=%u/%u\n",
                        (int)candidate->machine_index, (int)candidate->unknown_f4, (int)candidate->baseline_update_id,
                        (int)candidate->last_update_id, encoded_size, built, calls);
                }
            }
#endif
            if (0 < encoded_size) {
                halo::networking::network_session_send_to_machine(*(int8_t *)&candidate->machine_index, network_server, 1, network_message_scratch, encoded_size, 0, 0, 0, 0);
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

    halo::platform::read_performance_counter(&counter);
    now_ms = (int32_t)((counter.quad_part * 1000) / halo::cseries::globals().performance_frequency);

    timer = &client->timer;
    timer->deadline_ms = now_ms + delay_ms;
    timer->context = context;
    timer->active = 1;
    timer->triggered = 0;
    timer->retrigger_ms = 0;

    halo::networking::network_channel_remote_address_or_default(client->channel, &client->server_address);
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
#if defined(__EMSCRIPTEN__)
        // Web diagnostic: which path ended the session (each one leaves its own join error code; -1 = a client path).
        fprintf(stderr, "web: leaving the session: join_error=%d hosting=%d client_disconnect_reason=%d\n", network_join_error_code,
            network_server != 0, network_client != 0 ? (int)network_client->disconnect_reason : -1);
#endif
        network_game_mode = halo::networking::k_game_mode_local;
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
        halo::networking::network_client_globals_dispose();
        if (network_server != 0) {
            halo::networking::network_game_server_host_dispose(network_server);
            network_server = 0;
            network_server_host_valid = 0;
        }
        split_screen_quit_prompt_string = (split_screen_quit_prompt_string & 0x00ff0000u) | 0x0100ffffu;
        network_join_error_reason = 0;
    } else {
        dispatch_result = halo::networking::network_client_state_dispatch(network_client);
        result = 0;
        if (dispatch_result != 0) {
            if (network_client->disconnect_reason == 0) {
                int16_t progress_out = 0;
                halo::networking::net_state::last_connect_progress_percent = halo::networking::network_client_connect_progress_percent(network_client, &progress_out);
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
    bool finished = false;

    channel = client->channel;
    flags = channel->flags;
    if ((~(uint8_t)(flags >> 4) & 1) != 0 && (flags & 6) != 0 &&
        channel->endpoint != 0 && (channel->endpoint->flags & 1) != 0) {
        if (network_server == 0 || network_disconnect_timeout_flag != 0) {
            if ((flags >> 5 & 1) != 0) {
                halo::interface::ui_network_wait_timeout_start();
            }
            client->connection_stalled = (uint8_t)(flags >> 5) & 1;
        }
        service_ok = halo::networking::network_channel_service_light(channel, 0x3a98, 0);
        if (network_game_mode == halo::networking::k_game_mode_host) {
            halo::networking::network_channel_record_timestamp(channel);
        }
        if (service_ok != 0) {
            result = 0;
            if (halo::networking::network_game_process_incoming_messages(client)) {
                result = halo::networking::network_channel_service_retransmit_only(channel);
            }
            finished = true;
        } else {
            channel = client->channel;
            if ((~(uint8_t)(channel->flags >> 4) & 1) != 0 && (channel->flags & 6) != 0 &&
                channel->endpoint != 0 && (channel->endpoint->flags & 1) != 0) {
                result = 0;
                finished = true;
            }
        }
    }
    if (!finished) {
        if (network_join_error_code == -1) {
            network_join_error_code = 4;
        }
        result = 0;
    }
    halo::networking::network_connection_send_keepalive(client);

    endpoint = &client->connection;
    if (network_ping_debug_log_enabled == 1 && (uint32_t)(uint16_t)endpoint->message_count != network_ping_debug_last_sample &&
        (uint16_t)endpoint->message_count % 10 == 0) {
        int32_t server_base_time;
        int32_t challenge_time;
        int32_t now2;

        network_ping_debug_last_sample = (uint16_t)endpoint->message_count;
        halo::main::console_print_error_va(0, "current ping time[%d]  samples received[%d]  samples sent[%d]\n",
            endpoint->current_ping_ms, endpoint->retry_count, (uint16_t)endpoint->message_count);
        server_base_time = static_cast<message_delta_sample_ring_buffer *>(endpoint->control_block)->cached_average;

        challenge_time = halo::networking::message_delta_sample_ring_buffer_average((message_delta_sample_ring_buffer *)endpoint->control_block);
        now2 = halo::cseries::time_query_performance_counter_ms();
        halo::main::console_print_error_va(0, "current time delta[%d]  latency[%d]  server time[%d]\n",
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
    int16_t capacity;
    uint16_t *record;

    memcpy(record_copy, source, sizeof(record_copy));
    if (record_copy[30] == 0xff) {
        record_copy[30] = 0;
    }

    capacity = 0x600;
    if (halo::memory::data_packet_group_encode_packet(&network_game_messages_group, encoded, record_copy, &capacity, 0x12, 1) == 0) {
        return 0;
    }

    record = halo::networking::network_message_block_build(network_challenge_packet_block, (uint32_t *)encoded, 3,
                                        (uint32_t)capacity);
    if (record == 0) {
        return 0;
    }

    return halo::networking::channel_queue_packet(client->channel, record) ? 1 : 0;
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

    ok = halo::networking::network_player_entry_validate(entry);
    if (ok == 0) {
        return 0;
    }

    ok = halo::networking::network_player_entry_add(&client->session, entry);
    if (ok != 0 && client->state == 3) {
        row = &client->session.players[(int8_t)entry->slot_index];
        ok = (char)halo::networking::network_channel_key_open(row);
        if (ok == 0) {
            return 0;
        }

        player_handle = (datum_index)halo::networking::player_data_iterator_advance((int16_t)row->slot_index);

        if ((int32_t)row->machine_index == (int32_t)client->machine_index) {
            halo::game::game_set_local_player(player_handle, (int16_t)row->machine_player_index);
        }
        halo::memory::datum_new_at_index_with_salt(player_handle, halo::game::globals().update_client_queues);
        if (network_server != 0) {
            halo::game::update_server_queue_create_entry(player_handle);
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
    halo::networking::message_delta_protocol_initialize();

    history = (player_update_history *)halo::platform::heap_allocate(0, 0x2c);
    client->update_history = history;
    history->next_update_id = 0;
    history->head = 0;
    history->tail = 0;
    for (i = 0; i < 8; i = i + 1) {
        history->statistics[i] = 0;
    }

    client->channel = halo::networking::network_channel_new(2);
    if (client->channel == 0) {
        halo::networking::network_session_destroy(client);
        client = 0;
    } else {
        halo::networking::network_game_session_reset(&client->session);
        client->flags = client->flags & 0xfff9;
        client->machine_index = halo::k_word_none;
        client->state = 0;
        client->disconnect_reason = 0;
        client->unknown_ec8 = 0;
        client->last_update_id = 0;
        client->last_update_received_ms = 0;
        client->connection_stalled = 0;
        client->game_start_countdown_seconds = halo::k_word_none;
        client->network_error_displayed = 0;

        memset(&client->timer, 0, offsetof(network_client_globals, last_update_sent) - offsetof(network_client_globals, timer));
        client->team_index = -1;
    }

    memset(&client->last_update_sent, 0, sizeof(client->last_update_sent));
    halo::networking::network_stats_summary_log_open();
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
    uint32_t local_buffer[8];
    uint16_t *challenge;
    int32_t message_type;
    int32_t i;

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
    challenge = halo::networking::network_prepare_challenge_packet(message_type, local_buffer);
    if (challenge == 0) {
        return 0;
    }

    return halo::networking::channel_queue_packet(client->channel, challenge) ? 1 : 0;
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
    uint16_t *challenge;
    uint32_t challenge_payload[4];

    network_channel *channel;
    int32_t bits_to_send;
    int32_t total_bits;
    char retransmit_ok;

    player_index = static_cast<int16_t>(source[1]);
    if (player_index < 0 || player_index >= 0x10) {
        return;
    }
    client->machine_index = player_index;
    group_value = *source;
    client->state = 2;

    if (network_server != 0) {
        network_server->session.salt = group_value;
    }
    if (network_client != 0) {
        network_client->session.salt = group_value;
    }

    challenge = halo::networking::network_prepare_challenge_packet(0x11, challenge_payload);
    if (challenge != 0) {
        halo::networking::channel_queue_packet(client->channel, challenge);
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
        if (halo::networking::network_player_entry_validate(entry) != 0 && entry->machine_index == (int8_t)candidate[0x1c] &&
            entry->machine_player_index == (int8_t)candidate[0x1d]) {
            break;
        }
        i = i + 1;
        entry = &client->session.players[i];
        if (i > 15) {
            return 0;
        }
    }

    player_slot = halo::networking::player_data_iterator_advance((int8_t)client->session.players[i].slot_index);
    if (client->session.map_loaded != 0 && player_slot != 0 && (uint32_t)player_slot != halo::k_dword_none &&
        table_index != -1) {
        halo::game::player_at(player_slot)->quit_tick = table_index;
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

    if (client->state != 2) {
        return 1;
    }
    payload = message_value;
    challenge = halo::networking::network_prepare_challenge_packet(0x13, &payload);
    if (challenge != 0) {
        halo::networking::channel_queue_packet(client->channel, challenge);
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
    endpoint->ready = 0;
    if (endpoint->control_block != 0) {
        halo::platform::heap_free(endpoint->control_block);
        endpoint->control_block = 0;
    }
    memset(endpoint, 0, 6 * sizeof(uint32_t));
    endpoint->last_send_ms = 0;
    endpoint->message_count = 0;
    endpoint->retry_count = 0;
    endpoint->current_ping_ms = 0;
    endpoint->ready = 0;
    endpoint->unknown_23 = 0;
    endpoint->control_block = 0;
    memcpy(endpoint, source, sizeof(s_network_address)); // original copies 0x18, reading a stack dword past the 0x14-byte address
    endpoint->ready = 1;
    control_block = halo::platform::heap_allocate(0, 0x264);
    memset(control_block, 0, 2 * sizeof(uint32_t));
    endpoint->control_block = control_block;
    return 1;
}

/**
 * Starts a connection to `target`: stamps the attempt (start time and the 9-dword `session_info`), opens the
 * transport connection to `connect_address` through the channel's endpoint and, when that succeeds, puts the
 * client in the connecting state. A failed attempt sets join error 7 unless one is already pending. The
 * connection endpoint is then rebuilt from `target` (its previous control block is dropped, not freed) and
 * armed through network_connection_endpoint_set. Returns 1 when the transport connection was opened.
 *
 * @address 0x4d8cf0
 */
uint8_t ConnectionView::initiate(const uint32_t *target, const uint32_t *session_info, const uint32_t *connect_address)
{
    network_client_globals *client = self;
    network_connection_attempt_state *attempt = &client->connect_attempt;
    network_receive_queue *endpoint = client->channel->endpoint;
    large_integer counter;
    uint8_t connected = endpoint != 0;
    int32_t i;

    client->unknown_ec4 = 1;
    attempt->unknown_00 = 0;

    halo::platform::read_performance_counter(&counter);
    attempt->started_ms = (int32_t)((counter.quad_part * 1000) / halo::cseries::globals().performance_frequency);
    attempt->elapsed_counter = 0;
    attempt->loading_started = 0;
    for (i = 0; i < 9; i = i + 1) {
        attempt->session_info[i] = session_info[i];
    }

    if (connected) {
        halo::networking::network_address_to_string((s_network_address *)connect_address);
        if (halo::networking::network_channel_attempt_connect((s_network_address *)connect_address, endpoint, 0x96640, 1) != 0) {
            connected = 0;
        }
    }
    if (connected) {
        client->state = k_network_client_state_connecting;
    } else if (network_join_error_code == -1) {
        network_join_error_code = 7;
    }

    memset(&client->connection, 0, sizeof(client->connection));
    halo::networking::network_connection_endpoint_set(target, client);
    return connected;
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

            halo::networking::message_delta_sample_record_and_append((int32_t)deadline_ms, (int32_t)now, remote_time,
                                                   (message_delta_sample_ring_buffer *)endpoint->control_block);
            doubled = (int16_t)halo::networking::message_delta_sample_ring_buffer_average(
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
    uint16_t *challenge;
    uint32_t challenge_payload[4];

    uint8_t out_flag;

    halo::platform::read_performance_counter(&counter);
    now_ms = (uint32_t)((counter.quad_part * 1000) / halo::cseries::globals().performance_frequency);

    endpoint = &client->connection;
#if defined(__EMSCRIPTEN__)
    {
        // Web diagnostic: whether this client's keepalive is due / queued (every 5 s).
        static uint32_t last_report;

        if (now_ms - last_report > 5000) {
            last_report = now_ms;
            fprintf(stderr, "web: keepalive ready=%d since_last=%d channel_flags=%x count=%d\n", endpoint->ready,
                (int)(now_ms - endpoint->last_send_ms), (unsigned)client->channel->flags, endpoint->message_count);
        }
    }
#endif
    if (endpoint->ready == 1 && 3000u < now_ms - (uint32_t)endpoint->last_send_ms) {  // unsigned, as 0x4d944f (jbe)

        challenge_payload[0] = now_ms;  // the original sends the send time (0x4d9460)
        challenge = halo::networking::network_prepare_challenge_packet(1, challenge_payload);
        if (challenge != 0) {
            out_flag = 0;
            if ((client->channel->flags & 1) == 0) {
                halo::networking::network_channel_reliable_pool_store(client->channel, (uint8_t *)challenge, &out_flag, 0, 1, (uint32_t)halo::networking::packet_block_bit_count(challenge));
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

    ok = halo::networking::network_channel_service(client->channel, 15000, 0);
    if (ok != 0) {
        if (halo::networking::network_game_process_incoming_messages(client)) {
            ok = halo::networking::network_client_identity_tick(client);
            if (ok != 0) {
                halo::networking::network_connection_send_keepalive(client);
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
        halo::networking::network_host_presence_broadcast_tick(client);
        if (halo::networking::network_channel_service(client->channel, 15000, 0) != 0 &&
            halo::networking::network_game_process_incoming_messages(client) != 0) {
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
    uint16_t *challenge;
    network_channel *channel;
    int32_t bits_to_send;
    char retransmit_ok;

    halo::platform::read_performance_counter(&counter);
    now_ms = (int32_t)((counter.quad_part * 1000) / halo::cseries::globals().performance_frequency);

    if (client->last_presence_broadcast_ms + 1000 < now_ms) {
        client->last_presence_broadcast_ms = now_ms;
        if (halo::cache::cache_file_request_map(main_globals_data.multiplayer_map_name, 1) != 0) {
            memset(buffer, 0, sizeof(buffer));
            strncpy((char *)buffer, network_build_string, 0x100);

            challenge = halo::networking::network_prepare_challenge_packet(0x15, buffer);
            if (challenge != 0) {
                halo::networking::channel_queue_packet(client->channel, challenge);
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
    auto service_channel = [client]() -> int32_t {
        int32_t ok = halo::networking::network_channel_service(client->channel, 5000, 0);

        if (ok != 0) {
            return halo::networking::network_game_process_incoming_messages(client);
        }
        return 0;
    };

    halo::platform::read_performance_counter(&counter);
    now_ms = (int32_t)((counter.quad_part * 1000) / halo::cseries::globals().performance_frequency);

    channel = client->channel;
    halo::platform::read_performance_counter(&counter);
    channel->last_activity_ms = (int32_t)((counter.quad_part * 1000) / halo::cseries::globals().performance_frequency);

    endpoint = channel->endpoint;
    attempt = &client->connect_attempt;

    if ((channel->flags & 6) == 0 || endpoint == 0 || (endpoint->flags & 1) == 0) {
        if (endpoint != 0 && endpoint->last_error == -0x18) {
            if (network_join_error_code == -1) {
                network_join_error_code = halo::networking::network_signal_quality_glyph(endpoint->reject_reason);
                return 0;
            }
        } else if (endpoint != 0 && -1 < (int8_t)endpoint->flags) {
            if (attempt->unknown_00 == 0) {
                if ((client->flags & 4) == 0) {
                    if (3000 < (uint32_t)((now_ms + (int32_t)attempt->elapsed_counter * -3000) - attempt->started_ms)) {
                        halo::networking::network_join_status_text_update(1, client);
                    }
                    return service_channel();
                }
            } else {
                if ((uint32_t)(now_ms - attempt->started_ms) <= (uint32_t)network_connect_timeout_ms) {
                    return service_channel();
                }
                attempt->unknown_00 = 0;
            }
            halo::networking::network_receive_queue_close_socket(endpoint);
        }
        if (network_join_error_code == -1) {
            network_join_error_code = 3;
        }
        return 0;
    }

    attempt->unknown_00 = 0;
    if (attempt->loading_started == 0) {
        attempt->elapsed_counter = 0;
        halo::interface::console_printf_verbose((ColorARGB *)0, halo::mutable_literal("Loading"));
        interface_loading_screen_progress = 0;
        if (network_game_mode == halo::networking::k_game_mode_host) {
            if (join_ui_state != 1) {
                if (join_ui_state != 2 && join_ui_state == 4) {
                    interface_loading_screen_request_id = -1;
                }
                join_ui_state = 8;
                attempt->loading_started = 1;
                return service_channel();
            }
        } else if (join_ui_state != 1 && join_ui_state != 2) {
            if (join_ui_state == 4) {
                interface_loading_screen_request_id = -1;
                attempt->loading_started = 1;
                return service_channel();
            }
            join_ui_state = 7;
        }
        attempt->loading_started = 1;
    }
    return service_channel();
}

/**
 *
 * @address 0x4daa20
 */
uint32_t JoinView::handshake_tick()
{
    network_client_globals *client = self;

    handshake_frame frame;
    network_channel *channel;
    large_integer counter;
    int32_t now_ms;
    uint32_t result;
    int32_t loopback_ip;
    bool failed = false;

    halo::platform::read_performance_counter(&counter);
    channel = client->channel;
    halo::platform::read_performance_counter(&counter);
    now_ms = (int32_t)((counter.quad_part * 1000) / halo::cseries::globals().performance_frequency);
    channel->last_activity_ms = now_ms;

    result = 1;
    if (network_server == 0) {

        result = (uint32_t)halo::networking::network_channel_service(client->channel, 5000, 0);
        if ((char)result == 0) {
            failed = true;
        } else {
            result = halo::networking::network_game_process_incoming_messages(client);
            if (result) {
                halo::networking::network_connection_send_keepalive(client);
            }
        }
    } else if (network_server->state == 1) {
        memset(&frame.target, 0, offsetof(handshake_frame, unused_178) - offsetof(handshake_frame, target));

        loopback_ip = network_local_address;
        if (network_local_address == 0) {
            loopback_ip = 0x7f000001;
        }
        frame.connect_address.ipv4 = loopback_ip;
        frame.connect_address.size = 4;
        frame.connect_address.port = (int16_t)network_game_socket_port;
        frame.target_flag = 1;
        wcsncpy(reinterpret_cast<wchar_t *>(frame.session.name), reinterpret_cast<const wchar_t *>(network_server->password), 8);
        frame.session.name_terminator = 0;

        halo::networking::network_debug_fill_canary_buffer(frame.session.canary);

        result = (uint32_t)halo::networking::network_connection_initiate(client, reinterpret_cast<const uint32_t *>(&frame.target),
                                                         reinterpret_cast<const uint32_t *>(&frame.session),
                                                         reinterpret_cast<const uint32_t *>(&frame.connect_address));
        if ((char)result == 0) {
            failed = true;
        }
    }
    if (failed && network_join_error_code == -1) {
        network_join_error_code = 7;
    }
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

    if (halo::game::lockstep::join_from_browser(server_browser_join_target)) {
        network_join_target_address[0] = 0;
        server_browser_join_target_has_password = 0;
        server_browser_join_target = 0;
        return 1;
    }
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
        byte_a = halo::networking::gamespy_array_length((void *)((int32_t)(uintptr_t)server_browser_join_target));
        halo::networking::gamespy_array_length((void *)((int32_t)(uintptr_t)server_browser_join_target));
        byte_b = halo::networking::gamespy_array_length((void *)((int32_t)(uintptr_t)server_browser_join_target));
        byte_c = halo::networking::gamespy_array_length((void *)((int32_t)(uintptr_t)server_browser_join_target));
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

    halo::networking::network_channels_open();
#if defined(__EMSCRIPTEN__)
    fprintf(stderr, "web: browser join host=%s port=%u async=%d channels=%d\n", host_buffer, port & 0xffff,
        needs_async_resolve, network_channels_open_ok);
#endif
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
            uint32_t result = halo::networking::network_game_client_connect_to_address((wchar_t *)network_join_target_address,
                                                                      connect_string);
            network_join_target_address[0] = 0;
            server_browser_join_target_has_password = 0;
            server_browser_join_target = 0;
            return result;
        }
    }

    {
        uint32_t resolve_handle = halo::networking::gamespy_array_length((void *)network_game_socket);

        char *hostname = SBServerGetPublicAddress((int32_t)(uintptr_t)server_browser_join_target);
        uint32_t resolve_port = SBServerGetPublicQueryPort((int32_t)(uintptr_t)server_browser_join_target);
        int32_t request_id = halo::networking::network_random_offset(10000);
        uint32_t result;

        ServerBrowserSendNatNegotiateCookieToServer(master_server_query_engine, hostname, resolve_port, request_id);
        result = NNBeginNegotiationWithSocket((int32_t)resolve_handle, request_id, 1, halo::cseries::function_do_nothing,
            halo::networking::network_join_hostname_resolved_callback, 0);
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
