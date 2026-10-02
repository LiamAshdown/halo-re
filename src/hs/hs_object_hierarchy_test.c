// hs_object_hierarchy_test  (Ghidra: FUN_00487c10)
// address 0x487c10, size 218 bytes
// name confidence: 0.3 (out/phase4/hs_functions.md: "Recursively checks whether an object or any
//   of its attached child objects matches a small object-type bitmask or a specific flag")
// rewrite confidence: 0.9 (VERIFIED against objdump)
// evidence: out/phase4/hs_types_notes.md object field offsets (0xb4 type, 0x114/0x118/0x11c
//   sibling/child/parent).
// register convention: none (void); object index is the recognized stack parameter (param_1).
// UNSURE: the two walks are not both over children despite the function summary -- the first
//   loop (via +0x118/+0x114) walks the child list recursively as described, but the second (via
//   +0x11c on the ORIGINAL object, not on each visited node) walks the ancestor/parent chain,
//   testing player_index_from_unit_index on each ancestor. player_index_from_unit_index's own meaning (object -> something that
//   is -1 when absent, used identically in hs_object_detach_and_place_at_location) is not
//   recovered either; "player association" is a guess from context, not evidence.

#include "tags.h"
#include "memory.h"
#include "hs.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint32_t player_index_from_unit_index(datum_index object_index); // game module, 0x474db0; UNSURE semantics

extern data_array *object_data; // 0x008603b0, stride 0x0c, object data pointer at +0x08

// hs_object_record: defined in types/hs.h (foreign-module slice; was a local TYPES-GAP copy)

static hs_object_record *hs_object_record_get(datum_index object_index)
{
    return *(hs_object_record **)((uint8_t *)object_data->data +
        (object_index & 0xffff) * 0x0c + 8);
}

// Returns 1 if `object_index` itself has an association via player_index_from_unit_index, or any object in its
// child subtree does (recursively), or any object in its ancestor chain does, or its own type is
// one of bits 2/3/4 (mask 0x1c) with flags_1f4 bit 1 set; otherwise 0.
char hs_object_hierarchy_test(datum_index object_index)
{
    hs_object_record *object;
    hs_object_record *node;
    datum_index child;
    datum_index ancestor;

    object = hs_object_record_get(object_index);
    if (player_index_from_unit_index(object_index) != 0xffffffff) {
        return 1;
    }

    child = object->child;
    while (child != k_datum_index_none) {
        node = hs_object_record_get(child);
        if (hs_object_hierarchy_test(child) != 0) {
            return 1;
        }
        child = node->sibling;
    }

    ancestor = object->parent;
    while (ancestor != k_datum_index_none) {
        node = hs_object_record_get(ancestor);
        if (player_index_from_unit_index(ancestor) != 0xffffffff) {
            return 1;
        }
        ancestor = node->parent;
    }

    if ((1 << (object->type & 0x1f) & 0x1c) != 0 && (object->flags_1f4 & 2) != 0) {
        return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x487c10):

undefined4 FUN_00487c10(uint param_1)

{
  int iVar1;
  uint uVar2;
  int iVar3;
  char cVar4;
  int iVar5;
  int iVar6;

  iVar3 = DAT_008603b0;
  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  iVar5 = FUN_00474db0(param_1);
  if (iVar5 != -1) {
    return 1;
  }
  uVar2 = *(uint *)(iVar1 + 0x118);
  while (uVar2 != 0xffffffff) {
    iVar3 = *(int *)(*(int *)(iVar3 + 0x34) + 8 + (uVar2 & 0xffff) * 0xc);
    cVar4 = FUN_00487c10(uVar2);
    if (cVar4 != '\0') {
      return 1;
    }
    uVar2 = *(uint *)(iVar3 + 0x114);
    iVar3 = DAT_008603b0;
  }
  uVar2 = *(uint *)(iVar1 + 0x11c);
  while (uVar2 != 0xffffffff) {
    iVar5 = *(int *)(*(int *)(iVar3 + 0x34) + 8 + (uVar2 & 0xffff) * 0xc);
    iVar6 = FUN_00474db0(uVar2);
    if (iVar6 != -1) {
      return 1;
    }
    uVar2 = *(uint *)(iVar5 + 0x11c);
  }
  if (((1 << (*(byte *)(iVar1 + 0xb4) & 0x1f) & 0x1cU) != 0) && ((*(byte *)(iVar1 + 500) & 2) != 0))
  {
    return 1;
  }
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
