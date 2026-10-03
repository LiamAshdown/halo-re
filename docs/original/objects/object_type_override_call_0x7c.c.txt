// object_type_override_call_0x7c
// address 0x4f4760, size 83 bytes
// name confidence: 0.6 (still FUN_004f4760 in Ghidra; named from types/objects.h's
//   object_type_definition.override_call_7c field comment)
// rewrite confidence: 0.75
// evidence: same lookup pattern as object_type_override_call_0x6c.
// register convention: object index in ESI (unaff_ESI).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data; // 0x008603b0
extern object_type_definition *object_type_definitions[k_maximum_object_types]; // 0x0069bfdc

void object_type_override_call_0x7c(uint32_t object_index) // blam-cc: ESI -> object_index
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    object_type_definition *def = object_type_definitions[obj->type];
    int16_t i;

    for (i = k_maximum_object_subdefinitions - 1; i >= 0; i--) {
        object_type_definition *sub = def->subdefinitions[i];
        if (sub != 0 && sub->override_call_7c != 0) {
            ((void (*)(uint32_t))sub->override_call_7c)(object_index); // push esi; call [eax+0x7c] (0x4f47ad)
            return;
        }
    }
}

#if 0
Original Ghidra decompilation (0x4f4760):

void FUN_004f4760(void)

{
  int iVar1;
  short sVar2;
  uint unaff_ESI;

  sVar2 = 0xf;
  while ((iVar1 = *(int *)((&PTR_PTR_0069bfdc)
                           [*(short *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                               (unaff_ESI & 0xffff) * 0xc) + 0xb4)] +
                          sVar2 * 4 + 0x80), iVar1 == 0 || (*(int *)(iVar1 + 0x7c) == 0))) {
    sVar2 = sVar2 + -1;
    if (sVar2 < 0) {
      return;
    }
  }
  (**(code **)(iVar1 + 0x7c))();
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
