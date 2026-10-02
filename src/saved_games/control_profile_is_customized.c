// control_profile_is_customized  (Ghidra: control_profile_is_customized, already named)
// address 0x53b370, size 243 bytes
// name confidence: 0.5   rewrite confidence: 0.65
// evidence: out/phase4/saved_games_functions.md; objdump confirms the result (Ghidra's local
// bVar1, computed but never visibly returned) is AL (0x53b45a/0x53b462), so this is a bool
// return, not void as Ghidra guessed.
// register convention: profile in EDI, gamepad_index in EBX.

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

// blam-cc: profile in EDI, gamepad_index in EBX
// True if the gamepad slot is in use (a nonzero first name word) and any of its button, action,
// axis or pov bindings differs from the unbound default.
uint8_t control_profile_is_customized(saved_player_profile *profile, int32_t gamepad_index)
{
    int32_t i, j;

    if (profile == 0 || gamepad_index < 0 || 4 <= gamepad_index) {
        return 0;
    }
    if (profile->gamepads[gamepad_index].name[0] == 0) {
        return 0;
    }

    for (i = 0; i < k_control_gamepad_button_count; i = i + 1) {
        if (profile->gamepad_button_bindings[gamepad_index][i] != k_control_binding_unbound) {
            return 1;
        }
    }
    for (i = 0; i < 2; i = i + 1) {
        if (profile->gamepad_action_buttons[gamepad_index][i] != -1) {
            return 1;
        }
    }
    for (i = 0; i < k_control_gamepad_axis_count; i = i + 1) {
        if (profile->gamepad_axis_bindings[gamepad_index][i][0] != k_control_binding_unbound ||
            profile->gamepad_axis_bindings[gamepad_index][i][1] != k_control_binding_unbound) {
            return 1;
        }
    }
    for (i = 0; i < k_control_gamepad_pov_count; i = i + 1) {
        for (j = 0; j < k_control_gamepad_pov_direction_count; j = j + 1) {
            if (profile->gamepad_pov_bindings[gamepad_index][i][j] != k_control_binding_unbound) {
                return 1;
            }
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x53b370):

void control_profile_is_customized(void)

{
  bool bVar1;
  short *psVar2;
  int iVar3;
  int iVar4;
  int unaff_EBX;
  int unaff_EDI;

  bVar1 = false;
  if ((((unaff_EDI != 0) && (-1 < unaff_EBX)) && (unaff_EBX < 4)) &&
     (*(short *)(unaff_EBX * 0x220 + 0x1108 + unaff_EDI) != 0)) {
    iVar4 = 0;
    psVar2 = (short *)(unaff_EBX * 0x40 + 0x22a + unaff_EDI);
    do {
      if (*psVar2 != 0x7fff) {
        bVar1 = true;
        break;
      }
      iVar4 = iVar4 + 1;
      psVar2 = psVar2 + 1;
    } while (iVar4 < 0x20);
    iVar4 = 0;
    if (!bVar1) {
      for (; iVar4 < 2; iVar4 = iVar4 + 1) {
        if (*(short *)(unaff_EDI + 0x32a + (iVar4 + unaff_EBX * 2) * 2) != -1) {
          bVar1 = true;
          break;
        }
      }
    }
    if (!bVar1) {
      psVar2 = (short *)(unaff_EDI + 0x33a + unaff_EBX * 0x80);
      for (iVar4 = 0; iVar4 < 0x20; iVar4 = iVar4 + 1) {
        if ((*psVar2 != 0x7fff) ||
           (*(short *)(unaff_EDI + 0x33c + (unaff_EBX * 0x20 + iVar4) * 4) != 0x7fff)) {
          bVar1 = true;
          break;
        }
        psVar2 = psVar2 + 2;
      }
    }
    for (iVar4 = 0; (!bVar1 && (iVar4 < 0x10)); iVar4 = iVar4 + 1) {
      iVar3 = 0;
      psVar2 = (short *)((unaff_EBX * 0x10 + iVar4) * 0x10 + 0x53a + unaff_EDI);
      do {
        if (*psVar2 != 0x7fff) {
          bVar1 = true;
          break;
        }
        iVar3 = iVar3 + 1;
        psVar2 = psVar2 + 1;
      } while (iVar3 < 8);
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
