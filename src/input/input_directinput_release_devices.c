// input_directinput_release_devices  (Ghidra: already named)
// address 0x490580, size 160 bytes
// name confidence: 0.8   rewrite confidence: 0.6
// evidence: out/phase4/input_functions.md summary "Unacquires and releases all DirectInput
// device objects (joysticks, mouse, keyboard) and the DirectInput object itself."; vtable +0x20
// is Unacquire, +8 is Release (same offsets as input_device_release.c).
// COM method typedefs: types/input.h (the DirectInput 8 method block).
// register convention: no parameters, no return value.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"
#include "fn_input.h"

extern void *joystick_devices[8];      // 0x006b1848, IDirectInputDevice8A*
extern input_device input_devices[8];  // 0x006b1868
extern void *mouse_device;             // 0x006b1804
extern void *keyboard_device;          // 0x006b1800
extern void *direct_input;             // 0x006b15fc, IDirectInput8A*


// Unacquires and releases every joystick, the mouse, and the keyboard DirectInput device object
// (clearing their cached input_devices entries), then releases the shared IDirectInput8A object.
void input_directinput_release_devices(void)
{
    int32_t i;
    void *device;
    void **vtable;
    uint32_t *cursor;
    int32_t count;

    for (i = 0; i < 8; i++) {
        device = joystick_devices[i];
        if (device != 0) {
            vtable = *(void ***)device;
            ((idirectinputdevice8_unacquire_proc)vtable[8])(device);
            ((idirectinputdevice8_release_proc)vtable[2])(device);
            joystick_devices[i] = 0;

            cursor = (uint32_t *)&input_devices[i];
            for (count = 0x90; count != 0; count--) {
                *cursor = 0;
                cursor = cursor + 1;
            }
        }
    }

    if (mouse_device != 0) {
        vtable = *(void ***)mouse_device;
        ((idirectinputdevice8_unacquire_proc)vtable[8])(mouse_device);
        ((idirectinputdevice8_release_proc)vtable[2])(mouse_device);
        mouse_device = 0;
    }
    if (keyboard_device != 0) {
        vtable = *(void ***)keyboard_device;
        ((idirectinputdevice8_unacquire_proc)vtable[8])(keyboard_device);
        ((idirectinputdevice8_release_proc)vtable[2])(keyboard_device);
        keyboard_device = 0;
    }
    if (direct_input != 0) {
        vtable = *(void ***)direct_input;
        ((idirectinput8_release_proc)vtable[2])(direct_input);
        direct_input = 0;
    }
}

#if 0
Original Ghidra decompilation (0x490580):

void input_directinput_release_devices(void)

{
  int *piVar1;
  int iVar2;
  undefined4 *puVar3;
  int iVar4;
  int *piVar5;
  undefined4 *puVar6;

  puVar3 = &DAT_006b1868;
  piVar5 = &DAT_006b1848;
  iVar4 = 8;
  do {
    piVar1 = (int *)*piVar5;
    if (piVar1 != (int *)0x0) {
      (**(code **)(*piVar1 + 0x20))(piVar1);
      (**(code **)(*(int *)*piVar5 + 8))((int *)*piVar5);
      *piVar5 = 0;
      puVar6 = puVar3;
      for (iVar2 = 0x90; iVar2 != 0; iVar2 = iVar2 + -1) {
        *puVar6 = 0;
        puVar6 = puVar6 + 1;
      }
    }
    piVar5 = piVar5 + 1;
    puVar3 = puVar3 + 0x90;
    iVar4 = iVar4 + -1;
  } while (iVar4 != 0);
  if (DAT_006b1804 != (int *)0x0) {
    (**(code **)(*DAT_006b1804 + 0x20))(DAT_006b1804);
    (**(code **)(*DAT_006b1804 + 8))(DAT_006b1804);
    DAT_006b1804 = (int *)0x0;
  }
  if (DAT_006b1800 != (int *)0x0) {
    (**(code **)(*DAT_006b1800 + 0x20))(DAT_006b1800);
    (**(code **)(*DAT_006b1800 + 8))(DAT_006b1800);
    DAT_006b1800 = (int *)0x0;
  }
  if (DAT_006b15fc != (int *)0x0) {
    (**(code **)(*DAT_006b15fc + 8))(DAT_006b15fc);
    DAT_006b15fc = (int *)0x0;
  }
  return;
}
#endif
