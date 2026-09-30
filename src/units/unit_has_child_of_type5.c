// unit_has_child_of_type5  (Ghidra: FUN_00570d70; renamed from the phase2 proposal)
// address 0x570d70, size 89 bytes
// name confidence: 0.3 (phase2 proposal at 0.3, matches functions.md summary)
// rewrite confidence: 0.5
// evidence: types/objects.h object.type (0x0b4, bit 5 -> ObjectType 5 = projectile),
//   .next_object (0x114), .first_child_object (0x118).
// register convention: unit object index in ECX (in_ECX).
//   // blam-cc: ECX -> unit_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "fn_units.h"

extern data_array *object_data; // 0x008603b0

// Returns whether the unit has any attached child object of type 5 (projectile), walking the
// same child-object sibling chain as unit_propagate_position_delta_to_children.
uint8_t unit_has_child_of_type5(uint32_t unit_index)
{
    object *obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    datum_index child = obj->first_child_object;

    if (child == k_datum_index_none) {
        return 0;
    }

    for (;;) {
        object *child_obj = ((object_header *)object_data->data)[child & 0xffff].data;
        if ((1 << (child_obj->type & 0x1f) & 0x20) != 0) {
            return 1;
        }
        child = child_obj->next_object;
        if (child == k_datum_index_none) {
            return 0;
        }
    }
}

#if 0
Original Ghidra decompilation (0x570d70):

undefined1 FUN_00570d70(void)

{
  int iVar1;
  undefined1 uVar2;
  uint in_ECX;
  uint uVar3;

  uVar3 = *(uint *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_ECX & 0xffff) * 0xc) + 0x118);
  uVar2 = 0;
  if (uVar3 != 0xffffffff) {
    while (iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar3 & 0xffff) * 0xc),
          (1 << (*(byte *)(iVar1 + 0xb4) & 0x1f) & 0x20U) == 0) {
      uVar3 = *(uint *)(iVar1 + 0x114);
      if (uVar3 == 0xffffffff) {
        return uVar2;
      }
    }
    uVar2 = 1;
  }
  return uVar2;
}
#endif
