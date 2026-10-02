// object_type_override_call_0x74
// address 0x4f4700, size 88 bytes
// name confidence: 0.6 (still FUN_004f4700 in Ghidra; named from types/objects.h's
//   object_type_definition.override_call_74 field comment, "0x74 defaults to true when absent")
// rewrite confidence: 0.75
// evidence: same lookup pattern as object_type_override_call_0x6c.
// register convention: object index in EDI (unaff_EDI).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data; // 0x008603b0
extern object_type_definition *object_type_definitions[k_maximum_object_types]; // 0x0069bfdc

uint8_t object_type_override_call_0x74(uint32_t object_index) // blam-cc: EDI -> object_index
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    object_type_definition *def = object_type_definitions[obj->type];
    int16_t i;

    for (i = k_maximum_object_subdefinitions - 1; i >= 0; i--) {
        object_type_definition *sub = def->subdefinitions[i];
        if (sub != 0 && sub->override_call_74 != 0) {
            return ((uint8_t (*)(uint32_t))sub->override_call_74)(object_index); // push edi; call [ecx+0x74] (0x4f474e)
        }
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4f4700):

undefined1 FUN_004f4700(void)

{
  int iVar1;
  undefined1 uVar2;
  short sVar3;
  uint unaff_EDI;

  sVar3 = 0xf;
  while ((iVar1 = *(int *)((&PTR_PTR_0069bfdc)
                           [*(short *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                               (unaff_EDI & 0xffff) * 0xc) + 0xb4)] +
                          sVar3 * 4 + 0x80), iVar1 == 0 || (*(int *)(iVar1 + 0x74) == 0))) {
    sVar3 = sVar3 + -1;
    if (sVar3 < 0) {
      return 1;
    }
  }
  uVar2 = (**(code **)(iVar1 + 0x74))();
  return uVar2;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
