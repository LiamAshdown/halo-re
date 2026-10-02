// ui_controls_options_reload_profile  (Ghidra: FUN_004a0b50, renamed)
// renamed from FUN_004a0b50 in the naming pass
// address 0x4a0b50, size 109 bytes, callers=0 in this build
// name confidence: 0.35   rewrite confidence: 0.7
// evidence: functions.md: "Reloads the current player profile to apply any pending option
// changes, then clears the pending-changes flag." Same "cond & address-looking-literal" decompile
// oddity as FUN_004a0050.c; implemented per the same reasoning (selected_saved_item's low nibble
// nonzero -- i.e. NOT a profile -- gates the reload), not a literal transcription of the raw AND.
// register convention: none (void).

// Phase-4 s2 review: the gate is the sbb/not/and select of the working copy buffer at 0x00714e80
// (a record, not a pointer): the profile is reloaded only when the low nibble of
// selected_saved_item is 0; player_profile_load gets local player 0 in EAX. The earlier rewrite had the test inverted and
// read 0x00714e80 as a pointer.
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include <string.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int32_t selected_saved_item;             // 0x00714e7c
extern int32_t profile_slot_lookup_cache_00692ac8; // 0x00692ac8, TYPES-GAP
extern int32_t saved_player_profile_slots_handle;            // 0x00714dd4
extern uint8_t profile_globals_block[0x60a4];    // 0x00712dd8, saved profile slot 0 first

extern void player_profile_load(int16_t player_index, void *source_profile, int32_t profile_id); // 0x495970

uint8_t ui_controls_options_reload_profile(void)
{
    profile_slot_lookup_cache_00692ac8 = -1;
    if ((selected_saved_item & 0xf) == 0) { // a profile is being edited: reload it from its slot
        if (saved_player_profile_slots_handle != -1) {
            uint8_t profile_copy[0x1ffc];

            memcpy(profile_copy, profile_globals_block, sizeof(profile_copy));
            player_profile_load(0, profile_copy, saved_player_profile_slots_handle); // EAX 0, EDX copy, stack profile
        }
        selected_saved_item = -1;
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4a0b50):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 FUN_004a0b50(void)

{
  int iVar1;
  undefined4 *puVar2;
  undefined4 *puVar3;
  undefined4 local_2008 [2047];
  undefined4 uStack_c;

  uStack_c = 0x4a0b60;
  DAT_00692ac8 = 0xffffffff;
  if ((~-(uint)((_DAT_00714e7c & 0xf) != 0) & 0x714e80) != 0) {
    if (DAT_00714dd4 != -1) {
      puVar2 = &DAT_00712dd8;
      puVar3 = local_2008;
      for (iVar1 = 0x7ff; iVar1 != 0; iVar1 = iVar1 + -1) {
        *puVar3 = *puVar2;
        puVar2 = puVar2 + 1;
        puVar3 = puVar3 + 1;
      }
      player_profile_load(DAT_00714dd4);
    }
    _DAT_00714e7c = 0xffffffff;
  }
  return 1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
