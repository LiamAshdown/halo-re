// ui_level_select_confirm_choice  (Ghidra: FUN_0049ce00, renamed)
// renamed from FUN_0049ce00 in the naming pass
// address 0x49ce00, size 409 bytes, callers=0 in this build (dead code, part of the same
// unreachable level-select cluster as ui_build_level_select_list/FUN_0049cc80)
// name confidence: 0.3   rewrite confidence: 0.85
// evidence: functions.md: "Validates the level currently selected in the level-list widget
// against the active player's unlock progress and, if unlocked, records its scenario path as the
// level to start." Shares player_profile_load.c's already-established saved_game_get_directory_by_handle/
// saved_game_last_profile_clear cached-profile-slot bookkeeping verbatim.
// register convention: cdecl, the one recognized stack parameter (widget).
// UNSURE / TYPES-GAP: same local_2008/acStack_1eea stack-overlay reasoning as
// ui_build_level_select_list.c (one profile-shaped copy with the per-level unlock byte at
// offset 0x11c). local_2014 (compared as `local_2010 == local_2014 + 1`) is never written in this
// decompile; preserved as reading whatever the stack held there (effectively always false in
// practice), not invented as some other condition. Offset 0x3c on the widget is, as in the sibling
// level-select functions, a plain int16 selection snapshot rather than any named widget_instance
// field. DAT_00719754's high byte, DAT_00719778, DAT_00719779[] and DAT_00719878 are new globals
// not covered by types/interface.h.
// reconciled: R55 per-level progress byte is profile +0x11e, not +0x11c (+0x11c is the flags word): 0x49ce00 copies to esp+0x20 and reads [esp+reg+0x13e] (0x49ce8f, 0x49cef3)

// REWRITTEN (from objdump 0x49ce00..0x49cf98): the level id comes from ui_lists[ui_list_current] at the
//   widget's +0x3c index (-1 when out of range). One local player: profile block 0 (0x712dd8, 0x1ffc bytes) is
//   copied and player_profile_scan_campaign_progress(ECX = &type, EDX = copy, ESI = &last_level) run; the level
//   counts as unlocked when its progress byte (copy +0x11e + level) is set, it is last_level + 1, or it is level
//   0. Then the cached profile-name bookkeeping (as in player_profile_load) runs. Two local players: the same
//   test for each profile block in turn (stride 0x2004), without the bookkeeping. Unlocked: the level's path
//   (campaign_level_entry table 0x692acc, 4 bytes per entry) goes to 0x719779 (255 chars), 0x719757 = 0,
//   0x719878 = 0, 0x719778 = 1, 0x719739 = 0, returns 1. Otherwise (or any other player count) it plays sound
//   effect 4 and returns 0. The draft called the scan with no arguments (EDX was garbage: the crash).
#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include <string.h>
#include "objects.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int32_t ui_list_current;                        // 0x00692c04
extern growable_array ui_lists[3];                      // 0x006b3830
extern int16_t local_player_count;                      // 0x006894b8
extern uint8_t profile_globals_block[0x60a4];           // 0x00712dd8
extern int32_t saved_player_profile_slots_handle;                   // 0x00714dd4
extern int32_t cached_profile_slot;                     // 0x0068e66c
extern char last_profile_name[];                        // 0x00718e80
extern campaign_level_entry known_campaign_levels_00692acc[10]; // 0x00692acc
extern uint8_t split_screen_quit_prompt_armed; // 0x00719757
extern char unknown_00719779[0x100];        // 0x00719779
extern uint8_t selected_level_active_00719878;          // 0x00719878
extern uint8_t selected_level_pending_00719778;         // 0x00719778
extern uint8_t network_wait_flag_00719739;              // 0x00719739

extern void widget_play_sound_effect(int16_t effect_id); // 0x498e90, blam-cc: AX -> effect_id
extern void player_profile_scan_campaign_progress(int16_t *out_type, saved_player_profile *profile, int16_t *out_level);
    // 0x539e00, blam-cc: ECX -> out_type, EDX -> profile, ESI -> out_level
extern uint8_t saved_game_get_directory_by_handle(int32_t handle, char *out_directory);
    // 0x53d080, blam-cc: EAX -> handle, ESI -> out_directory
extern void saved_game_last_profile_clear(char *name);  // 0x53d220

// Copies local player `player`'s profile block and tests whether `level_id` is available to it.
static uint8_t level_unlocked_for(int16_t player, int32_t level_id)
{
    uint8_t profile_copy[0x1ffc];
    int16_t type;
    int16_t last_level;

    memcpy(profile_copy, profile_globals_block + player * 0x2004, sizeof(profile_copy));
    player_profile_scan_campaign_progress(&type, (saved_player_profile *)profile_copy, &last_level);
    return profile_copy[0x11e + level_id] != 0 || level_id == last_level + 1 || level_id == 0;
}

uint8_t ui_level_select_confirm_choice(widget_instance *widget)
{
    int16_t list_index = *(int16_t *)&((struct widget_instance *)widget)->text; // UNSURE: raw offset, see header
    int32_t level_id = -1;
    uint8_t unlocked = 0;
    growable_array *list = &ui_lists[ui_list_current];
    int16_t player;

    if (list_index >= 0 && list_index < list->count) {
        level_id = ((ui_list_item *)list->data)[list_index].id;
    }
    if (local_player_count == 1) {
        unlocked = level_unlocked_for(0, level_id);
        if (cached_profile_slot != saved_player_profile_slots_handle) {
            if (saved_player_profile_slots_handle != -1) {
                saved_game_get_directory_by_handle(saved_player_profile_slots_handle, last_profile_name);
            }
            cached_profile_slot = saved_player_profile_slots_handle;
        }
        if (last_profile_name[0] != 0) {
            saved_game_last_profile_clear(last_profile_name);
        }
    } else if (local_player_count == 2) {
        for (player = 0; player <= 1 && !unlocked; player++) {
            unlocked = level_unlocked_for(player, level_id);
        }
    }
    if (unlocked != 1) {
        widget_play_sound_effect(4);
        return unlocked;
    }
    split_screen_quit_prompt_armed = 0;
    strncpy(unknown_00719779, known_campaign_levels_00692acc[level_id].path, 0xff);
    selected_level_active_00719878 = 0;
    selected_level_pending_00719778 = 1;
    network_wait_flag_00719739 = 0;
    return 1;
}

#if 0
Original Ghidra decompilation (0x49ce00):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

char FUN_0049ce00(int param_1)

{
  int iVar1;
  short sVar2;
  undefined4 *puVar3;
  undefined4 *puVar4;
  char local_2015;
  short local_2014;
  int local_2010;
  undefined4 local_2008 [71];
  char acStack_1eea [7902];
  undefined4 uStack_c;

  uStack_c = 0x49ce10;
  iVar1 = (int)*(short *)(param_1 + 0x3c);
  local_2015 = '\0';
  local_2010 = -1;
  if ((-1 < iVar1) && (iVar1 < (int)(&DAT_006b3834)[DAT_00692c04 * 3])) {
    local_2010 = *(int *)((&DAT_006b3838)[DAT_00692c04 * 3] + 8 + iVar1 * 0x10);
  }
  if (DAT_006894b8 == 1) {
    puVar3 = &DAT_00712dd8;
    puVar4 = local_2008;
    for (iVar1 = 0x7ff; iVar1 != 0; iVar1 = iVar1 + -1) {
      *puVar4 = *puVar3;
      puVar3 = puVar3 + 1;
      puVar4 = puVar4 + 1;
    }
    FUN_00539e00();
    if (((acStack_1eea[local_2010] != '\0') || (local_2010 == local_2014 + 1)) || (local_2010 == 0))
    {
      local_2015 = '\x01';
    }
    if (DAT_0068e66c != DAT_00714dd4) {
      if (DAT_00714dd4 != -1) {
        FUN_0053d080();
      }
      DAT_0068e66c = DAT_00714dd4;
    }
    if (DAT_00718e80 != '\0') {
      saved_game_last_profile_clear();
    }
    if (local_2015 == '\x01') {
LAB_0049cf58:
      DAT_00719754._3_1_ = 0;
      _strncpy(&DAT_00719779,(&PTR_s_levels_a10_a10_00692acc)[local_2010],0xff);
      DAT_00719878 = 0;
      DAT_00719778 = 1;
      DAT_00719739 = 0;
      return local_2015;
    }
  }
  else if (DAT_006894b8 == 2) {
    sVar2 = 0;
    do {
      puVar3 = &DAT_00712dd8 + sVar2 * 0x801;
      puVar4 = local_2008;
      for (iVar1 = 0x7ff; iVar1 != 0; iVar1 = iVar1 + -1) {
        *puVar4 = *puVar3;
        puVar3 = puVar3 + 1;
        puVar4 = puVar4 + 1;
      }
      FUN_00539e00();
      if (((acStack_1eea[local_2010] != '\0') || (local_2010 == local_2014 + 1)) ||
         (local_2010 == 0)) {
        local_2015 = '\x01';
        goto LAB_0049cf58;
      }
      sVar2 = sVar2 + 1;
    } while (sVar2 < 2);
  }
  widget_play_sound_effect();
  return local_2015;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
