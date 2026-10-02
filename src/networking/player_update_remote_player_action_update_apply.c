// player_update_remote_player_action_update_apply  (Ghidra: FUN_004e5720; renamed, see evidence)
// address 0x4e5720, size 336 bytes
// name confidence: 0.4   rewrite confidence: 0.35
// evidence: out/phase4/networking_functions.md: "Applies a baseline or delta-encoded
// remote-player action update, structurally paralleling the client handler but invoked with a
// plain pointer argument."; shares its table/validation shape with this batch's
// player_update_client_remote_player_action_update_from_network.c (0x4e5620), which documents
// the PTR_DAT_00687558 remap table and the player_data validation pattern this file reuses.
// register convention: __cdecl, one stack parameter (Ghidra recovered it directly, unlike
// 0x4e5620's equivalent argument which arrives in EDX).
// REVIEW PASS 2026-09-20: rewired against the disassembly (objdump -d -M intel
// --start-address=0x4e5720 --stop-address=0x4e5872 bin/halo.exe). Four fixes:
//   - FUN_004ec590 takes the DECODE CONTEXT in EAX and the destination buffer in ECX
//     (0x4e5762 `mov eax,ebp` / 0x4e5764 `lea ecx,[esp+0x14]`). It was being handed the
//     message_delta_decode_state and no destination at all, which is also why the old note
//     below concluded the decoded record was never written anywhere visible: it is written,
//     through that ECX pointer;
//   - message_delta_read_changed_subfields takes the state in EDI (0x4e5851 `mov edi,ebx`);
//   - handle_remote_player_action_update takes the decoded record in EAX (0x4e57c6
//     `lea eax,[esp+0x1c]`); that argument was missing;
//   - the delta path has its own inner validation before it seeds the previous-state buffer
//     from the player, and that check includes local_player_index == -1 (0x4e581c
//     `cmp WORD PTR [eax+0x2],0xffff`), which the earlier rewrite dropped.
// is_baseline lives in the incoming parameter slot ([esp+0x78] == the caller stack slot holding
// decode_context) and is written one byte at a time, so the dword handed to
// handle_remote_player_action_update has three stale pointer bytes on top. The callee only tests
// AL, so only the byte is modelled.
// UNSURE: when the delta path fails its inner validation, the previous-state buffer is never
// initialized before being copied into the destination and handed to
// message_delta_read_changed_subfields -- preserved as a genuinely-uninitialized read.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <string.h>
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
extern int32_t message_delta_read_changed_subfields(message_delta_decode_state *state,
    void *field_bindings, const void *previous, void *destination);
    // blam-cc: EDI -> state, stack -> field_bindings, previous, destination; 0x4ed1d0
extern void handle_remote_player_action_update(remote_player_action_state *control_source,
    remote_player_update_header *header, uint8_t is_baseline); // this module, 0x4e60c0

// Looks up (and remaps in place) decode_context's remote-player index, decodes a baseline or
// delta action update into a local staging buffer (see the UNSURE notes for why the buffer's
// final contents are not the whole story), then re-validates the remapped index a second time --
// requiring it to name a live, non-local player -- before dispatching to
// handle_remote_player_action_update.
void player_update_remote_player_action_update_apply(int32_t **decode_context)
{
    remote_player_update_header *header;
    message_delta_decode_state *state;
    int32_t remapped_index;
    uint8_t is_baseline;
    remote_player_action_state decoded;  // [esp+0x14], the destination
    remote_player_action_state previous; // [esp+0x44], seeded from the player when it validates

    header = (remote_player_update_header *)decode_context[0x11];
    remapped_index = -1;
    if (header->player_index != 0) {
        int32_t *table_base = *(int32_t **)&machine_table->handles;
        remapped_index = table_base[header->player_index];
    }
    header->player_index = remapped_index;

    state = (message_delta_decode_state *)decode_context[0];
    if (state->incremental == 0) {
        memset(&decoded, 0, sizeof(decoded));
        is_baseline = 1;
        if (message_delta_decode_compound_field(decode_context, &decoded) != 1) {
            return;
        }
    } else {
        player *candidate;

        is_baseline = 0;
        candidate = 0;
        if (remapped_index != -1) {
            int16_t index = (int16_t)remapped_index;
            if (index >= 0 && index < player_data->maximum_count) {
                player *maybe = (player *)((uint8_t *)player_data->data
                    + (int32_t)player_data->size * (int32_t)index);
                int16_t salt = (int16_t)((uint32_t)remapped_index >> 16);
                if (maybe->identifier != 0 && (salt == 0 || maybe->identifier == salt)
                    && maybe->local_player_index == -1) {
                    candidate = maybe;
                }
            }
        }
        if (candidate != 0) {
            memcpy(&previous, &candidate->unknown_f0, sizeof(previous));
        }
        // UNSURE: previous is left uninitialized here when candidate == 0, see file header
        decoded = previous;
        state->bits_read += message_delta_read_changed_subfields(state, decode_context + 1,
            &previous, &decoded);
        state->changed = 1;
    }

    // Shared tail (0x4e5776): the remapped index is re-read out of the header and revalidated,
    // because the delta branch above may have been taken with a stale or invalid one.
    remapped_index = header->player_index;
    if (remapped_index != -1) {
        int16_t index = (int16_t)remapped_index;
        if (index >= 0 && index < player_data->maximum_count) {
            player *maybe = (player *)((uint8_t *)player_data->data
                + (int32_t)player_data->size * (int32_t)index);
            int16_t salt = (int16_t)((uint32_t)remapped_index >> 16);
            if (maybe->identifier != 0 && (salt == 0 || maybe->identifier == salt)
                && maybe->local_player_index == -1) {
                handle_remote_player_action_update(&decoded, header, is_baseline);
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x4e5720), from tools/pack.py 0x4e5720:

void FUN_004e5720(int *param_1)

{
  int *piVar1;
  int *piVar2;
  int *piVar3;
  char cVar4;
  int iVar5;
  short sVar6;
  int iVar7;
  short sVar8;
  undefined4 *puVar9;
  undefined4 *puVar10;
  undefined4 local_60 [12];
  undefined4 local_30 [12];

  piVar3 = param_1;
  piVar1 = (int *)param_1[0x11];
  iVar7 = -1;
  if (*piVar1 != 0) {
    iVar7 = *(int *)(*(int *)(PTR_DAT_00687558 + 0x28) + *piVar1 * 4);
  }
  *piVar1 = iVar7;
  piVar2 = (int *)*param_1;
  if (*piVar2 == 0) {
    puVar9 = local_60;
    for (iVar7 = 0xc; iVar7 != 0; iVar7 = iVar7 + -1) {
      *puVar9 = 0;
      puVar9 = puVar9 + 1;
    }
    param_1 = (int *)0x1;
    cVar4 = FUN_004ec590();
    if (cVar4 != '\x01') {
      return;
    }
  }
  else {
    param_1 = (int *)0x0;
    if (((iVar7 != -1) && (sVar6 = (short)iVar7, -1 < sVar6)) &&
       (sVar6 < *(short *)(DAT_0087a480 + 0x20))) {
      iVar5 = (int)*(short *)(DAT_0087a480 + 0x22) * (int)sVar6;
      sVar6 = *(short *)(iVar5 + *(int *)(DAT_0087a480 + 0x34));
      iVar5 = iVar5 + *(int *)(DAT_0087a480 + 0x34);
      if (((sVar6 != 0) && ((sVar8 = (short)((uint)iVar7 >> 0x10), sVar8 == 0 || (sVar6 == sVar8))))
         && (*(short *)(iVar5 + 2) == -1)) {
        puVar9 = (undefined4 *)(iVar5 + 0xf0);
        puVar10 = local_30;
        for (iVar7 = 0xc; iVar7 != 0; iVar7 = iVar7 + -1) {
          *puVar10 = *puVar9;
          puVar9 = puVar9 + 1;
          puVar10 = puVar10 + 1;
        }
      }
    }
    puVar9 = local_30;
    puVar10 = local_60;
    for (iVar7 = 0xc; iVar7 != 0; iVar7 = iVar7 + -1) {
      *puVar10 = *puVar9;
      puVar9 = puVar9 + 1;
      puVar10 = puVar10 + 1;
    }
    iVar7 = message_delta_read_changed_subfields(piVar3 + 1,local_30,local_60);
    piVar2[3] = piVar2[3] + iVar7;
    *(undefined1 *)((int)piVar2 + 0x1d) = 1;
  }
  iVar7 = *piVar1;
  if (((iVar7 != -1) && (sVar6 = (short)iVar7, -1 < sVar6)) &&
     (sVar6 < *(short *)(DAT_0087a480 + 0x20))) {
    iVar5 = (int)*(short *)(DAT_0087a480 + 0x22) * (int)sVar6;
    sVar6 = *(short *)(iVar5 + *(int *)(DAT_0087a480 + 0x34));
    if ((sVar6 != 0) &&
       (((sVar8 = (short)((uint)iVar7 >> 0x10), sVar8 == 0 || (sVar6 == sVar8)) &&
        (*(short *)(iVar5 + *(int *)(DAT_0087a480 + 0x34) + 2) == -1)))) {
      handle_remote_player_action_update(piVar1,param_1);
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
