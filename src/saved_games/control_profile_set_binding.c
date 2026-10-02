// control_profile_set_binding  (Ghidra: control_profile_set_binding, already named)
// address 0x53ae10, size 489 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: out/phase4/saved_games_functions.md; already named and given a real prototype by
// its caller, src/interface's binding-row input handler. Writes `value` into the working
// copy's binding field named by `binding` (types/saved_games.h saved_item_working_copy at
// 0x00714e80, cast to saved_player_profile: keyboard_bindings 0x134, mouse_button_bindings
// 0x20e, mouse_axis_bindings 0x21e/0x220, gamepad_button_bindings 0x22a, gamepad_action_
// buttons 0x32a/0x32c, gamepad_axis_bindings 0x33a/0x33c, gamepad_pov_bindings 0x53a --
// every one of these byte offsets in the objdump matches the corresponding field offset in
// the struct exactly). For a gamepad button assigned to action 8 or 9 it also clears the
// OTHER action's button slot if it currently names the same physical button (so the two
// special actions never alias the same button); this is the only path that reaches into
// gamepad_action_buttons. Confirmed field-by-field against objdump 0x53ae10..0x53aff8: the
// value argument is the one stack word (`mov bp, [esp+0x10]` read before any push shifts
// esp further), matching Ghidra's `short param_1`; ESI is the binding descriptor exactly as
// in 0x53aa20 / 0x53ad00.
// register convention: ESI binding (in), one stack argument (int16_t value); returns bool
// in EAX (al), default 1, cleared to 0 only by the two gamepad range checks.
//   // blam-cc: binding -> ESI, value -> stack

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int32_t selected_saved_item; // 0x00714e7c, low nibble: 0 profile, 1 variant
extern saved_player_profile saved_item_working_copy; // 0x00714e80, the interface's live-edit copy

extern int32_t input_device_get_axis_count(int32_t device_index); // 0x491610, gamepad axis count, blam-cc: ECX device_index
extern int32_t input_device_get_button_count(int32_t device_index); // 0x491630, gamepad button count, blam-cc: ECX device_index
extern int32_t input_device_get_pov_count(int32_t device_index); // 0x491650, gamepad pov-hat count, blam-cc: ECX device_index

// VERIFIED against disassembly 0x53ae10..0x53aff8 (2026-09-30); fixed: an unknown device type returns 0, not 1.
// blam-cc: binding -> ESI, value -> stack
uint8_t control_profile_set_binding(const control_binding_descriptor *binding, int16_t value)
{
    saved_player_profile *profile;

    if ((selected_saved_item & 0xf) != 0) {
        return 1;
    }
    profile = &saved_item_working_copy;

    if (binding->device_type == _control_device_keyboard) {
        profile->keyboard_bindings[binding->input_index] = value;
        return 1;
    }

    if (binding->device_type == _control_device_mouse) {
        if (binding->input_kind != _control_input_axis) {
            profile->mouse_button_bindings[binding->input_index] = value;
            return 1;
        }
        if (binding->direction != 1) {
            profile->mouse_axis_bindings[binding->input_index][1] = value;
            return 1;
        }
        profile->mouse_axis_bindings[binding->input_index][0] = value;
        return 1;
    }

    if (binding->device_type == _control_device_gamepad) {
        int32_t device = binding->device_index;

        if (binding->input_kind == _control_input_axis) {
            int32_t index = binding->input_index;

            if (index < input_device_get_axis_count(device)) {
                if (binding->direction != 1) {
                    profile->gamepad_axis_bindings[device][index][1] = value;
                } else {
                    profile->gamepad_axis_bindings[device][index][0] = value;
                }
                return 1;
            }
        } else if (binding->input_kind == _control_input_pov) {
            int32_t index = binding->input_index;

            if (index < input_device_get_pov_count(device)) {
                profile->gamepad_pov_bindings[device][index][binding->direction] = value;
                return 1;
            }
        } else {
            int32_t index = binding->input_index;

            if (index < input_device_get_button_count(device)) {
                if (value == 8) {
                    profile->gamepad_action_buttons[device][0] = (int16_t)index;
                    if (profile->gamepad_action_buttons[device][1] == (int16_t)index) {
                        profile->gamepad_action_buttons[device][1] = (int16_t)k_control_binding_unbound;
                    }
                    return 1;
                }
                if (value == 9) {
                    profile->gamepad_action_buttons[device][1] = (int16_t)index;
                    if (profile->gamepad_action_buttons[device][0] == (int16_t)index) {
                        profile->gamepad_action_buttons[device][0] = (int16_t)k_control_binding_unbound;
                    }
                    return 1;
                }
                profile->gamepad_button_bindings[device][index] = value;
                return 1;
            }
        }
        return 0;
    }

    // 0x53ae4a: any device type other than 1..3 jumps to 0x53af45, which returns false (the draft returned true)
    return 0;
}

#if 0
Original Ghidra decompilation (0x53ae10):

undefined4 control_profile_set_binding(short param_1)

{
  short sVar1;
  uint uVar2;
  int iVar3;
  int iVar4;
  short *unaff_ESI;

  uVar2 = ~-(uint)((DAT_00714e7c & 0xf) != 0) & 0x714e80;
  if (uVar2 != 0) {
    sVar1 = *unaff_ESI;
    if (sVar1 != 1) {
      if (sVar1 == 2) {
        if (unaff_ESI[2] != 1) {
          *(short *)(uVar2 + 0x20e + unaff_ESI[3] * 2) = param_1;
          return 1;
        }
        if (*(int *)(unaff_ESI + 4) != 1) {
          *(short *)(uVar2 + 0x220 + unaff_ESI[3] * 4) = param_1;
          return 1;
        }
        *(short *)(uVar2 + 0x21e + unaff_ESI[3] * 4) = param_1;
        return 1;
      }
      if (sVar1 == 3) {
        sVar1 = unaff_ESI[1];
        if (unaff_ESI[2] == 1) {
          iVar3 = FUN_00491610();
          iVar4 = (int)unaff_ESI[3];
          if (iVar4 < iVar3) {
            if (*(int *)(unaff_ESI + 4) != 1) {
              *(short *)(uVar2 + 0x33c + (sVar1 * 0x20 + iVar4) * 4) = param_1;
              return 1;
            }
            *(short *)(uVar2 + 0x33a + (sVar1 * 0x20 + iVar4) * 4) = param_1;
            return 1;
          }
        }
        else if (unaff_ESI[2] == 2) {
          iVar3 = FUN_00491650();
          if (unaff_ESI[3] < iVar3) {
            *(short *)(uVar2 + 0x53a +
                      (*(int *)(unaff_ESI + 4) + (sVar1 * 0x10 + (int)unaff_ESI[3]) * 8) * 2) =
                 param_1;
            return 1;
          }
        }
        else {
          iVar3 = FUN_00491630();
          if (unaff_ESI[3] < iVar3) {
            if (param_1 == 8) {
              *(short *)(uVar2 + 0x32a + sVar1 * 4) = unaff_ESI[3];
              if (*(short *)(uVar2 + 0x32c + unaff_ESI[1] * 4) != unaff_ESI[3]) {
                return 1;
              }
              *(undefined2 *)(uVar2 + 0x32c + unaff_ESI[1] * 4) = 0xffff;
              return 1;
            }
            if (param_1 != 9) {
              *(short *)(uVar2 + 0x22a + (sVar1 * 0x20 + (int)unaff_ESI[3]) * 2) = param_1;
              return 1;
            }
            *(short *)(uVar2 + 0x32c + unaff_ESI[1] * 4) = unaff_ESI[3];
            if (*(short *)(uVar2 + 0x32a + unaff_ESI[1] * 4) != unaff_ESI[3]) {
              return 1;
            }
            *(undefined2 *)(uVar2 + 0x32a + unaff_ESI[1] * 4) = 0xffff;
            return 1;
          }
        }
      }
      return 0;
    }
    *(short *)(uVar2 + 0x134 + unaff_ESI[3] * 2) = param_1;
  }
  return 1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
