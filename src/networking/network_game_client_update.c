// network_game_client_update  (Ghidra: network_game_client_update, already named)
// address 0x4daf80, size 377 bytes
// name confidence: 0.55   rewrite confidence: 0.35
// evidence: out/phase4/networking_functions.md summary ("Main per-tick network update for an
// actively-connected client: services the channel, processes incoming messages, flushes
// outgoing data, and (when a debug flag is set) periodically logs ping/latency/timing"). Reuses
// the network_client_globals::connection fields already named in
// network_connection_send_keepalive.c (message_count@0xad0, retry_count@0xad2, unknown_20@0xad4,
// control_block@0xad8) and the channel-flags bit layout confirmed in network_host_update_tick.c.
// register convention: __cdecl, single stack parameter `client`.
// // blam-cc: stack -> client
// UNSURE: `ui_network_wait_timeout_start`, `network_channel_service_light`, `network_channel_service_retransmit_only` are called with no visible arguments
// and are not in this task's range; declared exactly as shown.
// UNSURE: `DAT_0071c2dc` is documented only loosely in types/networking.h ("shortens the
// disconnect timeout when clear"); named `network_disconnect_timeout_flag` here.
// UNSURE: `DAT_00710306` (a debug-print gate byte) and `DAT_0071c2c4` (the last-printed ping
// sample cache) are not declared in types/networking.h; named generically.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "fn_networking.h"
#include "fn_interface.h"

extern network_server_globals *network_server; // 0x0071c2d4
extern uint8_t network_disconnect_timeout_flag; // 0x0071c2dc, UNSURE name; see file header

extern char network_channel_service_light(int32_t flag); // 0x4dd240, UNSURE argument; not in this batch
extern int16_t network_game_mode; // 0x00719720


extern int8_t network_channel_service_retransmit_only(void); // 0x4dd330, not in this batch
extern int16_t network_join_error_code; // 0x00718fa4, the pending join/disconnect error
                                        // string index; -1 means none. WORD-sized everywhere
                                        // (cmp/mov WORD PTR ds:0x718fa4), consumed and reset by
                                        // the main-menu display_error call at 0x4a9ff0.

extern uint8_t network_ping_debug_log_enabled; // 0x00710306, UNSURE name
extern uint32_t network_ping_debug_last_sample; // 0x0071c2c4, UNSURE name
extern void console_print_error_va(uint8_t clear_first, const char *format, ...); // 0x4c67c0, AL clear_first
extern int32_t time_query_performance_counter_ms(void); // 0x449210, cseries: current time in milliseconds


// blam-cc: stack -> client
int8_t network_game_client_update(network_client_globals *client)
{
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
            client->wait_timeout_active = (uint8_t)(flags >> 5) & 1;
        }
        service_ok = network_channel_service_light(0); // UNSURE argument
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
            endpoint->unknown_20, endpoint->retry_count, (uint16_t)endpoint->message_count);
        server_base_time = *(int32_t *)endpoint->control_block;
        // 0x4db0ca: ECX = the connection's sample ring (client + 0xad8)
        challenge_time = message_delta_sample_ring_buffer_average((message_delta_sample_ring_buffer *)endpoint->control_block);
        now2 = time_query_performance_counter_ms();
        console_print_error_va(0, "current time delta[%d]  latency[%d]  server time[%d]\n",
            server_base_time, challenge_time, now2 + server_base_time);
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x4daf80):

undefined1 __cdecl network_game_client_update(int param_1)

{
  int iVar1;
  int *piVar2;
  char cVar3;
  bool bVar4;
  undefined1 uVar5;
  undefined4 uVar6;
  int iVar7;
  uint uVar8;

  uVar8 = (*(int **)(param_1 + 0xadc))[0x2a3];
  if (((((~(byte)(uVar8 >> 4) & 1) != 0) && ((uVar8 & 6) != 0)) &&
      (iVar1 = **(int **)(param_1 + 0xadc), iVar1 != 0)) && ((*(byte *)(iVar1 + 0xc) & 1) != 0)) {
    if ((DAT_0071c2d4 == 0) || (DAT_0071c2dc != '\0')) {
      if ((uVar8 >> 5 & 1) != 0) {
        FUN_0049c810();
      }
      *(byte *)(param_1 + 0xee1) = (byte)(uVar8 >> 5) & 1;
    }
    cVar3 = FUN_004dd240(0);
    if (DAT_00719720 == 2) {
      network_channel_record_timestamp(*(int *)(param_1 + 0xadc));
    }
    if (cVar3 != '\0') {
      bVar4 = network_game_process_incoming_messages(param_1);
      uVar5 = 0;
      if (bVar4) {
        uVar5 = FUN_004dd330();
      }
      goto LAB_004db07a;
    }
    piVar2 = *(int **)(param_1 + 0xadc);
    if ((((~(byte)((uint)piVar2[0x2a3] >> 4) & 1) != 0) && ((*(byte *)(piVar2 + 0x2a3) & 6) != 0))
       && ((*piVar2 != 0 && (uVar5 = 0, (*(byte *)(*piVar2 + 0xc) & 1) != 0)))) goto LAB_004db07a;
  }
  if (DAT_00718fa4 == -1) {
    DAT_00718fa4 = 4;
  }
  uVar5 = 0;
LAB_004db07a:
  FUN_004d9400();
  if (((DAT_00710306 == '\x01') &&
      (uVar8 = (uint)*(ushort *)(param_1 + 0xad0), uVar8 != DAT_0071c2c4)) && (uVar8 % 10 == 0)) {
    DAT_0071c2c4 = uVar8;
    console_print_error_va
              ("current ping time[%d]  samples received[%d]  samples sent[%d]\n",
               *(undefined2 *)(param_1 + 0xad4),*(undefined2 *)(param_1 + 0xad2),uVar8);
    iVar1 = **(int **)(param_1 + 0xad8);
    uVar6 = FUN_004ed350();
    iVar7 = FUN_00449210();
    console_print_error_va
              ("current time delta[%d]  latency[%d]  server time[%d]\n",iVar1,uVar6,iVar7 + iVar1);
  }
  return uVar5;
}
#endif
