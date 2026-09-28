// message_delta_parameters_protocol_free_registered  (Ghidra: message_delta_parameters_protocol_free_registered, already named)
// address 0x4ebd50, size 72 bytes
// name confidence: 0.55   rewrite confidence: 0.8
// evidence: out/phase4/networking_functions.md summary; out/phase4/networking_types_notes.md
// "message delta protocol": message_delta_parameters_protocol_register and _pack_values both
// stride 0x006b86c0 by 3 dwords (name, type, value), matching message_delta_parameter in
// types/networking.h.
// register convention: no register-passed arguments; the function takes none.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern uint8_t message_delta_parameters_enabled;      // 0x0071cfa8
extern int32_t message_delta_parameter_count;         // 0x0071cfb0
extern message_delta_parameter message_delta_parameters[]; // 0x006b86c0

// Frees every GlobalAlloc'd name string in the dynamic-parameters registration table (only
// while the parameters protocol is enabled) and resets the registration count to zero.
void message_delta_parameters_protocol_free_registered(void)
{
    int32_t i;

    if (message_delta_parameters_enabled == 1) {
        for (i = 0; i < message_delta_parameter_count; i++) {
            GlobalFree(message_delta_parameters[i].name);
            message_delta_parameters[i].name = 0;
        }
        message_delta_parameter_count = 0;
    }
}

#if 0
Original Ghidra decompilation (0x4ebd50):

void message_delta_parameters_protocol_free_registered(void)

{
  int iVar1;
  undefined4 *puVar2;
  int iVar3;

  if (DAT_0071cfa8 == '\x01') {
    iVar3 = 0;
    if (0 < DAT_0071cfb0) {
      puVar2 = &DAT_006b86c0;
      do {
        GlobalFree((HGLOBAL)*puVar2);
        iVar1 = DAT_0071cfb0;
        *puVar2 = 0;
        iVar3 = iVar3 + 1;
        puVar2 = puVar2 + 3;
      } while (iVar3 < iVar1);
    }
    DAT_0071cfb0 = 0;
  }
  return;
}
#endif
