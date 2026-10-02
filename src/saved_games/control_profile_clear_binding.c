// control_profile_clear_binding  (Ghidra: control_profile_clear_binding, already named)
// address 0x53ad00, size 257 bytes
// name confidence: 0.5   rewrite confidence: 0.9
// evidence: out/phase4/saved_games_functions.md; already named and given a real prototype by
// its two callers (src/interface/controls_binding_clear.c: "extern void
// control_profile_clear_binding(const int16_t *record); // 0x53ad00, blam-cc: ESI record").
// Unbinds whatever device slot `binding` names by writing k_control_binding_unbound into the
// matching entry of the same seven scan tables control_profile_find_binding_for_action
// (0x53aa20, rewritten alongside this function) reads: keyboard 0x714fb4[0x6d], mouse button
// 0x71508e[8], mouse axis 0x71509e[3][2], gamepad button 0x7150aa[4][32], gamepad axis
// 0x7151ba[4][32][2], gamepad pov 0x7153ba[4][16][8]. Confirmed field-by-field against
// objdump 0x53ad00..0x53adfd: every table index/stride matches 0x53aa20's own disassembly
// exactly (e.g. gamepad button `shl eax,0x5; add eax,ecx; mov [eax*2+0x7150aa]`, i.e.
// (device*32+index)*2 bytes). Does not touch the gamepad action-button table (0x7151aa),
// consistent with the caller only ever clearing button/axis/pov slots, never the two
// dedicated action-8/action-9 buttons directly.
// register convention: ESI binding (in only); no return value, no stack arguments.
//   // blam-cc: binding -> ESI

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

extern int16_t control_keyboard_scan_table[k_control_keyboard_key_count]; // 0x00714fb4
extern int16_t control_mouse_button_scan_table[k_control_mouse_button_count]; // 0x0071508e
extern int16_t control_mouse_axis_scan_table[k_control_mouse_axis_count][2]; // 0x0071509e
extern int16_t control_gamepad_button_scan_table[k_control_gamepad_count][k_control_gamepad_button_count]; // 0x007150aa
extern int16_t control_gamepad_axis_scan_table[k_control_gamepad_count][k_control_gamepad_axis_count][2]; // 0x007151ba
extern int16_t control_gamepad_pov_scan_table[k_control_gamepad_count][k_control_gamepad_pov_count][k_control_gamepad_pov_direction_count]; // 0x007153ba

extern int32_t input_device_get_axis_count(int32_t device_index); // 0x491610, gamepad axis count, blam-cc: ECX device_index
extern int32_t input_device_get_pov_count(int32_t device_index); // 0x491650, gamepad pov-hat count, blam-cc: ECX device_index

// blam-cc: binding -> ESI
void control_profile_clear_binding(const control_binding_descriptor *binding)
{
    if ((selected_saved_item & 0xf) != 0) {
        return;
    }

    if (binding->device_type == _control_device_keyboard) {
        control_keyboard_scan_table[binding->input_index] = (int16_t)k_control_binding_unbound;
        return;
    }

    if (binding->device_type == _control_device_mouse) {
        if (binding->input_kind != _control_input_axis) {
            control_mouse_button_scan_table[binding->input_index] = (int16_t)k_control_binding_unbound;
            return;
        }
        if (binding->direction != 1) {
            control_mouse_axis_scan_table[binding->input_index][1] = (int16_t)k_control_binding_unbound;
            return;
        }
        control_mouse_axis_scan_table[binding->input_index][0] = (int16_t)k_control_binding_unbound;
        return;
    }

    if (binding->device_type == _control_device_gamepad) {
        if (binding->input_kind == _control_input_axis) {
            int32_t index = binding->input_index;
            int32_t device = binding->device_index;

            if (index < input_device_get_axis_count(device)) {
                if (binding->direction != 1) {
                    control_gamepad_axis_scan_table[device][index][1] = (int16_t)k_control_binding_unbound;
                } else {
                    control_gamepad_axis_scan_table[device][index][0] = (int16_t)k_control_binding_unbound;
                }
            }
        } else if (binding->input_kind == _control_input_pov) {
            int32_t index = binding->input_index;
            int32_t device = binding->device_index;

            if (index < input_device_get_pov_count(device)) {
                control_gamepad_pov_scan_table[device][index][binding->direction] = (int16_t)k_control_binding_unbound;
            }
        } else {
            control_gamepad_button_scan_table[binding->device_index][binding->input_index] = (int16_t)k_control_binding_unbound;
        }
    }
}

#if 0
Original Ghidra decompilation (0x53ad00):

void control_profile_clear_binding(void)

{
  short sVar1;
  short sVar2;
  int iVar3;
  int iVar4;
  short *unaff_ESI;

  if ((DAT_00714e7c & 0xf) == 0) {
    sVar1 = *unaff_ESI;
    if (sVar1 == 1) {
      *(undefined2 *)((int)&DAT_00714fb4 + unaff_ESI[3] * 2) = 0x7fff;
      return;
    }
    if (sVar1 == 2) {
      if (unaff_ESI[2] != 1) {
        (&DAT_0071508e)[unaff_ESI[3]] = 0x7fff;
        return;
      }
      if (*(int *)(unaff_ESI + 4) != 1) {
        (&DAT_007150a0)[unaff_ESI[3] * 2] = 0x7fff;
        return;
      }
      (&DAT_0071509e)[unaff_ESI[3] * 2] = 0x7fff;
      return;
    }
    if (sVar1 == 3) {
      if (unaff_ESI[2] == 1) {
        iVar4 = (int)unaff_ESI[3];
        sVar1 = unaff_ESI[1];
        iVar3 = FUN_00491610();
        if (iVar4 < iVar3) {
          if (*(int *)(unaff_ESI + 4) != 1) {
            *(undefined2 *)((int)&DAT_007151ba + (sVar1 * 0x20 + iVar4) * 4 + 2) = 0x7fff;
            return;
          }
          *(undefined2 *)(&DAT_007151ba + sVar1 * 0x20 + iVar4) = 0x7fff;
          return;
        }
      }
      else if (unaff_ESI[2] == 2) {
        sVar1 = unaff_ESI[3];
        sVar2 = unaff_ESI[1];
        iVar3 = FUN_00491650();
        if (sVar1 < iVar3) {
          *(undefined2 *)
           ((int)&DAT_007153ba + (*(int *)(unaff_ESI + 4) + (sVar2 * 0x10 + (int)sVar1) * 8) * 2) =
               0x7fff;
          return;
        }
      }
      else {
        *(undefined2 *)((int)&DAT_007150aa + (unaff_ESI[1] * 0x20 + (int)unaff_ESI[3]) * 2) = 0x7fff
        ;
      }
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
