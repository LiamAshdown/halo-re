// control_profile_find_or_create_gamepad_slot  (Ghidra: FUN_0053b470, renamed)
// address 0x53b470, size 133 bytes
// name confidence: 0.45   rewrite confidence: 0.55
// evidence: out/phase4/saved_games_functions.md summary "Finds or reuses a free control-profile
// slot for a device and stores new binding data into it." If the device (source) isn't already
// assigned a slot (control_profile_gamepad_slot_find returns -1), finds the first empty slot
// (name[0] == 0), resets it, copies the whole controls_gamepad_record in, and finalizes it via
// control_profile_finalize_slot (0x53b500, this session's sibling file).
// register convention: __cdecl (Ghidra-recognized); source and profile are the recognized stack
// parameters.

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

extern int32_t control_profile_gamepad_slot_find(saved_player_profile *profile, controls_gamepad_record *key); // 0x53b6b0, this module
extern void control_profile_reset_slot(saved_player_profile *profile, int32_t gamepad_index); // 0x53b2b0, this module
extern uint8_t control_profile_finalize_slot(saved_player_profile *profile, int32_t gamepad_index); // 0x53b500, this module

// VERIFIED against disassembly 0x53b470..0x53b4f4 (2026-09-30): the NULL/duplicate early exits (return 0), the first free slot
//   test (first 16-bit unit of the 0x220 byte record == 0), reset / 0x88-dword copy / finalize order and the return values match;
//   the callee register conventions (find: EDX profile, EBX key; reset: ESI, EDX; finalize: EDI + stack) are as the callees declare.
//   A difftest crash here would come from one of those three callees on random data.
uint8_t control_profile_find_or_create_gamepad_slot(controls_gamepad_record *source, saved_player_profile *profile)
{
    int32_t i;

    if (profile == 0 || control_profile_gamepad_slot_find(profile, source) != -1) {
        return 0;
    }

    for (i = 0; i < k_control_gamepad_count; i = i + 1) {
        if (profile->gamepads[i].name[0] == 0) {
            control_profile_reset_slot(profile, i);
            profile->gamepads[i] = *source;
            control_profile_finalize_slot(profile, i);
            return 1;
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x53b470):

undefined4 FUN_0053b470(undefined4 *param_1,int param_2)

{
  int iVar1;
  short *psVar2;
  int iVar3;
  undefined4 *puVar4;

  if ((param_2 != 0) && (iVar1 = FUN_0053b6b0(), iVar1 == -1)) {
    iVar1 = 0;
    psVar2 = (short *)(param_2 + 0x1108);
    do {
      if (((iVar1 < 0) || (3 < iVar1)) || (*psVar2 == 0)) {
        control_profile_reset_slot();
        puVar4 = (undefined4 *)(iVar1 * 0x220 + 0x1108 + param_2);
        for (iVar3 = 0x88; iVar3 != 0; iVar3 = iVar3 + -1) {
          *puVar4 = *param_1;
          param_1 = param_1 + 1;
          puVar4 = puVar4 + 1;
        }
        FUN_0053b500(iVar1);
        return 1;
      }
      iVar1 = iVar1 + 1;
      psVar2 = psVar2 + 0x110;
    } while (iVar1 < 4);
  }
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
