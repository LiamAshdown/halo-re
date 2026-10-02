// user_profile_signin_state_is_valid  (Ghidra: FUN_00551620; renamed, no established name)
// address 0x551620, size 35 bytes
// name confidence: 0.25   rewrite confidence: 0.4
// evidence: out/phase4/game_functions.md (no summary beyond the default); the shape (a non-null
//   pointer whose dword at +4 is one of three small enum-like values 0/1/2) matches a
//   sign-in-state gate used elsewhere in this module's save-game path; the exact struct is not
//   otherwise named anywhere in this batch's evidence.
// register convention: none -- no stack parameters, no register arguments recovered.
// UNSURE: the identity of the struct pointed to by DAT_00721f24 and the meaning of its +4
//   field's three accepted values (0, 1, 2); kept as a raw offset access.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void *global_sound_effect_object; // 0x00721f24, UNSURE identity, see header note

// Returns 1 if global_sound_effect_object is non-NULL and its dword at +4 is 0, 1, or 2.
uint32_t user_profile_signin_state_is_valid(void)
{
    if (global_sound_effect_object != 0) {
        int32_t state = *(int32_t *)((uint8_t *)global_sound_effect_object + 4);
        if (state == 0 || state == 1 || state == 2) {
            return 1;
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x551620), from tools/pack.py 0x551620:

undefined4 FUN_00551620(void)

{
  int iVar1;

  if ((DAT_00721f24 != 0) &&
     (((iVar1 = *(int *)(DAT_00721f24 + 4), iVar1 == 0 || (iVar1 == 1)) || (iVar1 == 2)))) {
    return 1;
  }
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
