// input_accumulate_axis_value  (Ghidra: FUN_0048ca10; renamed per its behavior)
// address 0x48ca10, size 302 bytes
// name confidence: 0.4   rewrite confidence: 0.75
// evidence: field offsets (settings + 0x810/0x814/0x818/0x81c, state + 0x14/0x18/0x1c/0x20)
//   match types/input.h's player_control_settings field map (forward_rate/strafe_rate/
//   look_x_rate/look_y_rate) and local_player_input_state (throttle_x/throttle_y/look_x/look_y)
//   exactly. The action ids in the switch (0x13..0x1a) match the input_action enum's digital
//   axis actions one for one.
// register convention: EAX -> settings, ECX -> state, DX -> action.
//   // blam-cc: EAX -> settings, ECX -> state, DX -> action
// reconciled: R19 player_control_settings unknown ranges named from the input.h field map (keyboard, mouse_button/mouse_axis, gamepad_button, gamepad_action_button, gamepad_axis, gamepad_pov, forward_rate..mouse_strafe_scale, mouse_look_x/y_sensitivity, gamepad_axis_scale_x/y, gamepad_rate_80/40, look_inverted/_driving); same offsets and widths

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"

// Accumulates a digitally-pressed control's contribution into the corresponding movement/look
// axis of *state (throttle_x for forward/backward, throttle_y for left/right, look_x for
// look_left/look_right, look_y for look_up/look_down), scaled by the matching per-tick rate in
// *settings, clamped to [-1, 1]. Does nothing for any other action id.
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
void input_accumulate_axis_value(player_control_settings *settings, local_player_input_state *state, int16_t action)
{
    const float *rates = &settings->forward_rate; // [0] forward, [1] strafe, [2] look_x, [3] look_y
    float value;
    float *axis;

    switch (action) {
    case _input_action_forward:
        axis = &state->throttle_x;
        value = *axis + rates[0];
        break;
    case _input_action_backward:
        axis = &state->throttle_x;
        value = *axis - rates[0];
        break;
    case _input_action_left:
        axis = &state->throttle_y;
        value = *axis + rates[1];
        break;
    case _input_action_right:
        axis = &state->throttle_y;
        value = *axis - rates[1];
        break;
    case _input_action_look_up:
        axis = &state->look_y;
        value = *axis + rates[3];
        break;
    case _input_action_look_down:
        axis = &state->look_y;
        value = *axis - rates[3];
        break;
    case _input_action_look_left:
        axis = &state->look_x;
        value = *axis + rates[2];
        break;
    case _input_action_look_right:
        axis = &state->look_x;
        value = *axis - rates[2];
        break;
    default:
        return;
    }

    if (value < -1.0f) {
        *axis = -1.0f;
    } else if (1.0f < value) {
        *axis = 1.0f;
    } else {
        *axis = value;
    }
}

#if 0
Original Ghidra decompilation (0x48ca10), from tools/pack.py 0x48ca10:

void FUN_0048ca10(void)

{
  float fVar1;
  int in_EAX;
  int in_ECX;
  undefined2 in_DX;

  switch(in_DX) {
  case 0x13:
    fVar1 = *(float *)(in_EAX + 0x810) + *(float *)(in_ECX + 0x14);
    goto LAB_0048ca2f;
  case 0x14:
    fVar1 = *(float *)(in_ECX + 0x14) - *(float *)(in_EAX + 0x810);
LAB_0048ca2f:
    if (-1.0 <= fVar1) {
      if (1.0 < fVar1) {
        fVar1 = 1.0;
      }
      *(float *)(in_ECX + 0x14) = fVar1;
      return;
    }
    *(undefined4 *)(in_ECX + 0x14) = 0xbf800000;
    return;
  case 0x15:
    fVar1 = *(float *)(in_EAX + 0x814) + *(float *)(in_ECX + 0x18);
    goto LAB_0048ca75;
  case 0x16:
    fVar1 = *(float *)(in_ECX + 0x18) - *(float *)(in_EAX + 0x814);
LAB_0048ca75:
    if (-1.0 <= fVar1) {
      if (1.0 < fVar1) {
        fVar1 = 1.0;
      }
      *(float *)(in_ECX + 0x18) = fVar1;
      return;
    }
    *(undefined4 *)(in_ECX + 0x18) = 0xbf800000;
    return;
  case 0x17:
    fVar1 = *(float *)(in_EAX + 0x81c) + *(float *)(in_ECX + 0x20);
    goto LAB_0048cabb;
  case 0x18:
    fVar1 = *(float *)(in_ECX + 0x20) - *(float *)(in_EAX + 0x81c);
LAB_0048cabb:
    if (-1.0 <= fVar1) {
      if (1.0 < fVar1) {
        fVar1 = 1.0;
      }
      *(float *)(in_ECX + 0x20) = fVar1;
      return;
    }
    *(undefined4 *)(in_ECX + 0x20) = 0xbf800000;
    return;
  case 0x19:
    fVar1 = *(float *)(in_EAX + 0x818) + *(float *)(in_ECX + 0x1c);
    break;
  case 0x1a:
    fVar1 = *(float *)(in_ECX + 0x1c) - *(float *)(in_EAX + 0x818);
    break;
  default:
    goto switchD_0048ca1f_default;
  }
  if (fVar1 < -1.0) {
    *(undefined4 *)(in_ECX + 0x1c) = 0xbf800000;
    return;
  }
  if (1.0 < fVar1) {
    fVar1 = 1.0;
  }
  *(float *)(in_ECX + 0x1c) = fVar1;
switchD_0048ca1f_default:
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
