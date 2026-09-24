// object_collect_local_player_relevant_objects  (Ghidra: FUN_004fa1a0; renamed, Blam-style,
// not previously named)
// address 0x4fa1a0, size 233 bytes
// name confidence: 0.25 (matches functions.md's summary: "Finds the local player's object and,
//   if any bit is set in a per-definition flag bitfield, hands off to object_get_orientation
//   for that bit range")
// rewrite confidence: 0.3 (raised from 0.2 by the phase-4 review pass: the leaf/cluster lookup was corrected against the disassembly) (this function and object_type_definitions_collect_by_flag_bits
//   (0x4fa280, this batch, formerly misnamed "object_get_orientation") share a set of
//   in_stack_*/unaff_* values Ghidra could not cleanly separate, meaning the two are almost
//   certainly one algorithm split at an arbitrary point; each is transliterated close to its
//   own decompiled shape rather than merged, since merging risks inventing behaviour neither
//   Ghidra output actually shows.)
// evidence: global 0x00746f90 global_globals, 0x00746f9c structure_bsp_globals (leaf table at
//   +0xe4, per src/objects/object_set_cluster_and_parent.c), 0x006b8cbc
//   object_globals_pointer, 0x008603cc object_cluster_stamp, 0x008603d4
//   collideable_object_references; callees bsp3d_node_find_leaf (leaf/visibility probe, established
//   call shape from object_set_cluster_and_parent.c), object_type_definitions_collect_by_flag_bits
//   (0x4fa280, this batch).
// register convention: UNRESOLVED (no parameters visible at all; every value in the body is
//   either a global or a callee return). Matches Ghidra's own "FUN_004fa1a0(void)".
// UNSURE: the {leaf-bit-array, cluster-object-reference} pair this walks resembles
//   object_globals's cluster PVS bitset (cluster_pvs_current, 16 dwords) but is read through
//   structure_bsp_globals (+0x134 count, +0x14c array) rather than object_globals_pointer, so
//   it is not mapped onto that field here.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern void *global_globals; // 0x00746f90
extern uint8_t *structure_bsp_globals; // 0x00746f9c
extern object_globals *object_globals_pointer; // 0x006b8cbc
extern int32_t object_cluster_stamp; // 0x008603cc
extern data_array *collideable_object_references; // 0x008603d4

extern int32_t bsp3d_node_find_leaf(void *globals, real_point3d *point, int32_t index); // 0x5013a0
extern int32_t object_type_definitions_collect_by_flag_bits(int32_t bit_index, int32_t remaining_bits,
    int16_t range_index, int16_t range_count, int32_t *bit_array, int32_t cluster_stamp_snapshot,
    uint8_t (*filter)(uint32_t, void *), void *filter_context, int32_t count, int32_t max_count,
    datum_index *out); // 0x4fa280, this batch, UNSURE: see file header and that file's own header

int32_t object_collect_local_player_relevant_objects(void)
{
    // UNSURE: the EDX operand (the probe point) is genuinely never written in this function --
    // objdump 0x4fa1a0..0x4fa1ae is `mov ecx,ds:0x746f90 / sub esp,0x18 / push edi / xor eax,eax /
    // xor edi,edi / call 0x5013a0`, so EDX arrives from whatever the caller left behind. Passed
    // as 0 rather than inventing a point.
    int32_t leaf = bsp3d_node_find_leaf(global_globals, 0, 0);
    uint8_t *bsp = structure_bsp_globals;

    if (leaf == -1) {
        return 0;
    }

    {
        // PHASE-4 REVIEW: 0x4fa1bc..0x4fa1d1 is `and eax,0x7fffffff / mov ecx,[ebx+0xe4] /
        // shl eax,4 / mov ax,[eax+ecx+8]` -- structure_bsp_globals+0xe4 is a POINTER to the
        // ScenarioStructureBSPLeaf array and the leaf index is masked before scaling. The
        // earlier rewrite folded 0xe4 into the byte offset and dropped the mask.
        int16_t cluster = *(int16_t *)(*(uint8_t **)(bsp + 0xe4) +
                                       (uint32_t)(leaf & 0x7fffffff) * 0x10 + 8);
        if (cluster == -1) {
            return 0;
        }

        object_globals_pointer->collecting_in_clusters = 1;
        {
            int32_t bit_count = (*(int32_t *)(bsp + 0x134) + 0x1f) >> 5;
            int32_t *bits = (int32_t *)(*(int32_t *)(bsp + 0x14c) + cluster * bit_count * 4);
            int16_t i;

            object_cluster_stamp = object_cluster_stamp + 1;

            for (i = 0; i < (int16_t)bit_count; i++) {
                if (bits[i] != 0) {
                    int32_t hi = i * 0x20 + 0x20;
                    if (hi > *(int32_t *)(bsp + 0x134)) {
                        hi = *(int32_t *)(bsp + 0x134);
                    }
                    if (i * 0x20 < hi) {
                        // UNSURE: the real arguments (a filter callback, its context, an output
                        // array and a limit) are not available at this call site either; see
                        // both files' headers.
                        return object_type_definitions_collect_by_flag_bits(i * 0x20, hi - i * 0x20,
                            i, (int16_t)bit_count, bits, object_cluster_stamp, 0, 0, 0, 0, 0);
                    }
                }
            }
        }
    }

    object_globals_pointer->collecting_in_clusters = 0;
    return 0;
}

#if 0
Original Ghidra decompilation (0x4fa1a0):

undefined4 FUN_004fa1a0(void)

{
  int iVar1;
  int *piVar2;
  undefined4 uVar3;
  short sVar4;
  int iVar5;
  short sVar6;

  iVar1 = FUN_005013a0();
  iVar5 = DAT_00746f9c;
  if (iVar1 == -1) {
    return 0;
  }
  sVar6 = *(short *)(iVar1 * 0x10 + 8 + *(int *)(DAT_00746f9c + 0xe4));
  if (sVar6 == -1) {
    return 0;
  }
  *(undefined1 *)(DAT_006b8cbc + 1) = 1;
  iVar1 = *(int *)(iVar5 + 0x134) + 0x1f >> 5;
  piVar2 = (int *)(*(int *)(iVar5 + 0x14c) + sVar6 * iVar1 * 4);
  DAT_008603cc = DAT_008603cc + 1;
  sVar4 = (short)iVar1;
  sVar6 = 0;
  if (0 < sVar4) {
    do {
      if (*piVar2 != 0) {
        iVar1 = (short)(sVar6 * 0x20) + 0x20;
        if (*(int *)(iVar5 + 0x134) < iVar1) {
          iVar1 = *(int *)(iVar5 + 0x134);
        }
        if ((short)(sVar6 * 0x20) < (short)iVar1) {
          uVar3 = object_get_orientation();
          return uVar3;
        }
      }
      sVar6 = sVar6 + 1;
      piVar2 = piVar2 + 1;
      iVar5 = DAT_00746f9c;
    } while (sVar6 < sVar4);
  }
  *(undefined1 *)(DAT_006b8cbc + 1) = 0;
  return 0;
}
#endif
