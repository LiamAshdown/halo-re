// object_type_definitions_notify_0x24
// address 0x4f3e30, size 106 bytes
// name confidence: 0.6 (still FUN_004f3e30 in Ghidra; named from types/objects.h's
//   object_type_definition.notify_created field comment, "0x24 object_type_definitions_notify_0x24")
// rewrite confidence: 0.75
// evidence: types/objects.h object_header (type byte at 0x03), object (type at 0xb4),
//   object_type_definition (subdefinitions[16] at 0x80, notify_created at 0x24); global
//   0x008603b0 object_data; global 0x0069bfdc object_type_definitions[12].
// register convention: object index in EBX (unaff_EBX -- read only for the type lookup, never
//   passed to the callee, so Ghidra could not see it as a declared parameter).
// UNSURE: the vtable slot 0x24 hook itself takes no arguments in the original, which is why the
//   object index is only used to select the type's sub-definition chain and not forwarded.

// reconciled: the original passes each callback its arguments (0xnotify_0x24: push ... push ebx; call eax); the draft called it with none,
//   so every object-type callback read a garbage object index.
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern data_array *object_data; // 0x008603b0
extern object_type_definition *object_type_definitions[k_maximum_object_types]; // 0x0069bfdc

void object_type_definitions_notify_0x24(uint32_t object_index, uint32_t argument) // blam-cc: EBX -> object_index
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    object_type_definition *def = object_type_definitions[obj->type];
    int16_t i;

    for (i = 0; i < k_maximum_object_subdefinitions; i++) {
        object_type_definition *sub = def->subdefinitions[i];
        if (sub == 0) {
            break;
        }
        if (sub->notify_created != 0) {
            ((void (*)(uint32_t, uint32_t))sub->notify_created)(object_index, argument);
        }
    }
}

#if 0
Original Ghidra decompilation (0x4f3e30):

void FUN_004f3e30(void)

{
  int *piVar1;
  undefined *puVar2;
  int iVar3;
  uint unaff_EBX;
  short sVar4;

  puVar2 = (&PTR_PTR_0069bfdc)
           [*(short *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (unaff_EBX & 0xffff) * 0xc) +
                      0xb4)];
  piVar1 = (int *)(puVar2 + 0x80);
  sVar4 = 0;
  iVar3 = *(int *)(puVar2 + 0x80);
  while (iVar3 != 0) {
    if (*(code **)(*piVar1 + 0x24) != (code *)0x0) {
      (**(code **)(*piVar1 + 0x24))();
    }
    sVar4 = sVar4 + 1;
    piVar1 = (int *)(puVar2 + sVar4 * 4 + 0x80);
    iVar3 = *(int *)(puVar2 + sVar4 * 4 + 0x80);
  }
  return;
}
#endif
