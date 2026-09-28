// object_type_definitions_collect_by_flag_bits  (Ghidra inherited name "object_get_orientation",
// which does not match the code; per out/phase4/objects_types_notes.md: "0x4fa280
// object_get_orientation (the second one, distinct from 0x4f6970): walks a bitfield calling
// object_tree_collect_matching; no orientation math present")
// address 0x4fa280, size 272 bytes
// name confidence: 0.2 (deliberately not "object_get_orientation" -- see the types notes above;
//   renamed to describe what the code actually does)
// rewrite confidence: 1.0 (FRAGMENT: 0x4fa280 lies inside object_collect_local_player_relevant_objects (0x4fa1a0..0x4fa39e; the word loop re-enters at 0x4fa237 and 0x4fa278 jumps here); no real callers; that function is the real rewrite) (Ghidra could not resolve this function's true entry point or
//   parameter list: it reads roughly a dozen "in_stack_*"/"unaff_*" values with no call site
//   -- including this batch's own sole caller, object_collect_local_player_relevant_objects.c,
//   which invokes it with zero visible arguments -- that could confirm any of them. This is a
//   best-effort structural transliteration of the decompiled C, not a verified rewrite; every
//   parameter name below is a guess at Ghidra's own in_stack_* ordering.)
// evidence: global 0x008603b0 object_data, 0x008603cc object_cluster_stamp, 0x008603d0
//   collideable_cluster_first, 0x008603d4 collideable_object_references, 0x006b8cbc
//   object_globals_pointer; types/objects.h object (cluster_stamp 0x014),
//   object_cluster_reference (object_index 0x04, next_reference 0x08, same shape
//   object_collect_in_clusters.c, this batch, already walks); callee
//   object_tree_collect_matching (0x4fa0f0, this batch).
// register convention: UNRESOLVED, see file header.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern data_array *object_data; // 0x008603b0
extern int32_t object_cluster_stamp; // 0x008603cc
extern datum_index *collideable_cluster_first; // 0x008603d0, UNSURE: see file header (Ghidra's DAT_008603d0)
extern data_array *collideable_object_references; // 0x008603d4
extern object_globals *object_globals_pointer; // 0x006b8cbc

extern int32_t object_tree_collect_matching(uint32_t object_index, uint8_t (*filter)(uint32_t, void *),
    void *filter_context, int32_t count, int32_t max_count, datum_index *out); // 0x4fa0f0, this batch

// UNSURE throughout: see file header. Parameter names are a best-effort guess at Ghidra's own
// in_stack_* set; this is not a verified reconstruction.
int32_t object_type_definitions_collect_by_flag_bits(int32_t bit_index, int32_t remaining_bits,
    int16_t range_index, int16_t range_count, int32_t *bit_array, int32_t cluster_stamp_snapshot,
    uint8_t (*filter)(uint32_t, void *), void *filter_context, int32_t count, int32_t max_count,
    datum_index *out)
{
    int32_t result = count;

    do {
        if ((bit_array[bit_index >> 5] & (1u << (bit_index & 0x1f))) != 0) {
            datum_index ref = collideable_cluster_first[bit_index];

            while (ref != k_datum_index_none) {
                object_cluster_reference *node = (object_cluster_reference *)
                    collideable_object_references->data + (ref & 0xffff);
                object *obj = ((object_header *)object_data->data)[node->object_index & 0xffff].data;

                if (obj->cluster_stamp != cluster_stamp_snapshot) {
                    obj->cluster_stamp = cluster_stamp_snapshot;
                    result = object_tree_collect_matching(node->object_index, filter, filter_context, result, max_count, out);
                    cluster_stamp_snapshot = object_cluster_stamp;
                }
                ref = node->next_reference;
            }
        }
        bit_index = bit_index + 1;
        remaining_bits = remaining_bits - 1;
    } while (remaining_bits != 0);

    for (;;) {
        range_index = range_index + 1;
        bit_array = bit_array + 1;
        if (range_count <= range_index) {
            object_globals_pointer->collecting_in_clusters = 0;
            return result;
        }
        if (*bit_array != 0) {
            int32_t hi = (int16_t)(range_index * 0x20) + 0x20;
            if (hi > range_count) { // UNSURE: preserved from the original's own field re-read
                hi = range_count;
            }
            if ((int16_t)(range_index * 0x20) < (int16_t)hi) {
                break;
            }
        }
    }

    return object_type_definitions_collect_by_flag_bits(bit_index, remaining_bits, range_index,
        range_count, bit_array, cluster_stamp_snapshot, filter, filter_context, result, max_count, out);
}

#if 0
Original Ghidra decompilation (0x4fa280):

undefined4 object_get_orientation(void)

{
  uint uVar1;
  undefined4 uVar2;
  uint uVar3;
  int iVar4;
  int in_EDX;
  int unaff_EBX;
  int unaff_EBP;
  undefined4 unaff_EDI;
  int in_stack_00000010;
  int in_stack_00000014;
  short in_stack_00000018;
  short in_stack_0000001c;
  int *in_stack_00000020;
  int in_stack_00000024;
  undefined4 in_stack_0000002c;
  undefined4 in_stack_00000030;
  undefined4 in_stack_00000034;
  undefined4 in_stack_00000038;

  do {
    if ((*(uint *)(in_stack_00000024 + (unaff_EBX >> 5) * 4) & 1 << ((byte)unaff_EBX & 0x1f)) != 0)
    {
      uVar3 = *(uint *)(DAT_008603d0 + unaff_EBX * 4);
      if (uVar3 == 0xffffffff) {
        uVar3 = 0xffffffff;
        uVar1 = 0xffffffff;
      }
      else {
        uVar3 = uVar3 & 0xffff;
        uVar1 = *(uint *)(*(int *)(unaff_EBP + 0x34) + 8 + uVar3 * 0xc);
        uVar3 = *(uint *)(*(int *)(unaff_EBP + 0x34) + uVar3 * 0xc + 4);
      }
      while (uVar3 != 0xffffffff) {
        iVar4 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar3 & 0xffff) * 0xc);
        if (*(int *)(iVar4 + 0x14) != in_EDX) {
          *(int *)(iVar4 + 0x14) = in_EDX;
          unaff_EDI = object_tree_collect_matching
                                (uVar3,in_stack_0000002c,in_stack_00000030,unaff_EDI,
                                 in_stack_00000034,in_stack_00000038);
          in_EDX = DAT_008603cc;
          unaff_EBP = DAT_008603d4;
        }
        unaff_EBX = in_stack_00000010;
        if (uVar1 == 0xffffffff) {
          uVar3 = 0xffffffff;
          uVar1 = 0xffffffff;
        }
        else {
          uVar3 = uVar1 & 0xffff;
          uVar1 = *(uint *)(*(int *)(unaff_EBP + 0x34) + 8 + uVar3 * 0xc);
          uVar3 = *(uint *)(*(int *)(unaff_EBP + 0x34) + uVar3 * 0xc + 4);
        }
      }
    }
    unaff_EBX = unaff_EBX + 1;
    in_stack_00000014 = in_stack_00000014 + -1;
    in_stack_00000010 = unaff_EBX;
  } while (in_stack_00000014 != 0);
  do {
    do {
      in_stack_0000001c = in_stack_0000001c + 1;
      in_stack_00000020 = in_stack_00000020 + 1;
      if (in_stack_00000018 <= in_stack_0000001c) {
        *(undefined1 *)(DAT_006b8cbc + 1) = 0;
        return unaff_EDI;
      }
    } while (*in_stack_00000020 == 0);
    iVar4 = (short)(in_stack_0000001c * 0x20) + 0x20;
    if (*(int *)(DAT_00746f9c + 0x134) < iVar4) {
      iVar4 = *(int *)(DAT_00746f9c + 0x134);
    }
  } while ((short)iVar4 <= (short)(in_stack_0000001c * 0x20));
  uVar2 = object_get_orientation();
  return uVar2;
}
#endif
