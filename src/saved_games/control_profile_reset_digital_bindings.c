// control_profile_reset_digital_bindings  (Ghidra: control_profile_reset_digital_bindings, already named)
// address 0x539ff0, size 212 bytes
// name confidence: 0.55   rewrite confidence: 0.6
// evidence: out/phase4/saved_games_functions.md; out/phase4/saved_games_types_notes.md's
// saved_player_profile::keyboard_bindings note ("0x134, action per key;
// control_profile_reset_digital_bindings fills 0x7fff then 21 defaults"). All 109 slots are
// first set to k_control_binding_unbound, then 21 specific keys are given default action ids;
// each byte offset here is converted to a keyboard_bindings[] index (offset - 0x134) / 2.
// register convention: profile in EDX (matching the module's register-convention note for
// control_profile_reset_analog_bindings/FUN_0053a150).
// UNSURE: which physical key each keyboard_bindings index names (no scancode table recovered
// in this batch) and which logical action each literal id (0..0x16) names.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"

// blam-cc: profile in EDX
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
void control_profile_reset_digital_bindings(saved_player_profile *profile)
{
    int32_t i;

    for (i = 0; i < k_control_keyboard_key_count; i = i + 1) {
        profile->keyboard_bindings[i] = k_control_binding_unbound;
    }

    profile->keyboard_bindings[0] = 9;
    profile->keyboard_bindings[32] = 0x13;
    profile->keyboard_bindings[46] = 0x14;
    profile->keyboard_bindings[45] = 0x15;
    profile->keyboard_bindings[47] = 0x16;
    profile->keyboard_bindings[49] = 1;
    profile->keyboard_bindings[30] = 3;
    profile->keyboard_bindings[34] = 0xd;
    profile->keyboard_bindings[48] = 4;
    profile->keyboard_bindings[59] = 0xe;
    profile->keyboard_bindings[72] = 0;
    profile->keyboard_bindings[69] = 10;
    profile->keyboard_bindings[31] = 5;
    profile->keyboard_bindings[58] = 0xb;
    profile->keyboard_bindings[33] = 2;
    profile->keyboard_bindings[56] = 8;
    profile->keyboard_bindings[35] = 0xf;
    profile->keyboard_bindings[36] = 0x10;
    profile->keyboard_bindings[50] = 0x11;
    profile->keyboard_bindings[1] = 0xc;
    profile->keyboard_bindings[13] = 0x12;
}

#if 0
Original Ghidra decompilation (0x539ff0):

void control_profile_reset_digital_bindings(void)

{
  int iVar1;
  int in_EDX;
  undefined4 *puVar2;

  puVar2 = (undefined4 *)(in_EDX + 0x134);
  for (iVar1 = 0x36; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar2 = 0x7fff7fff;
    puVar2 = puVar2 + 1;
  }
  *(undefined2 *)puVar2 = 0x7fff;
  *(undefined2 *)(in_EDX + 0x134) = 9;
  *(undefined2 *)(in_EDX + 0x174) = 0x13;
  *(undefined2 *)(in_EDX + 400) = 0x14;
  *(undefined2 *)(in_EDX + 0x18e) = 0x15;
  *(undefined2 *)(in_EDX + 0x192) = 0x16;
  *(undefined2 *)(in_EDX + 0x196) = 1;
  *(undefined2 *)(in_EDX + 0x170) = 3;
  *(undefined2 *)(in_EDX + 0x178) = 0xd;
  *(undefined2 *)(in_EDX + 0x194) = 4;
  *(undefined2 *)(in_EDX + 0x1aa) = 0xe;
  *(undefined2 *)(in_EDX + 0x1c4) = 0;
  *(undefined2 *)(in_EDX + 0x1be) = 10;
  *(undefined2 *)(in_EDX + 0x172) = 5;
  *(undefined2 *)(in_EDX + 0x1a8) = 0xb;
  *(undefined2 *)(in_EDX + 0x176) = 2;
  *(undefined2 *)(in_EDX + 0x1a4) = 8;
  *(undefined2 *)(in_EDX + 0x17a) = 0xf;
  *(undefined2 *)(in_EDX + 0x17c) = 0x10;
  *(undefined2 *)(in_EDX + 0x198) = 0x11;
  *(undefined2 *)(in_EDX + 0x136) = 0xc;
  *(undefined2 *)(in_EDX + 0x14e) = 0x12;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
