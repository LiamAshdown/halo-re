// control_profile_reset_slot  (Ghidra: control_profile_reset_slot, already named)
// address 0x53b2b0, size 184 bytes
// name confidence: 0.55   rewrite confidence: 0.75
// evidence: out/phase4/saved_games_functions.md; out/phase4/saved_games_types_notes.md's
// gamepads[4] note ("0x53b2b0 / 0x53b470 (0x88-dword slot)"). Zeroes the whole
// controls_gamepad_record for the given gamepad, then unbinds every button/action/axis/pov
// binding for that gamepad -- the same reset player_profile_initialize performs for all 4
// gamepads at once.
// register convention: profile in ESI, gamepad_index in EDX.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"

// blam-cc: profile in ESI, gamepad_index in EDX
void control_profile_reset_slot(saved_player_profile *profile, int32_t gamepad_index)
{
    uint32_t *zero;
    int32_t i, j;

    if (profile == 0 || gamepad_index < 0 || 4 <= gamepad_index) {
        return;
    }

    zero = (uint32_t *)&profile->gamepads[gamepad_index];
    for (i = 0x88; i != 0; i = i - 1) {
        *zero = 0;
        zero = zero + 1;
    }

    for (i = 0; i < k_control_gamepad_button_count; i = i + 1) {
        profile->gamepad_button_bindings[gamepad_index][i] = k_control_binding_unbound;
    }
    profile->gamepad_action_buttons[gamepad_index][0] = -1;
    profile->gamepad_action_buttons[gamepad_index][1] = -1;
    for (i = 0; i < k_control_gamepad_axis_count; i = i + 1) {
        profile->gamepad_axis_bindings[gamepad_index][i][0] = k_control_binding_unbound;
        profile->gamepad_axis_bindings[gamepad_index][i][1] = k_control_binding_unbound;
    }
    for (i = 0; i < k_control_gamepad_pov_count; i = i + 1) {
        for (j = 0; j < k_control_gamepad_pov_direction_count; j = j + 1) {
            profile->gamepad_pov_bindings[gamepad_index][i][j] = k_control_binding_unbound;
        }
    }
}

#if 0
Original Ghidra decompilation (0x53b2b0):

void control_profile_reset_slot(void)

{
  undefined2 *puVar1;
  int iVar2;
  int in_EDX;
  int unaff_ESI;
  undefined4 *puVar3;

  if (((unaff_ESI != 0) && (-1 < in_EDX)) && (in_EDX < 4)) {
    puVar3 = (undefined4 *)(in_EDX * 0x220 + 0x1108 + unaff_ESI);
    for (iVar2 = 0x88; iVar2 != 0; iVar2 = iVar2 + -1) {
      *puVar3 = 0;
      puVar3 = puVar3 + 1;
    }
    puVar3 = (undefined4 *)(in_EDX * 0x40 + 0x22a + unaff_ESI);
    *puVar3 = 0x7fff7fff;
    puVar3[1] = 0x7fff7fff;
    puVar3[2] = 0x7fff7fff;
    puVar3[3] = 0x7fff7fff;
    puVar3[4] = 0x7fff7fff;
    puVar3[5] = 0x7fff7fff;
    puVar3[6] = 0x7fff7fff;
    puVar3[7] = 0x7fff7fff;
    puVar3[8] = 0x7fff7fff;
    puVar3[9] = 0x7fff7fff;
    puVar3[10] = 0x7fff7fff;
    puVar3[0xb] = 0x7fff7fff;
    puVar3[0xc] = 0x7fff7fff;
    puVar3[0xd] = 0x7fff7fff;
    puVar3[0xe] = 0x7fff7fff;
    puVar3[0xf] = 0x7fff7fff;
    *(undefined4 *)(unaff_ESI + 0x32a + in_EDX * 4) = 0xffffffff;
    puVar1 = (undefined2 *)(in_EDX * 0x80 + 0x33c + unaff_ESI);
    iVar2 = 0x20;
    do {
      puVar1[-1] = 0x7fff;
      *puVar1 = 0x7fff;
      puVar1 = puVar1 + 2;
      iVar2 = iVar2 + -1;
    } while (iVar2 != 0);
    puVar3 = (undefined4 *)(in_EDX * 0x100 + 0x53a + unaff_ESI);
    for (iVar2 = 0x40; iVar2 != 0; iVar2 = iVar2 + -1) {
      *puVar3 = 0x7fff7fff;
      puVar3 = puVar3 + 1;
    }
  }
  return;
}
#endif
