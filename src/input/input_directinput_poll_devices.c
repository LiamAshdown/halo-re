// input_directinput_poll_devices  (Ghidra: already named)
// address 0x490760, size 823 bytes
// name confidence: 0.5   rewrite confidence: 0.5
// evidence: out/phase4/input_functions.md summary "Per-frame poll of the keyboard, mouse, and
// joystick DirectInput devices, updating the raw press/hold state arrays and handling
// device-lost reacquire and buffer overflow."; every field matches types/input.h exactly:
// key_frames/key_release_pending aging (the release-pending byte clears the frame count, else it
// saturates at 255, matching key_release_pending's doc comment), the DIK->key table
// (scan_code_to_key, 0x0065bd58), the special-cased tab key (_input_key_tab == 0x1e, cleared
// while alt is held, matching input_key's own comment), the menu-mode gate on marking a
// same-frame press+release (mode_flags bit _input_mode_menu_bit), mouse_state's full zero-out on
// failure, and the joystick Poll+GetDeviceState+input_joystick_state_process sequence with a
// neutral-state reset on failure. input_suppressed is unconditionally cleared here, matching its
// own "only ever written 0" note.
// register convention: no parameters, no return value.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"

#include <string.h>

extern uint8_t input_acquired;                 // 0x006b15f8
extern uint8_t input_suppressed;               // 0x006b15f9
extern void *keyboard_device;                  // 0x006b1800
extern void *mouse_device;                     // 0x006b1804
extern int16_t key_event_read_index;           // 0x006b16fa
extern int16_t key_event_count;                // 0x006b16fc
extern uint8_t key_frames[0x6d];               // 0x006b1620
extern uint8_t key_release_pending[0x6d];      // 0x006b168d
extern int16_t scan_code_to_key[0x100];        // 0x0065bd58
extern input_abstraction_globals input_globals; // 0x00710328
extern int32_t game_time_force_single_tick;    // 0x007196d8, game module
extern mouse_state live_mouse_state;           // 0x006b180c
extern int32_t input_device_count;             // 0x006b1844
extern input_device input_devices[8];          // 0x006b1868
extern void *joystick_devices[8];              // 0x006b1848
extern joystick_state joystick_states[4];      // 0x006b2a68
extern joystick_state joystick_neutral_state;  // 0x006b2cf8

extern void input_error_log_once(int32_t error_code, char *description, ...); // this module, 0x492150
extern void input_mouse_state_process(mouse_state *dest, di_mouse_state2 *raw); // this module, 0x491bc0,
    // blam-cc: dest on the stack, raw in ECX
extern void input_joystick_state_process(joystick_raw_state *raw, joystick_state *dest,
    input_device *device); // this module, 0x491fd0, blam-cc: device in EBX


// The two "device needs reacquiring" HRESULTs this poll checks for, from the binary's own
// literals (-0x7ff8fff4 and -0x7ff8ffe2): 0x8007000c is DIERR_NOTACQUIRED
// (HRESULT_FROM_WIN32(ERROR_INVALID_ACCESS)) and 0x8007001e is DIERR_INPUTLOST
// (HRESULT_FROM_WIN32(ERROR_READ_FAULT)), both as defined in dinput.h.
#define k_dierr_reacquire_a ((int32_t)0x8007000cu) // DIERR_NOTACQUIRED
#define k_dierr_reacquire_b ((int32_t)0x8007001eu) // DIERR_INPUTLOST

// Per-frame poll: ages the keyboard hold counters and drains the buffered key events (handling
// overflow and device loss), reads the mouse and every mapped joystick's raw state into the
// engine's mouse_state/joystick_states (zeroing/reseeding them to neutral on failure), while
// clearing input_suppressed and the key event ring.
void input_directinput_poll_devices(void)
{
    void **vtable;
    int32_t hr;
    uint32_t event_count;
    di_device_object_data event;
    di_mouse_state2 mouse_raw;
    joystick_raw_state joystick_raw;
    int32_t i;
    int32_t key_index;
    int32_t slot;

    if (input_acquired == 0) {
        return;
    }

    input_suppressed = 0;
    key_event_read_index = 0;
    key_event_count = 0;

    if (keyboard_device != 0) {
        for (i = 0; i < 0x6d; i++) {
            if (key_release_pending[i] == 1) {
                key_frames[i] = 0;
            } else if (key_frames[i] != 0) {
                key_frames[i] = (key_frames[i] < 0xff) ? (uint8_t)(key_frames[i] + 1) : 0xff;
            }
        }
        for (i = 0; i < 0x6d; i++) {
            key_release_pending[i] = 0;
        }

        event_count = 1;
        for (;;) {
            vtable = *(void ***)keyboard_device;
            hr = ((idirectinputdevice8_getdevicedata_proc)vtable[10])(keyboard_device, 0x14,
                &event, &event_count, 0);

            if (hr > 0) {
                if (hr == 1) { // DI_BUFFEROVERFLOW
                    input_error_log_once(1, (char *)"keyboard_buffer_overflow");
                    event_count = 0xffffffff;
                    ((idirectinputdevice8_getdevicedata_proc)vtable[10])(keyboard_device, 0x14,
                        (di_device_object_data *)0, &event_count, 0);
                } else {
                    input_error_log_once(hr, (char *)"IDirectInputDevice_GetDeviceData (mouse)");
                }
                break;
            }

            if (hr == 0) {
                if (event_count != 1) {
                    break;
                }
                key_index = scan_code_to_key[event.offset];
                if (key_index == -1) {
                    continue;
                }
                if (key_index == _input_key_tab && GetAsyncKeyState(0x12) < 0) { // VK_MENU
                    key_frames[_input_key_tab] = 0;
                } else if (((uint8_t)event.data & 0x80) != 0) {
                    key_frames[key_index] = 1;
                } else if (key_frames[key_index] == 1 && (input_globals.mode_flags & _input_mode_menu_bit) == 0) {
                    key_release_pending[key_index] = 1;
                } else {
                    key_frames[key_index] = 0;
                }
                continue;
            }

            if (hr == k_dierr_reacquire_b || hr == k_dierr_reacquire_a) {
                ((idirectinputdevice8_acquire_proc)vtable[7])(keyboard_device);
            } else {
                input_error_log_once(hr, (char *)"IDirectInputDevice_GetDeviceData (mouse)");
            }
            break;
        }
    }

    if (mouse_device != 0 && game_time_force_single_tick == 0) {
        vtable = *(void ***)mouse_device;
        hr = ((idirectinputdevice8_getdevicestate_proc)vtable[9])(mouse_device, 0x14, &mouse_raw);
        if (hr == k_dierr_reacquire_b || hr == k_dierr_reacquire_a) {
            ((idirectinputdevice8_acquire_proc)vtable[7])(mouse_device);
        } else if (hr == 0) {
            input_mouse_state_process(&live_mouse_state, &mouse_raw);
            goto joystick_poll;
        } else {
            input_error_log_once(hr, (char *)"GetDeviceState (mouse)");
        }
        if (hr < 0) {
            memset(&live_mouse_state, 0, sizeof(live_mouse_state));
        }
    }

joystick_poll:
    if (game_time_force_single_tick != 0) {
        return;
    }
    for (i = 0; i < 8; i++) {
        if (i < input_device_count && input_devices[i].slot != -1 && joystick_devices[i] != 0) {
            slot = input_devices[i].slot;

            vtable = *(void ***)joystick_devices[i];
            hr = ((idirectinputdevice8_poll_proc)vtable[25])(joystick_devices[i]);
            if (hr >= 0) {
                vtable = *(void ***)joystick_devices[i];
                hr = ((idirectinputdevice8_getdevicestate_proc)vtable[9])(joystick_devices[i], 0xe0, &joystick_raw);
            }

            if (hr == k_dierr_reacquire_b || hr == k_dierr_reacquire_a) {
                vtable = *(void ***)joystick_devices[i];
                ((idirectinputdevice8_acquire_proc)vtable[7])(joystick_devices[i]);
            } else if (hr == 0) {
                input_joystick_state_process(&joystick_raw, &joystick_states[slot], &input_devices[i]);
                continue;
            } else {
                input_error_log_once(hr, (char *)"Poll/GetDeviceState (gamepad)");
            }

            if (hr < 0) {
                joystick_states[slot] = joystick_neutral_state;
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x490760):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void input_directinput_poll_devices(void)

{
  short sVar1;
  int *piVar2;
  int iVar3;
  SHORT SVar4;
  byte *pbVar5;
  int iVar6;
  uint uVar7;
  int iVar8;
  undefined4 *puVar9;
  undefined4 *puVar10;
  undefined4 *puVar11;
  int local_f8;
  int local_f4;
  char cStack_f0;
  undefined1 auStack_e0 [224];

  if (DAT_006b15f8 == '\0') {
    return;
  }
  DAT_006b15f9 = 0;
  DAT_006b16fa = 0;
  DAT_006b16fc = 0;
  if (DAT_006b1800 != (int *)0x0) {
    local_f8 = 1;
    pbVar5 = &DAT_006b1620;
    iVar8 = 0x6d;
    do {
      if (pbVar5[0x6d] == 1) {
        *pbVar5 = 0;
      }
      else if (*pbVar5 != 0) {
        uVar7 = *pbVar5 + 1;
        if (0xff < uVar7) {
          uVar7 = 0xff;
        }
        *pbVar5 = (byte)uVar7;
      }
      pbVar5 = pbVar5 + 1;
      iVar8 = iVar8 + -1;
    } while (iVar8 != 0);
    puVar9 = &DAT_006b168d;
    for (iVar8 = 0x1b; iVar8 != 0; iVar8 = iVar8 + -1) {
      *puVar9 = 0;
      puVar9 = puVar9 + 1;
    }
    *(undefined1 *)puVar9 = 0;
LAB_004907f0:
    do {
      iVar8 = (**(code **)(*DAT_006b1800 + 0x28))(DAT_006b1800,0x14,&local_f4,&local_f8,0);
      if (0 < iVar8) {
        if (iVar8 == 1) {
          input_error_log_once(1,"keyboard_buffer_overflow");
          local_f8 = -1;
          (**(code **)(*DAT_006b1800 + 0x28))(DAT_006b1800,0x14,0,&local_f8,0);
        }
        else {
LAB_004908b3:
          input_error_log_once(iVar8,"IDirectInputDevice_GetDeviceData (mouse)");
        }
        break;
      }
      if (iVar8 == 0) {
        if (local_f8 != 1) break;
        sVar1 = *(short *)(&DAT_0065bd58 + local_f4 * 2);
        if (sVar1 == -1) goto LAB_004907f0;
        if ((sVar1 == 0x1e) && (SVar4 = GetAsyncKeyState(0x12), SVar4 < 0)) {
          DAT_006b163e = 0;
        }
        else {
          iVar8 = (int)sVar1;
          if (cStack_f0 < '\0') {
            (&DAT_006b1620)[iVar8] = 1;
          }
          else if (((&DAT_006b1620)[iVar8] == '\x01') && ((DAT_00712542 & 2) == 0)) {
            *(undefined1 *)((int)&DAT_006b168d + iVar8) = 1;
          }
          else {
            (&DAT_006b1620)[iVar8] = 0;
          }
        }
      }
      else {
        if ((iVar8 != -0x7ff8fff4) && (iVar8 != -0x7ff8ffe2)) goto LAB_004908b3;
        local_f8 = 0;
        (**(code **)(*DAT_006b1800 + 0x1c))(DAT_006b1800);
      }
    } while (local_f8 == 1);
  }
  if ((DAT_006b1804 != (int *)0x0) && (DAT_007196d8 == 0)) {
    iVar8 = (**(code **)(*DAT_006b1804 + 0x24))(DAT_006b1804,0x14,&local_f4);
    if ((iVar8 == -0x7ff8fff4) || (iVar8 == -0x7ff8ffe2)) {
      (**(code **)(*DAT_006b1804 + 0x1c))(DAT_006b1804);
    }
    else {
      if (iVar8 == 0) {
        FUN_00491bc0(&DAT_006b180c);
        goto LAB_0049098b;
      }
      input_error_log_once(iVar8,"GetDeviceState (mouse)");
    }
    if (iVar8 < 0) {
      DAT_006b180c = 0;
      DAT_006b1810 = 0;
      DAT_006b1814 = 0;
      _DAT_006b1818 = 0;
      _DAT_006b181c = 0;
      _DAT_006b1820 = 0;
      _DAT_006b1824 = 0;
    }
  }
LAB_0049098b:
  iVar8 = 0;
  if (DAT_007196d8 == 0) {
    puVar9 = &DAT_006b1868;
    do {
      if (0x6b2a67 < (int)puVar9) {
        return;
      }
      if ((((short)iVar8 < DAT_006b1844) && ((&DAT_006b1a98)[(short)iVar8 * 0x90] != -1)) &&
         (piVar2 = (int *)(&DAT_006b1848)[iVar8], piVar2 != (int *)0x0)) {
        iVar3 = puVar9[0x8c];
        iVar6 = (**(code **)(*piVar2 + 100))(piVar2);
        if (-1 < iVar6) {
          iVar6 = (**(code **)(*(int *)(&DAT_006b1848)[iVar8] + 0x24))
                            ((int *)(&DAT_006b1848)[iVar8],0xe0,auStack_e0);
        }
        if ((iVar6 == -0x7ff8fff4) || (iVar6 == -0x7ff8ffe2)) {
          (**(code **)(*(int *)(&DAT_006b1848)[iVar8] + 0x1c))((int *)(&DAT_006b1848)[iVar8]);
        }
        else {
          if (iVar6 == 0) {
            input_joystick_state_process(auStack_e0,&DAT_006b2a68 + iVar3 * 0x28);
            goto LAB_00490a78;
          }
          input_error_log_once(iVar6,"Poll/GetDeviceState (gamepad)");
        }
        if (iVar6 < 0) {
          puVar10 = &DAT_006b2cf8;
          puVar11 = &DAT_006b2a68 + iVar3 * 0x28;
          for (iVar6 = 0x28; iVar6 != 0; iVar6 = iVar6 + -1) {
            *puVar11 = *puVar10;
            puVar10 = puVar10 + 1;
            puVar11 = puVar11 + 1;
          }
        }
      }
LAB_00490a78:
      iVar8 = iVar8 + 1;
      puVar9 = puVar9 + 0x90;
    } while (DAT_007196d8 == 0);
  }
  return;
}
#endif
