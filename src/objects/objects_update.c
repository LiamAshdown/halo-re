// objects_update
// address 0x4f4e90, size 592 bytes
// name confidence: 0.7 (types/objects.h attributes this exact address to "objects_update" in
//   three separate places: the object 0x04 cluster-index note, the object_header flags enum
//   comments for 0x04/0x08/0x10/0x40, and the "objects_update" name used directly in the
//   object_header_flags block)
// rewrite confidence: 0.4
// evidence: types/objects.h object_globals (unknown_04, cluster_pvs_previous[16],
//   cluster_pvs_current[16]); object_header (identifier, flags -- active/needs_update/
//   delete_pending/just_created/connected/in_pvs_pass bits all match this function's own bit
//   tests exactly -- cluster_index, data); data_array (data, last_index); global 0x008603b0
//   object_data; global 0x006b8cbc object_globals_pointer; foreign globals 0x006f1d6c ("the
//   game time globals, +0x0c is the current tick"), 0x0087a478 ("the BSP cluster PVS source
//   copied into object_globals.cluster_pvs_current"), both from types/objects.h's own
//   "globals this module reads but does not own" list; callees object_mark_pending_delete
//   (0x4f50f0), object_clear_pending_delete_flag (0x4f5130), object_delete (0x4f5bd0),
//   object_update (0x4f7ef0), objects_garbage_collection (0x4f9c60), all named already.
// register convention: none (void), matches the other module init/update entry points.
// UNSURE: 0x00746f9c (used here only at +0x134, the live cluster count) and 0x006b0b80 (used
//   only at +2, a single gating byte) are foreign-module globals with no established layout;
//   kept as opaque byte pointers. UNSURE: the object+0x218 test in the second sweep reads into
//   the unit-type extension past the common object header (types/objects.h explicitly notes
//   that region belongs to the unit extension, not this module), so it is left as a raw offset.
// simplification: the original's two "copy N dwords then M remaining bytes" unrolled-memcpy
//   pairs always have M == 0 at this call site (the remainder loops are entered with a
//   hard-coded count of 0), so those dead tail loops are omitted here; behavior is identical.
//   The final byte-by-byte PVS compare is likewise folded into a dword-granularity compare
//   (byte_count is always word_count*4, so the two give identical equal/not-equal results).
// reconciled: R32 hs_game_time_globals -> game.h game_time_globals (current_tick->game_time, budget_flag_1/2->active/paused, seconds_per_tick->leftover_time; same offsets)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "hs.h"
#include "objects.h"

extern object_globals *object_globals_pointer; // 0x006b8cbc
extern data_array *object_data; // 0x008603b0
extern game_time_globals *game_time; // 0x006f1d6c, UNSURE: foreign module, +0xc is the current tick
extern uint8_t *bsp_cluster_pvs_source; // 0x0087a478, UNSURE: foreign module, +0x18 is the pvs bits
extern uint8_t *global_structure_bsp; // 0x00746f9c, UNSURE: foreign module, +0x134 is the
                                              //   live cluster count
extern uint8_t *object_update_gate_globals; // 0x006b0b80, UNSURE: foreign module, +0x2 is a
                                            //   single gating byte

extern void object_mark_pending_delete(uint32_t object_index); // 0x4f50f0, this batch
extern void object_clear_pending_delete_flag(uint32_t object_index); // 0x4f5130, this batch
extern void object_delete(uint32_t object_index); // 0x4f5bd0, this batch
extern void object_update(uint32_t object_index); // 0x4f7ef0
extern void object_delete_4f9030(uint32_t object_index, char recurse_siblings); // 0x4f9030
extern void objects_garbage_collection(void); // 0x4f9c60
extern void structure_decals_update_switch_transitions(void *previous_pvs, void *current_pvs, int32_t cluster_count); // 0x5530d0

void objects_update(void)
{
    object_globals *globals = object_globals_pointer;
    int restrict_to_units;
    int16_t cluster_count;
    int32_t word_count;
    int32_t i;
    int changed;
    object_header *headers;
    int16_t last_index;

    restrict_to_units = (*(uint8_t *)((uint8_t *)game_time + 0xc) & 1) != 0 && object_update_gate_globals[2] != 0;

    globals->unknown_04 = 0;

    cluster_count = *(int16_t *)(global_structure_bsp + 0x134);
    word_count = (cluster_count + 0x1f) >> 5;

    for (i = 0; i < word_count; i++) {
        globals->cluster_pvs_previous[i] = globals->cluster_pvs_current[i];
    }
    for (i = 0; i < word_count; i++) {
        globals->cluster_pvs_current[i] = *(uint32_t *)(bsp_cluster_pvs_source + 0x18 + i * 4);
    }

    changed = 0;
    for (i = 0; i < word_count; i++) {
        if (globals->cluster_pvs_previous[i] != globals->cluster_pvs_current[i]) {
            changed = 1;
            break;
        }
    }

    if (changed) {
        headers = (object_header *)object_data->data;
        last_index = object_data->last_index;
        for (i = 0; i < last_index; i++) {
            object_header *header = &headers[i];
            uint8_t flags;
            uint32_t handle;

            if (header->identifier == 0) {
                continue;
            }
            flags = header->flags;
            if ((flags & (_object_header_in_pvs_pass_bit | _object_header_connected_bit)) !=
                (_object_header_in_pvs_pass_bit | _object_header_connected_bit)) {
                continue;
            }
            handle = ((uint32_t)header->identifier << 16) | (uint16_t)i;
            if ((flags & _object_header_active_bit) == 0) {
                if ((int8_t)flags >= 0 && header->cluster_index != -1 &&
                    (globals->cluster_pvs_current[header->cluster_index >> 5] &
                     (1u << (header->cluster_index & 0x1f))) != 0) {
                    object_mark_pending_delete(handle);
                }
            } else if ((globals->cluster_pvs_current[header->cluster_index >> 5] &
                        (1u << (header->cluster_index & 0x1f))) == 0) {
                if ((header->data->flags & _object_connected_to_map_bit) == 0) {
                    object_clear_pending_delete_flag(handle);
                } else {
                    object_delete(handle);
                }
            }
        }
        structure_decals_update_switch_transitions(globals->cluster_pvs_previous, globals->cluster_pvs_current, cluster_count);
    }

    headers = (object_header *)object_data->data;
    last_index = object_data->last_index;
    for (i = 0; i < last_index; i++) {
        object_header *header = &headers[i];
        if (header->identifier != 0 && (header->flags & _object_header_active_bit) != 0 &&
            (header->flags & _object_header_needs_update_bit) == 0) {
            if (!restrict_to_units ||
                (((1 << (header->type & 0x1f)) & _object_mask_unit) != 0 &&
                 *(int32_t *)((uint8_t *)header->data + 0x218) != -1)) {
                object_update(((uint32_t)header->identifier << 16) | (uint16_t)i);
            }
        }
    }

    headers = (object_header *)object_data->data;
    last_index = object_data->last_index;
    for (i = 0; i < last_index; i++) {
        object_header *header = &headers[i];
        if (header->identifier != 0) {
            uint8_t original_flags = header->flags;
            uint32_t handle = ((uint32_t)header->identifier << 16) | (uint16_t)i;

            header->flags = original_flags & (uint8_t)~_object_header_just_created_bit;
            if ((original_flags & _object_header_needs_update_bit) != 0) {
                header->flags = original_flags &
                    (uint8_t)~(_object_header_needs_update_bit | _object_header_just_created_bit);
                object_update(handle);
            }
            if ((header->flags & _object_header_delete_pending_bit) != 0) {
                object_delete_4f9030(handle, 0);
            }
        }
    }

    objects_garbage_collection();
}

#if 0
Original Ghidra decompilation (0x4f4e90):

void FUN_004f4e90(void)

{
  char *pcVar1;
  char *pcVar2;
  byte bVar3;
  bool bVar4;
  uint uVar5;
  int iVar6;
  uint uVar7;
  int iVar8;
  char *pcVar9;
  short *psVar10;
  short sVar11;
  ushort uVar12;
  short sVar13;
  char *pcVar14;
  bool bVar15;

  iVar6 = DAT_006b8cbc;
  if (((*(byte *)(DAT_006f1d6c + 0xc) & 1) == 0) ||
     (bVar4 = true, *(char *)(DAT_006b0b80 + 2) == '\0')) {
    bVar4 = false;
  }
  *(undefined2 *)(DAT_006b8cbc + 4) = 0;
  pcVar1 = (char *)(iVar6 + 0xc);
  pcVar2 = (char *)(iVar6 + 0x4c);
  sVar13 = *(short *)(DAT_00746f9c + 0x134);
  uVar5 = sVar13 + 0x1f >> 5;
  iVar6 = uVar5 << 2;
  pcVar9 = pcVar2;
  pcVar14 = pcVar1;
  for (uVar7 = uVar5 & 0x3fffffff; uVar7 != 0; uVar7 = uVar7 - 1) {
    *(undefined4 *)pcVar14 = *(undefined4 *)pcVar9;
    pcVar9 = pcVar9 + 4;
    pcVar14 = pcVar14 + 4;
  }
  for (iVar8 = 0; iVar8 != 0; iVar8 = iVar8 + -1) {
    *pcVar14 = *pcVar9;
    pcVar9 = pcVar9 + 1;
    pcVar14 = pcVar14 + 1;
  }
  pcVar9 = (char *)(DAT_0087a478 + 0x18);
  pcVar14 = pcVar2;
  for (uVar5 = uVar5 & 0x3fffffff; uVar5 != 0; uVar5 = uVar5 - 1) {
    *(undefined4 *)pcVar14 = *(undefined4 *)pcVar9;
    pcVar9 = pcVar9 + 4;
    pcVar14 = pcVar14 + 4;
  }
  for (iVar8 = 0; iVar8 != 0; iVar8 = iVar8 + -1) {
    *pcVar14 = *pcVar9;
    pcVar9 = pcVar9 + 1;
    pcVar14 = pcVar14 + 1;
  }
  bVar15 = true;
  pcVar9 = pcVar1;
  pcVar14 = pcVar2;
  do {
    if (iVar6 == 0) break;
    iVar6 = iVar6 + -1;
    bVar15 = *pcVar9 == *pcVar14;
    pcVar9 = pcVar9 + 1;
    pcVar14 = pcVar14 + 1;
  } while (bVar15);
  if (!bVar15) {
    psVar10 = *(short **)(DAT_008603b0 + 0x34);
    if (0 < *(short *)(DAT_008603b0 + 0x2e)) {
      sVar11 = 0;
      do {
        if (((*psVar10 != 0) && (bVar3 = *(byte *)(psVar10 + 1), (bVar3 & 0x40) != 0)) &&
           ((bVar3 & 0x20) != 0)) {
          if ((bVar3 & 1) == 0) {
            if (((-1 < (char)bVar3) && (psVar10[2] != -1)) &&
               ((*(uint *)(pcVar2 + ((int)psVar10[2] >> 5) * 4) & 1 << ((byte)psVar10[2] & 0x1f)) !=
                0)) {
              FUN_004f50f0();
            }
          }
          else if ((*(uint *)(pcVar2 + ((int)psVar10[2] >> 5) * 4) & 1 << ((byte)psVar10[2] & 0x1f))
                   == 0) {
            if ((*(uint *)(*(int *)(psVar10 + 4) + 0x10) & 0x80000) == 0) {
              FUN_004f5130();
            }
            else {
              FUN_004f5bd0();
            }
          }
        }
        sVar11 = sVar11 + 1;
        psVar10 = psVar10 + 6;
      } while (sVar11 < *(short *)(DAT_008603b0 + 0x2e));
    }
    FUN_005530d0(pcVar1,pcVar2,(int)sVar13);
  }
  psVar10 = *(short **)(DAT_008603b0 + 0x34);
  uVar12 = 0;
  iVar6 = DAT_008603b0;
  if (0 < *(short *)(DAT_008603b0 + 0x2e)) {
    do {
      if (((*psVar10 != 0) && ((*(byte *)(psVar10 + 1) & 1) != 0)) &&
         (((*(byte *)(psVar10 + 1) & 4) == 0 &&
          ((!bVar4 ||
           (((1 << (*(byte *)((int)psVar10 + 3) & 0x1f) & 3U) != 0 &&
            (*(int *)(*(int *)(*(int *)(iVar6 + 0x34) + 8 + (uint)uVar12 * 0xc) + 0x218) != -1))))))
         )) {
        object_update((int)*psVar10 << 0x10 | (int)(short)uVar12);
        iVar6 = DAT_008603b0;
      }
      uVar12 = uVar12 + 1;
      psVar10 = psVar10 + 6;
    } while ((short)uVar12 < *(short *)(iVar6 + 0x2e));
  }
  psVar10 = *(short **)(iVar6 + 0x34);
  sVar13 = 0;
  if (0 < *(short *)(iVar6 + 0x2e)) {
    do {
      if (*psVar10 != 0) {
        bVar3 = *(byte *)(psVar10 + 1);
        *(byte *)(psVar10 + 1) = bVar3 & 0xef;
        if ((bVar3 & 4) != 0) {
          *(byte *)(psVar10 + 1) = bVar3 & 0xeb;
          object_update((int)*psVar10 << 0x10 | (int)sVar13);
          iVar6 = DAT_008603b0;
        }
        if ((*(byte *)(psVar10 + 1) & 8) != 0) {
          object_delete_4f9030((int)*psVar10 << 0x10 | (int)sVar13,0);
          iVar6 = DAT_008603b0;
        }
      }
      sVar13 = sVar13 + 1;
      psVar10 = psVar10 + 6;
    } while (sVar13 < *(short *)(iVar6 + 0x2e));
  }
  objects_garbage_collection();
  return;
}
#endif
