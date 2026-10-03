// input_reset_state_and_axis_configs  (Ghidra: already named)
// address 0x490aa0, size 164 bytes
// name confidence: 0.5   rewrite confidence: 0.7
// evidence: out/phase4/input_functions.md summary "Resets keyboard, mouse, and joystick runtime
// input state and reseeds each joystick's axis-binding configuration from the default profile.";
// types/input.h globals list matches every address: key_frames[0x6d] (0x006b1620),
// key_event_read_index/key_event_count (0x006b16fa/0x006b16fc), key_events[0x40] (0x006b16fe,
// only the first 0x10 records zeroed here, matching input_keyboard_device_create's own note),
// key_release_pending[0x6d] (0x006b168d), mouse_state (0x006b180c, 7 dwords), joystick_states[4]
// (0x006b2a68, stride 0xa0) and joystick_neutral_state (0x006b2cf8).
// register convention: no parameters, no return value.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"

#include <string.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint8_t key_frames[0x6d];           // 0x006b1620
extern int16_t key_event_read_index;       // 0x006b16fa
extern int16_t key_event_count;            // 0x006b16fc
extern ui_key_event key_events[k_input_key_event_capacity];      // 0x006b16fe
extern uint8_t key_release_pending[0x6d];  // 0x006b168d
extern mouse_state live_mouse_state;       // 0x006b180c
extern joystick_state joystick_states[4];  // 0x006b2a68
extern joystick_state joystick_neutral_state; // 0x006b2cf8

// Zeroes the keyboard key-frame and release-pending arrays, the first 16 slots of the key event
// ring (and its read index / count), the live mouse state, and reseeds every joystick slot's
// state from the (all-zero) joystick_neutral_state.
void input_reset_state_and_axis_configs(void)
{
    int32_t i;

    memset(key_frames, 0, sizeof(key_frames));
    key_event_read_index = 0;
    key_event_count = 0;
    memset(key_events, 0, sizeof(ui_key_event) * 0x10);
    memset(key_release_pending, 0, sizeof(key_release_pending));
    memset(&live_mouse_state, 0, sizeof(live_mouse_state));

    for (i = 0; i < 4; i++) {
        joystick_states[i] = joystick_neutral_state;
    }
}

#if 0
Original Ghidra decompilation (0x490aa0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void input_reset_state_and_axis_configs(void)

{
  int iVar1;
  undefined4 *puVar2;
  undefined4 *puVar3;

  puVar2 = (undefined4 *)&DAT_006b1620;
  for (iVar1 = 0x1b; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar2 = 0;
    puVar2 = puVar2 + 1;
  }
  *(undefined1 *)puVar2 = 0;
  DAT_006b16fa = 0;
  DAT_006b16fc = 0;
  puVar2 = &DAT_006b16fe;
  for (iVar1 = 0x10; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar2 = 0;
    puVar2 = puVar2 + 1;
  }
  puVar2 = &DAT_006b168d;
  for (iVar1 = 0x1b; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar2 = 0;
    puVar2 = puVar2 + 1;
  }
  *(undefined1 *)puVar2 = 0;
  DAT_006b180c = 0;
  DAT_006b1810 = 0;
  DAT_006b1814 = 0;
  _DAT_006b1818 = 0;
  _DAT_006b181c = 0;
  _DAT_006b1820 = 0;
  _DAT_006b1824 = 0;
  puVar2 = &DAT_006b2cf8;
  puVar3 = &DAT_006b2a68;
  for (iVar1 = 0x28; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar3 = *puVar2;
    puVar2 = puVar2 + 1;
    puVar3 = puVar3 + 1;
  }
  puVar2 = &DAT_006b2cf8;
  puVar3 = &DAT_006b2b08;
  for (iVar1 = 0x28; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar3 = *puVar2;
    puVar2 = puVar2 + 1;
    puVar3 = puVar3 + 1;
  }
  puVar2 = &DAT_006b2cf8;
  puVar3 = &DAT_006b2ba8;
  for (iVar1 = 0x28; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar3 = *puVar2;
    puVar2 = puVar2 + 1;
    puVar3 = puVar3 + 1;
  }
  puVar2 = &DAT_006b2cf8;
  puVar3 = &DAT_006b2c48;
  for (iVar1 = 0x28; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar3 = *puVar2;
    puVar2 = puVar2 + 1;
    puVar3 = puVar3 + 1;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
