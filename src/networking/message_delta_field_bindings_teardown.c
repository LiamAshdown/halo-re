// message_delta_field_bindings_teardown  (Ghidra: FUN_004ec900; named per this rewrite)
// address 0x4ec900, size 63 bytes
// name confidence: 0.35   rewrite confidence: 0.6
// evidence: objdump -d -M intel bin/halo.exe @0x4ec900: per-entry `cmp byte [edi],1` tests the
// low byte of message_delta_field_binding::initialized (edi = &fields[i].initialized, i.e.
// fields[i]+0xc); when set, tests field_type+0x64 (the same "already initialized" byte
// message_delta_field_bindings_lazy_init, 0x4ec840, writes), calls the per-type callback at
// table slot 0x0069a300 (0x0069a2f0+0x10, one slot further than that function's init callback),
// clears field_type+0x64, then clears the binding's own flag byte.
// register convention: the field-binding list header in ECX (in_ECX).
// blam-cc: ECX -> list
// UNSURE: whether 0x0069a300 is really "teardown" as opposed to some other per-message reset;
// it is only ever seen paired with message_delta_field_bindings_lazy_init's init/size slots.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "fn_networking.h"


extern message_delta_field_type_vtable message_delta_field_type_table[]; // 0x0069a2f0

// blam-cc: ECX -> list
// Tears down every field type in a message type's field-binding list that
// message_delta_field_bindings_lazy_init marked initialized, then clears both the per-type
// "initialized" byte and the per-binding flag byte this pass tests.
void message_delta_field_bindings_teardown(message_delta_static_fields *list)
{
    int32_t i;
    message_delta_field_binding *binding;
    uint8_t *binding_flag;
    uint8_t *type_flag;

    if (0 < list->count) {
        binding = list->fields;
        for (i = 0; i < list->count; i++, binding++) {
            binding_flag = &binding->initialized;
            if (*binding_flag == 1) {
                type_flag = (uint8_t *)binding->field_type + 0x64;
                if (*type_flag == 1) {
                    message_delta_field_type_table[*(int32_t *)binding->field_type].teardown(binding->field_type);
                    *type_flag = 0;
                }
                *binding_flag = 0;
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x4ec900):

void FUN_004ec900(void)

{
  int *piVar1;
  int *in_ECX;
  int iVar2;
  int *piVar3;

  iVar2 = *in_ECX;
  if (0 < iVar2) {
    piVar3 = in_ECX + 5;
    do {
      if ((char)*piVar3 == '\x01') {
        piVar1 = (int *)piVar3[-3];
        if ((char)piVar1[0x19] == '\x01') {
          (**(code **)(&DAT_0069a300 + *piVar1 * 0x18))(piVar1);
          *(undefined1 *)(piVar1 + 0x19) = 0;
        }
        *(char *)piVar3 = '\0';
      }
      piVar3 = piVar3 + 4;
      iVar2 = iVar2 + -1;
    } while (iVar2 != 0);
  }
  return;
}
#endif
