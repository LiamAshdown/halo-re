// player_update_client_remote_player_total_vehicle_update_from_network  (Ghidra:
// player_update_client_remote_player_total_vehicle_update_from_network, already named)
// address 0x4e5a30, size 519 bytes
// name confidence: 0.75   rewrite confidence: 0.7
// evidence: out/phase4/networking_functions.md; the strings "stateless" / "incremental" and the
// format "Received %s total vehicle update [%d]." (0x0066e1b0); types/game.h vehicle_update_body
// (0x40, forward at +0x28 and up at +0x34), which matches this function two cross-product /
// two normalize orthonormalization exactly; types/networking.h
// remote_player_vehicle_update_state, whose 0x70 size is fixed by the `mov ecx,0x1c ; rep stos`
// at 0x4e5ac6.
// Written from the disassembly (objdump -d -M intel --start-address=0x4e5a30
// --stop-address=0x4e5c40 bin/halo.exe). Ghidra fails badly on this one: it loses every
// register argument, turns the spilled stack slots into puStack_114 / pcStack_118 / puStack_11c
// pseudo-locals, and shows the four vector calls with no arguments at all.
// register convention: EDX -> decode_context (slot 0 = message_delta_decode_state, slot 1 onward
// = field bindings, slot 0x11 = remote_player_update_header).
//   // blam-cc: EDX -> decode_context
// The orthonormalization on the baseline path is
//     temp = forward x up ;  up = temp x forward ;  normalize(forward) ; normalize(up)
// vector3d_cross_product (0x4052c0) takes out in EAX, one operand in ECX and the other on the
// stack, and computes out = (stack operand) x (ECX operand) -- verified instruction by
// instruction at 0x4052c7..0x40530a. The same three cross products are open-coded inline by
// player_update_client_remote_player_vehicle_update_from_network (0x4e6510) on its own copy of
// the record, which is the independent confirmation of the operand order used here.
// UNSURE: the dword passed as handle_remote_player_action_update is_baseline is a stack slot
// that previously held the message_delta_decode_state pointer and is then overwritten one byte
// at a time (`mov BYTE PTR [esp+0x14],1`), so its upper three bytes are stale pointer bits --
// this is exactly the CONCAT31 Ghidra emits. Only the meaningful byte is passed here.
// UNSURE: vehicle_update_body::parent_or_tag (record dword 0) is carried through untouched by
// this function; it is the 0x4e6510 side that remaps it through the second remap table.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <string.h>

extern data_array *player_data; // 0x0087a480
extern void *remote_player_index_remap_table; // 0x00687558, table pointer at +0x28

extern uint8_t message_delta_decode_compound_field(void *decode_context, void *destination);
    // blam-cc: EAX -> decode_context, ECX -> destination; 0x4ec590, message-delta stateless
    // (baseline) decode. It forwards to message_delta_read_changed_subfields with a NULL
    // previous-state pointer and the caller destination (0x4ec591..0x4ec59a).
extern void message_delta_decode_compound_field_staged(void *decode_context);
    // blam-cc: EAX -> decode_context; 0x4ec670, the message-delta skip/drop path
extern int32_t message_delta_read_changed_subfields(message_delta_decode_state *state,
    void *field_bindings, const void *previous, void *destination);
    // blam-cc: EDI -> state, stack -> field_bindings, previous, destination; 0x4ed1d0
extern void vector3d_cross_product(real_vector3d *out, const real_vector3d *a,
    const real_vector3d *b); // 0x4052c0, math module; blam-cc: EAX -> out, ECX -> a, stack -> b
                             // computes out = b x a (see file header)
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990; blam-cc: ECX -> v
extern void player_update_history_log_printf_filtered(player *target_player, int32_t category,
    const char *format, ...); // this module, 0x4e5f20
extern void handle_remote_player_action_update(remote_player_action_state *control_source,
    remote_player_update_header *header, uint8_t is_baseline); // this module, 0x4e60c0
extern void player_update_client_remote_player_vehicle_update_from_network(
    datum_index player_index, int32_t update_id, int32_t control_sequence,
    vehicle_update_body vehicle); // this module, 0x4e6510

// Client-side handler for a combined ("total") remote-player vehicle update: remaps the wire
// player index in place, refuses anything that is not a live remote player, decodes the message
// stateless or incrementally into one 0x70 staging block (action record plus vehicle body),
// orthonormalizes and latches the vehicle body onto the player on the baseline path, and hands
// the two halves to handle_remote_player_action_update and
// player_update_client_remote_player_vehicle_update_from_network.
void player_update_client_remote_player_total_vehicle_update_from_network(int32_t **decode_context)
    // blam-cc: EDX -> decode_context
{
    remote_player_update_header *header;
    message_delta_decode_state *state;
    int32_t remapped_index;
    int16_t index;
    int16_t salt;
    player *candidate;
    remote_player_vehicle_update_state decoded;
    remote_player_vehicle_update_state previous;
    uint8_t is_baseline;
    const char *mode;

    header = (remote_player_update_header *)decode_context[0x11];

    remapped_index = -1;
    if (header->player_index != 0) {
        int32_t *table_base = *(int32_t **)((uint8_t *)remote_player_index_remap_table + 0x28);
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
            real_vector3d temp;

            vector3d_cross_product(&temp, &decoded.vehicle.up, &decoded.vehicle.forward);
            vector3d_cross_product(&decoded.vehicle.up, &decoded.vehicle.forward, &temp);
            vector3d_normalize_with_length(&decoded.vehicle.forward);
            vector3d_normalize_with_length(&decoded.vehicle.up);
            memcpy(&candidate->unknown_190, &decoded.vehicle, sizeof(decoded.vehicle));
        }
        is_baseline = 1;
        if (decoded_ok != 1) {
            return; // note: this path does NOT fall through to FUN_004ec670
        }
        mode = "stateless";
    } else {
        memcpy(&previous.action, &candidate->unknown_f0, sizeof(previous.action));
        memcpy(&previous.vehicle, &candidate->unknown_190, sizeof(previous.vehicle));
        decoded = previous;
        state->bits_read += message_delta_read_changed_subfields(state, decode_context + 1,
            &previous, &decoded);
        state->changed = 1;
        is_baseline = 0;
        mode = "incremental";
    }

    player_update_history_log_printf_filtered(candidate, 3,
        "Received %s total vehicle update [%d].", mode, header->update_id);
    handle_remote_player_action_update(&decoded.action, header, is_baseline);
    player_update_client_remote_player_vehicle_update_from_network(
        header->player_index, header->update_id, header->control_sequence, decoded.vehicle);
}

#if 0
Original Ghidra decompilation (0x4e5a30), from tools/pack.py 0x4e5a30:

void player_update_client_remote_player_total_vehicle_update_from_network(void)

{
  int *piVar1;
  int *piVar2;
  char cVar3;
  short sVar4;
  int iVar5;
  int iVar6;
  uint *in_EDX;
  short *psVar7;
  short *psVar8;
  undefined4 *puVar9;
  undefined4 *puVar10;
  undefined4 auStack_150 [9];
  undefined4 uStack_12c;
  int *piStack_128;
  uint uStack_124;
  undefined4 uStack_120;
  uint *puStack_11c;
  char *pcStack_118;
  undefined4 *puStack_114;
  uint local_fc;
  undefined1 local_f4 [12];
  undefined4 local_e8 [12];
  undefined4 local_b8 [10];
  undefined1 local_90 [24];
  undefined4 local_78 [12];
  undefined4 local_48 [17];

  piVar1 = (int *)in_EDX[0x11];
  iVar5 = -1;
  if (*piVar1 != 0) {
    iVar5 = *(int *)(*(int *)(PTR_DAT_00687558 + 0x28) + *piVar1 * 4);
  }
  *piVar1 = iVar5;
  if (((iVar5 != -1) && (sVar4 = (short)iVar5, -1 < sVar4)) &&
     (sVar4 < *(short *)(DAT_0087a480 + 0x20))) {
    psVar7 = (short *)((int)*(short *)(DAT_0087a480 + 0x22) * (int)sVar4 +
                      *(int *)(DAT_0087a480 + 0x34));
    if (((*psVar7 != 0) &&
        ((sVar4 = (short)((uint)iVar5 >> 0x10), sVar4 == 0 || (*psVar7 == sVar4)))) &&
       (psVar7[1] == -1)) {
      piVar2 = (int *)*in_EDX;
      if (*piVar2 == 0) {
        puVar9 = local_e8;
        for (iVar5 = 0x1c; iVar5 != 0; iVar5 = iVar5 + -1) {
          *puVar9 = 0;
          puVar9 = puVar9 + 1;
        }
        puStack_114 = (undefined4 *)0x4e5adc;
        cVar3 = FUN_004ec590();
        if (cVar3 == '\x01') {
          puStack_114 = (undefined4 *)local_90;
          pcStack_118 = (char *)0x4e5afc;
          vector3d_cross_product();
          pcStack_118 = local_f4;
          puStack_11c = (uint *)0x4e5b14;
          vector3d_cross_product();
          puStack_114 = (undefined4 *)0x4e5b23;
          vector3d_normalize_with_length();
          puStack_114 = (undefined4 *)0x4e5b31;
          vector3d_normalize_with_length();
          puVar9 = local_b8;
          psVar7 = psVar7 + 200;
          for (iVar5 = 0x10; iVar5 != 0; iVar5 = iVar5 + -1) {
            *(undefined4 *)psVar7 = *puVar9;
            puVar9 = puVar9 + 1;
            psVar7 = psVar7 + 2;
          }
        }
        local_fc = CONCAT31((int3)((uint)piVar2 >> 8),1);
        if (cVar3 != '\x01') {
          return;
        }
        pcStack_118 = "stateless";
      }
      else {
        psVar8 = psVar7 + 0x78;
        puVar9 = local_78;
        for (iVar5 = 0xc; iVar5 != 0; iVar5 = iVar5 + -1) {
          *puVar9 = *(undefined4 *)psVar8;
          psVar8 = psVar8 + 2;
          puVar9 = puVar9 + 1;
        }
        psVar7 = psVar7 + 200;
        puVar9 = local_48;
        for (iVar5 = 0x10; iVar5 != 0; iVar5 = iVar5 + -1) {
          *puVar9 = *(undefined4 *)psVar7;
          psVar7 = psVar7 + 2;
          puVar9 = puVar9 + 1;
        }
        puVar9 = local_78;
        puVar10 = local_e8;
        for (iVar5 = 0x1c; iVar5 != 0; iVar5 = iVar5 + -1) {
          *puVar10 = *puVar9;
          puVar9 = puVar9 + 1;
          puVar10 = puVar10 + 1;
        }
        puStack_114 = local_e8;
        pcStack_118 = (char *)local_78;
        puStack_11c = in_EDX + 1;
        uStack_120 = 0x4e5bb2;
        iVar5 = message_delta_read_changed_subfields();
        piVar2[3] = piVar2[3] + iVar5;
        *(undefined1 *)((int)piVar2 + 0x1d) = 1;
        local_fc = (uint)piVar2 & 0xffffff00;
        pcStack_118 = "incremental";
      }
      puStack_114 = (undefined4 *)(uint)*(byte *)(piVar1 + 1);
      puStack_11c = (uint *)0x66e1b0;
      uStack_120 = 3;
      uStack_124 = 0x4e5be7;
      player_update_history_log_printf_filtered();
      uStack_124 = local_fc;
      uStack_12c = 0x4e5bf6;
      piStack_128 = piVar1;
      handle_remote_player_action_update();
      iVar5 = piVar1[1];
      puVar9 = local_b8;
      puVar10 = auStack_150;
      for (iVar6 = 0x10; iVar6 != 0; iVar6 = iVar6 + -1) {
        *puVar10 = *puVar9;
        puVar9 = puVar9 + 1;
        puVar10 = puVar10 + 1;
      }
      player_update_client_remote_player_vehicle_update_from_network((char)iVar5,(char)piVar1[2]);
      return;
    }
  }
  puStack_114 = (undefined4 *)0x4e5c30;
  FUN_004ec670();
  return;
}
#endif
