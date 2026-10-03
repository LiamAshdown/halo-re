// input_record_windows_key_message  (Ghidra: already named)
// address 0x490d10, size 228 bytes
// name confidence: 0.55   rewrite confidence: 0.65
// evidence: out/phase4/input_functions.md summary "WndProc-driven handler that records a
// Windows keyboard message (key or character) with its modifier state into the text/bind-capture
// input buffer."; types/interface.h ui_key_event (modifiers/character/key_code, exactly the byte
// layout the decompile hand-assembles); types/input.h virtual_key_to_key[0x100] (0x0065ba58),
// character_to_key[0x80] (0x0065bc58), key_events[0x40] ring (0x006b16fe) and
// key_event_count (0x006b16fc, capacity k_input_key_event_capacity).
// register convention: wparam in EAX (in_EAX), message in ECX (in_ECX)

#include "win32.h"
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
extern uint8_t input_acquired;                    // 0x006b15f8
extern int16_t virtual_key_to_key[0x100];         // 0x0065ba58
extern int16_t character_to_key[0x80];            // 0x0065bc58
extern int16_t key_event_count;                   // 0x006b16fc
extern ui_key_event key_events[k_input_key_event_capacity]; // 0x006b16fe


// blam-cc: wparam in EAX, message in ECX
// Records one WM_KEYDOWN/WM_SYSKEYDOWN or WM_CHAR/WM_SYSCHAR message, with the current
// shift/control/alt state, into the key event ring, while input is acquired. A key-down message
// with no mapped key index, or a char message whose raw character byte is 0xff, is dropped.
// Ignores every other message.
void input_record_windows_key_message(uint32_t wparam, int32_t message)
{
    ui_key_event event;

    if (input_acquired == 0) {
        return;
    }

    if (message == 0x100 || message == 0x104) { // WM_KEYDOWN / WM_SYSKEYDOWN
        event.key_code = virtual_key_to_key[wparam];
        if (event.key_code == -1) {
            return;
        }
        event.character = 0xff;
    } else if (message == 0x102 || message == 0x106) { // WM_CHAR / WM_SYSCHAR
        event.character = (uint8_t)wparam;
        event.key_code = -1;
        if (wparam < 0x80) {
            event.key_code = character_to_key[wparam];
        }
        if (event.character == 0xff) {
            return;
        }
    } else {
        return;
    }

    event.modifiers = 0;
    if (((uint16_t)GetKeyState(0x10) >> 15) != 0) { // VK_SHIFT
        event.modifiers = event.modifiers | _input_modifier_shift_bit;
    }
    if (GetKeyState(0x11) < 0) { // VK_CONTROL
        event.modifiers = event.modifiers | _input_modifier_control_bit;
    }
    if (GetKeyState(0x12) < 0) { // VK_MENU
        event.modifiers = event.modifiers | _input_modifier_alt_bit;
    }

    if (key_event_count < k_input_key_event_capacity) {
        key_events[key_event_count] = event;
        key_event_count = key_event_count + 1;
    }
}

#if 0
Original Ghidra decompilation (0x490d10):

void input_record_windows_key_message(void)

{
  short sVar1;
  SHORT SVar2;
  uint in_EAX;
  int in_ECX;
  byte bVar3;
  undefined4 local_4;

  if (DAT_006b15f8 == '\0') {
    return;
  }
  if ((in_ECX == 0x100) || (in_ECX == 0x104)) {
    sVar1 = *(short *)(&DAT_0065ba58 + in_EAX * 2);
    if (sVar1 == -1) {
      return;
    }
    bVar3 = 0xff;
    local_4 = 0xff00;
LAB_00490d86:
    local_4 = CONCAT22(sVar1,(short)local_4);
    if (sVar1 != -1) goto LAB_00490d95;
  }
  else {
    if ((in_ECX != 0x102) && (in_ECX != 0x106)) {
      return;
    }
    bVar3 = (byte)in_EAX;
    local_4._0_2_ = (ushort)bVar3 << 8;
    local_4 = CONCAT22(0xffff,(short)local_4);
    if (in_EAX < 0x80) {
      sVar1 = *(short *)(&DAT_0065bc58 + in_EAX * 2);
      goto LAB_00490d86;
    }
  }
  if (bVar3 == 0xff) {
    return;
  }
LAB_00490d95:
  SVar2 = GetKeyState(0x10);
  bVar3 = (byte)((ushort)SVar2 >> 0xf);
  SVar2 = GetKeyState(0x11);
  if (SVar2 < 0) {
    bVar3 = bVar3 | 2;
  }
  SVar2 = GetKeyState(0x12);
  if (SVar2 < 0) {
    bVar3 = bVar3 | 4;
  }
  local_4 = CONCAT31(local_4._1_3_,bVar3);
  if (DAT_006b16fc < 0x40) {
    (&DAT_006b16fe)[DAT_006b16fc] = local_4;
    DAT_006b16fc = DAT_006b16fc + 1;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
