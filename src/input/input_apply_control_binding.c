// input_apply_control_binding  (Ghidra: input_apply_control_binding, already named)
// address 0x48b7b0, size 285 bytes
// name confidence: 0.5   rewrite confidence: 0.75
// evidence: field offsets match types/input.h's player_control_settings field map exactly
//   (keyboard 0x00710330, mouse_button 0x0071040a, mouse_axis 0x0071041a/0x0071041c,
//   gamepad_button 0x00710426, gamepad_axis 0x00710536/0x00710538, gamepad_pov 0x00710736, all
//   inside settings[0]). player_control_settings itself (types/interface.h) only exposes these
//   ranges as opaque unknown_/pad byte arrays (its own module has not named them yet), so this
//   file declares the live tables directly at their absolute addresses instead, reusing the
//   field names and element types saved_games.h already gives the same layout inside
//   saved_player_profile. The joystick branches read input_device::axis_count/button_count/
//   pov_count (0x006b1a9c/0x006b1aa0/0x006b1aa4 == input_devices[0] + 0x234/0x238/0x23c) exactly
//   as documented. device_type/input_kind constants match saved_games.h control_device_type /
//   control_input_kind. objdump 0x48b7b0 confirms ECX is used directly with no prologue move
//   (the live-in binding pointer); EBX is likewise never loaded from anywhere in this function,
//   matching hs_bind_control's `mov ebx,eax` (action index) immediately before this call.
// register convention: ECX -> binding, EBX -> action_index (int16_t truncation on store).
//   // blam-cc: ECX -> binding, EBX -> action_index

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
extern input_device input_devices[8];                        // 0x006b1868
extern int32_t joystick_slot_devices[4];                      // 0x006b2ce8

// Live binding tables inside input_abstraction_globals::settings[0] (0x00710328 + 0x8..0x80e).
// player_control_settings (types/interface.h) does not name these fields itself; the names and
// element types below match the equivalent saved_player_profile fields in types/saved_games.h.
extern int16_t keyboard_bindings[k_control_keyboard_key_count];                    // 0x00710330
extern int16_t mouse_button_bindings[k_control_mouse_button_count];                // 0x0071040a
extern int16_t mouse_axis_bindings[k_control_mouse_axis_count][2];                 // 0x0071041a
extern int16_t gamepad_button_bindings[k_control_gamepad_count][k_control_gamepad_button_count]; // 0x00710426
extern int16_t gamepad_axis_bindings[k_control_gamepad_count][k_control_gamepad_axis_count][2];  // 0x00710536
extern int16_t gamepad_pov_bindings[k_control_gamepad_count][k_control_gamepad_pov_count][k_control_gamepad_pov_direction_count]; // 0x00710736

// Writes a parsed device-input descriptor's game-control assignment into the matching
// keyboard/mouse/joystick binding table of the active binding profile (settings[0]). A joystick
// binding is range-checked against the target device's reported axis/button/POV count and
// silently dropped if the descriptor's input_index is out of range or the slot has no device.
// Returns 1 on a successful write, 0 otherwise (unrecognized device_type, or out-of-range
// joystick input).
uint8_t input_apply_control_binding(control_binding_descriptor *binding, int32_t action_index)
{
    int32_t device;
    int32_t count;

    switch (binding->device_type) {
    case _control_device_keyboard:
        keyboard_bindings[binding->input_index] = (int16_t)action_index;
        return 1;

    case _control_device_mouse:
        if (binding->input_kind != _control_input_axis) {
            mouse_button_bindings[binding->input_index] = (int16_t)action_index;
        } else if (binding->direction == 1) {
            mouse_axis_bindings[binding->input_index][0] = (int16_t)action_index;
        } else {
            mouse_axis_bindings[binding->input_index][1] = (int16_t)action_index;
        }
        return 1;

    case _control_device_gamepad:
        if (binding->input_kind == _control_input_axis) {
            count = 0;
            device = joystick_slot_devices[binding->device_index];
            if (device != -1) {
                count = input_devices[device].axis_count;
            }
            if (binding->input_index < count) {
                if (binding->direction == 1) {
                    gamepad_axis_bindings[binding->device_index][binding->input_index][0] = (int16_t)action_index;
                } else {
                    gamepad_axis_bindings[binding->device_index][binding->input_index][1] = (int16_t)action_index;
                }
                return 1;
            }
        } else if (binding->input_kind == _control_input_pov) {
            device = joystick_slot_devices[binding->device_index];
            count = 0;
            if (device != -1) {
                count = input_devices[device].pov_count;
            }
            if (binding->input_index < count) {
                gamepad_pov_bindings[binding->device_index][binding->input_index][binding->direction] = (int16_t)action_index;
                return 1;
            }
        } else {
            device = joystick_slot_devices[binding->device_index];
            count = 0;
            if (device != -1) {
                count = input_devices[device].button_count;
            }
            if (binding->input_index < count) {
                gamepad_button_bindings[binding->device_index][binding->input_index] = (int16_t)action_index;
                return 1;
            }
        }
        return 0;

    default:
        return 0;
    }
}

#if 0
Original Ghidra decompilation (0x48b7b0), from tools/pack.py 0x48b7b0:

undefined4 input_apply_control_binding(void)

{
  short sVar1;
  int iVar2;
  undefined4 uVar3;
  short *in_ECX;
  int iVar4;
  undefined2 unaff_BX;
  int iVar5;

  sVar1 = *in_ECX;
  uVar3 = 1;
  if (sVar1 == 1) {
    (&DAT_00710330)[in_ECX[3]] = unaff_BX;
    return uVar3;
  }
  if (sVar1 != 2) {
    if (sVar1 == 3) {
      iVar5 = 0;
      if (in_ECX[2] == 1) {
        if ((&DAT_006b2ce8)[in_ECX[1]] != -1) {
          iVar5 = *(int *)(&DAT_006b1a9c + (&DAT_006b2ce8)[in_ECX[1]] * 0x240);
        }
        if (in_ECX[3] < iVar5) {
          iVar5 = in_ECX[1] * 0x20 + (int)in_ECX[3];
          if (*(int *)(in_ECX + 4) == 1) {
            (&DAT_00710536)[iVar5 * 2] = unaff_BX;
            return uVar3;
          }
          (&DAT_00710538)[iVar5 * 2] = unaff_BX;
          return uVar3;
        }
      }
      else {
        iVar4 = (int)in_ECX[1];
        iVar2 = (&DAT_006b2ce8)[iVar4];
        if (in_ECX[2] == 2) {
          if (iVar2 != -1) {
            iVar5 = *(int *)(&DAT_006b1aa4 + iVar2 * 0x240);
          }
          if (in_ECX[3] < iVar5) {
            (&DAT_00710736)[*(int *)(in_ECX + 4) + (iVar4 * 0x10 + (int)in_ECX[3]) * 8] = unaff_BX;
            return uVar3;
          }
        }
        else {
          if (iVar2 != -1) {
            iVar5 = *(int *)(&DAT_006b1aa0 + iVar2 * 0x240);
          }
          if (in_ECX[3] < iVar5) {
            (&DAT_00710426)[iVar4 * 0x20 + (int)in_ECX[3]] = unaff_BX;
            return uVar3;
          }
        }
      }
    }
    return 0;
  }
  if (in_ECX[2] != 1) {
    (&DAT_0071040a)[in_ECX[3]] = unaff_BX;
    return uVar3;
  }
  if (*(int *)(in_ECX + 4) == 1) {
    (&DAT_0071041a)[in_ECX[3] * 2] = unaff_BX;
    return uVar3;
  }
  (&DAT_0071041c)[in_ECX[3] * 2] = unaff_BX;
  return uVar3;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
