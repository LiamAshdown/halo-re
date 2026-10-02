// ui_network_name_fields_reset  (Ghidra: FUN_004a49c0, renamed)
// renamed from FUN_004a49c0 in the naming pass
// address 0x4a49c0, size 99 bytes, callers=0 in this build
// name confidence: 0.3   rewrite confidence: 0.25
// evidence: functions.md: "Resets the network name/team text-entry globals to blank, guarded by
// the current-profile check used elsewhere." Same profile-copy-then-ignore pattern as
// ui_build_profile_list.c's -1 branch: the copy is made but its result (`local_1046`, an
// uninitialized wide buffer here) is used regardless via wcsncpy, matching the original exactly.
// register convention: none (void).

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include <string.h>

extern int32_t saved_player_profile_slots_handle;         // 0x00714dd4
extern uint8_t profile_globals_block[0x60a4];  // 0x00712dd8
extern uint16_t network_host_name_field_00719238[32]; // 0x00719238, TYPES-GAP
extern uint8_t network_host_name_flag_00719276;         // 0x00719276, TYPES-GAP
extern uint16_t network_host_subname_007191f0[9];        // 0x007191f0

extern void player_profile_set_default_server_options(void); // 0x53a150, foreign (profile module), UNSURE

uint32_t ui_network_name_fields_reset(void)
{
    uint16_t unused_name_source[2077]; // Ghidra's local_1046, never written before use either

    if (saved_player_profile_slots_handle == -1) {
        player_profile_set_default_server_options();
    } else {
        uint8_t profile_copy[0x2000];

        memcpy(profile_copy, profile_globals_block, sizeof(profile_copy));
    }

    wcsncpy((wchar_t *)network_host_name_field_00719238, (const wchar_t *)unused_name_source, 0x1f);
    network_host_name_flag_00719276 = 0;
    network_host_subname_007191f0[0] = 0;
    return 1;
}

#if 0
Original Ghidra decompilation (0x4a49c0):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 FUN_004a49c0(void)

{
  int iVar1;
  undefined4 *puVar2;
  undefined4 *puVar3;
  undefined4 local_2008 [1008];
  wchar_t local_1046 [2077];
  undefined4 uStack_c;

  uStack_c = 0x4a49d0;
  if (DAT_00714dd4 == -1) {
    FUN_0053a150();
  }
  else {
    puVar2 = &DAT_00712dd8;
    puVar3 = local_2008;
    for (iVar1 = 0x7ff; iVar1 != 0; iVar1 = iVar1 + -1) {
      *puVar3 = *puVar2;
      puVar2 = puVar2 + 1;
      puVar3 = puVar3 + 1;
    }
  }
  _wcsncpy((wchar_t *)&DAT_00719238,local_1046,0x1f);
  _DAT_00719276 = 0;
  _DAT_007191f0 = 0;
  return 1;
}
#endif
