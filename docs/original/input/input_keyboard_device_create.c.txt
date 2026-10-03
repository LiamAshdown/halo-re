// input_keyboard_device_create  (Ghidra: already named)
// address 0x4918a0, size 286 bytes
// name confidence: 0.75   rewrite confidence: 0.75
// evidence: out/phase4/input_functions.md summary "Creates and configures the DirectInput
// keyboard device object (data format, cooperative level, buffered-input property), logging and
// cleaning up on any failure."; objdump of 0x4918a0..0x4919bd resolves the full DIPROPDWORD
// Ghidra's pseudo-C only partly showed: {size 0x14, header_size 0x10, object 0 (DIPH_DEVICE),
// how 0, data 0x20} -- DIPROP_BUFFERSIZE 32. Also resets the keyboard runtime state
// (key_frames/key_events/key_release_pending, matching input_reset_state_and_axis_configs.c) and
// seeds every key_block_timer to {-1, -1}, matching that struct's own notes.
// UNSURE: the function always returns 1/true, even along every failure path (Ghidra's
// `CONCAT31(garbage, 1)`), matching objdump's unconditional `mov al, 1` before the shared return.
// register convention: __cdecl, no parameters; returns AL (always 1).
// COM method typedefs: types/input.h (the DirectInput 8 method block).

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
extern uint8_t key_frames[0x6d];              // 0x006b1620
extern int16_t key_event_read_index;          // 0x006b16fa
extern int16_t key_event_count;               // 0x006b16fc
extern ui_key_event key_events[k_input_key_event_capacity];         // 0x006b16fe
extern uint8_t key_release_pending[0x6d];     // 0x006b168d
extern key_block_timer key_block_timers[k_input_key_block_timer_count]; // 0x006b1600
extern void *direct_input;                    // 0x006b15fc, IDirectInput8A*
extern void *keyboard_device;                 // 0x006b1800, IDirectInputDevice8A*
extern input_guid guid_sys_keyboard;          // 0x0064e24c, GUID_SysKeyboard
extern di_data_format c_dfDIKeyboard;         // 0x0064dfdc
extern void *shell_window;                    // 0x007461c4, HWND

extern void input_error_log_once(int32_t error_code, char *description, ...); // this module, 0x492150


// Resets the keyboard runtime state and key block timers, then creates and configures the
// DirectInput keyboard device (cooperative level, key data format, and a 32-entry buffered
// input property), releasing it again on any failure. Always returns 1.
uint8_t input_keyboard_device_create(void)
{
    void **vtable;
    int32_t hr;
    char *description;
    di_property_dword buffer_size;
    int32_t i;

    memset(key_frames, 0, sizeof(key_frames));
    key_event_read_index = 0;
    key_event_count = 0;
    memset(key_events, 0, sizeof(ui_key_event) * 0x10);
    memset(key_release_pending, 0, sizeof(key_release_pending));

    for (i = 0; i < k_input_key_block_timer_count; i++) {
        key_block_timers[i].deadline = 0xffffffff;
        key_block_timers[i].key = -1;
    }

    vtable = *(void ***)direct_input;
    hr = ((idirectinput8_createdevice_proc)vtable[3])(direct_input, &guid_sys_keyboard,
        &keyboard_device, (void *)0);
    if (hr < 0) {
        description = (char *)"CreateDevice (keyboard)";
    } else {
        vtable = *(void ***)keyboard_device;
        hr = ((idirectinputdevice8_setcooplevel_proc)vtable[13])(keyboard_device, shell_window, 0x16);
        if (hr < 0) {
            description = (char *)"SetCooperativeLevel (keyboard)";
        } else {
            vtable = *(void ***)keyboard_device;
            hr = ((idirectinputdevice8_setdataformat_proc)vtable[11])(keyboard_device, &c_dfDIKeyboard);
            if (hr < 0) {
                description = (char *)"SetDataFormat (keyboard)";
            } else {
                buffer_size.header.size = 0x14;
                buffer_size.header.header_size = 0x10;
                buffer_size.header.object = 0;
                buffer_size.header.how = 0; // DIPH_DEVICE
                buffer_size.data = 0x20;

                vtable = *(void ***)keyboard_device;
                hr = ((idirectinputdevice8_setproperty_proc)vtable[6])(keyboard_device, 1, &buffer_size); // DIPROP_BUFFERSIZE
                if (hr >= 0) {
                    return 1;
                }
                description = (char *)"SetProperty (keyboard)";
            }
        }
    }

    input_error_log_once(hr, description);
    if (keyboard_device != (void *)0) {
        vtable = *(void ***)keyboard_device;
        ((idirectinputdevice8_unacquire_proc)vtable[8])(keyboard_device);
        ((idirectinputdevice8_release_proc)vtable[2])(keyboard_device);
        keyboard_device = (void *)0;
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4918a0):

int __cdecl input_keyboard_device_create(void)

{
  undefined2 *puVar1;
  int iVar2;
  undefined4 *puVar3;
  char *pcVar4;
  undefined4 *puStack_24;
  undefined4 uStack_20;

  puVar3 = (undefined4 *)&DAT_006b1620;
  for (iVar2 = 0x1b; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar3 = 0;
    puVar3 = puVar3 + 1;
  }
  *(undefined1 *)puVar3 = 0;
  DAT_006b16fa = 0;
  DAT_006b16fc = 0;
  puVar3 = &DAT_006b16fe;
  for (iVar2 = 0x10; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar3 = 0;
    puVar3 = puVar3 + 1;
  }
  puVar3 = &DAT_006b168d;
  for (iVar2 = 0x1b; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar3 = 0;
    puVar3 = puVar3 + 1;
  }
  *(undefined1 *)puVar3 = 0;
  puVar1 = &DAT_006b1604;
  do {
    *(undefined4 *)(puVar1 + -2) = 0xffffffff;
    *puVar1 = 0xffff;
    puVar1 = puVar1 + 4;
  } while ((int)puVar1 < 0x6b1624);
  uStack_20 = 0;
  puStack_24 = &DAT_006b1800;
  iVar2 = (**(code **)(*DAT_006b15fc + 0xc))(DAT_006b15fc,&DAT_0064e24c);
  if (iVar2 < 0) {
    pcVar4 = "CreateDevice (keyboard)";
  }
  else {
    iVar2 = (**(code **)(*DAT_006b1800 + 0x34))(DAT_006b1800,DAT_007461c4,0x16);
    if (iVar2 < 0) {
      pcVar4 = "SetCooperativeLevel (keyboard)";
    }
    else {
      iVar2 = (**(code **)(*DAT_006b1800 + 0x2c))(DAT_006b1800,&DAT_0064dfdc);
      if (iVar2 < 0) {
        pcVar4 = "SetDataFormat (keyboard)";
      }
      else {
        puStack_24 = (undefined4 *)0x14;
        uStack_20 = 0x10;
        iVar2 = (**(code **)(*DAT_006b1800 + 0x18))(DAT_006b1800,1,&puStack_24);
        if (-1 < iVar2) goto LAB_004919b6;
        pcVar4 = "SetProperty (keyboard)";
      }
    }
  }
  input_error_log_once(iVar2,pcVar4);
  iVar2 = 0;
  if (DAT_006b1800 != (int *)0x0) {
    (**(code **)(*DAT_006b1800 + 0x20))(DAT_006b1800);
    iVar2 = (**(code **)(*DAT_006b1800 + 8))(DAT_006b1800);
    DAT_006b1800 = (int *)0x0;
  }
LAB_004919b6:
  return CONCAT31((int3)((uint)iVar2 >> 8),1);
}

Disassembly (objdump -d, 0x4918a0..0x4919bd) resolving the full DIPROPDWORD and CreateDevice's
third (outer) argument that Ghidra's pseudo-C dropped:

  4918fe: push   %esi                 ; outer = NULL
  4918ff: push   $0x6b1800            ; &keyboard_device
  491904: push   $0x64e24c            ; &GUID_SysKeyboard
  491909: push   %eax                 ; direct_input
  49190a: call   *0xc(%ecx)           ; CreateDevice
  ...
  491949: movl   $0x14,0x10(%esp)     ; header.size
  491951: movl   $0x10,0x14(%esp)     ; header.header_size
  491959: mov    %esi,0x1c(%esp)      ; header.how = 0
  49195d: mov    %esi,0x18(%esp)      ; header.object = 0
  491961: movl   $0x20,0x20(%esp)     ; data = 32
  49196c: call   *0x18(%ecx)          ; SetProperty(device, 1, &buffer_size)
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
