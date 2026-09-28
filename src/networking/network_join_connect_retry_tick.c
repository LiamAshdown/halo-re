// network_join_connect_retry_tick  (Ghidra: FUN_004dab80; renamed, no prior name)
// address 0x4dab80, size 550 bytes
// name confidence: 0.4   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md summary ("Per-tick state machine that updates
// the join UI status text/timeout and retries the connect attempt while the client channel is
// not yet fully connected"). channel+0xa8c (piVar2[0x2a3]) is network_channel::flags;
// channel->endpoint (*piVar2) leads into network_receive_queue's own +0xc flags byte and +0xe
// last_error, both per types/networking.h. client+0xae0/+0xae4/+0xae8/+0xaec match
// network_client_globals::connect_attempt (a network_connection_attempt_state in
// types/networking.h since the review pass), shared with network_connection_initiate.c /
// chimera__on_connect.c / network_client_connect_progress_percent.c.
// register convention: the client pointer arrives in EAX (in_EAX). // blam-cc: EAX -> client
// UNSURE: `network_signal_quality_glyph` and `network_receive_queue_close_socket` are called with no visible arguments and are not in
// this task's range; declared as taking no arguments, exactly as shown.
// UNSURE: the `-0x18` last_error comparison and the `-1 < (char)endpoint->flags` sign test have
// no further-resolved meaning beyond their evidenced role here.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern int64_t performance_frequency; // 0x006ac8f8/0x006ac8fc
extern int16_t network_join_error_code; // 0x00718fa4, WORD-sized
extern int32_t network_signal_quality_glyph(void); // 0x440610, not in this batch
extern int32_t network_connect_timeout_ms; // 0x006894ac, per network_client_connect_progress_percent.c
extern void network_receive_queue_close_socket(void); // 0x442040, not in this batch
extern void console_printf_verbose(const char *text); // 0x496a80
extern int32_t interface_loading_screen_progress; // 0x00718f90
extern int16_t network_game_mode; // 0x00719720
extern int32_t join_ui_state; // 0x00718f8c
extern int32_t interface_loading_screen_request_id; // 0x0068e688
extern char network_channel_service(network_channel *channel, int32_t timeout_ms, network_channel **out_new_child); // 0x4dd110
extern int32_t network_game_process_incoming_messages(network_client_globals *client); // 0x4db180
extern void network_join_status_text_update(int32_t mode, network_client_globals *client); // 0x4db4c0
    // UNSURE: called here with no visible arguments; mode reconstructed as 1 (the animated
    // "Connecting..." case), matching this call site's own "still waiting, tick the dots" context.


// blam-cc: EAX -> client
int32_t network_join_connect_retry_tick(network_client_globals *client)
{
    large_integer counter;
    int32_t now_ms;
    network_channel *channel;
    network_receive_queue *endpoint;
    network_connection_attempt_state *attempt;
    int32_t ok;

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    now_ms = (int32_t)((counter.quad_part * 1000) / performance_frequency);

    channel = client->channel;
    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    channel->last_activity_ms = (int32_t)((counter.quad_part * 1000) / performance_frequency);

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
                if ((client->unknown_ede & 4) == 0) {
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
    if (attempt->unknown_0c == 0) {
        attempt->elapsed_counter = 0;
        console_printf_verbose("Loading");
        interface_loading_screen_progress = 0;
        if (network_game_mode == 2) {
            if (join_ui_state != 1) {
                if (join_ui_state != 2 && join_ui_state == 4) {
                    interface_loading_screen_request_id = -1;
                }
                join_ui_state = 8;
                attempt->unknown_0c = 1;
                goto service_channel;
            }
        } else if (join_ui_state != 1 && join_ui_state != 2) {
            if (join_ui_state == 4) {
                interface_loading_screen_request_id = -1;
                attempt->unknown_0c = 1;
                goto service_channel;
            }
            join_ui_state = 7;
        }
        attempt->unknown_0c = 1;
    }
service_channel:
    // 0x4dad7d: edi = client->channel; 0x4dad83: push ebp (zero for the whole function, set
    // by `xor ebp,ebp` at 0x4dabd8); 0x4dad84: eax = 0x1388 (5000 ms).
    ok = network_channel_service(client->channel, 5000, 0);
    if (ok != 0) {
        return network_game_process_incoming_messages(client);
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4dab80):

bool FUN_004dab80(void)

{
  int iVar1;
  int *piVar2;
  char cVar3;
  bool bVar4;
  int in_EAX;
  int iVar5;
  undefined4 uVar6;
  undefined8 uVar7;
  LARGE_INTEGER local_c;

  QueryPerformanceCounter(&local_c);
  uVar7 = __allmul(local_c.s.LowPart,local_c.s.HighPart,1000,0);
  iVar5 = __alldiv(uVar7,DAT_006ac8f8,DAT_006ac8fc);
  iVar1 = *(int *)(in_EAX + 0xadc);
  QueryPerformanceCounter(&local_c);
  uVar7 = __allmul(local_c.s.LowPart,local_c.s.HighPart,1000,0);
  uVar6 = __alldiv(uVar7,DAT_006ac8f8,DAT_006ac8fc);
  *(undefined4 *)(iVar1 + 4) = uVar6;
  piVar2 = *(int **)(in_EAX + 0xadc);
  if ((((piVar2[0x2a3] & 6U) == 0) || (*piVar2 == 0)) || ((*(byte *)(*piVar2 + 0xc) & 1) == 0)) {
    if (*(short *)(*piVar2 + 0xe) == -0x18) {
      if (DAT_00718fa4 == -1) {
        DAT_00718fa4 = FUN_00440610();
        return false;
      }
    }
    else {
      if (-1 < *(char *)(*piVar2 + 0xc)) {
        if (*(int *)(in_EAX + 0xae0) == 0) {
          if ((*(byte *)(in_EAX + 0xede) & 4) == 0) {
            if (3000 < (uint)((iVar5 + *(int *)(in_EAX + 0xae8) * -3000) - *(int *)(in_EAX + 0xae4))
               ) {
              FUN_004db4c0();
            }
            goto LAB_004dad7d;
          }
        }
        else {
          if ((uint)(iVar5 - *(int *)(in_EAX + 0xae4)) <= DAT_006894ac) goto LAB_004dad7d;
          *(undefined4 *)(in_EAX + 0xae0) = 0;
        }
        FUN_00442040();
      }
      if (DAT_00718fa4 == -1) {
        DAT_00718fa4 = 3;
      }
    }
    return false;
  }
  *(undefined4 *)(in_EAX + 0xae0) = 0;
  if (*(char *)(in_EAX + 0xaec) == '\0') {
    *(undefined4 *)(in_EAX + 0xae8) = 0;
    FUN_00496a80("Loading");
    DAT_00718f90 = 0;
    if (DAT_00719720 == 2) {
      if (DAT_00718f8c != 1) {
        if ((DAT_00718f8c != 2) && (DAT_00718f8c == 4)) {
          DAT_0068e688 = 0xffffffff;
        }
        DAT_00718f8c = 8;
        *(undefined1 *)(in_EAX + 0xaec) = 1;
        goto LAB_004dad7d;
      }
    }
    else if ((DAT_00718f8c != 1) && (DAT_00718f8c != 2)) {
      if (DAT_00718f8c == 4) {
        DAT_0068e688 = 0xffffffff;
        *(undefined1 *)(in_EAX + 0xaec) = 1;
        goto LAB_004dad7d;
      }
      DAT_00718f8c = 7;
    }
    *(undefined1 *)(in_EAX + 0xaec) = 1;
  }
LAB_004dad7d:
  cVar3 = FUN_004dd110(0);
  bVar4 = false;
  if (cVar3 != '\0') {
    bVar4 = network_game_process_incoming_messages(in_EAX);
  }
  return bVar4;
}
#endif
