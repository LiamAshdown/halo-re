// object_type_definitions_query_0x28
// address 0x4f3ea0, size 80 bytes
// name confidence: 0.6 (still FUN_004f3ea0 in Ghidra; named from types/objects.h's
//   object_type_definition.query_create field comment, "0x28 AND style,
//   object_type_definitions_query_0x28")
// rewrite confidence: 0.8
// evidence: types/objects.h object_header, object, object_type_definition; global 0x008603b0
//   object_data; global 0x0069bfdc object_type_definitions[12].
// register convention: object index in EAX (param_1), also forwarded as the sole argument to
//   every sub-definition's +0x28 hook.

// RETURN TYPE (phase-4 review pass): Ghidra returns this as CONCAT31(garbage, AL) / bool,
//   i.e. only the low byte is defined -- the top three bytes are whatever happened to be in
//   the register. The return type is uint8_t so no caller can depend on the garbage, which
//   is the same correction already recorded for object_type_definitions_query_0x44 0x4f41d0.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern data_array *object_data; // 0x008603b0
extern object_type_definition *object_type_definitions[k_maximum_object_types]; // 0x0069bfdc

uint8_t object_type_definitions_query_0x28(uint32_t object_index) // blam-cc: EAX -> object_index
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    object_type_definition *def = object_type_definitions[obj->type];
    int16_t i;

    for (i = 0; i < k_maximum_object_subdefinitions; i++) {
        object_type_definition *sub = def->subdefinitions[i];
        if (sub == 0) {
            break;
        }
        if (sub->query_create != 0) {
            int result = ((int (*)(uint32_t))sub->query_create)(object_index);
            if (result == 0) {
                return 0;
            }
        }
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4f3ea0):

uint FUN_004f3ea0(uint param_1)

{
  int *piVar1;
  undefined *puVar2;
  int iVar3;
  uint uVar4;
  short sVar5;

  puVar2 = (&PTR_PTR_0069bfdc)
           [*(short *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc) + 0xb4
                      )];
  iVar3 = *(int *)(puVar2 + 0x80);
  piVar1 = (int *)(puVar2 + 0x80);
  sVar5 = 0;
  while( true ) {
    if (iVar3 == 0) {
      return CONCAT31((int3)((uint)piVar1 >> 8),1);
    }
    if ((*(code **)(*piVar1 + 0x28) != (code *)0x0) &&
       (uVar4 = (**(code **)(*piVar1 + 0x28))(param_1), (char)uVar4 == '\0')) break;
    sVar5 = sVar5 + 1;
    iVar3 = *(int *)(puVar2 + sVar5 * 4 + 0x80);
    piVar1 = (int *)(puVar2 + sVar5 * 4 + 0x80);
  }
  return uVar4 & 0xffffff00;
}
#endif
