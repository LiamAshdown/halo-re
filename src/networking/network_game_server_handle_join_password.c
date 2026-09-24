// network_game_server_handle_join_password  (Ghidra: FUN_004e21d0; named per this rewrite)
// address 0x4e21d0, size 546 bytes
// name confidence: 0.4   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md: "Handles message type 0xe: validates a client's
// join password and, if accepted, admits the machine and replies with a challenge/accept
// packet." Field offsets (+4 unknown_004, +6 flags, +0xa0f game_over on `server`; +0 channel,
// +0xc machine_id, +0xe flags on `machine`) match types/networking.h exactly. The reason-code
// dispatch to a single shared network_server_notify_or_resend_challenge call is recovered from
// disassembly (objdump -d -M intel, bin/halo.exe), which Ghidra's own pseudo-C flattens away:
//   4e2311/4e23c9/4e23d4/4e23df/4e23ea: each path pushes `server` then sets ECX to a distinct
//   reason (2, 3, 1, 6, 0) before jumping to the single `mov edi,ebx / call 0x4e0af0` site.
// register convention: EBP = server (network_server_globals *, a genuine stack argument), EBX =
// machine (network_machine *, implicit passthrough -- never loaded within this function's own
// body), EDX = buffer (implicit passthrough, matching every sibling message-type handler in
// this batch).
//   // blam-cc: EDX -> buffer, EBX -> machine, stack -> server
// UNSURE (major): the exact split of data_packet_group_decode_packet's arguments between
// registers and stack at this call site does not fully resolve against its established
// simplified 6-argument prototype (see network_game_client_decode_beacon_reply.c); called here
// with that same simplified prototype and `buffer + 2` per the universal convention in this
// batch, rather than re-deriving the precise register assignment.
// UNSURE: several callees below (network_join_request_reset_state, network_debug_fill_canary_buffer,
// network_channel_remote_address_or_default) are declared here with a local prototype matching
// only what THIS call site needs, which disagrees with the fuller signature each function's own
// file reconstructed independently -- an accepted category of mismatch in this codebase (see
// src/networking/README.md, "Signature disagreements between an extern and its definition").
// UNSURE: immediately before the final network_session_send_to_machine call, disassembly shows
// `mov eax,ecx` (machine->machine_id) and `mov esi,ebp` (server) loaded into registers that are
// not read again before the call and are popped/discarded on return; preserved as dead stores
// (commented out) rather than invented as extra arguments, since every literal argument the
// call needs is already accounted for on the stack.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <string.h>
#include <wchar.h>

extern uint8_t network_pending_join_password_hint; // 0x006894a2 (UNSURE name)

extern int32_t data_packet_group_decode_packet(data_packet_group *group, void *decoded_body,
    const uint8_t *buffer, int16_t *out_a, int16_t *out_b, int32_t expected_class); // 0x4d09d0
extern data_packet_group network_game_messages_group; // 0x006994f8
extern char network_server_build_full_game_info_packet(network_machine *machine); // 0x4e0bd0, this module
extern void network_channel_remote_address_or_default(void); // 0x4dd390, this module (UNSURE args, see header)
extern char network_join_request_reset_state(void *scratch); // 0x4e0ab0, this module (UNSURE args, see header)
extern uint32_t *network_debug_fill_canary_buffer(void *scratch); // 0x4e0790, this module (UNSURE: the
    // established file declares this void and EDI-based; this call site clearly reuses EAX as
    // the filled buffer's address afterward)
extern void network_server_password_get(network_server_globals *server, wchar_t *dest); // 0x4e0930, this module
extern int32_t network_server_password_is_set(network_server_globals *server); // 0x4e08e0, this module
extern int32_t network_machine_reset(network_machine *machine); // 0x4df690, this module
extern uint32_t network_game_session_finalize_and_add_player(network_player_entry *entry,
    network_server_globals *server, network_machine *machine); // 0x4df840, this module
extern uint32_t network_game_broadcast_player_set_changed(void); // 0x4e1bf0, this module, called
    // with no visible arguments at this site (implicit passthrough, per that file's own header)
extern uint16_t *network_prepare_challenge_packet(int32_t message_type, void *payload); // 0x4deaf0
extern char network_session_send_to_machine(int32_t machine_id, void *data, int32_t bits,
    int32_t reliable, int32_t unknown_a, int32_t unknown_b, int32_t priority); // 0x4e1930
extern uint8_t network_server_notify_or_resend_challenge(int32_t reason, network_machine *machine,
    network_server_globals *server); // 0x4e0af0, this module (local prototype, see header UNSURE)

// blam-cc: EDX -> buffer, EBX -> machine, stack -> server
char network_game_server_handle_join_password(uint8_t *buffer, network_machine *machine, network_server_globals *server)
{
    uint32_t decoded_body[4];
    int16_t out_a, out_b;
    uint32_t *canary;
    wchar_t server_password[8];
    network_player_entry *entry;
    int32_t reason;

    if ((server->unknown_004 != 0 && server->unknown_004 != 1) || (machine->flags & 0x02) != 0) {
        return 1;
    }

    if ((machine->channel == 0 || machine->channel->connected == 0) && server->game_over != 0) {
        char result = network_server_build_full_game_info_packet(machine);
        if (result != 0) {
            return result;
        }
        return 1;
    }

    if (data_packet_group_decode_packet(&network_game_messages_group, decoded_body, buffer + 2,
                                         &out_a, &out_b, 3) == 0) {
        return 1;
    }
    network_channel_remote_address_or_default();

    if ((server->flags & 1) == 0 || (server->unknown_004 != 0 && server->unknown_004 != 1)) {
        return 1;
    }
    if (network_join_request_reset_state(&out_b) == 0) {
        reason = 6;
        goto notify;
    }

    canary = network_debug_fill_canary_buffer(&decoded_body[0]);
    if (memcmp(decoded_body, canary, sizeof(decoded_body)) != 0) {
        reason = 1;
        goto notify;
    }

    network_server_password_get(server, server_password);
    if (wcsncmp(server_password, (wchar_t *)decoded_body, 8) != 0 &&
        network_server_password_is_set(server) != 0) {
        reason = 2; // password configured and mismatched (distinct from the reason=1 canary mismatch above)
        goto notify;
    }

    if (network_machine_reset(machine) == 0) {
        reason = 3;
        goto notify;
    }

    entry = (network_player_entry *)((uint8_t *)decoded_body); // UNSURE: entry scratch offset (esp+0x9a)
    if (network_game_session_finalize_and_add_player(entry, server, machine) == 0) {
        reason = 3;
        goto notify;
    }

    network_game_broadcast_player_set_changed();
    if (network_pending_join_password_hint == 0) {
        machine->channel->rate_index = 4;
    } else {
        machine->channel->rate_index = 0; // UNSURE: sourced from a stack byte this rewrite cannot recover
    }
    {
        uint32_t payload[2] = {0, 0};
        uint16_t *packet = network_prepare_challenge_packet(0xa, payload);
        if (packet == 0) {
            return 1;
        }
        if (machine->machine_id == -1) {
            return 1;
        }
        network_session_send_to_machine(0, packet, (int32_t)(*packet >> 4) << 3, 1, 1, 3, 3);
        return 1;
    }

notify:
    return network_server_notify_or_resend_challenge(reason, machine, server);
}

#if 0
Original Ghidra decompilation (0x4e21d0), from tools/pack.py 0x4e21d0:

char FUN_004e21d0(int param_1,int param_2)

{
  char cVar1;
  int *piVar2;
  ushort *puVar3;
  int iVar4;
  int *unaff_EBX;
  int *piVar5;
  bool bVar6;
  undefined4 local_b0;
  undefined1 local_ac [4];
  wchar_t local_a8 [12];
  int local_90 [4];
  wchar_t local_80 [8];
  undefined2 local_70;
  undefined1 local_6e [73];
  byte local_25;

  if (((*(short *)(param_1 + 4) == 0) || (*(short *)(param_1 + 4) == 1)) &&
     ((*(byte *)((int)unaff_EBX + 0xe) >> 1 & 1) == 0)) {
    if (((*unaff_EBX == 0) || (*(char *)(*unaff_EBX + 0xa98) == '\0')) &&
       (*(char *)(param_1 + 0xa0f) != '\0')) {
      cVar1 = FUN_004e0bd0();
      if (cVar1 != '\0') {
        return cVar1;
      }
    }
    else {
      cVar1 = data_packet_group_decode_packet
                        (&PTR_s_network_game_messages_group_006994f8,local_90,param_2 + 2,&local_b0,
                         local_ac,3);
      if (cVar1 != '\0') {
        FUN_004dd390();
        if ((((*(byte *)(param_1 + 6) & 1) != 0) &&
            ((*(short *)(param_1 + 4) == 0 || (*(short *)(param_1 + 4) == 1)))) &&
           (cVar1 = FUN_004e0ab0(local_6e), cVar1 != '\0')) {
          piVar2 = (int *)FUN_004e0790();
          iVar4 = 4;
          bVar6 = true;
          piVar5 = local_90;
          do {
            if (iVar4 == 0) break;
            iVar4 = iVar4 + -1;
            bVar6 = *piVar5 == *piVar2;
            piVar5 = piVar5 + 1;
            piVar2 = piVar2 + 1;
          } while (bVar6);
          if (bVar6) {
            FUN_004e0930();
            local_70 = 0;
            iVar4 = _wcsncmp(local_80,local_a8,8);
            if ((((iVar4 == 0) || (cVar1 = FUN_004e08e0(), cVar1 == '\0')) &&
                (cVar1 = FUN_004df690(), cVar1 != '\0')) && (cVar1 = FUN_004df840(), cVar1 != '\0'))
            {
              FUN_004e1bf0(param_1);
              if (DAT_006894a2 == '\0') {
                *(undefined4 *)(*unaff_EBX + 0xa88) = 4;
              }
              else {
                *(uint *)(*unaff_EBX + 0xa88) = (uint)local_25;
              }
              local_b0 = 0;
              puVar3 = (ushort *)network_prepare_challenge_packet();
              if (puVar3 == (ushort *)0x0) {
                return '\x01';
              }
              if ((short)unaff_EBX[3] == -1) {
                return '\x01';
              }
              network_session_send_to_machine(0,puVar3,(uint)(*puVar3 >> 4) << 3,1,0,1,3);
              return '\x01';
            }
          }
        }
        FUN_004e0af0(param_1);
      }
    }
  }
  return '\x01';
}

Disassembly (objdump -d -M intel, bin/halo.exe) recovering the reason-code dispatch Ghidra's own
pseudo-C collapsed into a single argument-less `FUN_004e0af0(param_1)`:
  4e2311: mov ecx,2 / mov edi,ebx / call 0x4e0af0    ; reason 2: password configured and mismatched
                                                        (falls through from the password_is_set
                                                        check when wcsncmp also mismatched)
  4e23c9: push ebp / mov ecx,3 / jmp shared-call      ; reason 3: network_machine_reset or
                                                        network_game_session_finalize_and_add_player failed
  4e23d4: push ebp / mov ecx,1 / jmp shared-call      ; reason 1: canary/challenge memcmp mismatch
  4e23df: push ebp / mov ecx,6 / jmp shared-call      ; reason 6: network_join_request_reset_state failed
  4e23ea: push ebp / xor ecx,ecx / jmp shared-call    ; reason 0: session flags/state not ready
#endif
