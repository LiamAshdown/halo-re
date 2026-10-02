// network_game_server_handle_join_password  (Ghidra: FUN_004e21d0)
// address 0x4e21d0, size 546 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// REWRITTEN 2026-09-28 from objdump 0x4e21d0..0x4e23f1: the host's join request handler (message 0xe; EBX machine,
//   stack server, buffer, length). While the host is not in a game (+4 0 or 1) and the machine is not already joining
//   (+0xe bit 1): a machine without a connected channel after the game ended gets the full game info. Otherwise the
//   request (a 0x84 byte body) is decoded; unless the server accepts joins (+6 bit 0, state 0 or 1) it is refused
//   (0); the CD key response at body +0x22 must pass the host check (else 6); the first 16 bytes must match the
//   version canary (else 1); the 8-character password at body +0x10 must match a set password (else 2); the machine
//   is reset and its player (body +0x6e) added (else 3), the player set is broadcast, the channel rate (+0xa88)
//   becomes 4 or body +0x6b by the hint byte 0x006894a2, and a type 0xa accept packet is sent. Refusals send the
//   reason; every path returns 1.
// blam-cc: EBX -> machine, stack -> server, buffer, length

// VERIFIED against disassembly 0x4e21d0..0x4e23f2 (2026-09-30): compared every branch (state/flags gate, full-game-info shortcut, decode args, reject reasons 0/6/1/2/3, add player, rate 4 or body+0x6b, type 0xa accept packet via send_to_machine 7 stack args); FIXED broadcast_player_set_changed takes only the session (one stack arg)
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <string.h>
#include <wchar.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint16_t *network_prepare_challenge_packet(int32_t message_type, void *payload); // 0x4deaf0, blam-cc: EAX type, EDX payload
extern uint8_t network_session_send_to_machine(int32_t machine_id, network_server_globals *server, uint32_t status_bit, void *data,
    uint32_t bits, uint32_t reliable, uint32_t unknown_a, char force, uint32_t priority); // 0x4e1930, blam-cc: EAX machine_id, ESI server
extern data_packet_group network_game_messages_group; // 0x006994f8
extern int32_t data_packet_group_decode_packet(int16_t *remaining_length, data_packet_group *group, void *decoded_body,
    uint8_t *buffer, int16_t *out_type, uint16_t *out_version_used, int16_t expected_class); // 0x4d09d0, blam-cc: EAX remaining_length
extern uint32_t network_game_session_finalize_and_add_player(network_player_entry *entry, network_server_globals *server,
    network_machine *machine); // 0x4df840, blam-cc: EAX entry, ECX server, EDX machine
extern uint32_t network_game_broadcast_player_set_changed(uint8_t *param_1); // 0x4e1bf0, one stack argument (the session)
extern uint8_t network_server_notify_or_resend_challenge(int16_t reason, network_machine *machine,
    network_server_globals *server); // 0x4e0af0
extern uint8_t network_game_info_packet_flag; // 0x006894a2
extern char network_server_build_full_game_info_packet(network_machine *machine); // 0x4e0bd0
extern void network_channel_remote_address_or_default(network_channel *channel, network_resolved_address *out_address); // 0x4dd390
extern uint8_t network_join_request_reset_state(network_machine *machine, const char *response); // 0x4e0ab0, blam-cc: EAX machine
extern void network_debug_fill_canary_buffer(uint32_t *buffer); // 0x4e0790, blam-cc: EAX buffer
extern void network_server_password_get(network_server_globals *server, wchar_t *dest); // 0x4e0930, blam-cc: EAX server, ESI dest
extern int32_t network_server_password_is_set(network_server_globals *server); // 0x4e08e0, blam-cc: EAX server
extern int32_t network_machine_reset(network_machine *machine); // 0x4df690, blam-cc: ESI machine

char network_game_server_handle_join_password(network_machine *machine, network_server_globals *server, uint8_t *buffer,
    int32_t length)
{
    int16_t out_type;
    uint16_t out_version;
    uint32_t scratch[6];
    uint8_t body[0x90];
    int16_t remaining;
    int16_t reason;

    if (server->state != 0 && server->state != 1) {
        return 1;
    }
    remaining = (int16_t)(length - 2);
    if ((machine->flags & 0x02) != 0) {
        return 1;
    }
    if ((machine->channel == 0 || machine->channel->connected == 0) && server->game_over != 0) {
        char result = network_server_build_full_game_info_packet(machine);

        return result != 0 ? result : 1;
    }
    if (data_packet_group_decode_packet(&remaining, &network_game_messages_group, body, buffer + 2, &out_type, &out_version,
                                         3) == 0) {
        return 1;
    }
    network_channel_remote_address_or_default(machine->channel, (network_resolved_address *)scratch);
    if ((server->flags & 1) == 0 || (server->state != 0 && server->state != 1)) {
        reason = 0;
    } else if (network_join_request_reset_state(machine, (const char *)body + 0x22) == 0) {
        reason = 6;
    } else if ((network_debug_fill_canary_buffer(scratch), memcmp(body, scratch, 0x10)) != 0) {
        reason = 1;
    } else {
        network_server_password_get(server, (wchar_t *)scratch);
        *(uint16_t *)(body + 0x20) = 0;
        if (wcsncmp((const wchar_t *)(body + 0x10), (const wchar_t *)scratch, 8) != 0 && network_server_password_is_set(server) != 0) {
            reason = 2;
        } else if (network_machine_reset(machine) == 0 ||
                   network_game_session_finalize_and_add_player((network_player_entry *)(body + 0x6e), server, machine) == 0) {
            reason = 3;
        } else {
            uint32_t payload = 0;
            uint16_t *packet;

            network_game_broadcast_player_set_changed((uint8_t *)server);
            *(int32_t *)((uint8_t *)machine->channel + 0xa88) = network_game_info_packet_flag == 0 ? 4 : body[0x6b];
            packet = network_prepare_challenge_packet(0xa, &payload);
            if (packet != 0 && machine->machine_id != -1) {
                network_session_send_to_machine(machine->machine_id, server, 0, packet, (uint32_t)(*packet >> 4) << 3, 1, 0, 1, 3);
            }
            return 1;
        }
    }
    network_server_notify_or_resend_challenge(reason, machine, server);
    return 1;
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
