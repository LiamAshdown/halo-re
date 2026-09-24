// input_keyboard_set_capture_mode  (Ghidra: FUN_0048b650; renamed per
//   out/phase4/input_types_notes.md, "the keyboard vtable call +0x28 is GetDeviceData(0x14,
//   NULL, &INFINITE, 0), a buffer flush, not SetProperty")
// address 0x48b650, size 89 bytes
// name confidence: 0.55   rewrite confidence: 0.7
// evidence: sets/clears input_abstraction_globals::mode_flags bit 2
//   (_input_mode_keyboard_capture_bit), then, if the keyboard device exists, flushes its
//   buffered DirectInput data (vtable+0x28 == IDirectInputDevice8::GetDeviceData, matching the
//   identical tail already rewritten in src/interface/virtual_keyboard_close.c /
//   chat_close.c / virtual_keyboard_open.c) and clears key_frames / key_release_pending
//   (0x006b1620 / 0x006b168d, each 0x6d bytes per types/input.h). Both known call sites
//   (0x4aa8da, tail-jumped from 0x4c656a) pass AL = 1; the AL == 0 (release capture) path is
//   never exercised in this build but is preserved as written.
// register convention: AL -> enable_capture (bool), no return value.
//   // blam-cc: AL -> enable_capture

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"
#include <string.h>

extern input_abstraction_globals input_globals; // 0x00710328
extern void *keyboard_device;                                 // 0x006b1800, IDirectInputDevice8A
extern uint8_t key_frames[0x6d];                               // 0x006b1620
extern uint8_t key_release_pending[0x6d];                      // 0x006b168d

// Switches the keyboard device between normal input mode and rebind-capture mode, and clears
// the per-key press/hold state arrays whenever the keyboard device is present.
void input_keyboard_set_capture_mode(uint8_t enable_capture)
{
    if (enable_capture == 0) {
        input_globals.mode_flags = input_globals.mode_flags & ~_input_mode_keyboard_capture_bit;
    } else {
        input_globals.mode_flags = input_globals.mode_flags | _input_mode_keyboard_capture_bit;
    }

    if (keyboard_device != 0) {
        uint32_t flush_all = 0xffffffff; // INFINITE: GetDeviceData flushes every buffered element
        void **vtable = *(void ***)keyboard_device;
        ((idirectinputdevice8_getdevicedata_proc)vtable[0x28 / 4])(keyboard_device,
            sizeof(di_device_object_data), (di_device_object_data *)0, &flush_all, 0);
        memset(key_release_pending, 0, sizeof(key_release_pending));
        memset(key_frames, 0, sizeof(key_frames));
    }
}

#if 0
Original Ghidra decompilation (0x48b650), from tools/pack.py 0x48b650:

void FUN_0048b650(void)

{
  char in_AL;
  int iVar1;
  undefined4 *puVar2;
  undefined4 local_4;

  if (in_AL == '\0') {
    DAT_00712542 = DAT_00712542 & 0xfb;
  }
  else {
    DAT_00712542 = DAT_00712542 | 4;
  }
  if (DAT_006b1800 != (int *)0x0) {
    local_4 = 0xffffffff;
    (**(code **)(*DAT_006b1800 + 0x28))(DAT_006b1800,0x14,0,&local_4,0);
    puVar2 = &DAT_006b168d;
    for (iVar1 = 0x1b; iVar1 != 0; iVar1 = iVar1 + -1) {
      *puVar2 = 0;
      puVar2 = puVar2 + 1;
    }
    *(undefined1 *)puVar2 = 0;
    puVar2 = (undefined4 *)&DAT_006b1620;
    for (iVar1 = 0x1b; iVar1 != 0; iVar1 = iVar1 + -1) {
      *puVar2 = 0;
      puVar2 = puVar2 + 1;
    }
    *(undefined1 *)puVar2 = 0;
  }
  return;
}

objdump call-site evidence for the AL argument (both known callers):
  004aa8d1: mov al,0x1
  004aa8da: call 0x48b650
  004c6567: mov al,0x1
  004c656a: jmp 0x48b650                 ; tail call, same AL=1
#endif
