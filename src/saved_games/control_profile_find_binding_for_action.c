// control_profile_find_binding_for_action  (Ghidra: FUN_0053aa20, named in this rewrite)
// address 0x53aa20, size 723 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: out/phase4/saved_games_types_notes.md's control_binding_descriptor note
// ("0x53aa20 writes kind/index/direction ([2], [3], dword [4])") and its sole caller,
// src/interface/controls_enumerate_next_assignable_action.c (0x4b43e0), which already
// documents the real signature: "extern uint8_t FUN_0053aa20(const char *action_name,
// const int16_t *binding); // is the control bound to the action, blam-cc: ECX action_name,
// ESI binding". Confirmed against objdump 0x53aa20..0x53acf2: `mov ebx,ecx` immediately
// before `call 0x48fe60` (input_action_name_to_index, which takes its name in EBX) proves
// ECX holds action_name; every field access is through ESI. The seven scan tables this
// function walks (keyboard 0x714fb4[0x6d], mouse button 0x71508e[8], mouse axis
// 0x71509e[3][2], gamepad button 0x7150aa[4][32], gamepad action-buttons 0x7151aa[4][2],
// gamepad axis 0x7151ba[4][32][2], gamepad pov 0x7153ba[4][16][8]) sit back to back in
// .data (each one ends exactly where the next begins) and share the exact array shapes of
// the matching saved_player_profile binding fields; control_profile_clear_binding (0x53ad00,
// rewritten alongside this function) indexes the same seven tables with the same strides,
// confirmed there via objdump too. input_device_get_axis_count/_30/_50 (module "input", not yet rewritten)
// are the per-device axis/button/pov-hat counts from out/phase4/input_functions.md; objdump
// shows all three called with the device index in ECX (`mov ecx,edi; call 0x491630` etc).
// register convention: ECX action_name, ESI binding (in/out); returns bool in EAX (al).
//   // blam-cc: action_name -> ECX, binding -> ESI
// UNSURE: the gamepad pov-hat search (bottom loop) does not return as soon as it finds a
// match -- it keeps scanning every remaining pov index and overwrites binding->input_index/
// direction each time one matches, so the LAST matching pov index wins, not the first. This
// looks like a decompiler-faithful bug in the original code (every other table search in
// this function returns immediately on the first match) and is reproduced exactly, not
// "fixed".

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"

extern int32_t selected_saved_item; // 0x00714e7c, low nibble: 0 profile, 1 variant

extern int16_t control_keyboard_scan_table[k_control_keyboard_key_count]; // 0x00714fb4
extern int16_t control_mouse_button_scan_table[k_control_mouse_button_count]; // 0x0071508e
extern int16_t control_mouse_axis_scan_table[k_control_mouse_axis_count][2]; // 0x0071509e
extern int16_t control_gamepad_button_scan_table[k_control_gamepad_count][k_control_gamepad_button_count]; // 0x007150aa
extern int16_t control_gamepad_action_scan_buttons[k_control_gamepad_count][2]; // 0x007151aa, action 8 -> [0], action 9 -> [1]
extern int16_t control_gamepad_axis_scan_table[k_control_gamepad_count][k_control_gamepad_axis_count][2]; // 0x007151ba
extern int16_t control_gamepad_pov_scan_table[k_control_gamepad_count][k_control_gamepad_pov_count][k_control_gamepad_pov_direction_count]; // 0x007153ba

extern int16_t input_action_name_to_index(const char *action_name); // 0x48fe60, blam-cc: EBX action_name
extern int32_t input_device_get_button_count(int32_t device_index); // 0x491630, gamepad button count, blam-cc: ECX device_index
extern int32_t input_device_get_axis_count(int32_t device_index); // 0x491610, gamepad axis count, blam-cc: ECX device_index
extern int32_t input_device_get_pov_count(int32_t device_index); // 0x491650, gamepad pov-hat count, blam-cc: ECX device_index

// blam-cc: action_name -> ECX, binding -> ESI
uint8_t control_profile_find_binding_for_action(const char *action_name, control_binding_descriptor *binding)
{
    int16_t action_index;

    if ((selected_saved_item & 0xf) != 0) {
        return 0;
    }

    action_index = input_action_name_to_index(action_name);
    if (action_index == (int16_t)k_control_binding_unbound) {
        return 0;
    }

    if (binding->device_type == _control_device_keyboard) {
        int32_t i;

        i = binding->input_index;
        if (i > 0x6c) {
            return 0;
        }
        while (control_keyboard_scan_table[i] != action_index) {
            i = i + 1;
            if (i > 0x6c) {
                return 0;
            }
        }
        binding->input_kind = _control_input_button;
        binding->input_index = (int16_t)i;
        return 1;
    }

    if (binding->device_type != _control_device_mouse) {
        int32_t device;
        int32_t button_count, axis_count, pov_count;
        int32_t i;
        uint8_t found;

        if (binding->device_type != _control_device_gamepad) {
            return 0;
        }

        device = binding->device_index;
        if (device < 0 || 3 < device) {
            return 0;
        }

        button_count = input_device_get_button_count(device);
        axis_count = input_device_get_axis_count(device);
        pov_count = input_device_get_pov_count(device);

        if (action_index == 8) {
            if (control_gamepad_action_scan_buttons[device][0] == -1) {
                return 0;
            }
            binding->input_kind = _control_input_button;
            binding->input_index = control_gamepad_action_scan_buttons[device][0];
            return 1;
        }
        if (action_index == 9) {
            if (control_gamepad_action_scan_buttons[device][1] == -1) {
                return 0;
            }
            binding->input_kind = _control_input_button;
            binding->input_index = control_gamepad_action_scan_buttons[device][1];
            return 1;
        }

        for (i = 0; i < button_count; i = i + 1) {
            if (control_gamepad_button_scan_table[device][i] == action_index) {
                binding->input_kind = _control_input_button;
                binding->input_index = (int16_t)i;
                return 1;
            }
        }

        for (i = 0; i < axis_count; i = i + 1) {
            if (control_gamepad_axis_scan_table[device][i][0] == action_index) {
                binding->input_kind = _control_input_axis;
                binding->direction = 1;
                binding->input_index = (int16_t)i;
                return 1;
            }
            if (control_gamepad_axis_scan_table[device][i][1] == action_index) {
                binding->input_kind = _control_input_axis;
                binding->direction = 2;
                binding->input_index = (int16_t)i;
                return 1;
            }
        }

        if (pov_count < 1) {
            return 0;
        }
        // UNSURE (reproduced faithfully): does not return on the first pov match, keeps
        // scanning every remaining pov index -- the last match found wins.
        found = 0;
        for (i = 0; i < pov_count; i = i + 1) {
            int32_t d;

            for (d = 0; d < k_control_gamepad_pov_direction_count; d = d + 1) {
                if (control_gamepad_pov_scan_table[device][i][d] == action_index) {
                    binding->input_kind = _control_input_pov;
                    binding->direction = d;
                    binding->input_index = (int16_t)i;
                    found = 1;
                    break;
                }
            }
        }
        return found;
    }

    // mouse
    {
        int32_t i;

        for (i = 0; i < k_control_mouse_button_count; i = i + 1) {
            if (control_mouse_button_scan_table[i] == action_index) {
                binding->input_kind = _control_input_button;
                binding->input_index = (int16_t)i;
                return 1;
            }
        }
        for (i = 0; i < k_control_mouse_axis_count; i = i + 1) {
            if (control_mouse_axis_scan_table[i][0] == action_index) {
                binding->input_kind = _control_input_axis;
                binding->direction = 1;
                binding->input_index = (int16_t)i;
                return 1;
            }
            if (control_mouse_axis_scan_table[i][1] == action_index) {
                binding->input_kind = _control_input_axis;
                binding->direction = 2;
                binding->input_index = (int16_t)i;
                return 1;
            }
        }
        return 0;
    }
}

#if 0
Original Ghidra decompilation (0x53aa20):

undefined1 FUN_0053aa20(void)

{
  short sVar1;
  short sVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  short *psVar7;
  short *psVar8;
  short *unaff_ESI;
  int iVar9;
  undefined1 local_d;

  local_d = 0;
  if ((DAT_00714e7c & 0xf) == 0) {
    sVar2 = input_action_name_to_index();
    if (sVar2 == 0x7fff) {
      return 0;
    }
    sVar1 = *unaff_ESI;
    if (sVar1 == 1) {
      iVar3 = (int)unaff_ESI[3];
      if (0x6c < iVar3) {
        return 0;
      }
      psVar7 = (short *)((int)&DAT_00714fb4 + iVar3 * 2);
      while (*psVar7 != sVar2) {
        iVar3 = iVar3 + 1;
        psVar7 = psVar7 + 1;
        if (0x6c < iVar3) {
          return 0;
        }
      }
    }
    else {
      if (sVar1 != 2) {
        if (sVar1 != 3) {
          return 0;
        }
        sVar1 = unaff_ESI[1];
        if (sVar1 < 0) {
          return 0;
        }
        if (3 < sVar1) {
          return 0;
        }
        iVar9 = (int)sVar1;
        iVar3 = FUN_00491630();
        iVar4 = FUN_00491610();
        iVar5 = FUN_00491650();
        if (sVar2 == 8) {
          if (*(short *)(&DAT_007151aa + iVar9 * 4) == -1) {
            return 0;
          }
          unaff_ESI[2] = 0;
          unaff_ESI[3] = *(short *)(&DAT_007151aa + iVar9 * 4);
          return 1;
        }
        if (sVar2 == 9) {
          if (*(short *)(&DAT_007151ac + iVar9 * 4) == -1) {
            return 0;
          }
          unaff_ESI[2] = 0;
          unaff_ESI[3] = *(short *)(&DAT_007151ac + iVar9 * 4);
          return 1;
        }
        iVar6 = 0;
        if (0 < iVar3) {
          psVar7 = (short *)(&DAT_007150aa + iVar9 * 0x10);
          do {
            if (*psVar7 == sVar2) {
              unaff_ESI[2] = 0;
              unaff_ESI[3] = (short)iVar6;
              return 1;
            }
            iVar6 = iVar6 + 1;
            psVar7 = psVar7 + 1;
          } while (iVar6 < iVar3);
        }
        iVar3 = 0;
        if (0 < iVar4) {
          psVar7 = (short *)(&DAT_007151ba + iVar9 * 0x20);
          do {
            if (*psVar7 == sVar2) {
              unaff_ESI[2] = 1;
              unaff_ESI[4] = 1;
              unaff_ESI[5] = 0;
              unaff_ESI[3] = (short)iVar3;
              return 1;
            }
            if (*(short *)((int)&DAT_007151ba + (iVar9 * 0x20 + iVar3) * 4 + 2) == sVar2) {
              unaff_ESI[2] = 1;
              unaff_ESI[4] = 2;
              unaff_ESI[5] = 0;
              unaff_ESI[3] = (short)iVar3;
              return 1;
            }
            iVar3 = iVar3 + 1;
            psVar7 = psVar7 + 2;
          } while (iVar3 < iVar4);
        }
        iVar3 = 0;
        if (iVar5 < 1) {
          return 0;
        }
        psVar7 = (short *)(&DAT_007153ba + iVar9 * 0x40);
        do {
          iVar4 = 0;
          psVar8 = psVar7;
          do {
            if (*psVar8 == sVar2) {
              unaff_ESI[2] = 2;
              *(int *)(unaff_ESI + 4) = iVar4;
              unaff_ESI[3] = (short)iVar3;
              local_d = 1;
              break;
            }
            iVar4 = iVar4 + 1;
            psVar8 = psVar8 + 1;
          } while (iVar4 < 8);
          iVar3 = iVar3 + 1;
          psVar7 = psVar7 + 8;
          if (iVar5 <= iVar3) {
            return local_d;
          }
        } while( true );
      }
      iVar3 = 0;
      psVar7 = &DAT_0071508e;
      while (*psVar7 != sVar2) {
        iVar3 = iVar3 + 1;
        psVar7 = psVar7 + 1;
        if (7 < iVar3) {
          iVar3 = 0;
          psVar7 = &DAT_007150a0;
          while( true ) {
            if (psVar7[-1] == sVar2) {
              unaff_ESI[3] = (short)iVar3;
              unaff_ESI[2] = 1;
              unaff_ESI[4] = 1;
              unaff_ESI[5] = 0;
              return 1;
            }
            if (*psVar7 == sVar2) break;
            iVar3 = iVar3 + 1;
            psVar7 = psVar7 + 2;
            if (2 < iVar3) {
              return 0;
            }
          }
          unaff_ESI[3] = (short)iVar3;
          unaff_ESI[2] = 1;
          unaff_ESI[4] = 2;
          unaff_ESI[5] = 0;
          return 1;
        }
      }
    }
    unaff_ESI[2] = 0;
    unaff_ESI[3] = (short)iVar3;
    local_d = 1;
  }
  return local_d;
}
#endif
