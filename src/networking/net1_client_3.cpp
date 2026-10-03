#include "halo/networking/net1_client.hpp"
#include "halo/memory/api.hpp"
#include "halo/cseries/api.hpp"

extern "C" {
extern data_packet_group network_game_messages_group;
extern uint8_t network_statistics_logging_enabled;
extern void *network_summary_log_file;
extern char network_build_string[];
extern int16_t network_game_mode;
extern char network_game_scenario_load_request(network_game_session *session);
extern data_array *player_data;
extern player_globals *local_player_globals;
extern int32_t player_data_iterator_advance(int16_t step_count);
extern char network_player_entry_validate(void);
extern uint16_t network_challenge_packet_block;
extern uint32_t network_broadcast_body[];
extern char network_channel_stream_flush(network_channel_stream *stream, network_channel *channel, char mode);
extern void widget_close_all(void);
extern void game_engine_init_tick_record_for_mode(void);
extern void game_engine_reset_all_players(void);
extern network_server_globals *network_server;
extern void network_host_full_state_broadcast(network_server_globals *server);
extern int32_t join_ui_state;
extern int32_t interface_loading_screen_address_b;
extern int32_t interface_loading_screen_address_a;
}

namespace halo::networking {

/**
 * unresolved sub-regions; see UNSURE notes)
 * out/phase4/networking_functions.md summary ("Finalizes a connection's transition
 * into the joined/in-game state, sending the final join packet and arming post-join bookkeeping
 * timers"). `connection` is `ushort *` per Ghidra's own recovered signature; every offset below
 * is a WORD index doubled to a byte offset, matching this module's other word-indexed functions
 * (network_client_state_dispatch.c's client+0xeda, etc). connection+0x56e (byte 0xadc) is
 * channel; connection+0x58a (byte 0xb14) is &client->session; connection+0x760 (byte 0xec0) is
 *
 * @address 0x4d9960
 */
int32_t ConnectionView::finalize_join(uint16_t *connection)
{
    uint16_t *puVar7, *puVar1;
    uint32_t *puVar2;
    uint32_t uVar3, uVar8;
    char ok;
    int32_t iVar6, iVar12;
    int16_t sVar9;
    large_integer counter;
    int32_t now_ms;
    uint8_t encode_buffer[1540];
    int16_t capacity;
    uint32_t payload;
    int32_t i;

    if (halo::cseries::globals().debug_log_level > 2 && network_statistics_logging_enabled != 0 &&
        network_summary_log_file != 0) {
        fprintf((FILE *)network_summary_log_file, "%s\t", network_build_string);
    }

    iVar6 = *(int32_t *)((uint8_t *)connection + 0xadc);
    connection[0x76c] = 0xffff;

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    now_ms = (int32_t)((counter.quad_part * 1000) / halo::cseries::globals().performance_frequency);
    *(int32_t *)(iVar6 + 4) = now_ms;

    if (network_game_mode == 2) {
        *((uint8_t *)connection + 0xec0) = 1;
    } else {
        ok = network_game_scenario_load_request((network_game_session *)((uint8_t *)connection + 0xb14));
        if (ok != 1) {
            goto tail;
        }
    }

    iVar6 = 0;
    puVar7 = connection + 0x669;
    do {
        if ((int32_t)(int8_t)*puVar7 == (uint32_t)*connection) {
            puVar7 = connection + iVar6 * 0x10;
            iVar6 = (int32_t)(uint32_t)player_data;
            if ((int32_t)(int8_t)puVar7[0x669] == (uint32_t)*connection) {
                goto have_machine;
            }
            break;
        }
        iVar6 = iVar6 + 1;
        puVar7 = puVar7 + 0x10;
    } while (iVar6 < 0x10);
    goto after_search;

    while (1) {
        uVar8 = (uint32_t)player_data_iterator_advance((int8_t)*((uint8_t *)puVar7 + 0xcd5));
        sVar9 = (int16_t)(int8_t)*((uint8_t *)puVar7 + 0xcd3);
        if (-1 < (int8_t)*((uint8_t *)puVar7 + 0xcd3) && sVar9 < 1) {
            puVar2 = (uint32_t *)&local_player_globals->local_players[sVar9];
            uVar3 = *puVar2;
            if (uVar3 != 0xffffffff) {
                *(uint16_t *)((uVar3 & 0xffff) * 0x200 + 2 + *(int32_t *)((uint8_t *)player_data + 0x34)) = 0xffff;
                iVar6 = (int32_t)(uint32_t)player_data;
            }
            *puVar2 = uVar8;
            if (uVar8 != 0xffffffff) {
                *(int16_t *)((uVar8 & 0xffff) * 0x200 + 2 + *(int32_t *)((uint8_t *)player_data + 0x34)) = sVar9;
            }
        }
        puVar1 = puVar7 + 0x679;
        puVar7 = puVar7 + 0x10;
        if ((int32_t)(int8_t)*puVar1 != (uint32_t)*connection) {
            break;
        }
have_machine:
        ok = network_player_entry_validate();
        if (ok == 0) {
            break;
        }
    }

after_search:
    iVar6 = *(int32_t *)((uint8_t *)connection + 0xadc);
    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    now_ms = (int32_t)((counter.quad_part * 1000) / halo::cseries::globals().performance_frequency);
    *(int32_t *)(iVar6 + 4) = now_ms;

    payload = 0;
    capacity = 0x600;
    ok = (char)halo::memory::data_packet_group_encode_packet(&network_game_messages_group, encode_buffer, &payload, &capacity, 0x1a, 1);
    if (ok != 0) {
        uint32_t *src, *dst8;
        uint8_t *src_b, *dst_b;

        network_challenge_packet_block = (((int16_t)capacity + 2) * 0x10) | 0xc;
        src = (uint32_t *)encode_buffer;
        dst8 = network_broadcast_body;
        for (i = (capacity & 0xffff) >> 2; i != 0; i = i - 1) {
            *dst8 = *src;
            src = src + 1;
            dst8 = dst8 + 1;
        }
        src_b = (uint8_t *)src;
        dst_b = (uint8_t *)dst8;
        for (i = capacity & 3; i != 0; i = i - 1) {
            *dst_b = *src_b;
            src_b = src_b + 1;
            dst_b = dst_b + 1;
        }

        iVar6 = *(int32_t *)((uint8_t *)connection + 0xadc);
        iVar12 = (uint32_t)(network_challenge_packet_block >> 4) * 8;
        if ((*(uint8_t *)(iVar6 + 0xa8c) & 1) == 0) {
            if ((((*(int32_t *)(iVar6 + 0x24) + *(int32_t *)(iVar6 + 0x1c) * -8) -
                  *(int32_t *)(iVar6 + 0x20)) + 1 < iVar12 + 1) &&
                (ok = network_channel_stream_flush((network_channel_stream *)(iVar6 + 0x10), (network_channel *)iVar6, 1), ok == 0)) {
                goto tail;
            }
            {

                *(int32_t *)(iVar6 + 0xa80) = *(int32_t *)(iVar6 + 0xa80) + iVar12 + 1;
                { uint32_t item_flag = 0; halo::memory::bit_stream_write_bits_chunked((bit_stream *)((uint8_t *)iVar6 + 0x10), &item_flag, 1); }
                *(uint8_t *)(iVar6 + 0x2c) = 0;
                halo::memory::bit_stream_write_bits_chunked((bit_stream *)((uint8_t *)iVar6 + 0x10), (const uint32_t *)(&network_challenge_packet_block), iVar12);
                *(uint8_t *)(iVar6 + 0x2c) = 0;
            }
        }

        connection[0x76d] = 3;
        connection[0x766] = 0;
        connection[0x767] = 0;
        connection[0x768] = 0;
        connection[0x769] = 0;
        *((uint8_t *)connection + 0xee1) = 0;
        widget_close_all();
        game_engine_init_tick_record_for_mode();
        game_engine_reset_all_players();
        if (network_game_mode == 2 && ((*(uint8_t *)((uint8_t *)network_server + 6) >> 2 & 1) == 0)) {
            network_host_full_state_broadcast(network_server);
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
tail:
    return connection[0x76d] == 3;
}

}
