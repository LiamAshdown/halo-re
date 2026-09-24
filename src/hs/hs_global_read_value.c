// hs_global_read_value  (Ghidra: hs_global_read_value, already named)
// address 0x48aec0, size 202 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: types/hs.h hs_global_definition (type 0x04, address 0x08) and hs_global (value 0x04);
//   the type-code groups match hs_type exactly (5 boolean, 6 real, 7 short, 8 long, 9 string,
//   then every other real value type 0x0a..0x2b split into "2-byte, default 0xffff" for the
//   short-sized ones and "4-byte, default 0xffffffff" for the datum-handle-sized ones).
// register convention: hs_global_reference in EAX (in_EAX). Only builtin references (bit 15 set)
//   do anything; a scenario-defined reference is a silent no-op.
//   // blam-cc: EAX -> reference

#include "tags.h"
#include "memory.h"
#include "hs.h"

extern hs_global_definition *hs_global_definitions[k_hs_builtin_global_count]; // 0x0068b398
extern char *hs_empty_string; // 0x00688b50

extern data_array *hs_globals_data; // 0x0087a46c

// Copies the current value of the bound engine variable for builtin global `reference` into its
// hs_global storage slot, dispatched by the builtin's declared type. Does nothing for a
// scenario-defined reference (bit 15 clear) or for a builtin whose definition has no bound
// address, beyond writing that type's documented default.
void hs_global_read_value(hs_global_reference reference)
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
        slot->value.boolean_value = definition->address ? *(uint8_t *)definition->address : 0;
        break;
    case _hs_type_real:
        slot->value.long_value = definition->address ? *(int32_t *)definition->address : 0;
        break;
    case _hs_type_short:
        slot->value.short_value = definition->address ? *(int16_t *)definition->address : 0;
        break;
    case _hs_type_long:
        slot->value.long_value = definition->address ? *(int32_t *)definition->address : 0;
        break;
    case _hs_type_string:
        slot->value.string_value = definition->address ?
            *(char **)definition->address : hs_empty_string;
        break;
    case 10: case 11: case 12: case 13: case 14: case 15: case 16:
    case 18: case 19: case 20: case 21: case 22:
    case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x2b:
        slot->value.short_value = definition->address ? *(int16_t *)definition->address : (int16_t)0xffff;
        break;
    case 17:
    case 0x17: case 0x18: case 0x19: case 0x1a: case 0x1b: case 0x1c:
    case 0x1d: case 0x1e: case 0x1f:
    case 0x25: case 0x26: case 0x27: case 0x28: case 0x29: case 0x2a:
        slot->value.long_value = definition->address ? *(int32_t *)definition->address : -1;
        break;
    }
}

#if 0
Original Ghidra decompilation (0x48aec0):

void hs_global_read_value(void)

{
  int iVar1;
  undefined *puVar2;
  uint in_EAX;
  undefined2 *puVar3;

  if ((char)(in_EAX >> 8) < '\0') {
    puVar2 = (&PTR_PTR_0068b398)[in_EAX & 0x7fff];
    iVar1 = *(int *)(DAT_0087a46c + 0x34) + (in_EAX & 0x7fff) * 8;
    switch(*(undefined2 *)(puVar2 + 4)) {
    case 5:
      if (*(undefined1 **)(puVar2 + 8) == (undefined1 *)0x0) {
        *(undefined1 *)(iVar1 + 4) = 0;
        return;
      }
      *(undefined1 *)(iVar1 + 4) = **(undefined1 **)(puVar2 + 8);
      return;
    case 6:
      if (*(undefined4 **)(puVar2 + 8) == (undefined4 *)0x0) {
        *(undefined4 *)(iVar1 + 4) = 0;
        return;
      }
      *(undefined4 *)(iVar1 + 4) = **(undefined4 **)(puVar2 + 8);
      return;
    case 7:
      puVar3 = *(undefined2 **)(puVar2 + 8);
      if (puVar3 == (undefined2 *)0x0) {
        *(undefined2 *)(iVar1 + 4) = 0;
        return;
      }
LAB_0048af6d:
      *(undefined2 *)(iVar1 + 4) = *puVar3;
      return;
    case 8:
      if (*(undefined4 **)(puVar2 + 8) == (undefined4 *)0x0) {
        *(undefined4 *)(iVar1 + 4) = 0;
        return;
      }
      *(undefined4 *)(iVar1 + 4) = **(undefined4 **)(puVar2 + 8);
      return;
    case 9:
      if (*(undefined4 **)(puVar2 + 8) == (undefined4 *)0x0) {
        *(undefined **)(iVar1 + 4) = PTR_DAT_00688b50;
        return;
      }
      *(undefined4 *)(iVar1 + 4) = **(undefined4 **)(puVar2 + 8);
      return;
    case 10:
    case 0xb:
    case 0xc:
    case 0xd:
    case 0xe:
    case 0xf:
    case 0x10:
    case 0x12:
    case 0x13:
    case 0x14:
    case 0x15:
    case 0x16:
    case 0x20:
    case 0x21:
    case 0x22:
    case 0x23:
    case 0x24:
    case 0x2b:
      puVar3 = *(undefined2 **)(puVar2 + 8);
      if (puVar3 != (undefined2 *)0x0) goto LAB_0048af6d;
      *(undefined2 *)(iVar1 + 4) = 0xffff;
      break;
    case 0x11:
    case 0x17:
    case 0x18:
    case 0x19:
    case 0x1a:
    case 0x1b:
    case 0x1c:
    case 0x1d:
    case 0x1e:
    case 0x1f:
    case 0x25:
    case 0x26:
    case 0x27:
    case 0x28:
    case 0x29:
    case 0x2a:
      if (*(undefined4 **)(puVar2 + 8) == (undefined4 *)0x0) {
        *(undefined4 *)(iVar1 + 4) = 0xffffffff;
        return;
      }
      *(undefined4 *)(iVar1 + 4) = **(undefined4 **)(puVar2 + 8);
      return;
    }
  }
  return;
}
#endif
