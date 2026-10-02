// message_delta_definitions_teardown_field_bindings  (Ghidra: FUN_004ec750; named per this rewrite)
// address 0x4ec750, size 45 bytes
// name confidence: 0.3   rewrite confidence: 0.6
// evidence: objdump -d -M intel bin/halo.exe @0x4ec750: mirrors
// message_delta_definitions_invoke_field_bindings (0x4ec390) but calls
// message_delta_field_bindings_teardown (0x4ec900) on both lists and additionally clears
// definition->initialized (definition+0x18, matching types/networking.h
// message_delta_definition::initialized) for every entry.
// register convention: no register-passed arguments.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern message_delta_definition *message_delta_definitions[56]; // 0x0065d440
extern void message_delta_field_bindings_teardown(message_delta_static_fields *list); // 0x4ec900, this module

// Runs message_delta_field_bindings_teardown over both field-binding lists of every registered
// message type and clears each definition's initialized flag, undoing
// message_delta_field_layout_compute_size.
void message_delta_definitions_teardown_field_bindings(void)
{
    int32_t i;
    message_delta_definition *definition;

    for (i = 0; i < k_network_message_definition_count; i++) {
        definition = message_delta_definitions[i];
        message_delta_field_bindings_teardown(definition->statics);
        message_delta_field_bindings_teardown((message_delta_static_fields *)&definition->field_count);
        definition->initialized = 0;
    }
}

#if 0
Original Ghidra decompilation (0x4ec750):

void FUN_004ec750(void)

{
  undefined *puVar1;
  undefined **ppuVar2;

  ppuVar2 = &PTR_DAT_0065d440;
  do {
    puVar1 = *ppuVar2;
    FUN_004ec900();
    FUN_004ec900();
    ppuVar2 = ppuVar2 + 1;
    puVar1[0x18] = 0;
  } while ((int)ppuVar2 < 0x65d520);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
