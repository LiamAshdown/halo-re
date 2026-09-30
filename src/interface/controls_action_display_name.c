// controls_action_display_name  (Ghidra: FUN_004b44c0, named in phase 4)
// address 0x4b44c0, size 90 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: rewritten from objdump 0x4b44c0..0x4b4519 in the phase-4 review. EAX (the device)
// and EDI (the action name) pass straight through to controls_enumerate_next_assignable_action
// (0x4b43e0, with a stack record and the retry byte 1); a found binding is named by 0x48c7f0
// (EAX binding, ECX out) into the static wide buffer at 0x006b3d48, and an empty name becomes
// L"???" (0x00669cc8, copied with 0x625bba for wcslen of it). The first rewrite had neither
// register argument.
// register convention: EAX device, EDI action name.
//   // blam-cc: device -> EAX, action_name -> EDI

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "fn_interface.h"

extern uint16_t controls_action_name_buffer[]; // 0x006b3d48
extern const uint16_t hud_text_unbound[];      // 0x00669cc8, L"???"


extern void input_get_binding_display_name(uint8_t *binding, uint16_t *out_name); // 0x48c7f0; blam-cc: EAX -> binding, ECX -> out_name
extern uint32_t wcslen_halo(const uint16_t *text); // 0x625b7a, wcslen

// blam-cc: device -> EAX, action_name -> EDI
uint16_t *controls_action_display_name(int32_t device, const char *action_name)
{
    int16_t record[6];

    controls_action_name_buffer[0] = 0;
    if (controls_enumerate_next_assignable_action(device, record, action_name, 1) != 0) {
        input_get_binding_display_name((uint8_t *)record, controls_action_name_buffer);
    }
    if (controls_action_name_buffer[0] == 0) {
        wcscpy(controls_action_name_buffer, hud_text_unbound); // 0x625bba ignores the third (length) push
    }
    return controls_action_name_buffer;
}

#if 0
Original Ghidra decompilation (0x4b44c0):

undefined2 * FUN_004b44c0(void)

{
  char cVar1;

  DAT_006b3d48 = 0;
  cVar1 = FUN_004b43e0(1);
  if (cVar1 != '\0') {
    FUN_0048c7f0();
  }
  if (DAT_006b3d48 == 0) {
    FUN_00625b7a(&DAT_00669cc8);
    _wcscpy(&DAT_006b3d48,L"???");
  }
  return &DAT_006b3d48;
}
#endif
