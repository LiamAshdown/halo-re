// object_get_root_parent_placement
// address 0x4f5f70, size 137 bytes
// name confidence: 0.75 (still FUN_004f5f70 in Ghidra; types/objects.h's object.placement_id
//   comment names this exact function: "object_get_root_parent_placement 0x4f5f70 returns it")
// rewrite confidence: 0.85 (VERIFIED 2026-09-28 against objdump 0x4f5f70..0x4f5ff8.)
// evidence: types/objects.h object_header, object (parent_object 0x11c, placement_id 0x10c,
//   flags 0x10 with _object_has_collision_model_bit); globals 0x008603c0/0x008603c4/0x008603c8
//   (noncollideable_cluster_first/_object_references/_cluster_partition) and
//   0x008603d0/0x008603d4/0x008603d8 (the collideable equivalents), which the module globals
//   list documents as consecutive 4-byte-spaced globals -- the original walks that adjacency
//   directly (`puVar4 = &DAT_008603d0; ...; puVar4[1]; puVar4[2];`) instead of naming each one,
//   and this rewrite preserves that same address-arithmetic trick rather than substituting a
//   value that would not be a real, callable-later global address.
// register convention: object index in EAX (in_EAX), output {category table pointer,
//   marker/placement id} pair pointer in ESI (unaff_ESI). The out block is typed as
//   types/objects.h object_placement_cursor by the phase-4 review pass -- it is two dwords,
//   a globals-group address and a datum_index, not an array of pointers.
// UNSURE: whether "puVar4[2]" (the third of the three adjacent globals, i.e.
//   collideable_cluster_partition / noncollideable_cluster_partition, both declared void* in
//   types/objects.h) is genuinely used here as a data_array* of object_cluster_reference-shaped
//   records, or whether the positional read is accidentally landing on what is conceptually the
//   object_references array one slot earlier; the record shape read here (stride 0xc, fields at
//   +4 and +8) matches object_cluster_reference exactly either way, so the arithmetic is
//   preserved without resolving which of the two the header's names should apply to. UNSURE:
//   the function's return value keeps Ghidra's CONCAT22 of the resolved 16-bit field with the
//   high half of the record's own address, which looks like decompiler noise rather than real
//   data; only the low 16 bits (the field itself) are meaningful and are what this rewrite
//   returns.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data; // 0x008603b0
extern datum_index *noncollideable_cluster_first; // 0x008603c0
extern data_array *noncollideable_object_references; // 0x008603c4
extern void *noncollideable_cluster_partition; // 0x008603c8
extern datum_index *collideable_cluster_first; // 0x008603d0
extern data_array *collideable_object_references; // 0x008603d4
extern void *collideable_cluster_partition; // 0x008603d8

int16_t object_get_root_parent_placement(uint32_t object_index, object_placement_cursor *out_cursor)
    // blam-cc: EAX -> object_index, ESI -> out_cursor
{
    object_header *header;
    object *obj;
    int32_t *family; // UNSURE: see file header -- the address of whichever 3-global group applies
    data_array *reference_table;
    datum_index placement_id;

    {
        // Walk to the outermost (unparented) ancestor, exactly mirroring the original's
        // "uVar3 = -1; if (in_EAX != -1) { do { uVar3 = in_EAX; in_EAX = object[uVar3].
        // parent_object; } while (in_EAX != -1); }" -- if object_index starts at
        // k_datum_index_none, root stays k_datum_index_none too (an edge case the original
        // does not guard against either).
        uint32_t root = k_datum_index_none;
        if (object_index != k_datum_index_none) {
            do {
                root = object_index;
                header = (object_header *)object_data->data + (root & 0xffff);
                obj = header->data;
                object_index = obj->parent_object;
            } while (object_index != k_datum_index_none);
        }
        header = (object_header *)object_data->data + (root & 0xffff);
        obj = header->data;
    }

    if ((obj->flags & _object_has_collision_model_bit) == 0) {
        family = (int32_t *)&noncollideable_cluster_first;
        reference_table = (data_array *)noncollideable_cluster_partition;
    } else {
        family = (int32_t *)&collideable_cluster_first;
        reference_table = (data_array *)collideable_cluster_partition;
    }
    out_cursor->cluster_globals = family;

    placement_id = obj->placement_id;
    out_cursor->next_reference = placement_id;
    if (placement_id != k_datum_index_none) {
        object_cluster_reference *ref =
            (object_cluster_reference *)reference_table->data + (placement_id & 0xffff);
        out_cursor->next_reference = ref->next_reference;
        return (int16_t)ref->object_index;
    }
    return -1;
}

#if 0
Original Ghidra decompilation (0x4f5f70):

undefined4 FUN_004f5f70(void)

{
  int iVar1;
  uint in_EAX;
  int iVar2;
  uint uVar3;
  undefined4 *puVar4;
  undefined4 *unaff_ESI;

  iVar1 = DAT_008603b0;
  uVar3 = 0xffffffff;
  if (in_EAX != 0xffffffff) {
    do {
      uVar3 = in_EAX;
      in_EAX = *(uint *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar3 & 0xffff) * 0xc) +
                        0x11c);
    } while (in_EAX != 0xffffffff);
  }
  iVar2 = (uVar3 & 0xffff) * 0xc;
  puVar4 = &DAT_008603d0;
  if ((*(uint *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar2) + 0x10) & 0x2000000) == 0) {
    puVar4 = &DAT_008603c0;
  }
  *unaff_ESI = puVar4;
  uVar3 = *(uint *)(*(int *)(*(int *)(iVar1 + 0x34) + 8 + iVar2) + 0x10c);
  unaff_ESI[1] = uVar3;
  if (uVar3 != 0xffffffff) {
    iVar1 = *(int *)(puVar4[2] + 0x34) + (uVar3 & 0xffff) * 0xc;
    unaff_ESI[1] = *(undefined4 *)(iVar1 + 8);
    return CONCAT22((short)((uint)iVar1 >> 0x10),*(undefined2 *)(iVar1 + 4));
  }
  return 0xffffffff;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
