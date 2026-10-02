// message_delta_definitions_invoke_field_bindings  (Ghidra: FUN_004ec390; named per this rewrite)
// address 0x4ec390, size 43 bytes
// name confidence: 0.35   rewrite confidence: 0.55
// evidence: objdump -d -M intel bin/halo.exe @0x4ec390: for every entry of
// message_delta_definitions[56], loads `ebx = definition->statics` and calls
// message_delta_field_bindings_invoke (0x4ec700), then `ebx = &definition->field_count` (the
// inline field list at definition+0x20) and calls it again -- the same two field-binding lists
// message_delta_field_layout_compute_size (0x4ec790) walks. Its only caller is the message-type
// 0x22 case of the network game message dispatcher (out of this module's range, 0x4da320),
// immediately after message_delta_parameters_protocol_receive_update runs.
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
extern void message_delta_field_bindings_invoke(message_delta_static_fields *list); // 0x4ec700, this module

// Runs message_delta_field_bindings_invoke over both field-binding lists (the separately
// allocated statics list and the definition's own inline field list) of every registered
// message type.
void message_delta_definitions_invoke_field_bindings(void)
{
    int32_t i;
    message_delta_definition *definition;

    for (i = 0; i < k_network_message_definition_count; i++) {
        definition = message_delta_definitions[i];
        message_delta_field_bindings_invoke(definition->statics);
        message_delta_field_bindings_invoke((message_delta_static_fields *)&definition->field_count);
    }
}

#if 0
Original Ghidra decompilation (0x4ec390):

void FUN_004ec390(void)

{
  undefined **ppuVar1;

  ppuVar1 = &PTR_DAT_0065d440;
  do {
    FUN_004ec700();
    FUN_004ec700();
    ppuVar1 = ppuVar1 + 1;
  } while ((int)ppuVar1 < 0x65d520);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
