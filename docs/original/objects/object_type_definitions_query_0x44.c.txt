// object_type_definitions_query_0x44
// address 0x4f41d0, size 115 bytes
// name confidence: 0.6 (still FUN_004f41d0 in Ghidra; named from types/objects.h's
//   object_type_definition.query_44 field comment, "0x44 OR style, used by
//   object_children_recurse_prune")
// rewrite confidence: 0.8
// evidence: same as object_type_definitions_query_0x34, mirrored at a different vtable slot.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data; // 0x008603b0
extern object_type_definition *object_type_definitions[k_maximum_object_types]; // 0x0069bfdc

// The original returns CONCAT31(garbage, uVar5): only AL carries the result, so the return
// type is a byte here rather than int.
// FIXED (register inputs, objdump): the original never reads EAX as an input (it overwrites or only saves it); those parameters arrive on the stack (1 stack argument(s) read).
// blam-cc: stack -> object_index
uint8_t object_type_definitions_query_0x44(uint32_t object_index)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    object_type_definition *def = object_type_definitions[obj->type];
    int16_t i;
    uint8_t any_true = 0;

    for (i = 0; i < k_maximum_object_subdefinitions; i++) {
        object_type_definition *sub = def->subdefinitions[i];
        if (sub == 0) {
            break;
        }
        if (sub->query_44 != 0) {
            if (((int (*)(uint32_t))sub->query_44)(object_index) != 0) {
                any_true = 1;
            }
        }
    }
    return any_true;
}

#if 0
Original Ghidra decompilation (0x4f41d0):

undefined4 FUN_004f41d0(uint param_1)

{
  int *piVar1;
  undefined *puVar2;
  int iVar3;
  char cVar4;
  undefined1 uVar5;
  short sVar6;

  puVar2 = (&PTR_PTR_0069bfdc)
           [*(short *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc) + 0xb4
                      )];
  piVar1 = (int *)(puVar2 + 0x80);
  uVar5 = 0;
  sVar6 = 0;
  iVar3 = *(int *)(puVar2 + 0x80);
  while (iVar3 != 0) {
    if ((*(code **)(*piVar1 + 0x44) != (code *)0x0) &&
       (cVar4 = (**(code **)(*piVar1 + 0x44))(param_1), cVar4 != '\0')) {
      uVar5 = 1;
    }
    sVar6 = sVar6 + 1;
    piVar1 = (int *)(puVar2 + sVar6 * 4 + 0x80);
    iVar3 = *(int *)(puVar2 + sVar6 * 4 + 0x80);
  }
  return CONCAT31((int3)((uint)piVar1 >> 8),uVar5);
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
