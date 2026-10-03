// input_joystick_set_axis_scale_y  (Ghidra: FUN_0048c9a0; renamed per its behavior)
// address 0x48c9a0, size 109 bytes
// name confidence: 0.4   rewrite confidence: 0.75
// evidence: identical shape to input_joystick_set_axis_scale_x 0x48c930, one field over
//   (0x00710b5c == settings[slot] + 0x834, types/input.h's gamepad_axis_scale_y /
//   player_control_settings::gamepad_axis_scale_y).
// register convention: CX -> slot, stack -> value.
//   // blam-cc: CX -> slot, stack -> value
// reconciled: R19 player_control_settings unknown ranges named from the input.h field map (keyboard, mouse_button/mouse_axis, gamepad_button, gamepad_action_button, gamepad_axis, gamepad_pov, forward_rate..mouse_strafe_scale, mouse_look_x/y_sensitivity, gamepad_axis_scale_x/y, gamepad_rate_80/40, look_inverted/_driving); same offsets and widths

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
extern input_abstraction_globals input_globals; // 0x00710328

// Sets input_abstraction_globals.settings[slot]'s gamepad Y-axis sensitivity/deadzone scale
// (gamepad_axis_scale_y, 0x00710b5c + slot*0x85c), clamped to 0..1. Does nothing if slot is outside
// 0..k_control_gamepad_count-1.
void input_joystick_set_axis_scale_y(int16_t slot, float value)
{
    float *scale;

    if (slot >= 0 && slot < k_control_gamepad_count) {
        scale = &input_globals.settings[slot].gamepad_axis_scale_y;
        if (value < 0.0f) {
            *scale = 0.0f;
        } else if (1.0f < value) {
            *scale = 1.0f;
        } else {
            *scale = value;
        }
    }
}

#if 0
Original Ghidra decompilation (0x48c9a0), from tools/pack.py 0x48c9a0:

void FUN_0048c9a0(float param_1)

{
  short in_CX;

  if ((-1 < in_CX) && (in_CX < 4)) {
    if (param_1 < 0.0) {
      *(undefined4 *)(&DAT_00710b5c + in_CX * 0x85c) = 0;
      return;
    }
    if (1.0 < param_1) {
      *(undefined4 *)(&DAT_00710b5c + in_CX * 0x85c) = 0x3f800000;
      return;
    }
    *(float *)(&DAT_00710b5c + in_CX * 0x85c) = param_1;
  }
  return;
}

objdump call-site evidence (0x482359..0x482373), the same shape as the X setter:
  0048236c: mov ecx,[eax+0x4]     ; value
  0048236f: push ecx
  00482370: mov cx,[eax]          ; slot
  00482373: call 0x48c9a0
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
