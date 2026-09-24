// input_clear_control_binding  (Ghidra: input_clear_control_binding, already named)
// address 0x48b9b0, size 291 bytes
// name confidence: 0.5   rewrite confidence: 0.75
// evidence: mirror image of input_apply_control_binding 0x48b7b0 (identical field addresses),
//   writing k_input_unbound (0x7fff) instead of an action index. Confirmed EAX is the live-in
//   binding pointer both by hs_unbind_control's call site (`lea eax,[esp]; call
//   0x48b9b0`, no other setup) and by this function's own entry (`sVar1 = *in_EAX` with no
//   prologue move). Unlike the apply path, the gamepad button branch here has no
//   button_count range check (only the axis and POV branches range-check); this asymmetry is
//   preserved exactly as compiled. See input_apply_control_binding.c for why the binding tables
//   are declared as their own externs rather than through player_control_settings's fields
//   (that struct only exposes this region as opaque unknown_ arrays in types/interface.h).
// register convention: EAX -> binding, no return value.
//   // blam-cc: EAX -> binding

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"

extern input_device input_devices[8];                        // 0x006b1868
extern int32_t joystick_slot_devices[4];                      // 0x006b2ce8

// Live binding tables inside input_abstraction_globals::settings[0] (0x00710328 + 0x8..0x80e);
// see input_apply_control_binding.c for the naming rationale.
extern int16_t keyboard_bindings[k_control_keyboard_key_count];                    // 0x00710330
extern int16_t mouse_button_bindings[k_control_mouse_button_count];                // 0x0071040a
extern int16_t mouse_axis_bindings[k_control_mouse_axis_count][2];                 // 0x0071041a
extern int16_t gamepad_button_bindings[k_control_gamepad_count][k_control_gamepad_button_count]; // 0x00710426
extern int16_t gamepad_axis_bindings[k_control_gamepad_count][k_control_gamepad_axis_count][2];  // 0x00710536
extern int16_t gamepad_pov_bindings[k_control_gamepad_count][k_control_gamepad_pov_count][k_control_gamepad_pov_direction_count]; // 0x00710736

// Clears (unbinds) a device input's game-control assignment in the matching keyboard/mouse/
// joystick binding table of the active binding profile (settings[0]), writing k_input_unbound.
// The gamepad axis and POV branches range-check input_index against the mapped device's
// reported count first (as input_apply_control_binding does); the gamepad button branch does
// not, matching the binary.
void input_clear_control_binding(control_binding_descriptor *binding)
{
    int32_t device;
    int32_t count;

    switch (binding->device_type) {
    case _control_device_keyboard:
        keyboard_bindings[binding->input_index] = k_input_unbound;
        return;

    case _control_device_mouse:
        if (binding->input_kind != _control_input_axis) {
            mouse_button_bindings[binding->input_index] = k_input_unbound;
        } else if (binding->direction == 1) {
            mouse_axis_bindings[binding->input_index][0] = k_input_unbound;
        } else {
            mouse_axis_bindings[binding->input_index][1] = k_input_unbound;
        }
        return;

    case _control_device_gamepad:
        if (binding->input_kind == _control_input_axis) {
            count = 0;
            device = joystick_slot_devices[binding->device_index];
            if (device != -1) {
                count = input_devices[device].axis_count;
            }
            if (binding->input_index < count) {
                if (binding->direction == 1) {
                    gamepad_axis_bindings[binding->device_index][binding->input_index][0] = k_input_unbound;
                } else {
                    gamepad_axis_bindings[binding->device_index][binding->input_index][1] = k_input_unbound;
                }
            }
        } else if (binding->input_kind == _control_input_pov) {
            device = joystick_slot_devices[binding->device_index];
            count = 0;
            if (device != -1) {
                count = input_devices[device].pov_count;
            }
            if (binding->input_index < count) {
                gamepad_pov_bindings[binding->device_index][binding->input_index][binding->direction] = k_input_unbound;
            }
        } else {
            // No button_count range check here, matching the binary.
            gamepad_button_bindings[binding->device_index][binding->input_index] = k_input_unbound;
        }
        return;

    default:
        return;
    }
}

#if 0
Original Ghidra decompilation (0x48b9b0), from tools/pack.py 0x48b9b0:

void input_clear_control_binding(void)

{
  short sVar1;
  short *in_EAX;
  int iVar2;

  sVar1 = *in_EAX;
  if (sVar1 == 1) {
    (&DAT_00710330)[in_EAX[3]] = 0x7fff;
  }
  else {
    if (sVar1 == 2) {
      if (in_EAX[2] != 1) {
        (&DAT_0071040a)[in_EAX[3]] = 0x7fff;
        return;
      }
      if (*(int *)(in_EAX + 4) == 1) {
        (&DAT_0071041a)[in_EAX[3] * 2] = 0x7fff;
        return;
      }
      (&DAT_0071041c)[in_EAX[3] * 2] = 0x7fff;
      return;
    }
    if (sVar1 == 3) {
      if (in_EAX[2] == 1) {
        iVar2 = 0;
        if ((&DAT_006b2ce8)[in_EAX[1]] != -1) {
          iVar2 = *(int *)(&DAT_006b1a9c + (&DAT_006b2ce8)[in_EAX[1]] * 0x240);
        }
        if (in_EAX[3] < iVar2) {
          if (*(int *)(in_EAX + 4) == 1) {
            (&DAT_00710536)[(in_EAX[1] * 0x20 + (int)in_EAX[3]) * 2] = 0x7fff;
            return;
          }
          (&DAT_00710538)[(in_EAX[1] * 0x20 + (int)in_EAX[3]) * 2] = 0x7fff;
          return;
        }
      }
      else {
        if (in_EAX[2] != 2) {
          (&DAT_00710426)[in_EAX[1] * 0x20 + (int)in_EAX[3]] = 0x7fff;
          return;
        }
        iVar2 = 0;
        if ((&DAT_006b2ce8)[in_EAX[1]] != -1) {
          iVar2 = *(int *)(&DAT_006b1aa4 + (&DAT_006b2ce8)[in_EAX[1]] * 0x240);
        }
        if (in_EAX[3] < iVar2) {
          (&DAT_00710736)[*(int *)(in_EAX + 4) + (in_EAX[1] * 0x10 + (int)in_EAX[3]) * 8] = 0x7fff;
          return;
        }
      }
    }
  }
  return;
}
#endif
