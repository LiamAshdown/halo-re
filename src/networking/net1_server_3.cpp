#include "halo/networking/net1_server.hpp"
#include <string.h>
#include "halo/memory/api.hpp"
#include "halo/cseries/api.hpp"
#include "halo/networking/api.hpp"

extern "C" {
extern data_packet_group network_game_messages_group;
extern network_client_globals *network_client;
extern uint16_t network_challenge_packet_block;
extern uint8_t network_broadcast_body[1536];
}

namespace halo::networking {

/**
 * this module, 0x4e19c0
 *
 * @address 0x4e15a0
 */
uint8_t ServerView::heartbeat_tick()
{
    network_server_globals *server = self;
    large_integer counter;
    int32_t now_ms;
    uint8_t result;
    network_timer_pair *timer;
    uint8_t *base;

    base = (uint8_t *)server;
    timer = (network_timer_pair *)(base + 0x9c8);

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    now_ms = (int32_t)((counter.quad_part * 1000) / halo::cseries::globals().performance_frequency);
    result = 1;

    if (server->scenario_announced == 0) {
        int32_t i;

        for (i = 0; i < 16; i = i + 1) {
            network_machine *machine;

            machine = &server->machines[i];
            if (machine->channel != 0 && (machine->channel->flags & 0x10) == 0) {
                halo::networking::network_machine_timer_start(machine, 0);
            }
        }

        if (server->handshake_state == 1) {
            char have_players;
            char team_empty;
            char restarting;

            have_players = halo::networking::network_game_all_machines_have_player(server);
            team_empty = have_players != 0 ? halo::networking::network_game_any_team_empty(server) : 0;
            if (have_players == 0 || team_empty != 0) {
                restarting = 0;
                timer->remaining_ms = 0;
                timer->last_tick_ms = 0;
                *(int32_t *)(base + 0x9d0) = 0;
                server->handshake_state = 0;
            } else {
                restarting = 1;
                halo::networking::network_timer_advance(timer);
                if (timer->remaining_ms == 0 &&
                    halo::networking::network_server_any_machine_awaiting_flag(server) != 0 &&
                    server->handshake_blocked == 0) {
                    result = halo::networking::network_host_send_scenario_announcement(server);
                    goto scenario_check;
                }
                if ((uint32_t)(now_ms - static_cast<int32_t>(server->unknown_9d0)) < 0x3e9) {
                    goto scenario_check;
                }
            }
            server->handshake_flag = 0;
            if (restarting) {
                halo::networking::network_timer_advance(timer);
            }
            {
                void *packet;

                uint16_t countdown_seconds = restarting ? (uint16_t)(timer->remaining_ms / 1000) : 0xffff;

                packet = halo::networking::network_prepare_challenge_packet(9, &countdown_seconds);
                if (packet != 0 && halo::networking::network_session_broadcast_to_all(server, 0, packet, 1, 0, 1, 3) != 0) {
                    *(int32_t *)(base + 0x9d0) = now_ms;
                }
            }
        } else if (static_cast<int32_t>(server->last_challenge_sent_ms) + 5000 < now_ms) {
            void *packet;

            uint16_t empty_payload = 0;

            packet = halo::networking::network_prepare_challenge_packet(0xc, &empty_payload);
            halo::networking::network_session_broadcast_to_all(server, 0, packet, 1, 0, 1, 3);
            *(int32_t *)(base + 0x9bc) = now_ms;
        }
    } else if (static_cast<int32_t>(server->first_join_ms) != 0) {
        int32_t now2;

        now2 = halo::cseries::time_query_performance_counter_ms();
        if ((uint32_t)(now2 - static_cast<int32_t>(server->first_join_ms)) > 59999) {
            int32_t i;
            char has_client;

            for (i = 0; i < 16; i = i + 1) {
                uint16_t flags;

                flags = *(uint16_t *)((uint8_t *)&server->machines[i] + 0xe);
                if ((flags & 1) != 0 && (flags & 4) == 0) {
                    halo::networking::network_machine_timer_start(&server->machines[i], 0);
                }
            }
            has_client = (network_client != 0);
            server->state = 1;
            *(int32_t *)(base + 0x9c4) = 0;
            server->session.map_loaded = has_client ? *((uint8_t *)network_client + 0xec0) : 0;
        }
    }

scenario_check:
    if (server->new_server_pending == 1) {
        if (halo::networking::network_game_server_load_scenario() == 1) {
            server->state = 1;
        }
        server->new_server_pending = 0;
    }
    return result;
}

/**
 * this module, 0x4e19c0
 * Every ~5000ms, rebroadcasts the prepared challenge/info packet to the whole session and
 * updates the last-sent timestamp at server+0x9bc.
 *
 * @address 0x4e1450
 */
uint32_t ServerView::resend_challenge_periodic()
{
    network_server_globals *server = self;
    large_integer counter;
    uint32_t now_ms;
    uint32_t *last_sent;

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    now_ms = (uint32_t)((counter.quad_part * 1000) / halo::cseries::globals().performance_frequency);
    last_sent = (uint32_t *)((uint8_t *)server + 0x9bc);
    if (*last_sent + 5000u < now_ms) {
        void *packet;

        uint16_t empty_payload = 0;

        packet = halo::networking::network_prepare_challenge_packet(0xd, &empty_payload);
        halo::networking::network_session_broadcast_to_all(server, 0, packet, 1, 0, 1, 3);
        *last_sent = now_ms;
    }
    return 1;
}

/**
 * out/phase4/networking_functions.md: "The first time it runs for a round, sends a
 * type-0x21 packet to every channel entry flagged as needing a full state refresh, staggering
 * each send's embedded timestamp by 100ms." host->unknown_a0e (the "run once" latch, cleared
 * here) and host->game_over (+0xa0f, cleared here) match types/networking.h; the machines[]
 * iteration (stride 0x60, byte offset +0x3c6 == machines[0]+0xe == flags) matches
 * network_game_client_game_settings_updated.c's own machines[] loop over the same field.
 *
 * @address 0x4df510
 */
void HostServerView::full_state_broadcast()
{
    network_server_globals *host = self;
    int32_t timestamp;
    int32_t i;
    network_machine *machine;
    uint8_t encode_buffer[0x600];
    int16_t capacity;
    uint32_t payload[2];
    char encode_ok;
    uint32_t byte_count;

    if (host->full_state_broadcast_pending == 1) {
        host->full_state_broadcast_pending = 0;
        host->game_over = 0;
        timestamp = 1000;
        for (i = 0; i < 16; i++) {
            machine = &host->machines[i];
            if ((machine->flags & 2) != 0 || (machine->flags & 0x10) != 0) {
                payload[0] = 0;
                payload[1] = (uint32_t)timestamp;
                capacity = 0x600;
                encode_ok = (char)halo::memory::data_packet_group_encode_packet(&network_game_messages_group, encode_buffer, payload, &capacity, 0x21, 1);
                if (encode_ok != 0) {
                    network_challenge_packet_block = ((int16_t)capacity + 2) * 0x10 | 0xc;
                    memcpy(network_broadcast_body, encode_buffer, (uint32_t)capacity & 0xffff);
                    byte_count = (uint32_t)(network_challenge_packet_block >> 4) << 3;
                    if (halo::networking::network_session_send_to_machine(machine->machine_id, host, 0, &network_challenge_packet_block, byte_count, 1, 0, 0, 3) != 0) {
                        timestamp = timestamp + 100;
                    }
                }
            }
        }
    }
}

}
