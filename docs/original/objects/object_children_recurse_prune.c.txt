// object_children_recurse_prune
// address 0x4edc10, size 101 bytes
// name confidence: 0.6 (out/phase4/objects_types_notes.md names this function directly, in the
// object_type_definition.query_44 comment: "0x44 OR style, used by object_children_recurse_prune")
// rewrite confidence: 0.55
// evidence: types/objects.h object.first_child_object (0x118), object.next_object (0x114);
// FUN_004f41d0 is the +0x44 OR-style sub-definition predicate dispatch (out of this module's
// address range, so it keeps its Ghidra name).
// register convention: datum_index object_index on the stack (param_1).
// blam-cc: stack=object_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data; // 0x008603b0

extern uint8_t object_type_definitions_query_0x44(uint32_t object_index); // this module, 0x4f41d0

void object_children_recurse_prune(uint32_t object_index)
{
    object_header *headers = (object_header *)object_data->data;
    object *obj = headers[object_index & 0xffff].data;
    datum_index child_index = obj->first_child_object;

    while (child_index != (datum_index)0xffffffff) {
        object *child = headers[child_index & 0xffff].data;
        datum_index next_index = child->next_object;

        if (object_type_definitions_query_0x44(child_index) == 0) {
            object_children_recurse_prune(child_index);
        }

        child_index = next_index;
    }
}

#if 0
Original Ghidra decompilation (0x4edc10):

void FUN_004edc10(uint param_1)

{
  uint uVar1;
  uint uVar2;
  char cVar3;

  uVar1 = *(uint *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc) + 0x118);
  while (uVar2 = uVar1, uVar2 != 0xffffffff) {
    uVar1 = *(uint *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar2 & 0xffff) * 0xc) + 0x114);
    cVar3 = FUN_004f41d0(uVar2);
    if (cVar3 == '\0') {
      FUN_004edc10(uVar2);
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
