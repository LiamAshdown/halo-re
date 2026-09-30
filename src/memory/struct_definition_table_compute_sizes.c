// struct_definition_table_compute_sizes  (Ghidra: FUN_004d0980)
// address 0x4d0980, size 79 bytes
// name confidence: 0.6   rewrite confidence: 0.75
// evidence: only caller/callee is struct_definition_compute_size (0x4d0d50); the walked table is
// indexed exactly like data_packet_group (int16 type_count at +0x04, data_packet_type[] at +0x10,
// stride 8, definition at +4) per out/phase4/memory_types_notes.md and types/memory.h. This
// function lazily computes and caches (struct_definition::size_computed) every packet type's
// encoded size, the same one-shot flag struct_definition_encode/_decode check before trusting a
// field's computed_size.
// register convention: cdecl, no register args (param_1 in EAX per Ghidra's default __thiscall-ish
// display, but the call site passes it as an ordinary stack/register argument; no in_EAX/in_ECX
// aliasing observed in this function's body)

#include "tags.h"
#include "memory.h"
#include "fn_memory.h"

// blam-cc: struct_definition_compute_size(definition, out_size, fields, out_field_count)


void struct_definition_table_compute_sizes(data_packet_group *group)
{
    int16_t i;
    int16_t discarded_field_count; // UNSURE: Ghidra passes &param_1 here, clobbering the
                                    // function's own group-pointer parameter; a saved copy
                                    // (this loop's `group`) is what is actually iterated with,
                                    // so the clobbered value is never read. A throwaway local
                                    // reproduces that exactly without the confusing aliasing.
    int16_t discarded_size;        // Ghidra: local_4, a 4-byte stack buffer used only as a
                                    // discarded out_size destination.

    for (i = 0; i < group->type_count; i = i + 1) {
        struct_definition *definition = group->types[i].definition;
        if (definition != 0 && definition->size_computed == 0) {
            struct_definition_compute_size(definition, &discarded_size, definition->fields,
                &discarded_field_count);
            definition->size_computed = 1;
        }
    }
}

#if 0
Original Ghidra decompilation (0x4d0980):

void FUN_004d0980(int param_1)

{
  int iVar1;
  int iVar2;
  short sVar3;
  undefined1 local_4 [4];

  iVar2 = param_1;
  sVar3 = 0;
  if (0 < *(short *)(param_1 + 4)) {
    do {
      iVar1 = *(int *)(*(int *)(iVar2 + 0x10) + 4 + sVar3 * 8);
      if ((iVar1 != 0) && (*(char *)(iVar1 + 0x10) == '\0')) {
        struct_definition_compute_size(iVar1,local_4,*(undefined4 *)(iVar1 + 0xc),&param_1);
        *(undefined1 *)(iVar1 + 0x10) = 1;
      }
      sVar3 = sVar3 + 1;
    } while (sVar3 < *(short *)(iVar2 + 4));
  }
  return;
}
#endif
