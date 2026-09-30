// input_refresh_last_used_binding  (Ghidra: FUN_0048bae0; renamed -- the original one-line
//   summary ("resolves a raw input event to the control it is bound to") had the direction
//   backwards. objdump of the sole caller, input_get_last_used_binding 0x48bde0
//   (`mov esi,eax` then `mov ecx,[0x87a460]` / `mov ecx,edi` before each call, with esi left
//   untouched across both calls), shows ESI is a GAME CONTROL id carried in from that caller's
//   own EAX argument, and ECX is a device class (0 keyboard/mouse, 1..4 gamepad slot+1). This
//   function scans the device class's binding tables for an entry that maps to that control, and
//   caches the first physical input it finds as the control's last-used binding)
// address 0x48bae0, size 757 bytes
// name confidence: 0.4 (renamed from FUN_0048bae0)   rewrite confidence: 0.6
// evidence: objdump 0x48bae0..0x48bde0 traced in full. Field addresses match the binding-table
//   externs also used by input_apply_control_binding.c / input_clear_control_binding.c. The
//   gamepad axis and POV scans reuse `button_count` (0x006b1aa0 offset) as their own loop bound
//   instead of fetching axis_count/pov_count -- reproduced exactly, not corrected. The special
//   accept/back (action 8/9) fast path calls input_last_used_binding_set 0x490050 with AX
//   already holding the literal action id (8 or 9) from the immediately preceding `mov eax,0x8`
//   / `mov eax,0x9`, confirmed against that function's own prologue (already rewritten).
// register convention: ECX -> device_class, ESI -> action (only its low 16 bits, SI, are read).
//   // blam-cc: ECX -> device_class, ESI -> action

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"
#include "fn_input.h"

extern input_abstraction_globals input_globals; // 0x00710328
extern input_device input_devices[8];            // 0x006b1868
extern int32_t joystick_slot_devices[4];         // 0x006b2ce8

// Live binding tables inside input_abstraction_globals::settings[0]; see
// input_apply_control_binding.c for why these are declared as their own externs.
extern int16_t keyboard_bindings[k_control_keyboard_key_count];                    // 0x00710330
extern int16_t mouse_button_bindings[k_control_mouse_button_count];                // 0x0071040a
extern int16_t mouse_axis_bindings[k_control_mouse_axis_count][2];                 // 0x0071041a
extern int16_t gamepad_button_bindings[k_control_gamepad_count][k_control_gamepad_button_count]; // 0x00710426
extern int16_t gamepad_action_buttons[k_control_gamepad_count][2];                 // 0x00710526
extern int16_t gamepad_axis_bindings[k_control_gamepad_count][k_control_gamepad_axis_count][2];  // 0x00710536
extern int16_t gamepad_pov_bindings[k_control_gamepad_count][k_control_gamepad_pov_count][k_control_gamepad_pov_direction_count]; // 0x00710736


    // 0x00490050 (already rewritten), blam-cc: AX -> action, CX -> device_type, DX -> device_index,
    // stack -> input_kind, input_index, direction

// Scans one device class's binding tables for an entry that maps to game control `action`, and
// if one is found, caches it as that control's last-used binding (input_globals.last_used_
// bindings[action]) -- unless action is out of the last-used-binding table's range, in which
// case the scan still reports success but performs no write.
//
// device_class == 0 scans keyboard, then mouse buttons, then mouse axes (direction 2 checked
// before direction 1, matching the binary). device_class 1..4 scans gamepad slot
// (device_class - 1): first the accept/back (action 8/9) button shortcut, then buttons, then
// axes, then POV hats. The axis and POV scans are bounded by the slot's button_count, not its
// own axis_count/pov_count, exactly as compiled.
//
// Returns 1 if `action` is currently bound somewhere in this device class, 0 otherwise.
uint8_t input_refresh_last_used_binding(int32_t device_class, int16_t action)
{
    int16_t i;
    int32_t slot;
    int32_t device;
    int32_t count;
    int32_t octant;
    uint8_t found;
    int16_t special_button;

    if (device_class < 0 || device_class > 4) {
        return 0;
    }

    if (device_class == 0) {
        for (i = 0; i < (int16_t)k_control_keyboard_key_count; i++) {
            if (keyboard_bindings[i] == action) {
                if (action >= 0 && action < k_input_action_count) {
                    input_globals.last_used_bindings[action].device_type = _control_device_keyboard;
                    input_globals.last_used_bindings[action].device_index = 0;
                    input_globals.last_used_bindings[action].input_kind = _control_input_button;
                    input_globals.last_used_bindings[action].input_index = i;
                    input_globals.last_used_bindings[action].direction = 0;
                }
                return 1;
            }
        }

        for (i = 0; i < k_control_mouse_button_count; i++) {
            if (mouse_button_bindings[i] == action) {
                if (action >= 0 && action < k_input_action_count) {
                    input_globals.last_used_bindings[action].device_type = _control_device_mouse;
                    input_globals.last_used_bindings[action].device_index = 0;
                    input_globals.last_used_bindings[action].input_kind = _control_input_button;
                    input_globals.last_used_bindings[action].input_index = i;
                    input_globals.last_used_bindings[action].direction = 0;
                }
                return 1;
            }
        }

        for (i = 0; i < k_control_mouse_axis_count; i++) {
            if (mouse_axis_bindings[i][1] == action) {
                if (action >= 0 && action < k_input_action_count) {
                    input_globals.last_used_bindings[action].device_type = _control_device_mouse;
                    input_globals.last_used_bindings[action].device_index = 0;
                    input_globals.last_used_bindings[action].input_kind = _control_input_axis;
                    input_globals.last_used_bindings[action].input_index = i;
                    input_globals.last_used_bindings[action].direction = 2;
                }
                return 1;
            }
            if (mouse_axis_bindings[i][0] == action) {
                if (action >= 0 && action < k_input_action_count) {
                    input_globals.last_used_bindings[action].device_type = _control_device_mouse;
                    input_globals.last_used_bindings[action].device_index = 0;
                    input_globals.last_used_bindings[action].input_kind = _control_input_axis;
                    input_globals.last_used_bindings[action].input_index = i;
                    input_globals.last_used_bindings[action].direction = 1;
                }
                return 1;
            }
        }
        return 0;
    }

    slot = device_class - 1;
    count = 0;
    device = joystick_slot_devices[slot];
    if (device != -1) {
        count = input_devices[device].button_count;
    }

    found = 0;
    special_button = -1;
    if (action == _input_action_accept) {
        special_button = gamepad_action_buttons[slot][0];
    } else if (action == _input_action_back) {
        special_button = gamepad_action_buttons[slot][1];
    }
    if ((action == _input_action_accept || action == _input_action_back) && special_button != -1) {
        input_last_used_binding_set(action, _control_device_gamepad, (int16_t)slot,
                                     _control_input_button, special_button, 0);
        found = 1;
    }

    if (!found) {
        for (i = 0; i < count; i++) {
            if (gamepad_button_bindings[slot][i] == action) {
                if (action >= 0 && action < k_input_action_count) {
                    input_globals.last_used_bindings[action].device_type = _control_device_gamepad;
                    input_globals.last_used_bindings[action].device_index = (int16_t)slot;
                    input_globals.last_used_bindings[action].input_kind = _control_input_button;
                    input_globals.last_used_bindings[action].input_index = i;
                    input_globals.last_used_bindings[action].direction = 0;
                }
                found = 1;
                break;
            }
        }
    }

    if (!found) {
        // NOTE: reuses `count` (button_count), not axis_count, as the loop bound -- matches the
        // binary exactly (see the file header).
        for (i = 0; i < count; i++) {
            if (gamepad_axis_bindings[slot][i][1] == action) {
                if (action >= 0 && action < k_input_action_count) {
                    input_globals.last_used_bindings[action].device_type = _control_device_gamepad;
                    input_globals.last_used_bindings[action].device_index = (int16_t)slot;
                    input_globals.last_used_bindings[action].input_kind = _control_input_axis;
                    input_globals.last_used_bindings[action].input_index = i;
                    input_globals.last_used_bindings[action].direction = 2;
                }
                found = 1;
                break;
            }
            if (gamepad_axis_bindings[slot][i][0] == action) {
                if (action >= 0 && action < k_input_action_count) {
                    input_globals.last_used_bindings[action].device_type = _control_device_gamepad;
                    input_globals.last_used_bindings[action].device_index = (int16_t)slot;
                    input_globals.last_used_bindings[action].input_kind = _control_input_axis;
                    input_globals.last_used_bindings[action].input_index = i;
                    input_globals.last_used_bindings[action].direction = 1;
                }
                found = 1;
                break;
            }
        }

        if (!found) {
            // NOTE: reuses `count` (button_count), not pov_count, as the loop bound -- matches
            // the binary exactly.
            for (i = 0; i < count; i++) {
                for (octant = 0; octant < k_control_gamepad_pov_direction_count; octant++) {
                    if (gamepad_pov_bindings[slot][i][octant] == action) {
                        if (action >= 0 && action < k_input_action_count) {
                            input_globals.last_used_bindings[action].device_type = _control_device_gamepad;
                            input_globals.last_used_bindings[action].device_index = (int16_t)slot;
                            input_globals.last_used_bindings[action].input_kind = _control_input_pov;
                            input_globals.last_used_bindings[action].input_index = i;
                            input_globals.last_used_bindings[action].direction = octant;
                        }
                        return 1;
                    }
                }
            }
        }
    }

    return found;
}

#if 0
Original Ghidra decompilation (0x48bae0), from tools/pack.py 0x48bae0:

char FUN_0048bae0(void)

{
  int iVar1;
  short sVar2;
  char cVar3;
  int in_ECX;
  short *psVar4;
  int iVar5;
  int iVar6;
  short unaff_SI;
  short sVar7;
  int local_4;

  cVar3 = '\0';
  if (in_ECX < 0) {
    return '\0';
  }
  if (4 < in_ECX) {
    return '\0';
  }
  if (in_ECX == 0) {
    sVar7 = 0;
    psVar4 = &DAT_00710330;
    do {
      if (*psVar4 == unaff_SI) {
        if (unaff_SI < 0) {
          return '\x01';
        }
        if (0x1a < unaff_SI) {
          return '\x01';
        }
        iVar1 = unaff_SI * 0xc;
        *(undefined2 *)(&DAT_007127d4 + iVar1) = 1;
        *(undefined2 *)(&DAT_007127d6 + iVar1) = 0;
        *(undefined2 *)(&DAT_007127d8 + iVar1) = 0;
        *(short *)(&DAT_007127da + iVar1) = sVar7;
        *(undefined4 *)(&DAT_007127dc + iVar1) = 0;
        return '\x01';
      }
      psVar4 = psVar4 + 1;
      sVar7 = sVar7 + 1;
    } while ((int)psVar4 < 0x71040a);
    sVar7 = 0;
    psVar4 = &DAT_0071040a;
    do {
      if (*psVar4 == unaff_SI) {
        if (unaff_SI < 0) {
          return '\x01';
        }
        if (0x1a < unaff_SI) {
          return '\x01';
        }
        iVar1 = unaff_SI * 0xc;
        *(undefined2 *)(&DAT_007127d4 + iVar1) = 2;
        *(undefined2 *)(&DAT_007127d6 + iVar1) = 0;
        *(undefined2 *)(&DAT_007127d8 + iVar1) = 0;
        *(short *)(&DAT_007127da + iVar1) = sVar7;
        *(undefined4 *)(&DAT_007127dc + iVar1) = 0;
        return '\x01';
      }
      psVar4 = psVar4 + 1;
      sVar7 = sVar7 + 1;
    } while ((int)psVar4 < 0x71041a);
    sVar7 = 0;
    psVar4 = &DAT_0071041a;
    while( true ) {
      if (psVar4[1] == unaff_SI) {
        if ((-1 < unaff_SI) && (unaff_SI < 0x1b)) {
          iVar1 = unaff_SI * 0xc;
          *(undefined2 *)(&DAT_007127d4 + iVar1) = 2;
          *(undefined2 *)(&DAT_007127d6 + iVar1) = 0;
          *(undefined2 *)(&DAT_007127d8 + iVar1) = 1;
          *(short *)(&DAT_007127da + iVar1) = sVar7;
          *(undefined4 *)(&DAT_007127dc + iVar1) = 2;
        }
        return '\x01';
      }
      if (*psVar4 == unaff_SI) break;
      psVar4 = psVar4 + 2;
      sVar7 = sVar7 + 1;
      if (0x710425 < (int)psVar4) {
        return cVar3;
      }
    }
    if ((-1 < unaff_SI) && (unaff_SI < 0x1b)) {
      iVar1 = unaff_SI * 0xc;
      *(undefined2 *)(&DAT_007127d6 + iVar1) = 0;
      *(undefined2 *)(&DAT_007127d4 + iVar1) = 2;
      *(undefined2 *)(&DAT_007127d8 + iVar1) = 1;
      *(short *)(&DAT_007127da + iVar1) = sVar7;
      *(undefined4 *)(&DAT_007127dc + iVar1) = 1;
      return '\x01';
    }
    return '\x01';
  }
  iVar1 = in_ECX + -1;
  sVar7 = (short)iVar1;
  local_4 = 0;
  if ((&DAT_006b2ce8)[sVar7] != -1) {
    local_4 = *(int *)(&DAT_006b1aa0 + (&DAT_006b2ce8)[sVar7] * 0x240);
  }
  if (unaff_SI == 8) {
    sVar2 = (&DAT_00710526)[iVar1 * 2];
  }
  else {
    if (unaff_SI != 9) goto LAB_0048bc99;
    sVar2 = (&DAT_00710528)[iVar1 * 2];
  }
  if (sVar2 != -1) {
    FUN_00490050(0,sVar2,0);
    cVar3 = '\x01';
  }
LAB_0048bc99:
  iVar5 = 0;
  if (cVar3 == '\0') {
    for (; iVar5 < local_4; iVar5 = iVar5 + 1) {
      if ((&DAT_00710426)[iVar1 * 0x20 + iVar5] == unaff_SI) {
        if ((-1 < unaff_SI) && (unaff_SI < 0x1b)) {
          iVar6 = unaff_SI * 0xc;
          *(undefined2 *)(&DAT_007127d4 + iVar6) = 3;
          *(short *)(&DAT_007127d6 + iVar6) = sVar7;
          *(undefined2 *)(&DAT_007127d8 + iVar6) = 0;
          *(short *)(&DAT_007127da + iVar6) = (short)iVar5;
          *(undefined4 *)(&DAT_007127dc + iVar6) = 0;
        }
        cVar3 = '\x01';
        break;
      }
    }
  }
  iVar5 = 0;
  if (cVar3 == '\0') {
    for (; iVar5 < local_4; iVar5 = iVar5 + 1) {
      if ((&DAT_00710538)[(iVar1 * 0x20 + iVar5) * 2] == unaff_SI) {
        if ((-1 < unaff_SI) && (unaff_SI < 0x1b)) {
          iVar6 = unaff_SI * 0xc;
          *(undefined4 *)(&DAT_007127dc + iVar6) = 2;
LAB_0048bd5b:
          *(undefined2 *)(&DAT_007127d4 + iVar6) = 3;
          *(short *)(&DAT_007127d6 + iVar6) = sVar7;
          *(undefined2 *)(&DAT_007127d8 + iVar6) = 1;
          *(short *)(&DAT_007127da + iVar6) = (short)iVar5;
        }
LAB_0048bd6e:
        cVar3 = '\x01';
        break;
      }
      if ((&DAT_00710536)[(iVar1 * 0x20 + iVar5) * 2] == unaff_SI) {
        if ((-1 < unaff_SI) && (unaff_SI < 0x1b)) {
          iVar6 = unaff_SI * 0xc;
          *(undefined4 *)(&DAT_007127dc + iVar6) = 1;
          goto LAB_0048bd5b;
        }
        goto LAB_0048bd6e;
      }
    }
    iVar5 = 0;
    if (cVar3 == '\0') {
      for (; iVar5 < local_4; iVar5 = iVar5 + 1) {
        iVar6 = 0;
        psVar4 = &DAT_00710736 + (iVar1 * 0x10 + iVar5) * 8;
        do {
          if (*psVar4 == unaff_SI) {
            if ((-1 < unaff_SI) && (unaff_SI < 0x1b)) {
              iVar1 = unaff_SI * 0xc;
              *(undefined2 *)(&DAT_007127d4 + iVar1) = 3;
              *(short *)(&DAT_007127d6 + iVar1) = sVar7;
              *(undefined2 *)(&DAT_007127d8 + iVar1) = 2;
              *(short *)(&DAT_007127da + iVar1) = (short)iVar5;
              *(int *)(&DAT_007127dc + iVar1) = iVar6;
            }
            return '\x01';
          }
          iVar6 = iVar6 + 1;
          psVar4 = psVar4 + 1;
        } while (iVar6 < 8);
      }
    }
  }
  return cVar3;
}

objdump 0x48bae0..0x48bde0 (Intel syntax; the caller-context evidence for ECX/ESI and the
FUN_00490050 argument setup is quoted in the header comment above; full trace omitted here for
length -- see the pack.py output for the working notes).
#endif
