// console_deactivate  (Ghidra: console_deactivate, already named)
// address 0x4c64b0, size 119 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: types/main.h console_globals_data.active/enabled (0x006b7020/0x006b7021); the vtable+0x28
// call on keyboard_device (0x006b1800) matches types/input.h's idirectinputdevice8_getdevicedata_proc
// (GetDeviceData), called here with count = -1 to flush the buffered key event queue exactly as
// input_keyboard_set_capture_mode 0x48b650 documents in that header; key_frames/key_release_pending
// (0x006b1620/0x006b168d, both 0x6d bytes) are already named in src/input/input_directinput_poll_devices.c.
// register convention: __cdecl, no arguments.
// phase 4 review (disassembly 0x4c64b0..0x4c6526: console_close now gets EAX = &console_globals.terminal.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"
#include "main.h"
#include "fn_main.h"
#include "fn_interface.h"
#include <string.h>

extern console_globals console_globals_data; // 0x006b7020
extern input_abstraction_globals input_globals; // 0x00710328 (mode_flags at +0x221a, 0x00712542)
extern void *keyboard_device;           // 0x006b1800
extern uint8_t key_frames[0x6d];        // 0x006b1620
extern uint8_t key_release_pending[0x6d]; // 0x006b168d


// Closes the developer console (if it is both enabled and currently open), clears the console
// input-capture bit of the input mode flags, and, when the keyboard device is acquired, flushes
// its buffered key event queue and clears the key-frame/release-pending arrays.
void console_deactivate(void)
{
    uint32_t flush_all;

    if (console_globals_data.active != 0 && console_globals_data.enabled != 0) {
        console_close(&console_globals_data.terminal);   // 0x4c64c3 mov eax,0x6b7024
        input_globals.mode_flags = input_globals.mode_flags & (uint8_t)~_input_mode_keyboard_capture_bit;
        console_globals_data.active = 0;
        if (keyboard_device != 0) {
            flush_all = 0xffffffff;
            ((idirectinputdevice8_getdevicedata_proc)(*(void ***)keyboard_device)[0x28 / 4])
                (keyboard_device, sizeof(di_device_object_data), (di_device_object_data *)0, &flush_all, 0);
            memset(key_release_pending, 0, sizeof(key_release_pending));
            memset(key_frames, 0, sizeof(key_frames));
        }
    }
}

#if 0
Original Ghidra decompilation (0x4c64b0):

void __cdecl console_deactivate(void)

{
  int iVar1;
  undefined4 *puVar2;
  undefined4 local_4;

  if ((DAT_006b7020 != '\0') && (DAT_006b7021 != '\0')) {
    console_close();
    DAT_00712542 = DAT_00712542 & 0xfb;
    DAT_006b7020 = '\0';
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
  }
  return;
}
#endif
