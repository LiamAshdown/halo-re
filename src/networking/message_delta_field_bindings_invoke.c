// message_delta_field_bindings_invoke  (Ghidra: FUN_004ec700; named per this rewrite)
// address 0x4ec700, size 66 bytes
// name confidence: 0.3   rewrite confidence: 0.55
// evidence: objdump -d -M intel bin/halo.exe @0x4ec700: `mov eax,[ebx]` (count), `lea
// esi,[ebx+8]` (fields[] start, matching message_delta_static_fields in types/networking.h),
// per-entry `lea ecx,[ecx+ecx*2]` / `call [ecx*8+0x69a2fc]` -- kind*3*8 == kind*0x18, the same
// stride out/phase4/networking_types_notes.md documents for the per-field-type callback record.
// The stop condition (destination_offset==0 && source_offset==0 && field_type==0) is an all-zero padding
// entry at the end of a message type's static field-binding array.
// register convention: the field-binding list header in EBX (unaff_EBX).
// blam-cc: EBX -> list
// UNSURE: what this per-type "init" callback (the 0x69a2fc table slot) actually does; only that
// it is the same slot message_delta_field_bindings_lazy_init (0x4ec840) also calls before
// caching a field type's bit size.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif


extern message_delta_field_type_vtable message_delta_field_type_table[]; // 0x0069a2f0

// blam-cc: EBX -> list
// Walks a message type's field-binding list, calling each field type's callback (the same slot
// message_delta_field_bindings_lazy_init calls) with the field type pointer, until every entry
// has been visited or an all-zero sentinel entry is reached.
void message_delta_field_bindings_invoke(message_delta_static_fields *list)
{
    int32_t i;
    message_delta_field_binding *binding;

    if (0 < list->count) {
        binding = list->fields;
        for (i = 0; i < list->count; i++, binding++) {
            if (binding->destination_offset == 0 && binding->source_offset == 0 && binding->field_type == 0) {
                return;
            }
            message_delta_field_type_table[*(int32_t *)binding->field_type].initialize(binding->field_type);
        }
    }
}

#if 0
Original Ghidra decompilation (0x4ec700):

void FUN_004ec700(void)

{
  int *unaff_EBX;
  int *piVar1;
  int iVar2;

  iVar2 = 0;
  if (0 < *unaff_EBX) {
    piVar1 = unaff_EBX + 2;
    do {
      if (((piVar1[1] == 0) && (piVar1[2] == 0)) && (*piVar1 == 0)) {
        return;
      }
      (**(code **)(&DAT_0069a2fc + *(int *)*piVar1 * 0x18))((int *)*piVar1);
      iVar2 = iVar2 + 1;
      piVar1 = piVar1 + 4;
    } while (iVar2 < *unaff_EBX);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
