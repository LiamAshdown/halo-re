// object_type_override_call_0x6c
// address 0x4f45b0, size 103 bytes
// name confidence: 0.6 (still FUN_004f45b0 in Ghidra; named from types/objects.h's
//   object_type_definition.override_call_6c field comment)
// rewrite confidence: 0.75
// evidence: types/objects.h object_header, object, object_type_definition; global 0x008603b0
//   object_data; global 0x0069bfdc object_type_definitions[12].
// register convention: object index in EDI (unaff_EDI), resolved directly through object_data
//   rather than through object_try_and_get.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern data_array *object_data; // 0x008603b0
extern object_type_definition *object_type_definitions[k_maximum_object_types]; // 0x0069bfdc

int object_type_override_call_0x6c(uint32_t object_index) // blam-cc: EDI -> object_index
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    object_type_definition *def = object_type_definitions[obj->type];
    int16_t i;

    for (i = k_maximum_object_subdefinitions - 1; i >= 0; i--) {
        object_type_definition *sub = def->subdefinitions[i];
        if (sub != 0 && sub->override_call_6c != 0) {
            return ((int (*)(void))sub->override_call_6c)();
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4f45b0):

undefined4 FUN_004f45b0(void)

{
  int iVar1;
  undefined4 uVar2;
  short sVar3;
  uint unaff_EDI;

  sVar3 = 0xf;
  while ((iVar1 = *(int *)((&PTR_PTR_0069bfdc)
                           [*(short *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                               (unaff_EDI & 0xffff) * 0xc) + 0xb4)] +
                          sVar3 * 4 + 0x80), iVar1 == 0 || (*(int *)(iVar1 + 0x6c) == 0))) {
    sVar3 = sVar3 + -1;
    if (sVar3 < 0) {
      return 0;
    }
  }
  uVar2 = (**(code **)(iVar1 + 0x6c))();
  return uVar2;
}
#endif
