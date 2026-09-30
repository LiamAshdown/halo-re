// message_delta_protocol_initialize  (Ghidra: message_delta_protocol_initialize, already named)
// address 0x4ec2f0, size 60 bytes
// name confidence: 0.5   rewrite confidence: 0.7
// evidence: out/phase4/networking_types_notes.md "message delta protocol": iterates the
// definition pointer table 0x0065d440..0x0065d520 (56 entries) and the 28-record, 0x18-stride
// table at 0x0069a304 whose first byte this sets to 1 (contents otherwise unresolved).
// register convention: no register-passed arguments.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "fn_networking.h"

extern uint8_t message_delta_parameters_enabled;                // 0x0071cfa8
extern uint8_t message_delta_unknown_table_0069a304[28][0x18];  // 0x0069a304, UNSURE: contents beyond byte 0
extern message_delta_definition *message_delta_definitions[56]; // 0x0065d440


// One-time message-delta protocol startup: reloads parameters.cfg (when the parameters protocol
// is enabled), marks every entry of an unresolved 28-record table, and computes each registered
// message type's encoded layout size.
void message_delta_protocol_initialize(void)
{
    int32_t i;

    if (message_delta_parameters_enabled == 1) {
        message_delta_parameters_protocol_reload_from_config_file();
    }
    for (i = 0; i < 28; i++) {
        message_delta_unknown_table_0069a304[i][0] = 1;
    }
    for (i = 0; i < k_network_message_definition_count; i++) {
        message_delta_field_layout_compute_size(message_delta_definitions[i]);
    }
}

#if 0
Original Ghidra decompilation (0x4ec2f0):

void message_delta_protocol_initialize(void)

{
  undefined1 *puVar1;
  undefined **ppuVar2;

  if (DAT_0071cfa8 == '\x01') {
    message_delta_parameters_protocol_reload_from_config_file();
  }
  puVar1 = &DAT_0069a304;
  do {
    *puVar1 = 1;
    puVar1 = puVar1 + 0x18;
  } while ((int)puVar1 < 0x69a5a4);
  ppuVar2 = &PTR_DAT_0065d440;
  do {
    message_delta_field_layout_compute_size();
    ppuVar2 = ppuVar2 + 1;
  } while ((int)ppuVar2 < 0x65d520);
  return;
}
#endif
