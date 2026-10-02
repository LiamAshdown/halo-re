// message_delta_field_bindings_lazy_init  (Ghidra: FUN_004ec840; named per this rewrite)
// address 0x4ec840, size 184 bytes
// name confidence: 0.4   rewrite confidence: 0.55
// evidence: objdump -d -M intel bin/halo.exe @0x4ec840: per-entry test of the same
// message_delta_field_binding::initialized low byte message_delta_field_bindings_teardown clears,
// and of field_type+0x64 (an "already initialized" byte); on the first visit it calls the
// per-type "init" (0x0069a2fc, base+0xc) and "compute size" (0x0069a2f8, base+0x8) callbacks and
// caches the size at field_type+0x5c, matching "carrying its own bit size at +0x5c" in
// out/phase4/networking_types_notes.md.
// register convention: the field-binding list header in EBX (unaff_EBX).
// blam-cc: EBX -> list
// UNSURE: the trailing consistency check (return 1 only when every entry up to `count` was
// processed and the entry at `count` is itself an all-zero sentinel) is transcribed exactly;
// its return value is discarded by every caller in this module (message_delta_field_layout_compute_size
// calls this twice and never reads the result), so this is not independently exercised here.

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
// Lazily initializes each not-yet-initialized field type used by a message type's field-binding
// list (a field type's init/compute-size pair only ever runs once, cached via field_type+0x64),
// then marks every visited binding. Returns 1 only if the whole list was walked without hitting
// an early sentinel and the entry immediately after it is itself all-zero.
uint8_t message_delta_field_bindings_lazy_init(message_delta_static_fields *list)
{
    int32_t count;
    int32_t i;
    int32_t processed;
    message_delta_field_binding *binding;
    void *field_type;
    uint8_t *binding_flag;
    uint8_t *type_flag;

    count = list->count;
    if (count >= 0x41) {
        return 0;
    }
    i = 0;
    processed = 0;
    if (0 < count) {
        binding = list->fields;
        for (; i < count; i++, binding++) {
            if (binding->destination_offset == 0 && binding->source_offset == 0 && binding->field_type == 0) {
                break;
            }
            binding_flag = &binding->initialized;
            if (*binding_flag == 0) {
                field_type = binding->field_type;
                type_flag = (uint8_t *)field_type + 0x64;
                if (*type_flag == 0) {
                    message_delta_field_type_table[*(int32_t *)field_type].initialize((message_delta_field_type *)field_type);
                    *(int32_t *)((uint8_t *)field_type + 0x5c) =
                        message_delta_field_type_table[*(int32_t *)field_type].compute_size((message_delta_field_type *)field_type);
                    *type_flag = 1;
                }
                *binding_flag = 1;
            }
            processed = processed + 1;
        }
    }
    binding = &list->fields[i];
    if (processed == count &&
        binding->destination_offset == 0 && binding->source_offset == 0 && binding->field_type == 0) {
        return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4ec840):

uint FUN_004ec840(void)

{
  int *piVar1;
  int iVar2;
  uint3 uVar4;
  uint uVar3;
  uint *unaff_EBX;
  int iVar5;
  uint *puVar6;
  uint local_4;

  uVar3 = *unaff_EBX;
  if ((int)uVar3 < 0x41) {
    iVar5 = 0;
    local_4 = 0;
    if (0 < (int)uVar3) {
      puVar6 = unaff_EBX + 5;
      do {
        if (((puVar6[-2] == 0) && (puVar6[-1] == 0)) && (puVar6[-3] == 0)) break;
        if ((char)*puVar6 == '\0') {
          piVar1 = (int *)puVar6[-3];
          if ((char)piVar1[0x19] == '\0') {
            (**(code **)(&DAT_0069a2fc + *piVar1 * 0x18))(piVar1);
            iVar2 = (**(code **)(*piVar1 * 0x18 + 0x69a2f8))(piVar1);
            piVar1[0x17] = iVar2;
            *(undefined1 *)(piVar1 + 0x19) = 1;
          }
          *(undefined1 *)puVar6 = 1;
        }
        local_4 = local_4 + 1;
        iVar5 = iVar5 + 1;
        puVar6 = puVar6 + 4;
      } while (iVar5 < (int)*unaff_EBX);
    }
    puVar6 = unaff_EBX + iVar5 * 4 + 2;
    uVar4 = (uint3)((uint)puVar6 >> 8);
    if (((unaff_EBX[iVar5 * 4 + 3] == 0) && (puVar6[2] == 0)) && (*puVar6 == 0)) {
      uVar3 = CONCAT31(uVar4,1);
    }
    else {
      uVar3 = (uint)uVar4 << 8;
    }
    if ((local_4 == *unaff_EBX) && ((char)uVar3 != '\0')) {
      return CONCAT31((int3)(uVar3 >> 8),1);
    }
  }
  return uVar3 & 0xffffff00;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
