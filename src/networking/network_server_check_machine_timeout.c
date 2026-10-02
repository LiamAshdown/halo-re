// network_server_check_machine_timeout  (Ghidra: FUN_004e0ef0, unnamed)
// address 0x4e0ef0, size 727 bytes
// name confidence: 0.4   rewrite confidence: 0.25
// evidence: out/phase4/networking_functions.md: "Per-machine per-tick check that detects a
// stale/timed-out connection, tears down its machine-table slot, and broadcasts a
// player-left style notification." machine+0x14/+0x18 match network_machine::timer_14/timer_18;
// machine+0x1c matches the start of ::connect_state (read here as a raw 0x20-byte copy of the
// *player* entry it is being compared against, not the machine's own connect_state -- see the
// UNSURE note below); +0x3b8/+0x3bc/+0x408/+0x409/+0x40a/+0x40e/+0x414 (relative to a computed
// `server + i*0x60` base) resolve, once `+0x3b8` is folded back in, to exactly
// machines[i]'s channel/unknown_04/unknown_50/unknown_51/unknown_52/unknown_56/unknown_5c.
// register convention: both parameters are genuine stack (cdecl) parameters per Ghidra's own
// signature.
// blam-cc: stack -> server, machine
// REVIEW PASS 2026-09-20: the stack-copy ambiguity is resolved against the disassembly
// (objdump -d -M intel --start-address=0x4e0ef0 --stop-address=0x4e11d0 bin/halo.exe). The two
// loops in this function genuinely differ and both readings are now transcribed as written:
//   - the unknown_50 == 0 loop copies the 0x20-byte network_player_entry to the stack first and
//     then passes the COPY to all three calls (0x4e102b `lea eax,[esp+0x20]` before 0x4de9f0,
//     0x4e1049 `lea edx,[esp+0x20]` pushed to 0x4df0e0, 0x4e105b / 0x4e1086 `lea eax,[esp+0x20]`
//     before both 0x4de640 calls). An earlier rewrite modelled these as receiving the live entry;
//     they do not.
//   - the unknown_50 != 0 loop has no copy and passes the LIVE entry (0x4e0f80 `mov eax,esi`,
//     0x4e0f9a `push esi`).
// The odd `network_client != -0xb14` test is real: the binary computes
// `lea ebx,[eax+0xb14] ; test ebx,ebx` at 0x4e106d, i.e. it null-checks a pointer 0xb14 bytes
// into the client globals rather than the base.
// UNSURE: FUN_004de640 is called a second time, conditionally, immediately after the first
// call already removed the same entry -- transcribed exactly as decompiled even though this
// looks redundant.
// UNSURE: DAT_0069fdfc ("the rcon/console connection id") and gcd_disconnect_user/gcd_disconnect_all are
// GameSpy-adjacent library calls with no further-established signatures here.
// UNSURE: the wraparound-aware timeout test (`uVar2 <= uVar1`, etc., on timer_14/timer_18
// against the current millisecond clock) is preserved exactly as decompiled without
// simplification, since its edge-case behaviour around clock wraparound is not safe to guess.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <string.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern network_server_globals *network_server; // 0x0071c2d4
extern int64_t performance_frequency; // 0x006ac8f8/0x006ac8fc
extern network_client_globals *network_client; // 0x0071c2d8
extern int32_t network_console_connection_id; // 0x0069fdfc (UNSURE name)
extern uint8_t network_message_scratch[0x7ff8]; // 0x00871de0, shared with
    // network_game_broadcast_team_object_updates.c

extern char network_player_entry_validate(network_player_entry *entry);
    // blam-cc: EAX -> entry; 0x4de9f0. The EAX convention is pinned by 0x4e0f80 `mov eax,esi` /
    // 0x4e102b `lea eax,[esp+0x20]`, both immediately before the call.
extern uint32_t network_game_settings_broadcast_send(uint32_t round, uint32_t *record);
    // 0x4df0e0; both arguments are pushed: (server, the 32-byte player entry)
extern uint32_t network_player_entry_remove(network_player_entry *key, network_game_session *session);
    // blam-cc: EAX -> key, EBX -> session; 0x4de640. 0x4e105f passes the server session (EBX = server + 8),
    // 0x4e108a the client's (EBX = network_client + 0xb14).
extern void network_channel_remove_child(network_channel *channel); // other module (UNSURE)
extern void gcd_disconnect_user(int32_t id, int32_t value); // GameSpy library (UNSURE)
extern void gcd_disconnect_all(int32_t id); // GameSpy library (UNSURE)
extern void message_delta_parameters_protocol_send_update(void); // 0x4ebf50
extern int32_t message_delta_encode_message(int32_t extra_eax, int32_t extra_edx, int32_t flag, int32_t message_type,
    int32_t changed_offset, void **items, int32_t type_offset, int32_t count, char force_changed); // 0x4ec940, EAX buffer, EDX size
    // The call at 0x4e1187 also sets EAX = network_message_scratch and EDX = 0x7ff8, two
    // register arguments this declaration does not model.
extern char network_session_broadcast_to_all(network_server_globals *server, int32_t param_1,
    void *data, int32_t param_3, int32_t param_4, char force, int32_t param_6);
    // blam-cc: ECX -> server, stack -> param_1, data, param_3, param_4, force, param_6;
    // this module, 0x4e19c0

int32_t network_server_check_machine_timeout(network_server_globals *server, network_machine *machine)
{
    large_integer counter;
    uint32_t now_ms;
    uint32_t timer_18;
    uint32_t timer_14;
    int32_t result;

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    now_ms = (uint32_t)((counter.quad_part * 1000) / performance_frequency);
    timer_18 = (uint32_t)machine->timer_18;
    timer_14 = (uint32_t)machine->timer_14;
    result = 0;

    if (timer_14 <= timer_18) {
        if (timer_18 <= now_ms || now_ms < timer_14) {
            goto not_timed_out;
        }
        if (timer_14 <= timer_18) {
            return 1;
        }
    }
    if (now_ms < timer_18 || timer_14 <= now_ms) {
        return 1;
    }

not_timed_out:
    if (machine->player_joined == 0) {
        int16_t machine_id;

        machine_id = machine->machine_id;
        if (machine_id != -1) {
            network_player_entry *entry;
            int32_t remaining;
            int32_t i;

            entry = server->session.players;
            remaining = 16;
            do {
                network_player_entry copy;

                memcpy(&copy, entry, sizeof(copy));
                if (network_player_entry_validate(&copy) != 0 && // blam-cc: EAX -> &copy
                    (int32_t)copy.machine_index == machine_id) {
                    if (network_game_settings_broadcast_send((uint32_t)server, (uint32_t *)&copy) != 0) {
                        network_player_entry_remove(&copy, &server->session); // EAX &copy, EBX server + 8
                        if (network_client != 0 && (int32_t)network_client != -0xb14 &&
                            (machine->flags >> 2 & 1) != 0) {
                            network_player_entry_remove(&copy, &network_client->session); // EBX client + 0xb14
                        }
                    }
                }
                entry = entry + 1;
                remaining = remaining - 1;
            } while (remaining != 0);

            for (i = 0; i < 16; i = i + 1) {
                if (&server->machines[i] == machine) {
                    if (machine->channel != 0) {
                        network_channel_remove_child(machine->channel);
                    }
                    machine->channel = 0;
                    machine->unknown_04 = 0;
                    machine->unknown_08 = 0;
                    machine->machine_id = -1;
                    // 0x4e1114 is a WORD store at machine+0x0e, so it clears flags AND
                    // unknown_0f, not just flags
                    machine->flags = 0;
                    machine->unknown_0f = 0;
                    *(int32_t *)((uint8_t *)machine + 0x52) = 0;
                    *(int32_t *)((uint8_t *)machine + 0x56) = 0;
                    if (machine->gcd_user_id == -1) {
                        gcd_disconnect_all(network_console_connection_id);
                    } else {
                        gcd_disconnect_user(network_console_connection_id, machine->gcd_user_id);
                    }
                    machine->player_joined = 0;
                    machine->players_removed_broadcast = 0;
                    machine->gcd_user_id = -1;

                    message_delta_parameters_protocol_send_update();
                    {
                        network_game_session *session_ptr;
                        int32_t encoded;

                        session_ptr = &server->session;
                        encoded = message_delta_encode_message((int32_t)network_message_scratch, 0x7ff8, 0, 0x21, 0, (void **)&session_ptr, 0, 1, 0);
                        if (encoded > 0) {
                            network_session_broadcast_to_all(network_server, 1, network_message_scratch,
                                1, 0, 1, 3);
                        }
                    }
                    return 2;
                }
            }
        }
        return 0;
    }

    if (machine->players_removed_broadcast != 0) {
        return 1;
    }
    {
        char any_valid;
        network_player_entry *entry;
        int32_t i;
        int32_t local_result;

        any_valid = 0;
        entry = server->session.players;
        local_result = 0;
        for (i = 0; i < 16; i = i + 1) {
            if (network_player_entry_validate(entry) != 0 && // blam-cc: EAX -> entry (live)
                (int16_t)entry->machine_index == machine->machine_id) {
                if (network_game_settings_broadcast_send((uint32_t)server, (uint32_t *)entry) == 0) {
                    local_result = 0;
                    break;
                }
                any_valid = 1;
                local_result = 1;
            }
            entry = entry + 1;
        }
        if (!any_valid) {
            local_result = 1;
        }
        machine->players_removed_broadcast = 1;
        return local_result;
    }
}

#if 0
Original Ghidra decompilation (0x4e0ef0):

undefined4 FUN_004e0ef0(int param_1,void *param_2)

{
  uint uVar1;
  uint uVar2;
  bool bVar3;
  char cVar4;
  uint uVar5;
  void *pvVar6;
  undefined4 *puVar7;
  int iVar8;
  undefined4 *puVar9;
  int iVar10;
  undefined4 *puVar11;
  undefined8 uVar12;
  int local_30;
  LARGE_INTEGER local_28;
  undefined4 local_20 [7];
  char local_4;

  QueryPerformanceCounter(&local_28);
  uVar12 = __allmul(local_28.s.LowPart,local_28.s.HighPart,1000,0);
  uVar5 = __alldiv(uVar12,DAT_006ac8f8,DAT_006ac8fc);
  pvVar6 = param_2;
  uVar1 = *(uint *)((int)param_2 + 0x18);
  uVar2 = *(uint *)((int)param_2 + 0x14);
  local_30 = 0;
  if (uVar2 <= uVar1) {
    if ((uVar1 <= uVar5) || (uVar5 < uVar2)) goto LAB_004e0f5e;
    if (uVar2 <= uVar1) {
      return 1;
    }
  }
  if ((uVar5 < uVar1) || (uVar2 <= uVar5)) {
    return 1;
  }
LAB_004e0f5e:
  if (*(char *)((int)param_2 + 0x50) == '\0') {
    local_28.s.LowPart = (DWORD)*(short *)((int)param_2 + 0xc);
    if (local_28.s.LowPart != 0xffffffff) {
      puVar7 = (undefined4 *)(param_1 + 0x1aa);
      local_30 = 0x10;
      do {
        puVar9 = puVar7;
        puVar11 = local_20;
        for (iVar8 = 8; iVar8 != 0; iVar8 = iVar8 + -1) {
          *puVar11 = *puVar9;
          puVar9 = puVar9 + 1;
          puVar11 = puVar11 + 1;
        }
        cVar4 = FUN_004de9f0();
        if (((((cVar4 != '\0') && ((int)local_4 == local_28.s.LowPart)) &&
             (cVar4 = FUN_004df0e0(param_1,local_20), cVar4 != '\0')) &&
            ((FUN_004de640(), DAT_0071c2d8 != 0 && (DAT_0071c2d8 != -0xb14)))) &&
           ((*(byte *)((int)param_2 + 0xe) >> 2 & 1) != 0)) {
          FUN_004de640();
        }
        puVar7 = puVar7 + 8;
        local_30 = local_30 + -1;
      } while (local_30 != 0);
      iVar8 = 0;
      pvVar6 = (void *)(param_1 + 0x3b8);
      do {
        if (pvVar6 == param_2) {
          iVar10 = iVar8 * 0x60 + param_1;
          if (*(int *)(iVar10 + 0x3b8) != 0) {
            FUN_004dd090(*(int *)(iVar10 + 0x3b8));
          }
          *(undefined4 *)(iVar10 + 0x3b8) = 0;
          *(undefined4 *)(iVar10 + 0x3bc) = 0;
          *(undefined4 *)((iVar8 * 3 + 0x1e) * 0x20 + param_1) = 0;
          *(undefined2 *)(iVar10 + 0x3c4) = 0xffff;
          *(undefined2 *)(iVar10 + 0x3c6) = 0;
          *(undefined4 *)(iVar10 + 0x40a) = 0;
          *(undefined4 *)(iVar10 + 0x40e) = 0;
          if (*(int *)(iVar10 + 0x414) == -1) {
            FUN_0061b3f0(DAT_0069fdfc);
          }
          else {
            FUN_0061b350(DAT_0069fdfc,*(int *)(iVar10 + 0x414));
          }
          *(undefined1 *)(iVar10 + 0x408) = 0;
          *(undefined1 *)(iVar10 + 0x409) = 0;
          *(undefined4 *)(iVar10 + 0x414) = 0xffffffff;
          message_delta_parameters_protocol_send_update();
          local_28.s.LowPart = 0;
          param_2 = (void *)(param_1 + 8);
          iVar8 = message_delta_encode_message(0,0x21,0,&param_2,0,1,'\0');
          if (0 < iVar8) {
            FUN_004e19c0(1,&DAT_00871de0,1,0,1,3);
          }
          return 2;
        }
        iVar8 = iVar8 + 1;
        pvVar6 = (void *)((int)pvVar6 + 0x60);
      } while (iVar8 < 0x10);
    }
    return 0;
  }
  if (*(char *)((int)param_2 + 0x51) != '\0') {
    return 1;
  }
  bVar3 = false;
  iVar8 = param_1 + 0x1aa;
  iVar10 = 0;
  do {
    cVar4 = FUN_004de9f0();
    if ((cVar4 != '\0') && ((short)*(char *)(iVar8 + 0x1c) == *(short *)((int)pvVar6 + 0xc))) {
      cVar4 = FUN_004df0e0(param_1,iVar8);
      if (cVar4 == '\0') {
        local_30 = 0;
        break;
      }
      bVar3 = true;
      local_30 = 1;
    }
    iVar10 = iVar10 + 1;
    iVar8 = iVar8 + 0x20;
  } while (iVar10 < 0x10);
  if (!bVar3) {
    local_30 = 1;
  }
  *(undefined1 *)((int)pvVar6 + 0x51) = 1;
  return local_30;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
