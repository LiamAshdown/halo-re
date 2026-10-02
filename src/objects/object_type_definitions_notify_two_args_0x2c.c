// object_type_definitions_notify_two_args_0x2c
// address 0x4f3f20, size 110 bytes
// name confidence: 0.6 (still FUN_004f3f20 in Ghidra; named from types/objects.h's
//   object_type_definition.notify_two_args_2c field comment)
// rewrite confidence: 0.8
// evidence: types/objects.h object_header, object, object_type_definition; global 0x008603b0
//   object_data; global 0x0069bfdc object_type_definitions[12].

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data; // 0x008603b0
extern object_type_definition *object_type_definitions[k_maximum_object_types]; // 0x0069bfdc

// FIXED (register inputs, objdump: each stack slot's first use checked against the parameter): the original never reads EAX, EDX; object_index, event_argument arrive(s) on the stack (2 stack argument(s)).
// blam-cc: stack -> object_index, event_argument
void object_type_definitions_notify_two_args_0x2c(uint32_t object_index, uint32_t event_argument)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    object_type_definition *def = object_type_definitions[obj->type];
    int16_t i;

    for (i = 0; i < k_maximum_object_subdefinitions; i++) {
        object_type_definition *sub = def->subdefinitions[i];
        if (sub == 0) {
            break;
        }
        if (sub->notify_two_args_2c != 0) {
            ((void (*)(uint32_t, uint32_t))sub->notify_two_args_2c)(object_index, event_argument);
        }
    }
}

#if 0
Original Ghidra decompilation (0x4f3f20):

void FUN_004f3f20(uint param_1,undefined4 param_2)

{
  int *piVar1;
  undefined *puVar2;
  int iVar3;
  short sVar4;

  puVar2 = (&PTR_PTR_0069bfdc)
           [*(short *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc) + 0xb4
                      )];
  piVar1 = (int *)(puVar2 + 0x80);
  sVar4 = 0;
  iVar3 = *(int *)(puVar2 + 0x80);
  while (iVar3 != 0) {
    if (*(code **)(*piVar1 + 0x2c) != (code *)0x0) {
      (**(code **)(*piVar1 + 0x2c))(param_1,param_2);
    }
    sVar4 = sVar4 + 1;
    piVar1 = (int *)(puVar2 + sVar4 * 4 + 0x80);
    iVar3 = *(int *)(puVar2 + sVar4 * 4 + 0x80);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
