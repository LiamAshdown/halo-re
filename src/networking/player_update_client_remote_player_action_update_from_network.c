// player_update_client_remote_player_action_update_from_network  (Ghidra:
// player_update_client_remote_player_action_update_from_network, already named)
// address 0x4e5620, size 251 bytes
// name confidence: 0.55   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md; types/game.h player (the 12-dword control-record
// run from unknown_f0 through unknown_11c, which ends exactly at player+0x120 where
// player_update_queue update_history begins -- this function's own 12-dword (0x30 byte) copy
// loop is what pins that run as one contiguous block); types/networking.h/memory.h data_array.
// register convention: EDX -> decode_context, an array of pointers where slot 0 is the same
// record_ctx used throughout this family (mode flag at *record_ctx) and slot 0x11 is a pointer to
// the remote player's raw index field. Ghidra recovered this one directly (`int *in_EDX`), unlike
// the sibling functions where the equivalent argument arrives in EAX.
//   // blam-cc: EDX -> decode_context
// REVIEW PASS 2026-09-20: three argument-wiring errors fixed against the disassembly
// (objdump -d -M intel --start-address=0x4e5620 --stop-address=0x4e5720 bin/halo.exe):
//   - FUN_004ec590 takes the DECODE CONTEXT in EAX (0x4e56a0 `mov eax,edx`) and the staging
//     buffer in ECX (0x4e56a2 `lea ecx,[esp+0x10]`), not the message_delta_decode_state. The
//     decoder itself then loads state from context slot 0 (0x4ec591);
//   - message_delta_read_changed_subfields takes the state in EDI (0x4e56e2 `mov edi,ebx`),
//     which 0x4ed1d0 reads immediately (`mov eax,[edi+0x4]`). That argument was missing;
//   - handle_remote_player_action_update takes the staging buffer in EAX (0x4e56b2 /
//     0x4e56f1 `lea eax,[esp+...]`). Both calls were missing it entirely, which would have
//     applied whatever happened to be in EAX as the control record.
// UNSURE: PTR_DAT_00687558's exact shape; same lookup pattern as
// player_update_client_local_player_vehicle_update_from_network.c's vehicle_seat_lookup_table
// (this batch) at a different table address, so named analogously.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <string.h>
#include "objects.h"

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
extern void handle_remote_player_action_update(remote_player_action_state *control_source,
    remote_player_update_header *header, uint8_t is_baseline); // this module, 0x4e60c0

// Handles an incoming remote-player action update (baseline or delta-encoded), applying the
// decoded 12-dword control record directly onto the target player object's control-record block
// (player::unknown_f0 .. unknown_11c) before dispatching to handle_remote_player_action_update.
void player_update_client_remote_player_action_update_from_network(int32_t **decode_context) // blam-cc: EDX -> decode_context
{
    remote_player_update_header *header;
    message_delta_decode_state *state;
    int32_t remapped_index;
    int16_t index;
    int16_t salt;
    player *candidate;
    remote_player_action_state staged;

    header = (remote_player_update_header *)decode_context[0x11];
    memset(&staged, 0, sizeof(staged));

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
            player *maybe = (player *)((uint8_t *)player_data->data + (int32_t)player_data->size * (int32_t)index);
            salt = (int16_t)((uint32_t)remapped_index >> 16);
            if (maybe->identifier != 0 && (salt == 0 || maybe->identifier == salt)) {
                candidate = maybe;
            }
        }
    }

    if (candidate != 0) {
        state = (message_delta_decode_state *)decode_context[0];
        if (state->incremental != 0) {
            const void *control_record = &candidate->unknown_f0;

            memcpy(&staged, control_record, sizeof(staged));
            state->bits_read += message_delta_read_changed_subfields(state, decode_context + 1,
                control_record, &staged);
            state->changed = 1;
            handle_remote_player_action_update(&staged, header, 0);
            return;
        }
        if (message_delta_decode_compound_field(decode_context, &staged) == 1) {
            handle_remote_player_action_update(&staged, header, 1);
        }
        return;
    }
    message_delta_decode_compound_field_staged(decode_context);
}

#if 0
Original Ghidra decompilation (0x4e5620), from tools/pack.py 0x4e5620:

void player_update_client_remote_player_action_update_from_network(void)

{
  int *piVar1;
  int *piVar2;
  char cVar3;
  int iVar4;
  undefined4 *puVar5;
  short sVar6;
  int iVar7;
  int *in_EDX;
  undefined4 *puVar8;
  short sVar9;
  undefined4 *puVar10;
  undefined4 local_30 [12];

  piVar1 = (int *)in_EDX[0x11];
  puVar8 = local_30;
  for (iVar7 = 0xc; iVar7 != 0; iVar7 = iVar7 + -1) {
    *puVar8 = 0;
    puVar8 = puVar8 + 1;
  }
  iVar7 = -1;
  if (*piVar1 != 0) {
    iVar7 = *(int *)(*(int *)(PTR_DAT_00687558 + 0x28) + *piVar1 * 4);
  }
  *piVar1 = iVar7;
  if (((iVar7 != -1) && (sVar6 = (short)iVar7, -1 < sVar6)) &&
     (sVar6 < *(short *)(DAT_0087a480 + 0x20))) {
    iVar4 = (int)*(short *)(DAT_0087a480 + 0x22) * (int)sVar6;
    sVar6 = *(short *)(iVar4 + *(int *)(DAT_0087a480 + 0x34));
    if ((sVar6 != 0) && ((sVar9 = (short)((uint)iVar7 >> 0x10), sVar9 == 0 || (sVar6 == sVar9)))) {
      piVar2 = (int *)*in_EDX;
      if (*piVar2 != 0) {
        puVar5 = (undefined4 *)(iVar4 + *(int *)(DAT_0087a480 + 0x34) + 0xf0);
        puVar8 = puVar5;
        puVar10 = local_30;
        for (iVar7 = 0xc; iVar7 != 0; iVar7 = iVar7 + -1) {
          *puVar10 = *puVar8;
          puVar8 = puVar8 + 1;
          puVar10 = puVar10 + 1;
        }
        iVar7 = message_delta_read_changed_subfields(in_EDX + 1,puVar5,local_30);
        piVar2[3] = piVar2[3] + iVar7;
        *(undefined1 *)((int)piVar2 + 0x1d) = 1;
        handle_remote_player_action_update(piVar1,0);
        return;
      }
      cVar3 = FUN_004ec590();
      if (cVar3 != '\x01') {
        return;
      }
      handle_remote_player_action_update(piVar1,1);
      return;
    }
  }
  FUN_004ec670();
  return;
}
#endif
