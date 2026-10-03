#include "halo/networking/net1_client.hpp"
#include "halo/networking/channel_queue.hpp"
#include "halo/networking/game_mode.hpp"
#include "halo/core/datum.hpp"
#include "halo/memory/api.hpp"
#include "halo/cseries/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/game/records.hpp"
#include "halo/interface/api.hpp"
#include "halo/core/link.hpp"
#include "halo/game/vars.hpp"
#include "halo/main/vars.hpp"
#include "halo/networking/vars.hpp"
#include "halo/main/api.hpp"

static auto &network_game_messages_group = halo::link::ref<data_packet_group>(halo::networking::vars().network_game_messages_group);
static auto &network_statistics_logging_enabled = halo::link::ref<uint8_t>(halo::networking::vars().network_statistics_logging_enabled);
static auto &network_summary_log_file = halo::link::ref<void *>(halo::networking::vars().network_summary_log_file);
static auto &network_build_string = halo::link::ref<char []>(halo::networking::vars().network_build_string);
static auto &network_game_mode = halo::link::ref<int16_t>(halo::networking::vars().network_game_mode);
static auto &player_data = halo::link::ref<data_array *>(halo::game::vars().player_data);
static auto &local_player_globals = halo::link::ref<player_globals *>(halo::game::vars().local_player_globals);
static auto &network_challenge_packet_block = halo::link::ref<uint16_t>(halo::networking::vars().network_challenge_packet_block);
static auto &network_broadcast_body = halo::link::ref<uint32_t []>(halo::networking::vars().network_broadcast_body);
static auto &network_server = halo::link::ref<network_server_globals *>(halo::networking::vars().network_server);
static auto &join_ui_state = halo::link::ref<int32_t>(halo::networking::vars().join_ui_state);
static auto &interface_loading_screen_address_b = halo::link::ref<int32_t>(halo::main::vars().interface_loading_screen_address_b);
static auto &interface_loading_screen_address_a = halo::link::ref<int32_t>(halo::main::vars().interface_loading_screen_address_a);

namespace halo::networking {

/**
 * Finalizes a connection's transition into the joined state: stamps the channel, loads the scenario (as a client)
 * or marks the map loaded (as the host), binds the local players of this machine's player entries to their player
 * records, then queues the final join packet and resets the update bookkeeping. Returns whether the client reached
 * state 3.
 *
 * @address 0x4d9960
 */
int32_t ConnectionView::finalize_join(uint16_t *connection)
{
    network_client_globals *client = (network_client_globals *)connection;
    large_integer counter;
    int32_t now_ms;
    uint8_t encode_buffer[1540];
    int16_t capacity;
    uint32_t payload;
    char ok;

    if (halo::cseries::globals().debug_log_level > 2 && network_statistics_logging_enabled != 0 &&
        network_summary_log_file != 0) {
        fprintf((FILE *)network_summary_log_file, "%s\t", network_build_string);
    }

    client->game_start_countdown_seconds = halo::k_word_none;

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    now_ms = (int32_t)((counter.quad_part * 1000) / halo::cseries::globals().performance_frequency);
    client->channel->last_activity_ms = now_ms;

    if (network_game_mode == halo::networking::k_game_mode_host) {
        client->session.map_loaded = 1;
    } else {
        ok = halo::networking::network_game_scenario_load_request(&client->session);
        if (ok != 1) {
            return client->state == 3;
        }
    }

    {
        const uint32_t machine_index = client->machine_index;
        network_player_entry *entry = client->session.players;
        int32_t i;

        for (i = 0; i < 16; i = i + 1, entry = entry + 1) {
            if ((int32_t)entry->machine_index == machine_index) {
                break;
            }
        }
        if (i < 16 && halo::networking::network_player_entry_validate(entry) != 0) {
            for (;;) {
                uint32_t player_handle = (uint32_t)halo::networking::player_data_iterator_advance(entry->slot_index);
                int16_t local_index = (int16_t)entry->machine_player_index;

                if (-1 < entry->machine_player_index && local_index < 1) {
                    datum_index &local_player = halo::game::globals().local_player_globals->local_players[local_index];

                    if (local_player != halo::k_dword_none) {
                        halo::game::player_at(local_player)->local_player_index = -1;
                    }
                    local_player = player_handle;
                    if (player_handle != halo::k_dword_none) {
                        halo::game::player_at(player_handle)->local_player_index = local_index;
                    }
                }
                entry = entry + 1;
                if ((int32_t)entry->machine_index != machine_index ||
                    halo::networking::network_player_entry_validate(entry) == 0) {
                    break;
                }
            }
        }
    }

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    now_ms = (int32_t)((counter.quad_part * 1000) / halo::cseries::globals().performance_frequency);
    client->channel->last_activity_ms = now_ms;

    payload = 0;
    capacity = 0x600;
    ok = (char)halo::memory::data_packet_group_encode_packet(&network_game_messages_group, encode_buffer, &payload, &capacity, 0x1a, 1);
    if (ok != 0) {
        network_channel *channel = client->channel;

        network_challenge_packet_block = (((int16_t)capacity + 2) * 0x10) | 0xc;
        memcpy(network_broadcast_body, encode_buffer, (uint16_t)capacity);

        if (!halo::networking::channel_queue_packet(channel, &network_challenge_packet_block)) {
            return client->state == 3;
        }

        client->state = 3;
        client->last_update_id = 0;
        client->last_update_received_ms = 0;
        client->connection_stalled = 0;
        halo::interface::widget_close_all();
        halo::game::game_engine_init_tick_record_for_mode();
        halo::game::game_engine_reset_all_players();
        if (network_game_mode == halo::networking::k_game_mode_host && ((network_server->flags >> 2 & 1) == 0)) {
            halo::networking::network_host_full_state_broadcast(network_server);
        }
        if (join_ui_state != 0) {
            int32_t now2 = halo::cseries::time_query_performance_counter_ms();
            uint32_t delay = 0;

            if (interface_loading_screen_address_b != -1 &&
                (uint32_t)(now2 - interface_loading_screen_address_b) < 2000 && join_ui_state != 1) {
                delay = (uint32_t)(interface_loading_screen_address_b - now2) + 2000;
                if (delay > 2000) {
                    delay = 2000;
                }
            }
            interface_loading_screen_address_a = delay + 0x6d6 + now2;
        }
    }
    return client->state == 3;
}


}
