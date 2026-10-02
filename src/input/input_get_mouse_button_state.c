// input_get_mouse_button_state  (Ghidra: FUN_00490e00)
// address 0x490e00, size 34 bytes
// name confidence: 0.65   rewrite confidence: 0.7
// evidence: out/phase4/input_types_notes.md names this "input_get_mouse_button_state 0x490e00
// (+0x0c)"; 0x006b1818 is live_mouse_state.button_frames (mouse_state base 0x006b180c, +0xc).
// The one caller (interface 0x4c65fe: push 0x2 ; call 0x490e00 ; add esp,4 ; cmp al,1) only reads the result as a
// char, confirming the meaningful part of the return value is the low byte (the hold-frame
// count); the upper bytes Ghidra shows coming from the caller's stale EAX are not real output.
// register convention: mouse button index as the recognized parameter (param_1)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void *mouse_device;             // 0x006b1804, IDirectInputDevice8A
extern uint8_t input_suppressed;       // 0x006b15f9
extern mouse_state live_mouse_state;        // 0x006b180c

// Returns the current hold-frame count for mouse button button_index, or 0 while the mouse
// device is absent or input is suppressed.
uint8_t input_get_mouse_button_state(int16_t button_index)
{
    uint8_t result;

    result = 0;
    if (mouse_device != 0 && input_suppressed == 0) {
        result = live_mouse_state.button_frames[button_index];
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x490e00):

uint FUN_00490e00(short param_1)

{
  uint in_EAX;
  uint uVar1;

  uVar1 = in_EAX & 0xffffff00;
  if ((DAT_006b1804 != 0) && (DAT_006b15f9 == '\0')) {
    uVar1 = CONCAT31((int3)(char)((ushort)param_1 >> 8),(&DAT_006b1818)[param_1]);
  }
  return uVar1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
