// object_type_definitions_notify_0x50
// address 0x4f4330, size 104 bytes
// name confidence: 0.6 (still FUN_004f4330 in Ghidra; named from types/objects.h's
//   object_type_definition.notify_reset_scale field comment, "0x50
//   object_reset_default_scale_and_color" -- that name belongs to the caller at 0x4f5160, this
//   is the generic broadcaster it uses)
// rewrite confidence: 0.75
// evidence: same as object_type_definitions_notify_0x24.
// register convention: object index in EBX (unaff_EBX), not forwarded to the callee.

// reconciled: the original passes each callback its arguments (0xnotify_0x50: push ... push ebx; call eax); the draft called it with none,
//   so every object-type callback read a garbage object index.
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data; // 0x008603b0
extern object_type_definition *object_type_definitions[k_maximum_object_types]; // 0x0069bfdc

void object_type_definitions_notify_0x50(uint32_t object_index) // blam-cc: EBX -> object_index
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    object_type_definition *def = object_type_definitions[obj->type];
    int16_t i;

    for (i = 0; i < k_maximum_object_subdefinitions; i++) {
        object_type_definition *sub = def->subdefinitions[i];
        if (sub == 0) {
            break;
        }
        if (sub->notify_reset_scale != 0) {
            ((void (*)(uint32_t))sub->notify_reset_scale)(object_index);
        }
    }
}

#if 0
Original Ghidra decompilation (0x4f4330):

void FUN_004f4330(void)

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
    if (*(code **)(*piVar1 + 0x50) != (code *)0x0) {
      (**(code **)(*piVar1 + 0x50))();
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
