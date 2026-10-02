// input_directinput_acquire_devices  (Ghidra: already named)
// address 0x490620, size 191 bytes
// name confidence: 0.85   rewrite confidence: 0.6
// evidence: out/phase4/input_functions.md summary "Acquires the keyboard, mouse, and all
// connected joystick DirectInput devices and marks input as active."; vtable +0x1c is Acquire;
// the mouse's post-acquire GetProperty call is DIPROP_GRANULARITY (property id 3, MAKEDIPROP(3))
// of DIMOFS_Z (object 8, DIPH_BYOFFSET), matching types/input.h's di_property_dword and the
// mouse_wheel_granularity global documented there.
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
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint8_t input_acquired;         // 0x006b15f8
extern void *keyboard_device;          // 0x006b1800
extern void *mouse_device;             // 0x006b1804
extern int32_t mouse_wheel_granularity; // 0x006b1808
extern void *joystick_devices[8];      // 0x006b1848

extern void input_error_log_once(int32_t error_code, char *description, ...); // this module, 0x492150


// Marks input as active and acquires the keyboard, mouse (also reading its wheel granularity),
// and every connected joystick device; logs (but does not treat as fatal) any Acquire/GetProperty
// failure.
void input_directinput_acquire_devices(void)
{
    int32_t i;
    void *device;
    void **vtable;
    int32_t hr;
    di_property_dword granularity;

    input_acquired = 1;

    if (keyboard_device != 0) {
        vtable = *(void ***)keyboard_device;
        hr = ((idirectinputdevice8_acquire_proc)vtable[7])(keyboard_device);
        if (hr < 0) {
            input_error_log_once(hr, (char *)"Acquire (keyboard)");
        }
    }

    if (mouse_device != 0) {
        vtable = *(void ***)mouse_device;
        hr = ((idirectinputdevice8_acquire_proc)vtable[7])(mouse_device);
        if (hr < 0) {
            input_error_log_once(hr, (char *)"Acquire (mouse)");
        } else {
            granularity.header.size = 0x14;
            granularity.header.header_size = 0x10;
            granularity.header.object = 8;   // DIMOFS_Z
            granularity.header.how = 1;      // DIPH_BYOFFSET
            granularity.data = 0;
            hr = ((idirectinputdevice8_getproperty_proc)vtable[5])(mouse_device, 3, &granularity); // DIPROP_GRANULARITY
            if (hr >= 0) {
                mouse_wheel_granularity = granularity.data;
            }
        }
    }

    for (i = 0; i < 8; i++) {
        device = joystick_devices[i];
        if (device != 0) {
            vtable = *(void ***)device;
            hr = ((idirectinputdevice8_acquire_proc)vtable[7])(device);
            if (hr < 0) {
                input_error_log_once(hr, (char *)"Acquire (gamepad)");
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x490620):

void input_directinput_acquire_devices(void)

{
  int *piVar1;
  int iVar2;
  int iVar3;
  int *piVar4;
  undefined4 uStack_14;
  undefined4 uStack_10;
  undefined4 uStack_c;
  undefined4 uStack_8;
  undefined4 uStack_4;

  DAT_006b15f8 = 1;
  if (DAT_006b1800 != (int *)0x0) {
    iVar2 = (**(code **)(*DAT_006b1800 + 0x1c))(DAT_006b1800);
    if (iVar2 < 0) {
      input_error_log_once(iVar2,"Acquire (keyboard)");
    }
  }
  iVar2 = 8;
  if (DAT_006b1804 != (int *)0x0) {
    iVar3 = (**(code **)(*DAT_006b1804 + 0x1c))(DAT_006b1804);
    if (iVar3 < 0) {
      input_error_log_once(iVar3,"Acquire (mouse)");
    }
    else {
      uStack_14 = 0x14;
      uStack_10 = 0x10;
      uStack_c = 8;
      uStack_8 = 1;
      iVar3 = (**(code **)(*DAT_006b1804 + 0x14))(DAT_006b1804,3,&uStack_14);
      if (-1 < iVar3) {
        DAT_006b1808 = uStack_4;
      }
    }
  }
  piVar4 = &DAT_006b1848;
  do {
    piVar1 = (int *)*piVar4;
    if (piVar1 != (int *)0x0) {
      iVar3 = (**(code **)(*piVar1 + 0x1c))(piVar1);
      if (iVar3 < 0) {
        input_error_log_once(iVar3,"Acquire (gamepad)");
      }
    }
    piVar4 = piVar4 + 1;
    iVar2 = iVar2 + -1;
  } while (iVar2 != 0);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
