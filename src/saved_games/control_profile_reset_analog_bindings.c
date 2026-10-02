// control_profile_reset_analog_bindings  (Ghidra: control_profile_reset_analog_bindings, already named)
// address 0x53a0d0, size 116 bytes
// name confidence: 0.55   rewrite confidence: 0.65
// evidence: out/phase4/saved_games_functions.md; out/phase4/saved_games_types_notes.md's
// mouse_button_bindings[8] (0x20e) and mouse_axis_bindings[3][2] (0x21e) notes. Unbinds every
// mouse button and axis direction, then sets 3 button and 3 axis-direction defaults (axis 2's
// two directions and mouse buttons 3..7 stay unbound).
// register convention: profile in ECX.
// UNSURE: which physical mouse button/axis each index names, and which logical action each
// literal id (6, 7, 0xb, 0x17..0x1a) names.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"

// blam-cc: profile in ECX
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
void control_profile_reset_analog_bindings(saved_player_profile *profile)
{
    int32_t i;

    for (i = 0; i < k_control_mouse_button_count; i = i + 1) {
        profile->mouse_button_bindings[i] = k_control_binding_unbound;
    }
    for (i = 0; i < k_control_mouse_axis_count; i = i + 1) {
        profile->mouse_axis_bindings[i][0] = k_control_binding_unbound;
        profile->mouse_axis_bindings[i][1] = k_control_binding_unbound;
    }

    profile->mouse_axis_bindings[0][1] = 0x19;
    profile->mouse_button_bindings[0] = 7;
    profile->mouse_button_bindings[2] = 6;
    profile->mouse_button_bindings[1] = 0xb;
    profile->mouse_axis_bindings[1][0] = 0x17;
    profile->mouse_axis_bindings[1][1] = 0x18;
    profile->mouse_axis_bindings[0][0] = 0x1a;
}

#if 0
Original Ghidra decompilation (0x53a0d0):

void control_profile_reset_analog_bindings(void)

{
  undefined2 *puVar1;
  int in_ECX;
  int iVar2;

  *(undefined4 *)(in_ECX + 0x20e) = 0x7fff7fff;
  *(undefined4 *)(in_ECX + 0x212) = 0x7fff7fff;
  *(undefined4 *)(in_ECX + 0x216) = 0x7fff7fff;
  *(undefined4 *)(in_ECX + 0x21a) = 0x7fff7fff;
  iVar2 = 3;
  puVar1 = (undefined2 *)(in_ECX + 0x220);
  do {
    puVar1[-1] = 0x7fff;
    *puVar1 = 0x7fff;
    puVar1 = puVar1 + 2;
    iVar2 = iVar2 + -1;
  } while (iVar2 != 0);
  *(undefined2 *)(in_ECX + 0x220) = 0x19;
  *(undefined2 *)(in_ECX + 0x20e) = 7;
  *(undefined2 *)(in_ECX + 0x212) = 6;
  *(undefined2 *)(in_ECX + 0x210) = 0xb;
  *(undefined2 *)(in_ECX + 0x222) = 0x17;
  *(undefined2 *)(in_ECX + 0x224) = 0x18;
  *(undefined2 *)(in_ECX + 0x21e) = 0x1a;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
