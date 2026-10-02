// input_joystick_set_axis_scale_x  (Ghidra: FUN_0048c930; renamed per its behavior)
// address 0x48c930, size 109 bytes
// name confidence: 0.4   rewrite confidence: 0.75
// evidence: writes input_abstraction_globals::settings[slot] + 0x830, which types/input.h's
//   field map names gamepad_axis_scale_x (player_control_settings::gamepad_axis_scale_x in
//   types/interface.h, that struct's own module not having named it yet). objdump confirms CX
//   is the live-in slot index (`test cx,cx` at entry, no prologue move) and the value is a plain
//   cdecl stack float, matching the two call sites (`push [struct+4]; mov cx,[struct]; call`).
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

// Sets input_abstraction_globals.settings[slot]'s gamepad X-axis sensitivity/deadzone scale
// (gamepad_axis_scale_x, 0x00710b58 + slot*0x85c), clamped to 0..1. Does nothing if slot is outside
// 0..k_control_gamepad_count-1.
void input_joystick_set_axis_scale_x(int16_t slot, float value)
{
    float *scale;

    if (slot >= 0 && slot < k_control_gamepad_count) {
        scale = &input_globals.settings[slot].gamepad_axis_scale_x;
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
Original Ghidra decompilation (0x48c930), from tools/pack.py 0x48c930:

void FUN_0048c930(float param_1)

{
  short in_CX;

  if ((-1 < in_CX) && (in_CX < 4)) {
    if (param_1 < 0.0) {
      *(undefined4 *)(&DAT_00710b58 + in_CX * 0x85c) = 0;
      return;
    }
    if (1.0 < param_1) {
      *(undefined4 *)(&DAT_00710b58 + in_CX * 0x85c) = 0x3f800000;
      return;
    }
    *(float *)(&DAT_00710b58 + in_CX * 0x85c) = param_1;
  }
  return;
}

objdump call-site evidence (both statically resolvable callers, e.g. 0x4822b9..0x4822d3):
  004822cc: mov ecx,[eax+0x4]     ; value
  004822cf: push ecx
  004822d0: mov cx,[eax]          ; slot
  004822d3: call 0x48c930
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
