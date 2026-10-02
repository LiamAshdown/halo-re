// player_update_client_remote_player_total_biped_update_from_network  (Ghidra:
// player_update_client_remote_player_total_biped_update_from_network, already named)
// address 0x4e5870, size 440 bytes
// name confidence: 0.75   rewrite confidence: 0.75
// evidence: out/phase4/networking_functions.md; the two literal strings "stateless" /
// "incremental" and the format "Received %s total biped update [%d]." (0x0066e20c, 0x0066e200,
// 0x0066e1d8); types/game.h player (the 0xc-dword control run at +0xf0 and the three position
// dwords at +0x164); types/networking.h remote_player_biped_update_state, whose 0x3c size is
// fixed by this function's own `mov ecx,0xf ; rep stos` at 0x4e58fd.
// Written from the disassembly (objdump -d -M intel --start-address=0x4e5870
// --stop-address=0x4e5a30 bin/halo.exe), not from Ghidra's pseudo-C, which drops every register
// argument in the function and splits the one 15-dword staging buffer into
// local_78[12] + local_48 + local_44 + local_40 (and the previous-state buffer into
// local_3c[12] + local_c + local_8 + local_4) because it cannot prove they are one object.
// register convention: EDX -> decode_context, the pointer array whose slot 0 is the
// message_delta_decode_state, slot 1 onward the field bindings, slot 0x11 the
// remote_player_update_header.
//   // blam-cc: EDX -> decode_context
// UNSURE: the third argument handed to
// player_update_client_remote_player_position_update_from_network is header->control_sequence
// (the byte at +0x08) here, where the stand-alone
// player_update_client_remote_player_position_delta_from_network (0x4e5c40) passes
// header->baseline_id (the byte at +0x05) instead. Both readings are confirmed by disassembly
// (0x4e59f5 `mov dl,BYTE PTR [esi+0x8]` against 0x4e5d2a `mov cl,BYTE PTR [ebx+0x5]`); which of
// the two the callee slot player+0x15c is really meant to hold is not resolved.
// UNSURE: the dword the binary passes as handle_remote_player_action_update is_baseline is a
// stack slot whose upper three bytes are never initialized (only `mov BYTE PTR [esp+0x10],1`
// writes it; Ghidra models the same thing as CONCAT31 in the sibling 0x4e5a30). The callee reads
// it as a char, so the parameter is declared uint8_t and only the meaningful byte is passed.
// UNSURE: the eight dwords of remote_player_action_state between flags and direction are
// untyped; nothing in this batch reads them individually.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <string.h>
#include "objects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *player_data; // 0x0087a480
extern network_id_table *machine_table;

extern uint8_t message_delta_decode_compound_field(void *decode_context, void *destination);
    // blam-cc: EAX -> decode_context, ECX -> destination; 0x4ec590, message-delta stateless
    // (baseline) decode. It forwards to message_delta_read_changed_subfields with a NULL
    // previous-state pointer and the caller destination (0x4ec591..0x4ec59a).
extern void message_delta_decode_compound_field_staged(void *decode_context);
    // blam-cc: EAX -> decode_context; 0x4ec670, the message-delta skip/drop path
extern int32_t message_delta_read_changed_subfields(message_delta_decode_state *state,
    void *field_bindings, const void *previous, void *destination);
    // blam-cc: EDI -> state, stack -> field_bindings, previous, destination; 0x4ed1d0
extern void player_update_history_log_printf_filtered(player *target_player, int32_t category,
    const char *format, ...); // this module, 0x4e5f20
extern void handle_remote_player_action_update(remote_player_action_state *control_source,
    remote_player_update_header *header, uint8_t is_baseline); // this module, 0x4e60c0
extern void player_update_client_remote_player_position_update_from_network(
    datum_index player_index, int32_t update_id, int32_t control_sequence,
    real x, real y, real z); // this module, 0x4e6270

// Client-side handler for a combined ("total") remote-player biped update: remaps the wire
// player index in place, refuses anything that is not a live remote player, decodes the message
// either stateless (zeroed staging buffer, FUN_004ec590) or incrementally (staging buffer seeded
// from the player own control record and position, then message_delta_read_changed_subfields),
// then hands the two halves to handle_remote_player_action_update and
// player_update_client_remote_player_position_update_from_network.
void player_update_client_remote_player_total_biped_update_from_network(int32_t **decode_context)
    // blam-cc: EDX -> decode_context
{
    remote_player_update_header *header;
    message_delta_decode_state *state;
    int32_t remapped_index;
    int16_t index;
    int16_t salt;
    player *candidate;
    remote_player_biped_update_state decoded;
    remote_player_biped_update_state previous;
    uint8_t is_baseline;
    const char *mode;

    header = (remote_player_update_header *)decode_context[0x11];

    remapped_index = -1;
    if (header->player_index != 0) {
        int32_t *table_base = *(int32_t **)&machine_table->handles;
        remapped_index = table_base[header->player_index];
    }
    header->player_index = remapped_index;

    candidate = 0;
    if (remapped_index != -1) {
        index = (int16_t)remapped_index;
        if (index >= 0 && index < player_data->maximum_count) {
            player *maybe = (player *)((uint8_t *)player_data->data
                + (int32_t)player_data->size * (int32_t)index);
            salt = (int16_t)((uint32_t)remapped_index >> 16);
            if (maybe->identifier != 0 && (salt == 0 || maybe->identifier == salt)) {
                candidate = maybe;
            }
        }
    }
    // Only a genuinely remote player is accepted: local_player_index must still be -1.
    if (candidate == 0 || candidate->local_player_index != -1) {
        message_delta_decode_compound_field_staged(decode_context);
        return;
    }

    state = (message_delta_decode_state *)decode_context[0];
    if (state->incremental == 0) {
        uint8_t decoded_ok;

        memset(&decoded, 0, sizeof(decoded));
        decoded_ok = message_delta_decode_compound_field(decode_context, &decoded);
        if (decoded_ok == 1) {
            *(real *)&candidate->position_baseline_x = decoded.position.x;
            *(real *)&candidate->position_baseline_y = decoded.position.y;
            *(real *)&candidate->position_baseline_z = decoded.position.z;
        }
        is_baseline = 1;
        if (decoded_ok != 1) {
            return; // note: this path does NOT fall through to FUN_004ec670
        }
        mode = "stateless";
    } else {
        memcpy(&previous.action, &candidate->unknown_f0, sizeof(previous.action));
        previous.position.x = *(real *)&candidate->position_baseline_x;
        previous.position.y = *(real *)&candidate->position_baseline_y;
        previous.position.z = *(real *)&candidate->position_baseline_z;
        decoded = previous;
        state->bits_read += message_delta_read_changed_subfields(state, decode_context + 1,
            &previous, &decoded);
        state->changed = 1;
        is_baseline = 0;
        mode = "incremental";
    }

    player_update_history_log_printf_filtered(candidate, 3,
        "Received %s total biped update [%d].", mode, header->update_id);
    handle_remote_player_action_update(&decoded.action, header, is_baseline);
    player_update_client_remote_player_position_update_from_network(
        header->player_index, header->update_id, header->control_sequence,
        decoded.position.x, decoded.position.y, decoded.position.z);
}

#if 0
Original Ghidra decompilation (0x4e5870), from tools/pack.py 0x4e5870:

void player_update_client_remote_player_total_biped_update_from_network(void)

{
  int *piVar1;
  int *piVar2;
  char cVar3;
  short sVar4;
  int iVar5;
  char *pcVar6;
  int *in_EDX;
  int iVar7;
  undefined4 *puVar8;
  short sVar9;
  undefined4 *puVar10;
  undefined4 local_80;
  undefined4 local_78 [12];
  undefined4 local_48;
  undefined4 local_44;
  undefined4 local_40;
  undefined4 local_3c [12];
  undefined4 local_c;
  undefined4 local_8;
  undefined4 local_4;

  piVar1 = (int *)in_EDX[0x11];
  iVar5 = -1;
  if (*piVar1 != 0) {
    iVar5 = *(int *)(*(int *)(PTR_DAT_00687558 + 0x28) + *piVar1 * 4);
  }
  *piVar1 = iVar5;
  if (((iVar5 != -1) && (sVar4 = (short)iVar5, -1 < sVar4)) &&
     (sVar4 < *(short *)(DAT_0087a480 + 0x20))) {
    iVar7 = (int)*(short *)(DAT_0087a480 + 0x22) * (int)sVar4;
    sVar4 = *(short *)(iVar7 + *(int *)(DAT_0087a480 + 0x34));
    iVar7 = iVar7 + *(int *)(DAT_0087a480 + 0x34);
    if (((sVar4 != 0) && ((sVar9 = (short)((uint)iVar5 >> 0x10), sVar9 == 0 || (sVar4 == sVar9))))
       && (*(short *)(iVar7 + 2) == -1)) {
      piVar2 = (int *)*in_EDX;
      if (*piVar2 == 0) {
        puVar8 = local_78;
        for (iVar5 = 0xf; iVar5 != 0; iVar5 = iVar5 + -1) {
          *puVar8 = 0;
          puVar8 = puVar8 + 1;
        }
        cVar3 = FUN_004ec590();
        if (cVar3 == '\x01') {
          *(undefined4 *)(iVar7 + 0x164) = local_48;
          *(undefined4 *)(iVar7 + 0x168) = local_44;
          *(undefined4 *)(iVar7 + 0x16c) = local_40;
        }
        local_80 = 1;
        if (cVar3 != '\x01') {
          return;
        }
        pcVar6 = "stateless";
      }
      else {
        puVar8 = (undefined4 *)(iVar7 + 0xf0);
        puVar10 = local_3c;
        for (iVar5 = 0xc; iVar5 != 0; iVar5 = iVar5 + -1) {
          *puVar10 = *puVar8;
          puVar8 = puVar8 + 1;
          puVar10 = puVar10 + 1;
        }
        local_c = *(undefined4 *)(iVar7 + 0x164);
        local_8 = *(undefined4 *)(iVar7 + 0x168);
        local_4 = *(undefined4 *)(iVar7 + 0x16c);
        puVar8 = local_3c;
        puVar10 = local_78;
        for (iVar5 = 0xf; iVar5 != 0; iVar5 = iVar5 + -1) {
          *puVar10 = *puVar8;
          puVar8 = puVar8 + 1;
          puVar10 = puVar10 + 1;
        }
        iVar5 = message_delta_read_changed_subfields(in_EDX + 1,local_3c,local_78);
        piVar2[3] = piVar2[3] + iVar5;
        *(undefined1 *)((int)piVar2 + 0x1d) = 1;
        local_80 = 0;
        pcVar6 = "incremental";
      }
      player_update_history_log_printf_filtered
                (3,"Received %s total biped update [%d].",pcVar6,(char)piVar1[1]);
      handle_remote_player_action_update(piVar1,local_80);
      player_update_client_remote_player_position_update_from_network
                ((char)piVar1[1],(char)piVar1[2],local_48,local_44,local_40);
      return;
    }
  }
  FUN_004ec670();
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
