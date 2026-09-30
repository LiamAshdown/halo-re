// object_resolve_collideable_reference
// address 0x4f5f00, size 57 bytes
// name confidence: 0.3 (still FUN_004f5f00 in Ghidra; functions.md's summary -- "Resolves an
//   object id stored in a per-category index table (selected by param_2) to its datum address
//   and category/type tag" -- conf=0.3; renamed to reflect what the code actually does, which
//   is walking one link of the collideable_object_references chain)
// rewrite confidence: 0.95 (equivalent to its duplicate object_cluster_collideable_iterate_begin.c (0.95, objdump-verified))
// evidence: types/objects.h object_cluster_reference (identifier 0x00, object_index 0x04,
//   next_reference 0x08); globals 0x008603d0 collideable_cluster_first and 0x008603d4
//   collideable_object_references.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "fn_objects.h"

extern datum_index *collideable_cluster_first; // 0x008603d0
extern data_array *collideable_object_references; // 0x008603d4

// FIXED (register inputs, objdump: each stack slot's first use checked against the parameter): the original never reads EAX, ECX; next_reference, cluster_index arrive(s) on the stack (2 stack argument(s)).
// blam-cc: stack -> next_reference, cluster_index
datum_index object_resolve_collideable_reference(datum_index *next_reference, int16_t cluster_index)
{
    datum_index head = collideable_cluster_first[cluster_index];
    object_cluster_reference *ref;

    *next_reference = head;
    if (head != k_datum_index_none) {
        ref = (object_cluster_reference *)collideable_object_references->data + (head & 0xffff);
        *next_reference = ref->next_reference;
        return ref->object_index;
    }
    return k_datum_index_none;
}

#if 0
Original Ghidra decompilation (0x4f5f00):

undefined4 FUN_004f5f00(uint *param_1,short param_2)

{
  int iVar1;
  uint uVar2;

  uVar2 = *(uint *)(DAT_008603d0 + param_2 * 4);
  *param_1 = uVar2;
  if (uVar2 != 0xffffffff) {
    iVar1 = *(int *)(DAT_008603d4 + 0x34) + (uVar2 & 0xffff) * 0xc;
    *param_1 = *(uint *)(iVar1 + 8);
    return *(undefined4 *)(iVar1 + 4);
  }
  return 0xffffffff;
}
#endif
