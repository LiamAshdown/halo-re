// player_update_client_remote_player_position_delta_from_network  (Ghidra:
// player_update_client_remote_player_position_delta_from_network, already named)
// address 0x4e5c40, size 277 bytes
// name confidence: 0.55   rewrite confidence: 0.45
// evidence: out/phase4/networking_functions.md; shares its remap-table/validation shape with this
// batch's player_update_client_remote_player_action_update_from_network.c (0x4e5620); its final
// call pins player_update_client_remote_player_position_update_from_network.c's (this batch,
// 0x4e6270) first two parameters, confirming they are (new_update_id, a byte) rather than
// Ghidra's own guess for that function's signature.
// register convention: EAX -> decode_context, same slot-0/slot-0x11 shape as 0x4e5620, but
// arriving in EAX here rather than EDX.
//   // blam-cc: EAX -> decode_context
// REVIEW PASS 2026-09-20: rewired against the disassembly (objdump -d -M intel
// --start-address=0x4e5c40 --stop-address=0x4e5d60 bin/halo.exe). Three fixes:
//   - FUN_004ec590 / FUN_004ec600 take the DECODE CONTEXT in EAX and the staging buffer in ECX
//     (0x4e5cb7 and 0x4e5d02 `lea ecx,[esp+0x10]`/`[esp+0x14]`), not the decode state. That is
//     also what resolves the old "never actually written" note below: the three position floats
//     ARE written, by the decoder, through the ECX pointer this call site was not passing;
//   - player_update_client_remote_player_position_update_from_network takes the remapped player
//     datum in EAX (0x4e5d34 `mov eax,edi`). That argument was missing entirely;
//   - its first two stack arguments are single BYTES (`movzx edx,BYTE PTR [ebx+0x4]`,
//     `mov cl,BYTE PTR [ebx+0x5]`), not the whole dword at header+4.
// UNSURE: FUN_004ec600 is the forced-changed variant of the compound decoder; its single stack
// argument is 0 at both of its call sites in this family and was not chased further.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <stdint.h>
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
extern uint8_t message_delta_decode_compound_field_forced(void *decode_context, void *destination, int32_t force_changed);
    // blam-cc: EAX -> decode_context, ECX -> destination, stack -> force_changed; 0x4ec600
extern void message_delta_decode_compound_field_staged(void *decode_context);
    // blam-cc: EAX -> decode_context; 0x4ec670, the message-delta skip/drop path
extern void player_update_client_remote_player_position_update_from_network(
    datum_index player_index, int32_t update_id, int32_t control_sequence,
    real x, real y, real z); // this module, 0x4e6270

// Looks up (and remaps in place) decode_context's remote-player index, decodes a baseline or
// delta position update (player::unknown_164/168/16c), then forwards the result to
// player_update_client_remote_player_position_update_from_network.
void player_update_client_remote_player_position_delta_from_network(int32_t **decode_context) // blam-cc: EAX -> decode_context
{
    remote_player_update_header *header;
    int32_t remapped_index;
    int16_t index;
    int16_t salt;
    player *candidate;
    message_delta_decode_state *state;
    real_point3d position;

    header = (remote_player_update_header *)decode_context[0x11];
    remapped_index = -1;
    if (header->player_index != 0) {
        int32_t *table_base = *(int32_t **)&machine_table->handles;
        remapped_index = table_base[header->player_index];
    }
    // note: no write-back into header->player_index on this path

    candidate = 0;
    if (remapped_index != -1) {
        index = (int16_t)remapped_index;
        if (index >= 0 && index < player_data->maximum_count) {
            player *maybe = (player *)((uint8_t *)player_data->data + (int32_t)player_data->size * (int32_t)index);
            salt = (int16_t)((uint32_t)remapped_index >> 16);
            if (maybe->identifier != 0 && (salt == 0 || maybe->identifier == salt)) {
                candidate = maybe;
            }
        }
    }
    if (candidate == 0) {
        message_delta_decode_compound_field_staged(decode_context);
        return;
    }

    state = (message_delta_decode_state *)decode_context[0];
    if (state->incremental == 0) {
        if (message_delta_decode_compound_field(decode_context, &position) != 1) {
            return;
        }
        *(real *)&candidate->position_baseline_x = position.x;
        *(real *)&candidate->position_baseline_y = position.y;
        *(real *)&candidate->position_baseline_z = position.z;
    } else {
        position.x = *(real *)&candidate->position_baseline_x;
        position.y = *(real *)&candidate->position_baseline_y;
        position.z = *(real *)&candidate->position_baseline_z;
        if (message_delta_decode_compound_field_forced(decode_context, &position, 0) != 1) {
            return;
        }
    }
    player_update_client_remote_player_position_update_from_network(
        remapped_index, header->update_id, header->baseline_id,
        position.x, position.y, position.z);
}

#if 0
Original Ghidra decompilation (0x4e5c40), from tools/pack.py 0x4e5c40:

void player_update_client_remote_player_position_delta_from_network(void)

{
  int *piVar1;
  char cVar2;
  undefined4 *in_EAX;
  short sVar3;
  int iVar4;
  short sVar5;
  int iVar6;
  undefined4 local_c;
  undefined4 local_8;
  undefined4 local_4;

  piVar1 = (int *)in_EAX[0x11];
  iVar6 = -1;
  if (*piVar1 != 0) {
    iVar6 = *(int *)(*(int *)(PTR_DAT_00687558 + 0x28) + *piVar1 * 4);
  }
  if (((iVar6 != -1) && (sVar5 = (short)iVar6, -1 < sVar5)) &&
     (sVar5 < *(short *)(DAT_0087a480 + 0x20))) {
    iVar4 = (int)*(short *)(DAT_0087a480 + 0x22) * (int)sVar5;
    sVar5 = *(short *)(iVar4 + *(int *)(DAT_0087a480 + 0x34));
    iVar4 = iVar4 + *(int *)(DAT_0087a480 + 0x34);
    if ((sVar5 != 0) && ((sVar3 = (short)((uint)iVar6 >> 0x10), sVar3 == 0 || (sVar5 == sVar3)))) {
      if (*(int *)*in_EAX == 0) {
        cVar2 = FUN_004ec590();
        if (cVar2 != '\x01') {
          return;
        }
        *(undefined4 *)(iVar4 + 0x164) = local_c;
        *(undefined4 *)(iVar4 + 0x168) = local_8;
        *(undefined4 *)(iVar4 + 0x16c) = local_4;
      }
      else {
        local_c = *(undefined4 *)(iVar4 + 0x164);
        local_8 = *(undefined4 *)(iVar4 + 0x168);
        local_4 = *(undefined4 *)(iVar4 + 0x16c);
        cVar2 = FUN_004ec600(0);
        if (cVar2 != '\x01') {
          return;
        }
      }
      player_update_client_remote_player_position_update_from_network
                ((char)piVar1[1],*(undefined1 *)((int)piVar1 + 5),local_c,local_8,local_4);
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
