// input_mouse_device_create  (Ghidra: already named)
// address 0x4919c0, size 188 bytes
// name confidence: 0.75   rewrite confidence: 0.8
// evidence: out/phase4/input_functions.md summary "Creates and configures the DirectInput mouse
// device object, first determining left/right button swap state from the system settings,
// logging and cleaning up on any failure."; types/input.h documents mouse_button_map's swap
// seeding exactly ("entries 0 and 1 to {0,2}, or {2,0} when GetSystemMetrics(SM_SWAPBUTTON) is
// nonzero"). Ghidra fully recovered every DirectInput call's arguments here.
// UNSURE: like input_keyboard_device_create, always returns 1/true even on failure.
// register convention: __cdecl, no parameters; returns AL (always 1).
// COM method typedefs: types/input.h (the DirectInput 8 method block).

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
extern int16_t mouse_button_map[k_input_mouse_button_count]; // 0x0068e534
extern void *direct_input;      // 0x006b15fc, IDirectInput8A*
extern void *mouse_device;      // 0x006b1804, IDirectInputDevice8A*
extern input_guid guid_sys_mouse; // 0x0064e25c, GUID_SysMouse
extern di_data_format c_dfDIMouse2; // 0x0064e1e4
extern void *shell_window;      // 0x007461c4, HWND

extern void input_error_log_once(int32_t error_code, char *description, ...); // this module, 0x492150


// Seeds mouse_button_map[0..1] from the system's left/right swap setting, then creates and
// configures the DirectInput mouse device (cooperative level, DIMOUSESTATE2 data format),
// releasing it again on any failure. Always returns 1.
uint8_t input_mouse_device_create(void)
{
    void **vtable;
    int32_t hr;
    char *description;

    if (GetSystemMetrics(0x17) == 0) { // SM_SWAPBUTTON
        mouse_button_map[0] = 0;
        mouse_button_map[1] = 2;
    } else {
        mouse_button_map[0] = 2;
        mouse_button_map[1] = 0;
    }

    vtable = *(void ***)direct_input;
    hr = ((idirectinput8_createdevice_proc)vtable[3])(direct_input, &guid_sys_mouse,
        &mouse_device, (void *)0);
    if (hr < 0) {
        description = (char *)"CreateDevice (mouse)";
    } else {
        vtable = *(void ***)mouse_device;
        hr = ((idirectinputdevice8_setcooplevel_proc)vtable[13])(mouse_device, shell_window, 5);
        if (hr < 0) {
            description = (char *)"SetCooperativeLevel (mouse)";
        } else {
            vtable = *(void ***)mouse_device;
            hr = ((idirectinputdevice8_setdataformat_proc)vtable[11])(mouse_device, &c_dfDIMouse2);
            if (hr >= 0) {
                return 1;
            }
            description = (char *)"SetDataFormat (mouse)";
        }
    }

    input_error_log_once(hr, description);
    if (mouse_device != (void *)0) {
        vtable = *(void ***)mouse_device;
        ((idirectinputdevice8_unacquire_proc)vtable[8])(mouse_device);
        ((idirectinputdevice8_release_proc)vtable[2])(mouse_device);
        mouse_device = (void *)0;
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4919c0):

int __cdecl input_mouse_device_create(void)

{
  int iVar1;
  char *pcVar2;

  iVar1 = GetSystemMetrics(0x17);
  if (iVar1 == 0) {
    DAT_0068e534 = 0;
    DAT_0068e536 = 2;
  }
  else {
    DAT_0068e534 = 2;
    DAT_0068e536 = 0;
  }
  iVar1 = (**(code **)(*DAT_006b15fc + 0xc))(DAT_006b15fc,&DAT_0064e25c,&DAT_006b1804,0);
  if (iVar1 < 0) {
    pcVar2 = "CreateDevice (mouse)";
  }
  else {
    iVar1 = (**(code **)(*DAT_006b1804 + 0x34))(DAT_006b1804,DAT_007461c4,5);
    if (iVar1 < 0) {
      pcVar2 = "SetCooperativeLevel (mouse)";
    }
    else {
      iVar1 = (**(code **)(*DAT_006b1804 + 0x2c))(DAT_006b1804,&DAT_0064e1e4);
      if (-1 < iVar1) goto LAB_00491a79;
      pcVar2 = "SetDataFormat (mouse)";
    }
  }
  input_error_log_once(iVar1,pcVar2);
  iVar1 = 0;
  if (DAT_006b1804 != (int *)0x0) {
    (**(code **)(*DAT_006b1804 + 0x20))(DAT_006b1804);
    iVar1 = (**(code **)(*DAT_006b1804 + 8))(DAT_006b1804);
    DAT_006b1804 = (int *)0x0;
  }
LAB_00491a79:
  return CONCAT31((int3)((uint)iVar1 >> 8),1);
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
