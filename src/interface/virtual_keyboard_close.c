// virtual_keyboard_close  (Ghidra: virtual_keyboard_close, already named)
// address 0x4a9250, size 175 bytes
// name confidence: 0.55 (existing Ghidra name)   rewrite confidence: 0.8
// evidence: types/interface.h virtual_keyboard_globals (active, destination, maximum_length,
// text, committed all match); shares the DirectInput device reset tail with
// virtual_keyboard_open.c.
// register convention: __cdecl, no parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "fn_interface.h"
#include <wchar.h>
#include <string.h>

extern virtual_keyboard_globals virtual_keyboard; // 0x007193a8
extern uint8_t controls_input_capture_flags; // 0x00712542, UNSURE (per src/interface/widget_close_all.c)
extern void widget_play_sound_effect(int16_t effect_id); // 0x498e90, blam-cc: AX effect_id
extern void **keyboard_device; // 0x006b1800, UNSURE: DirectInput device COM pointer
extern uint8_t key_frames[0x6d]; // 0x006b1620
extern uint8_t key_release_pending[0x6d]; // 0x006b168d


// Commits the virtual keyboard's edited text back to the caller's buffer, clears the keyboard's
// state and plays the UI sound effect, then resets the DirectInput keyboard device the same way
// virtual_keyboard_open does.
uint8_t virtual_keyboard_close(void)
{
    virtual_keyboard.active = 0;
    if (virtual_keyboard.destination != 0) {
        int32_t wide_chars = (uint16_t)virtual_keyboard.maximum_length >> 1;
        wcsncpy((wchar_t *)virtual_keyboard.destination, (const wchar_t *)virtual_keyboard.text, wide_chars);
        *(int16_t *)((uint8_t *)virtual_keyboard.destination +
                      ((virtual_keyboard.maximum_length & ~1) - 2)) = 0;
    }
    virtual_keyboard.destination = 0;
    virtual_keyboard.text[0] = 0; // a word store at 0x007193d0
    virtual_keyboard.committed = 0;
    widget_play_sound_effect(3);

    controls_input_capture_flags &= 0xfb;

    if (keyboard_device != 0) {
        int32_t minus_one = -1;
        void **vtable = *(void ***)keyboard_device;
        ((directinput_set_property_fn)vtable[0x28 / 4])(keyboard_device, 0x14, 0, &minus_one, 0);
        memset(key_release_pending, 0, sizeof(key_release_pending));
        memset(key_frames, 0, sizeof(key_frames));
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4a9250):

int __cdecl virtual_keyboard_close(void)

{
  int iVar1;
  undefined4 *puVar2;
  undefined4 local_4;

  DAT_007193a8 = 0;
  if (DAT_007193c0 != (wchar_t *)0x0) {
    _wcsncpy(DAT_007193c0,&DAT_007193d0,(uint)(DAT_007193b4 >> 1));
    *(undefined2 *)((int)DAT_007193c0 + ((DAT_007193b4 & 0xfffffffe) - 2)) = 0;
  }
  DAT_007193c0 = (wchar_t *)0x0;
  DAT_007193d0 = 0;
  DAT_007193bc._2_1_ = 0;
  widget_play_sound_effect();
  DAT_00712542 = DAT_00712542 & 0xfb;
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
  return 1;
}
#endif
