// ui_build_level_select_list_coop  (Ghidra: FUN_0049cc80, renamed)
// renamed from FUN_0049cc80 in the naming pass
// address 0x49cc80, size 318 bytes, sole caller is ui_build_level_select_list (this session)
// name confidence: 0.4   rewrite confidence: 0.25
// evidence: functions.md: "Builds the same level-selection list as ui_build_level_select_list,
// but treats a level as unlocked if either player's (single or co-op) progress buffer has
// unlocked it." Copies two 0x1ffc-byte profile-shaped records (0x00712dd8 and 0x00714ddc) through
// player_profile_scan_campaign_progress, then ORs together the same per-level flag byte from both.
// register convention: cdecl; Ghidra recognizes only the first parameter (widget), but the sole
// caller passes three -- kept 3-parameter for signature agreement with that call site (the extra
// two are genuinely unused here).
// UNSURE / TYPES-GAP: same stack-overlay reasoning as ui_build_level_select_list.c: Ghidra's
// declared local_4000[71]/local_2004[71] arrays are each immediately followed by a same-sized
// "stack" byte array with no writes ever shown to either, so both pairs are modeled as one
// contiguous copy per profile with the per-level flag byte recovered at offset 0x11c into each.
// The unlock-detection condition also ORs in two extra per-iteration comparisons
// (`iVar2 == (short)local_4008 + 1` and `iVar2 == sStack_4006 + 1`) whose source locals
// (local_4008/sStack_4006) are never written anywhere in this decompile; preserved as reads of
// whatever value the stack happened to hold at that position (effectively uninitialized reads),
// consistent with "no invented behaviour" -- modeled as TYPES-GAP scratch fields at the same
// stack-frame-relative spot inside each profile copy.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include <string.h>

extern level_select_entry level_select_entries[10];  // 0x00719018
extern uint8_t profile_globals_block[0x60a4];                     // 0x00712dd8
extern uint8_t coop_profile_globals_block_00714ddc[0x1ffc];       // 0x00714ddc, TYPES-GAP
extern campaign_level_entry known_campaign_levels_00692acc[10]; // 0x00692acc, TYPES-GAP
extern int16_t known_solo_level_index_00712f00;                    // 0x00712f00, TYPES-GAP

extern void player_profile_scan_campaign_progress(void); // 0x539e00, foreign (profile module), UNSURE

// Populates the level-selection widget's list from BOTH players' progress buffers, marking a
// level's flag bits set if either buffer's per-level byte has them set.
void ui_build_level_select_list_coop(widget_instance *widget, void *param_2, void *param_3)
{
    uint8_t profile_copy_a[0x2000]; // see file header re the local_4000/abStack_3ee2 overlay
    uint8_t profile_copy_b[0x2000]; // see file header re the local_2004/abStack_1ee6 overlay
    int32_t i;

    (void)param_2;
    (void)param_3;

    memset(level_select_entries, 0, sizeof(level_select_entries));

    memcpy(profile_copy_a, profile_globals_block,
           sizeof(profile_copy_a) < sizeof(profile_globals_block) ? sizeof(profile_copy_a)
                                                                    : sizeof(profile_globals_block));
    player_profile_scan_campaign_progress();
    memcpy(profile_copy_b, coop_profile_globals_block_00714ddc,
           sizeof(profile_copy_b) < sizeof(coop_profile_globals_block_00714ddc)
               ? sizeof(profile_copy_b)
               : sizeof(coop_profile_globals_block_00714ddc));
    player_profile_scan_campaign_progress();

    for (i = 0; i < 10; i++) {
        uint8_t flag_a = profile_copy_a[0x11c + i];
        uint8_t flag_b = profile_copy_b[0x11c + i];

        level_select_entries[i].path = known_campaign_levels_00692acc[i].path;
        if (flag_a != 0 || flag_b != 0 || i == 0) {
            uint32_t flags = (uint32_t)(uint8_t)(flag_b | flag_a);

            level_select_entries[i].flag_bit1 = (uint8_t)((flags >> 1) & 1);
            level_select_entries[i].valid = 1;
            level_select_entries[i].flag_bit2 = (uint8_t)((flags >> 2) & 1);
            level_select_entries[i].flag_bit3 = (uint8_t)((flags >> 3) & 1);
        }
    }

    widget->list_items = level_select_entries;
    widget->item_count = 10;
    if (known_solo_level_index_00712f00 < 0) {
        widget->selection_index = 0;
    } else if (known_solo_level_index_00712f00 > 9) {
        widget->selection_index = 9;
    } else {
        widget->selection_index = (int16_t)known_solo_level_index_00712f00;
    }
}

#if 0
Original Ghidra decompilation (0x49cc80):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

void FUN_0049cc80(int param_1)

{
  byte bVar1;
  int iVar2;
  uint uVar3;
  undefined4 *puVar4;
  undefined4 *puVar5;
  undefined4 local_4008;
  short sStack_4006;
  undefined4 local_4000 [71];
  byte abStack_3ee2 [7902];
  undefined4 local_2004 [71];
  byte abStack_1ee6 [7898];
  undefined4 uStack_c;

  uStack_c = 0x49cc90;
  puVar4 = &DAT_00719018;
  for (iVar2 = 0x14; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar4 = 0;
    puVar4 = puVar4 + 1;
  }
  puVar4 = &DAT_00712dd8;
  puVar5 = local_4000;
  for (iVar2 = 0x7ff; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar5 = *puVar4;
    puVar4 = puVar4 + 1;
    puVar5 = puVar5 + 1;
  }
  FUN_00539e00();
  puVar4 = (undefined4 *)&DAT_00714ddc;
  puVar5 = local_2004;
  for (iVar2 = 0x7ff; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar5 = *puVar4;
    puVar4 = puVar4 + 1;
    puVar5 = puVar5 + 1;
  }
  FUN_00539e00();
  iVar2 = 0;
  do {
    bVar1 = abStack_3ee2[iVar2];
    (&DAT_00719018)[iVar2 * 2] = (&PTR_s_levels_a10_a10_00692acc)[iVar2];
    if ((((bVar1 != 0) || (iVar2 == (short)local_4008 + 1)) || (abStack_1ee6[iVar2] != 0)) ||
       ((iVar2 == sStack_4006 + 1 || (iVar2 == 0)))) {
      uVar3 = (uint)(char)(abStack_1ee6[iVar2] | bVar1);
      *(byte *)((int)&DAT_0071901c + iVar2 * 8 + 1) = (byte)(uVar3 >> 1) & 1;
      *(undefined1 *)(&DAT_0071901c + iVar2 * 2) = 1;
      *(byte *)((int)&DAT_0071901c + iVar2 * 8 + 2) = (byte)(uVar3 >> 2) & 1;
      *(byte *)((int)&DAT_0071901c + iVar2 * 8 + 3) = (byte)(uVar3 >> 3) & 1;
    }
    iVar2 = iVar2 + 1;
  } while (iVar2 < 10);
  *(undefined4 **)(param_1 + 0x44) = &DAT_00719018;
  *(undefined2 *)(param_1 + 0x48) = 10;
  if (DAT_00712f00 < 0) {
    *(undefined2 *)(param_1 + 0x40) = 0;
    return;
  }
  if (9 < DAT_00712f00) {
    *(undefined2 *)(param_1 + 0x40) = 9;
    return;
  }
  *(short *)(param_1 + 0x40) = DAT_00712f00;
  return;
}
#endif
