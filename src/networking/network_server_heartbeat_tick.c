// network_server_heartbeat_tick  (Ghidra: FUN_004e15a0, unnamed)
// address 0x4e15a0, size 627 bytes
// name confidence: 0.4   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md: "Main per-frame networking heartbeat that
// times out unestablished machines and advances the connection/challenge handshake state
// machine." server+0x9c8/+0x9cc/+0x9d0/+0x9d4/+0x9d5/+0x9d6 are exactly the fields
// network_client_connection_handshake_tick.c reconstructs as a network_timer_pair plus three
// state bytes; server+0x9c4/+0x004/+session.unknown_3ac match the same reset sequence as
// network_game_server_handle_client_join.c; server+0x9bc matches
// network_server_resend_challenge_periodic.c's timestamp exactly (duplicated inline here
// rather than calling that function).
// register convention: EDI = server (network_server_globals *).
// blam-cc: EDI -> server
// UNSURE: FUN_00449210 is called here and its result IS used (unlike its other, argument-less,
// side-effect-only call sites elsewhere in this batch), so it is modelled here as returning an
// int32_t tick value; FUN_004df1c0 and FUN_004e04f0/FUN_004e0480/FUN_004e14e0 are all called
// with no visible arguments in the original even though they need `server`; passed explicitly
// here, consistent with how this batch's other files resolve the same situation.
// UNSURE: bit 0x04 of the ushort read at machine+0xe (flags+unknown_0f combined) in the second
// branch is treated as the same "processed this round" bit used in
// network_game_server_handle_client_join.c, though it is not an enumerated
// network_machine_flags value.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern int32_t QueryPerformanceCounter(large_integer *counter);
extern int64_t performance_frequency; // 0x006ac8f8/0x006ac8fc
extern network_client_globals *network_client; // 0x0071c2d8
extern int32_t time_query_performance_counter_ms(void); // other module; returns a tick value here (UNSURE)
extern void *network_prepare_challenge_packet(void); // 0x4deaf0, this module
extern void network_timer_advance(network_timer_pair *timer); // 0x4deb50, this module
extern void network_machine_timer_start(network_machine *machine, int32_t duration_ms); // 0x4df090, other module
extern char network_host_send_scenario_announcement(network_server_globals *server); // 0x4df1c0, this module (UNSURE args)
extern char network_game_all_machines_have_player(network_server_globals *server); // 0x4e04f0, this batch
extern char network_game_any_team_empty(network_server_globals *server); // 0x4e0480, this batch
extern char network_game_server_load_scenario(void); // 0x4e0720, this batch
extern uint8_t network_server_any_machine_awaiting_flag(network_server_globals *server); // 0x4e14e0, this batch
extern char network_session_broadcast_to_all(network_server_globals *server, int32_t param_1,
    void *data, int32_t param_3, int32_t param_4, char force, int32_t param_6);
    // blam-cc: ECX -> server, stack -> param_1, data, param_3, param_4, force, param_6;
    // this module, 0x4e19c0

uint8_t network_server_heartbeat_tick(network_server_globals *server)
{
    large_integer counter;
    int32_t now_ms;
    uint8_t result;
    network_timer_pair *timer;
    uint8_t *base;

    base = (uint8_t *)server;
    timer = (network_timer_pair *)(base + 0x9c8);

    QueryPerformanceCounter(&counter);
    now_ms = (int32_t)((counter.quad_part * 1000) / performance_frequency);
    result = 1;

    if (*(uint8_t *)(base + 0x9f9) == 0) {
        int32_t i;

        for (i = 0; i < 16; i = i + 1) {
            network_machine *machine;

            machine = &server->machines[i];
            if (machine->channel != 0 && (machine->channel->flags & 0x10) == 0) { // not dead
                network_machine_timer_start(machine, 0);
            }
        }

        if (*(uint8_t *)(base + 0x9d4) == 1) {
            char have_players;
            char team_empty;
            char restarting;

            have_players = network_game_all_machines_have_player(server);
            team_empty = have_players != 0 ? network_game_any_team_empty(server) : 0;
            if (have_players == 0 || team_empty != 0) {
                restarting = 0;
                timer->remaining_ms = 0;
                timer->last_tick_ms = 0;
                *(int32_t *)(base + 0x9d0) = 0;
                *(uint8_t *)(base + 0x9d4) = 0;
            } else {
                restarting = 1;
                network_timer_advance(timer);
                if (timer->remaining_ms == 0 &&
                    network_server_any_machine_awaiting_flag(server) != 0 &&
                    *(uint8_t *)(base + 0x9d5) == 0) {
                    result = network_host_send_scenario_announcement(server);
                    goto scenario_check;
                }
                if ((uint32_t)(now_ms - *(int32_t *)(base + 0x9d0)) < 0x3e9) {
                    goto scenario_check;
                }
            }
            *(uint8_t *)(base + 0x9d6) = 0;
            if (restarting) {
                network_timer_advance(timer);
            }
            {
                void *packet;

                packet = network_prepare_challenge_packet();
                if (packet != 0 && network_session_broadcast_to_all(server, 0, packet, 1, 0, 1, 3) != 0) {
                    *(int32_t *)(base + 0x9d0) = now_ms;
                }
            }
        } else if (*(int32_t *)(base + 0x9bc) + 5000 < now_ms) {
            void *packet;

            packet = network_prepare_challenge_packet();
            network_session_broadcast_to_all(server, 0, packet, 1, 0, 1, 3);
            *(int32_t *)(base + 0x9bc) = now_ms;
        }
    } else if (*(int32_t *)(base + 0x9c4) != 0) {
        int32_t now2;

        now2 = time_query_performance_counter_ms();
        if ((uint32_t)(now2 - *(int32_t *)(base + 0x9c4)) > 59999) {
            int32_t i;
            char has_client;

            for (i = 0; i < 16; i = i + 1) {
                uint16_t flags;

                flags = *(uint16_t *)((uint8_t *)&server->machines[i] + 0xe);
                if ((flags & 1) != 0 && (flags & 4) == 0) {
                    network_machine_timer_start(&server->machines[i], 0);
                }
            }
            has_client = (network_client != 0);
            server->unknown_004 = 1;
            *(int32_t *)(base + 0x9c4) = 0;
            server->session.unknown_3ac = has_client ? *((uint8_t *)network_client + 0xec0) : 0;
        }
    }

scenario_check:
    if (*(uint8_t *)(base + 0x9fa) == 1) {
        if (network_game_server_load_scenario() == 1) {
            server->unknown_004 = 1;
        }
        *(uint8_t *)(base + 0x9fa) = 0;
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x4e15a0):

undefined1 FUN_004e15a0(void)

{
  char cVar1;
  undefined1 uVar2;
  int iVar3;
  int iVar4;
  ushort *puVar5;
  int *piVar6;
  int unaff_EDI;
  bool bVar7;
  undefined8 uVar8;
  undefined1 local_11;
  LARGE_INTEGER local_c;

  QueryPerformanceCounter(&local_c);
  uVar8 = __allmul(local_c.s.LowPart,local_c.s.HighPart,1000,0);
  iVar3 = __alldiv(uVar8,DAT_006ac8f8,DAT_006ac8fc);
  local_11 = 1;
  if (*(char *)(unaff_EDI + 0x9f9) == '\0') {
    piVar6 = (int *)(unaff_EDI + 0x3b8);
    iVar4 = 0x10;
    do {
      if ((*piVar6 != 0) && ((~(byte)(*(uint *)(*piVar6 + 0xa8c) >> 4) & 1) == 0)) {
        FUN_004df090(0);
      }
      piVar6 = piVar6 + 0x18;
      iVar4 = iVar4 + -1;
    } while (iVar4 != 0);
    if (*(char *)(unaff_EDI + 0x9d4) == '\x01') {
      cVar1 = FUN_004e04f0();
      if ((cVar1 == '\0') || (cVar1 = FUN_004e0480(), cVar1 != '\0')) {
        bVar7 = false;
        *(undefined4 *)(unaff_EDI + 0x9c8) = 0;
        *(undefined4 *)(unaff_EDI + 0x9cc) = 0;
        *(undefined4 *)(unaff_EDI + 0x9d0) = 0;
        *(undefined4 *)(unaff_EDI + 0x9d4) = 0;
      }
      else {
        bVar7 = true;
        FUN_004deb50();
        if (((*(int *)(unaff_EDI + 0x9c8) == 0) && (cVar1 = FUN_004e14e0(), cVar1 != '\0')) &&
           (*(char *)(unaff_EDI + 0x9d5) == '\0')) {
          local_11 = FUN_004df1c0();
          goto LAB_004e17e9;
        }
        if (iVar3 - *(int *)(unaff_EDI + 0x9d0) < 0x3e9) goto LAB_004e17e9;
      }
      *(undefined1 *)(unaff_EDI + 0x9d6) = 0;
      if (bVar7) {
        FUN_004deb50();
      }
      iVar4 = network_prepare_challenge_packet();
      if ((iVar4 != 0) && (cVar1 = FUN_004e19c0(0,iVar4,1,0,1,3), cVar1 != '\0')) {
        *(int *)(unaff_EDI + 0x9d0) = iVar3;
      }
    }
    else if (*(int *)(unaff_EDI + 0x9bc) + 5000 < iVar3) {
      iVar4 = network_prepare_challenge_packet();
      FUN_004e19c0(0,iVar4,1,0,1,3);
      *(int *)(unaff_EDI + 0x9bc) = iVar3;
    }
  }
  else if ((*(int *)(unaff_EDI + 0x9c4) != 0) &&
          (iVar3 = FUN_00449210(), 59999 < (uint)(iVar3 - *(int *)(unaff_EDI + 0x9c4)))) {
    puVar5 = (ushort *)(unaff_EDI + 0x3c6);
    iVar3 = 0x10;
    do {
      if (((*puVar5 & 1) != 0) && ((*puVar5 & 4) == 0)) {
        FUN_004df090(0);
      }
      iVar4 = DAT_0071c2d8;
      puVar5 = puVar5 + 0x30;
      iVar3 = iVar3 + -1;
    } while (iVar3 != 0);
    bVar7 = DAT_0071c2d8 == 0;
    *(undefined2 *)(unaff_EDI + 4) = 1;
    *(undefined4 *)(unaff_EDI + 0x9c4) = 0;
    if (bVar7) {
      uVar2 = 0;
    }
    else {
      uVar2 = *(undefined1 *)(iVar4 + 0xec0);
    }
    *(undefined1 *)(unaff_EDI + 0x3b4) = uVar2;
  }
LAB_004e17e9:
  if (*(char *)(unaff_EDI + 0x9fa) == '\x01') {
    cVar1 = FUN_004e0720();
    if (cVar1 == '\x01') {
      *(undefined2 *)(unaff_EDI + 4) = 1;
    }
    *(undefined1 *)(unaff_EDI + 0x9fa) = 0;
  }
  return local_11;
}
#endif
