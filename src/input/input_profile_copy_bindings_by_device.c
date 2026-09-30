// input_profile_copy_bindings_by_device  (Ghidra: FUN_00490280)
// address 0x490280, size 660 bytes
// name confidence: 0.4   rewrite confidence: 0.55
// evidence: out/phase4/input_types_notes.md names this "input_profile_copy_bindings_by_device"
// and states it "copies ranges between two saved_player_profile records (saved_games.h). Its
// only caller is 0x4901b0, so it stays here, but its type is the profile module's." Every field
// offset below was matched against types/saved_games.h's saved_player_profile layout, which
// pins the finding that category 1 (gamepad fields) copies only gamepad slot 0 in every one of
// its sub-ranges (button/axis/pov bindings, the action-button pair, the whole
// controls_gamepad_record, and the per-gamepad rate bytes), never all four gamepads, even though
// the source/destination fields are 4-gamepad arrays. That is reproduced literally; it is not
// this function generalizing to other slots elsewhere, since it has exactly one caller and that
// caller only ever passes category 2.
// UNSURE: category 2 also ORs 0x0f into every byte of campaign_progress (10 bytes) instead of
// copying it from src; kept exactly as decompiled.
// register convention: category in EAX (in_EAX), destination profile in EDX (in_EDX), source
// profile in EBX (unaff_EBX)
// reconciled: R21 saved_player_profile unknown_12f/131/93c[6]/954/955/960[2] -> look_inverted, look_inverted_driving, forward_rate..mouse_strafe_scale, mouse_look_x/y_sensitivity, gamepad_axis_scale_x/y

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"
#include "fn_input.h"

#include <string.h>

// blam-cc: category in EAX, destination in EDX, source in EBX
// Copies a category of binding/settings fields from src to dst:
//   2 -- identity (name, player_color, campaign_progress OR-mask) plus keyboard/mouse bindings
//        and the mouse-scale rates
//   0 -- keyboard/mouse bindings and the mouse-scale rates only
//   1 -- gamepad slot 0's bindings, action buttons, device record and per-pad rates only
//   any other value -- nothing category-specific
// Every category except -1 additionally copies the four digital movement/look rates and the
// look-inversion bytes. Returns 0 immediately for category -1, 1 otherwise.
uint8_t input_profile_copy_bindings_by_device(int32_t category, saved_player_profile *dst,
                                               saved_player_profile *src)
{
    int32_t i;

    if (category == 2) {
        memcpy(dst->name, src->name, sizeof(dst->name));
        dst->name[k_player_profile_name_length - 1] = 0;
        dst->player_color = src->player_color;

        for (i = 0; i < k_campaign_level_count; i++) {
            dst->campaign_progress[i] |= 0x0f;
        }

        memcpy(dst->keyboard_bindings, src->keyboard_bindings, sizeof(dst->keyboard_bindings));
        memcpy(dst->mouse_button_bindings, src->mouse_button_bindings, sizeof(dst->mouse_button_bindings));
        memcpy(dst->mouse_axis_bindings, src->mouse_axis_bindings, sizeof(dst->mouse_axis_bindings));

        dst->mouse_forward_scale = src->mouse_forward_scale;
        dst->mouse_strafe_scale = src->mouse_strafe_scale;
        dst->mouse_look_x_sensitivity = src->mouse_look_x_sensitivity;
        dst->mouse_look_y_sensitivity = src->mouse_look_y_sensitivity;
        dst->master_volume = src->master_volume;
        dst->effects_volume = src->effects_volume;
        dst->music_volume = src->music_volume;
    } else if (category == 0) {
        memcpy(dst->keyboard_bindings, src->keyboard_bindings, sizeof(dst->keyboard_bindings));
        memcpy(dst->mouse_button_bindings, src->mouse_button_bindings, sizeof(dst->mouse_button_bindings));
        memcpy(dst->mouse_axis_bindings, src->mouse_axis_bindings, sizeof(dst->mouse_axis_bindings));

        dst->mouse_forward_scale = src->mouse_forward_scale;
        dst->mouse_strafe_scale = src->mouse_strafe_scale;
        dst->mouse_look_x_sensitivity = src->mouse_look_x_sensitivity;
        dst->mouse_look_y_sensitivity = src->mouse_look_y_sensitivity;
    } else if (category == 1) {
        dst->gamepad_action_buttons[0][0] = src->gamepad_action_buttons[0][0];
        dst->gamepad_action_buttons[0][1] = src->gamepad_action_buttons[0][1];
        memcpy(dst->gamepad_button_bindings[0], src->gamepad_button_bindings[0],
               sizeof(dst->gamepad_button_bindings[0]));
        memcpy(dst->gamepad_axis_bindings[0], src->gamepad_axis_bindings[0],
               sizeof(dst->gamepad_axis_bindings[0]));
        memcpy(dst->gamepad_pov_bindings[0], src->gamepad_pov_bindings[0],
               sizeof(dst->gamepad_pov_bindings[0]));
        dst->gamepads[0] = src->gamepads[0];
        dst->gamepad_axis_scale_x = src->gamepad_axis_scale_x;
        dst->gamepad_axis_scale_y = src->gamepad_axis_scale_y;
        dst->gamepad_rate_a[0] = src->gamepad_rate_a[0];
        dst->gamepad_rate_b[0] = src->gamepad_rate_b[0];
    } else if (category == -1) {
        return 0;
    }

    dst->forward_rate = src->forward_rate;
    dst->strafe_rate = src->strafe_rate;
    dst->look_x_rate = src->look_x_rate;
    dst->look_y_rate = src->look_y_rate;
    dst->look_sensitivity = src->look_sensitivity;
    dst->look_inverted = src->look_inverted;
    dst->look_inverted_driving = src->look_inverted_driving;
    dst->unknown_132 = src->unknown_132;
    return 1;
}

#if 0
Original Ghidra decompilation (0x490280):

undefined4 FUN_00490280(void)

{
  undefined4 uVar1;
  int in_EAX;
  byte *pbVar2;
  int iVar3;
  int in_EDX;
  int unaff_EBX;
  undefined4 *puVar4;
  undefined4 *puVar5;

  if (in_EAX == 2) {
    *(undefined4 *)(in_EDX + 2) = *(undefined4 *)(unaff_EBX + 2);
    *(undefined4 *)(in_EDX + 6) = *(undefined4 *)(unaff_EBX + 6);
    *(undefined4 *)(in_EDX + 10) = *(undefined4 *)(unaff_EBX + 10);
    *(undefined4 *)(in_EDX + 0xe) = *(undefined4 *)(unaff_EBX + 0xe);
    *(undefined4 *)(in_EDX + 0x12) = *(undefined4 *)(unaff_EBX + 0x12);
    *(undefined4 *)(in_EDX + 0x16) = *(undefined4 *)(unaff_EBX + 0x16);
    *(undefined2 *)(in_EDX + 0x18) = 0;
    *(undefined2 *)(in_EDX + 0x11a) = *(undefined2 *)(unaff_EBX + 0x11a);
    pbVar2 = (byte *)(in_EDX + 0x11e);
    iVar3 = 10;
    do {
      *pbVar2 = *pbVar2 | 0xf;
      pbVar2 = pbVar2 + 1;
      iVar3 = iVar3 + -1;
    } while (iVar3 != 0);
    puVar4 = (undefined4 *)(unaff_EBX + 0x134);
    puVar5 = (undefined4 *)(in_EDX + 0x134);
    for (iVar3 = 0x36; iVar3 != 0; iVar3 = iVar3 + -1) {
      *puVar5 = *puVar4;
      puVar4 = puVar4 + 1;
      puVar5 = puVar5 + 1;
    }
    *(undefined2 *)puVar5 = *(undefined2 *)puVar4;
    *(undefined4 *)(in_EDX + 0x20e) = *(undefined4 *)(unaff_EBX + 0x20e);
    *(undefined4 *)(in_EDX + 0x212) = *(undefined4 *)(unaff_EBX + 0x212);
    *(undefined4 *)(in_EDX + 0x216) = *(undefined4 *)(unaff_EBX + 0x216);
    *(undefined4 *)(in_EDX + 0x21a) = *(undefined4 *)(unaff_EBX + 0x21a);
    *(undefined4 *)(in_EDX + 0x21e) = *(undefined4 *)(unaff_EBX + 0x21e);
    *(undefined4 *)(in_EDX + 0x222) = *(undefined4 *)(unaff_EBX + 0x222);
    *(undefined4 *)(in_EDX + 0x226) = *(undefined4 *)(unaff_EBX + 0x226);
    *(undefined4 *)(in_EDX + 0x94c) = *(undefined4 *)(unaff_EBX + 0x94c);
    *(undefined4 *)(in_EDX + 0x950) = *(undefined4 *)(unaff_EBX + 0x950);
    *(undefined1 *)(in_EDX + 0x954) = *(undefined1 *)(unaff_EBX + 0x954);
    *(undefined1 *)(in_EDX + 0x955) = *(undefined1 *)(unaff_EBX + 0x955);
    *(undefined1 *)(in_EDX + 0xb78) = *(undefined1 *)(unaff_EBX + 0xb78);
    *(undefined1 *)(in_EDX + 0xb79) = *(undefined1 *)(unaff_EBX + 0xb79);
    *(undefined1 *)(in_EDX + 0xb7a) = *(undefined1 *)(unaff_EBX + 0xb7a);
  }
  else if (in_EAX == 0) {
    puVar4 = (undefined4 *)(unaff_EBX + 0x134);
    puVar5 = (undefined4 *)(in_EDX + 0x134);
    for (iVar3 = 0x36; iVar3 != 0; iVar3 = iVar3 + -1) {
      *puVar5 = *puVar4;
      puVar4 = puVar4 + 1;
      puVar5 = puVar5 + 1;
    }
    *(undefined2 *)puVar5 = *(undefined2 *)puVar4;
    *(undefined4 *)(in_EDX + 0x20e) = *(undefined4 *)(unaff_EBX + 0x20e);
    *(undefined4 *)(in_EDX + 0x212) = *(undefined4 *)(unaff_EBX + 0x212);
    *(undefined4 *)(in_EDX + 0x216) = *(undefined4 *)(unaff_EBX + 0x216);
    *(undefined4 *)(in_EDX + 0x21a) = *(undefined4 *)(unaff_EBX + 0x21a);
    *(undefined4 *)(in_EDX + 0x21e) = *(undefined4 *)(unaff_EBX + 0x21e);
    *(undefined4 *)(in_EDX + 0x222) = *(undefined4 *)(unaff_EBX + 0x222);
    *(undefined4 *)(in_EDX + 0x226) = *(undefined4 *)(unaff_EBX + 0x226);
    *(undefined4 *)(in_EDX + 0x94c) = *(undefined4 *)(unaff_EBX + 0x94c);
    *(undefined4 *)(in_EDX + 0x950) = *(undefined4 *)(unaff_EBX + 0x950);
    *(undefined1 *)(in_EDX + 0x954) = *(undefined1 *)(unaff_EBX + 0x954);
    *(undefined1 *)(in_EDX + 0x955) = *(undefined1 *)(unaff_EBX + 0x955);
  }
  else if (in_EAX == 1) {
    *(undefined4 *)(in_EDX + 0x32a) = *(undefined4 *)(unaff_EBX + 0x32a);
    puVar4 = (undefined4 *)(unaff_EBX + 0x22a);
    puVar5 = (undefined4 *)(in_EDX + 0x22a);
    for (iVar3 = 0x10; iVar3 != 0; iVar3 = iVar3 + -1) {
      *puVar5 = *puVar4;
      puVar4 = puVar4 + 1;
      puVar5 = puVar5 + 1;
    }
    puVar4 = (undefined4 *)(unaff_EBX + 0x33a);
    puVar5 = (undefined4 *)(in_EDX + 0x33a);
    for (iVar3 = 0x20; iVar3 != 0; iVar3 = iVar3 + -1) {
      *puVar5 = *puVar4;
      puVar4 = puVar4 + 1;
      puVar5 = puVar5 + 1;
    }
    puVar4 = (undefined4 *)(unaff_EBX + 0x53a);
    puVar5 = (undefined4 *)(in_EDX + 0x53a);
    for (iVar3 = 0x40; iVar3 != 0; iVar3 = iVar3 + -1) {
      *puVar5 = *puVar4;
      puVar4 = puVar4 + 1;
      puVar5 = puVar5 + 1;
    }
    puVar4 = (undefined4 *)(unaff_EBX + 0x1108);
    puVar5 = (undefined4 *)(in_EDX + 0x1108);
    for (iVar3 = 0x88; iVar3 != 0; iVar3 = iVar3 + -1) {
      *puVar5 = *puVar4;
      puVar4 = puVar4 + 1;
      puVar5 = puVar5 + 1;
    }
    *(undefined4 *)(in_EDX + 0x960) = *(undefined4 *)(unaff_EBX + 0x960);
    *(undefined4 *)(in_EDX + 0x964) = *(undefined4 *)(unaff_EBX + 0x964);
    *(undefined1 *)(in_EDX + 0x956) = *(undefined1 *)(unaff_EBX + 0x956);
    *(undefined1 *)(in_EDX + 0x95a) = *(undefined1 *)(unaff_EBX + 0x95a);
  }
  else if (in_EAX == -1) {
    return 0;
  }
  *(undefined4 *)(in_EDX + 0x93c) = *(undefined4 *)(unaff_EBX + 0x93c);
  *(undefined4 *)(in_EDX + 0x940) = *(undefined4 *)(unaff_EBX + 0x940);
  *(undefined4 *)(in_EDX + 0x944) = *(undefined4 *)(unaff_EBX + 0x944);
  uVar1 = *(undefined4 *)(unaff_EBX + 0x948);
  *(undefined4 *)(in_EDX + 0x948) = uVar1;
  *(undefined1 *)(in_EDX + 0x12e) = *(undefined1 *)(unaff_EBX + 0x12e);
  *(undefined1 *)(in_EDX + 0x12f) = *(undefined1 *)(unaff_EBX + 0x12f);
  *(undefined1 *)(in_EDX + 0x131) = *(undefined1 *)(unaff_EBX + 0x131);
  *(undefined1 *)(in_EDX + 0x132) = *(undefined1 *)(unaff_EBX + 0x132);
  return CONCAT31((int3)((uint)uVar1 >> 8),1);
}
#endif
