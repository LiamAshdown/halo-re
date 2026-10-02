// object_type_definitions_query_0x34
// address 0x4f4000, size 115 bytes
// name confidence: 0.6 (still FUN_004f4000 in Ghidra; named from types/objects.h's
//   object_type_definition.query_34 field comment, "0x34 OR style")
// rewrite confidence: 0.8
// evidence: same as object_type_definitions_query_0x28, mirrored as an OR-style dispatch.

// RETURN TYPE (phase-4 review pass): Ghidra returns this as CONCAT31(garbage, AL) / bool,
//   i.e. only the low byte is defined -- the top three bytes are whatever happened to be in
//   the register. The return type is uint8_t so no caller can depend on the garbage, which
//   is the same correction already recorded for object_type_definitions_query_0x44 0x4f41d0.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data; // 0x008603b0
extern object_type_definition *object_type_definitions[k_maximum_object_types]; // 0x0069bfdc

// FIXED (register inputs, objdump): the original never reads EAX as an input (it overwrites or only saves it); those parameters arrive on the stack (1 stack argument(s) read).
// blam-cc: stack -> object_index
uint8_t object_type_definitions_query_0x34(uint32_t object_index)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    object_type_definition *def = object_type_definitions[obj->type];
    int16_t i;
    int any_true = 0;

    for (i = 0; i < k_maximum_object_subdefinitions; i++) {
        object_type_definition *sub = def->subdefinitions[i];
        if (sub == 0) {
            break;
        }
        if (sub->query_34 != 0) {
            if (((int (*)(uint32_t))sub->query_34)(object_index) != 0) {
                any_true = 1;
            }
        }
    }
    return any_true;
}

#if 0
Original Ghidra decompilation (0x4f4000):

undefined4 FUN_004f4000(uint param_1)

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
    if ((*(code **)(*piVar1 + 0x34) != (code *)0x0) &&
       (cVar4 = (**(code **)(*piVar1 + 0x34))(param_1), cVar4 != '\0')) {
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
