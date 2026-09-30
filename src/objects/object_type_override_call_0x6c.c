// object_type_override_call_0x6c
// address 0x4f45b0, size 103 bytes
// name confidence: 0.6 (still FUN_004f45b0 in Ghidra; named from types/objects.h's
//   object_type_definition.override_call_6c field comment)
// rewrite confidence: 0.75
// evidence: types/objects.h object_header, object, object_type_definition; global 0x008603b0
//   object_data; global 0x0069bfdc object_type_definitions[12].
// VERIFIED against disassembly 0x4f45b0..0x4f4616 (2026-09-30); fixed: the three stack arguments are forwarded to the per-type
//   hook (network_server_broadcast_object_type_changes passes buffer 0x871de0, budget 0x7ff8 and a baseline flag).
// register convention: object index in EDI (unaff_EDI), resolved directly through object_data
//   rather than through object_try_and_get.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern data_array *object_data; // 0x008603b0
extern object_type_definition *object_type_definitions[k_maximum_object_types]; // 0x0069bfdc

// blam-cc: EDI -> object_index, stack -> buffer, bit_budget, full_update
int object_type_override_call_0x6c(uint32_t object_index, void *buffer, int32_t bit_budget, int32_t full_update)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    object_type_definition *def = object_type_definitions[obj->type];
    int16_t i;

    for (i = k_maximum_object_subdefinitions - 1; i >= 0; i--) {
        object_type_definition *sub = def->subdefinitions[i];
        if (sub != 0 && sub->override_call_6c != 0) {
            // 0x4f45fe..0x4f460e: push full_update, bit_budget, buffer, EDI; call [sub+0x6c]; the callee pops nothing extra (cdecl)
            return ((int (*)(uint32_t, void *, int32_t, int32_t))sub->override_call_6c)(object_index, buffer, bit_budget, full_update);
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
