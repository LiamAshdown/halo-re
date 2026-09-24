// object_type_override_call_0x68
// address 0x4f4560, size 74 bytes
// name confidence: 0.6 (still FUN_004f4560 in Ghidra; named from types/objects.h's
//   object_type_definition.override_call_68 field comment)
// rewrite confidence: 0.7
// evidence: same as object_type_override_get_0x64.
// register convention: none (void); operates on the "current object".

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern object *object_try_and_get(datum_index object_index, uint32_t type_mask);
    // 0x4f6ec0; object handle in ECX, type mask on the stack. Verified against the body at
    // 0x4f6ec0 (cmp ecx,-1 / test cx,cx / and param_1 & 1 << header->type) and against the
    // call site in this file.
extern object_type_definition *object_type_definitions[k_maximum_object_types]; // 0x0069bfdc

void object_type_override_call_0x68(uint32_t object_index) // blam-cc: ESI -> object_index
{
    object *obj = object_try_and_get(object_index, _object_mask_all);
    object_type_definition *def;
    int16_t i;

    if (obj == 0) {
        return;
    }
    def = object_type_definitions[obj->type];
    for (i = k_maximum_object_subdefinitions - 1; i >= 0; i--) {
        object_type_definition *sub = def->subdefinitions[i];
        if (sub != 0 && sub->override_call_68 != 0) {
            ((void (*)(void))sub->override_call_68)();
            return;
        }
    }
}

#if 0
Original Ghidra decompilation (0x4f4560):

void FUN_004f4560(void)

{
  int iVar1;
  int iVar2;
  short sVar3;

  iVar2 = object_try_and_get(0xffffffff);
  if (iVar2 != 0) {
    sVar3 = 0xf;
    while ((iVar1 = *(int *)((&PTR_PTR_0069bfdc)[*(short *)(iVar2 + 0xb4)] + sVar3 * 4 + 0x80),
           iVar1 == 0 || (*(int *)(iVar1 + 0x68) == 0))) {
      sVar3 = sVar3 + -1;
      if (sVar3 < 0) {
        return;
      }
    }
    (**(code **)(iVar1 + 0x68))();
  }
  return;
}
#endif
