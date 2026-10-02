// hs_global_write_value  (Ghidra: hs_global_write_value, already named)
// address 0x48b030, size 124 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: types/hs.h hs_global_definition (type 0x04, address 0x08) and hs_global (value 0x04);
//   mirror image of hs_global_read_value's type-code groups (this direction writes 4 bytes for
//   real/long/object-family types and 2 bytes for short/enum/trigger-volume-family types, with a
//   handful of types that hs_global_read_value groups differently -- preserved exactly, not
//   reconciled, since the two switches are not simple mirrors of each other).
// register convention: hs_global_reference in EAX (in_EAX). Only builtin references (bit 15 set)
//   do anything.
//   // blam-cc: EAX -> reference

#include "tags.h"
#include "memory.h"
#include "hs.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern hs_global_definition *hs_global_definitions[k_hs_builtin_global_count]; // 0x0068b398
extern data_array *hs_globals_data; // 0x0087a46c

// Writes builtin global `reference`'s current hs_global storage value back into its bound engine
// variable, dispatched by the builtin's declared type. Does nothing for a scenario-defined
// reference, or for a builtin whose definition has no bound address.
void hs_global_write_value(hs_global_reference reference)
{
    hs_global_definition *definition;
    hs_global *slot;

    if ((reference & k_hs_global_builtin_bit) == 0) {
        return;
    }

    definition = hs_global_definitions[reference & k_hs_global_index_mask];
    slot = (hs_global *)((uint8_t *)hs_globals_data->data +
        (reference & k_hs_global_index_mask) * 8);

    switch (definition->type) {
    case _hs_type_boolean:
        if (definition->address) {
            *(uint8_t *)definition->address = slot->value.boolean_value;
        }
        break;
    case _hs_type_real: case _hs_type_long:
    case 0x18: case 0x1a: case 0x1c: case 0x1e: case 0x26: case 0x28: case 0x2a:
        if (definition->address) {
            *(int32_t *)definition->address = slot->value.long_value;
        }
        break;
    case _hs_type_short:
    case 11: case 13: case 15: case 19: case 21:
    case 0x21: case 0x23: case 0x2b:
        if (definition->address) {
            *(int16_t *)definition->address = slot->value.short_value;
        }
        break;
    case _hs_type_string:
    case 17:
    case 0x17: case 0x19: case 0x1b: case 0x1d: case 0x1f:
    case 0x25: case 0x27: case 0x29:
        if (definition->address) {
            *(int32_t *)definition->address = slot->value.long_value;
        }
        break;
    case 10: case 12: case 14: case 16: case 18: case 20: case 22:
    case 0x20: case 0x22: case 0x24:
        if (definition->address) {
            *(int16_t *)definition->address = slot->value.short_value;
        }
        break;
    }
}

#if 0
Original Ghidra decompilation (0x48b030):

void hs_global_write_value(void)

{
  int iVar1;
  undefined *puVar2;
  uint in_EAX;

  if ((char)(in_EAX >> 8) < '\0') {
    puVar2 = (&PTR_PTR_0068b398)[in_EAX & 0x7fff];
    iVar1 = *(int *)(DAT_0087a46c + 0x34) + (in_EAX & 0x7fff) * 8;
    switch(*(undefined2 *)(puVar2 + 4)) {
    case 5:
      if (*(undefined1 **)(puVar2 + 8) != (undefined1 *)0x0) {
        **(undefined1 **)(puVar2 + 8) = *(undefined1 *)(iVar1 + 4);
        return;
      }
      break;
    case 6:
    case 8:
    case 0x18:
    case 0x1a:
    case 0x1c:
    case 0x1e:
    case 0x26:
    case 0x28:
    case 0x2a:
      if (*(undefined4 **)(puVar2 + 8) != (undefined4 *)0x0) {
        **(undefined4 **)(puVar2 + 8) = *(undefined4 *)(iVar1 + 4);
        return;
      }
      break;
    case 7:
    case 0xb:
    case 0xd:
    case 0xf:
    case 0x13:
    case 0x15:
    case 0x21:
    case 0x23:
    case 0x2b:
      if (*(undefined2 **)(puVar2 + 8) != (undefined2 *)0x0) {
        **(undefined2 **)(puVar2 + 8) = *(undefined2 *)(iVar1 + 4);
      }
      break;
    case 9:
    case 0x11:
    case 0x17:
    case 0x19:
    case 0x1b:
    case 0x1d:
    case 0x1f:
    case 0x25:
    case 0x27:
    case 0x29:
      if (*(undefined4 **)(puVar2 + 8) != (undefined4 *)0x0) {
        **(undefined4 **)(puVar2 + 8) = *(undefined4 *)(iVar1 + 4);
        return;
      }
      break;
    case 10:
    case 0xc:
    case 0xe:
    case 0x10:
    case 0x12:
    case 0x14:
    case 0x16:
    case 0x20:
    case 0x22:
    case 0x24:
      if (*(undefined2 **)(puVar2 + 8) != (undefined2 *)0x0) {
        **(undefined2 **)(puVar2 + 8) = *(undefined2 *)(iVar1 + 4);
        return;
      }
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
