// input_directinput_unacquire_devices  (Ghidra: already named)
// address 0x4906e0, size 128 bytes
// name confidence: 0.85   rewrite confidence: 0.7
// evidence: out/phase4/input_functions.md summary "Unacquires the keyboard, mouse, and joystick
// DirectInput devices without releasing them, marking input as inactive."; vtable +0x20 is
// Unacquire (IDirectInputDevice8 index 8), matching the other DirectInput lifecycle files.
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

extern void *joystick_devices[8]; // 0x006b1848
extern void *mouse_device;        // 0x006b1804
extern void *keyboard_device;     // 0x006b1800
extern uint8_t input_acquired;    // 0x006b15f8

extern void input_error_log_once(int32_t error_code, char *description, ...); // this module, 0x492150


// Unacquires (but does not release) every joystick, the mouse, and the keyboard, then marks
// input as inactive.
void input_directinput_unacquire_devices(void)
{
    int32_t i;
    void *device;
    void **vtable;
    int32_t hr;

    for (i = 0; i < 8; i++) {
        device = joystick_devices[i];
        if (device != 0) {
            vtable = *(void ***)device;
            hr = ((idirectinputdevice8_unacquire_proc)vtable[8])(device);
            if (hr < 0) {
                input_error_log_once(hr, (char *)"Unacquire (gamepad)");
            }
        }
    }

    if (mouse_device != 0) {
        vtable = *(void ***)mouse_device;
        hr = ((idirectinputdevice8_unacquire_proc)vtable[8])(mouse_device);
        if (hr < 0) {
            input_error_log_once(hr, (char *)"Unacquire (mouse)");
        }
    }

    input_acquired = 0;

    if (keyboard_device != 0) {
        vtable = *(void ***)keyboard_device;
        hr = ((idirectinputdevice8_unacquire_proc)vtable[8])(keyboard_device);
        if (hr < 0) {
            input_error_log_once(hr, (char *)"Unacquire (keyboard)");
        }
    }
}

#if 0
Original Ghidra decompilation (0x4906e0):

void input_directinput_unacquire_devices(void)

{
  int *piVar1;
  int iVar2;
  int *piVar3;
  int iVar4;

  piVar3 = &DAT_006b1848;
  iVar4 = 8;
  do {
    piVar1 = (int *)*piVar3;
    if (piVar1 != (int *)0x0) {
      iVar2 = (**(code **)(*piVar1 + 0x20))(piVar1);
      if (iVar2 < 0) {
        input_error_log_once(iVar2,"Unacquire (gamepad)");
      }
    }
    piVar3 = piVar3 + 1;
    iVar4 = iVar4 + -1;
  } while (iVar4 != 0);
  if (DAT_006b1804 != (int *)0x0) {
    iVar4 = (**(code **)(*DAT_006b1804 + 0x20))(DAT_006b1804);
    if (iVar4 < 0) {
      input_error_log_once(iVar4,"Unacquire (mouse)");
    }
  }
  DAT_006b15f8 = 0;
  if (DAT_006b1800 != (int *)0x0) {
    iVar4 = (**(code **)(*DAT_006b1800 + 0x20))(DAT_006b1800);
    if (iVar4 < 0) {
      input_error_log_once(iVar4,"Unacquire (keyboard)");
    }
  }
  return;
}
#endif
