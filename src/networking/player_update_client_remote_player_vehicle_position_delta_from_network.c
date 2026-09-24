// player_update_client_remote_player_vehicle_position_delta_from_network  (Ghidra:
// player_update_client_remote_player_vehicle_position_delta_from_network, already named)
// address 0x4e5d60, size 312 bytes
// name confidence: 0.55   rewrite confidence: 0.7
// evidence: out/phase4/networking_functions.md; structurally the vehicle twin of
// player_update_client_remote_player_position_delta_from_network (0x4e5c40, this module) and the
// stand-alone half of player_update_client_remote_player_total_vehicle_update_from_network
// (0x4e5a30, this module) -- same remap, same 0x40-byte vehicle_update_body staged at
// player+0x190, same orthonormalization, same forward to 0x4e6510.
// Written from the disassembly (objdump -d -M intel --start-address=0x4e5d60
// --stop-address=0x4e5ea0 bin/halo.exe); Ghidra drops the register arguments and shows the two
// cross products and two normalizes with no operands at all.
// register convention: EAX -> decode_context (slot 0 = message_delta_decode_state, slot 0x11 =
// remote_player_update_header). Unlike the two "total" handlers this one arrives in EAX, not EDX.
//   // blam-cc: EAX -> decode_context
// Three differences from the "total" vehicle handler, all confirmed in the disassembly and all
// deliberate in the original:
//   1. the remapped index is NOT written back into header->player_index (no store at 0x4e5d80);
//   2. there is no local_player_index == -1 gate, so this message is accepted for a local player
//      too (0x4e5dd1 follows the salt check directly);
//   3. the staging buffer is not zeroed before the baseline decode, so any subfield
//      FUN_004ec590 does not write stays uninitialized. Preserved as written rather than
//      "fixed".
// UNSURE: FUN_004ec600 is the forced-changed variant of the compound decoder; its ECX
// destination and its single stack argument (always 0 here) are read off this call site and off
// 0x4e5c40, not off its own body.
// UNSURE: the second argument forwarded to 0x4e6510 is header->baseline_id (byte at +0x05) here,
// where the "total" handler passes header->control_sequence (byte at +0x08). See
// player_update_client_remote_player_total_biped_update_from_network.c for the same split.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <string.h>

extern data_array *player_data; // 0x0087a480
extern void *remote_player_index_remap_table; // 0x00687558, table pointer at +0x28

extern uint8_t FUN_004ec590(void *decode_context, void *destination);
    // blam-cc: EAX -> decode_context, ECX -> destination; 0x4ec590, message-delta stateless
    // (baseline) decode. It forwards to message_delta_read_changed_subfields with a NULL
    // previous-state pointer and the caller destination (0x4ec591..0x4ec59a).
extern uint8_t FUN_004ec600(void *decode_context, void *destination, int32_t force_changed);
    // blam-cc: EAX -> decode_context, ECX -> destination, stack -> force_changed; 0x4ec600
extern void FUN_004ec670(void *decode_context);
    // blam-cc: EAX -> decode_context; 0x4ec670, the message-delta skip/drop path
extern void vector3d_cross_product(real_vector3d *out, const real_vector3d *a,
    const real_vector3d *b); // 0x4052c0; blam-cc: EAX -> out, ECX -> a, stack -> b;
                             // computes out = b x a
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990; blam-cc: ECX -> v
extern void player_update_client_remote_player_vehicle_update_from_network(
    datum_index player_index, int32_t update_id, int32_t control_sequence,
    vehicle_update_body vehicle); // this module, 0x4e6510

// Client-side handler for a stand-alone remote-player vehicle position update: looks the player
// up through the remap table, decodes the 0x40-byte vehicle body either stateless (and then
// orthonormalizes it and latches it onto the player) or as a delta against the copy already on
// the player, and forwards the result to
// player_update_client_remote_player_vehicle_update_from_network.
void player_update_client_remote_player_vehicle_position_delta_from_network(int32_t **decode_context)
    // blam-cc: EAX -> decode_context
{
    remote_player_update_header *header;
    message_delta_decode_state *state;
    int32_t remapped_index;
    int16_t index;
    int16_t salt;
    player *candidate;
    vehicle_update_body decoded; // UNSURE: not zeroed before the baseline decode, see header

    header = (remote_player_update_header *)decode_context[0x11];

    remapped_index = -1;
    if (header->player_index != 0) {
        int32_t *table_base = *(int32_t **)((uint8_t *)remote_player_index_remap_table + 0x28);
        remapped_index = table_base[header->player_index];
    }
    // note: no write-back into header->player_index on this path

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
    if (candidate == 0) {
        FUN_004ec670(decode_context);
        return;
    }

    state = (message_delta_decode_state *)decode_context[0];
    if (state->incremental == 0) {
        real_vector3d temp;

        if (FUN_004ec590(decode_context, &decoded) != 1) {
            return;
        }
        vector3d_cross_product(&temp, &decoded.up, &decoded.forward);
        vector3d_cross_product(&decoded.up, &decoded.forward, &temp);
        vector3d_normalize_with_length(&decoded.forward);
        vector3d_normalize_with_length(&decoded.up);
        memcpy(&candidate->unknown_190, &decoded, sizeof(decoded));
    } else {
        memcpy(&decoded, &candidate->unknown_190, sizeof(decoded));
        if (FUN_004ec600(decode_context, &decoded, 0) != 1) {
            return;
        }
    }

    player_update_client_remote_player_vehicle_update_from_network(
        remapped_index, header->update_id, header->baseline_id, decoded);
}

#if 0
Original Ghidra decompilation (0x4e5d60), from tools/pack.py 0x4e5d60:

void player_update_client_remote_player_vehicle_position_delta_from_network(void)

{
  int *piVar1;
  char cVar2;
  undefined4 *in_EAX;
  short sVar3;
  short sVar4;
  int iVar5;
  int iVar6;
  undefined4 *puVar7;
  undefined4 *puVar8;
  undefined4 auStack_9c [13];
  undefined4 uStack_68;
  undefined1 *puStack_64;
  undefined1 *puStack_60;
  undefined1 local_4c [12];
  undefined4 local_40 [10];
  undefined1 local_18 [24];

  piVar1 = (int *)in_EAX[0x11];
  iVar5 = -1;
  if (*piVar1 != 0) {
    iVar5 = *(int *)(*(int *)(PTR_DAT_00687558 + 0x28) + *piVar1 * 4);
  }
  if (((iVar5 != -1) && (sVar4 = (short)iVar5, -1 < sVar4)) &&
     (sVar4 < *(short *)(DAT_0087a480 + 0x20))) {
    iVar6 = (int)*(short *)(DAT_0087a480 + 0x22) * (int)sVar4;
    sVar4 = *(short *)(iVar6 + *(int *)(DAT_0087a480 + 0x34));
    iVar6 = iVar6 + *(int *)(DAT_0087a480 + 0x34);
    if ((sVar4 != 0) && ((sVar3 = (short)((uint)iVar5 >> 0x10), sVar3 == 0 || (sVar4 == sVar3)))) {
      if (*(int *)*in_EAX == 0) {
        puStack_60 = (undefined1 *)0x4e5de1;
        cVar2 = FUN_004ec590();
        if (cVar2 != '\x01') {
          return;
        }
        puStack_60 = local_18;
        puStack_64 = (undefined1 *)0x4e5dfb;
        vector3d_cross_product();
        puStack_64 = local_4c;
        uStack_68 = 0x4e5e0d;
        vector3d_cross_product();
        puStack_60 = (undefined1 *)0x4e5e19;
        vector3d_normalize_with_length();
        puStack_60 = (undefined1 *)0x4e5e24;
        vector3d_normalize_with_length();
        puVar7 = local_40;
        puVar8 = (undefined4 *)(iVar6 + 400);
        for (iVar5 = 0x10; iVar5 != 0; iVar5 = iVar5 + -1) {
          *puVar8 = *puVar7;
          puVar7 = puVar7 + 1;
          puVar8 = puVar8 + 1;
        }
      }
      else {
        puVar7 = (undefined4 *)(iVar6 + 400);
        puVar8 = local_40;
        for (iVar5 = 0x10; iVar5 != 0; iVar5 = iVar5 + -1) {
          *puVar8 = *puVar7;
          puVar7 = puVar7 + 1;
          puVar8 = puVar8 + 1;
        }
        puStack_60 = (undefined1 *)0x0;
        puStack_64 = (undefined1 *)0x4e5e57;
        cVar2 = FUN_004ec600();
        if (cVar2 != '\x01') {
          return;
        }
      }
      iVar5 = piVar1[1];
      puVar7 = local_40;
      puVar8 = auStack_9c;
      for (iVar6 = 0x10; iVar6 != 0; iVar6 = iVar6 + -1) {
        *puVar8 = *puVar7;
        puVar7 = puVar7 + 1;
        puVar8 = puVar8 + 1;
      }
      player_update_client_remote_player_vehicle_update_from_network
                ((char)iVar5,*(undefined1 *)((int)piVar1 + 5));
      return;
    }
  }
  puStack_60 = (undefined1 *)0x4e5e90;
  FUN_004ec670();
  return;
}
#endif
