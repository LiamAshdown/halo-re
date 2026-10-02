// ui_profile_list_apply_selection  (Ghidra: FUN_0049dfc0, renamed)
// renamed from FUN_0049dfc0 in the naming pass
// address 0x49dfc0, size 206 bytes, callers=0 in this build
// name confidence: 0.35   rewrite confidence: 0.25
// evidence: functions.md: "Applies the currently selected entry of the profile list by validating
// it and loading the associated saved player profile." Reads through `widget->parent`'s own list
// (this widget is a child control, e.g. an activation button, of the actual profile-list widget).
// Reuses player_profile_get, player_profile_find_index_by_id and player_profile_load's already-
// established signatures, and ui_check_for_pause_game.c's ui_player_help_string_* names.
// register convention: matches ui_event_function (widget, event, out_handled); event is unused.
// UNSURE: the middle branch (entry id >= 0, distinct from exactly -1) arms the "help string"
// dialog and just plays a sound rather than loading anything -- preserved exactly as decompiled
// even though it reads oddly next to the id < -1 branch that DOES attempt to load. Both
// player_profile_find_index_by_id's call and its return value's use as player_profile_load's
// player_index argument are inferred from context (Ghidra drops all of player_profile_load's
// arguments at this call site).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int16_t quit_confirm_error_string_index; // 0x00718fac
extern int16_t quit_confirm_error_unknown_ae; // 0x00718fae
extern uint8_t quit_confirm_error_modal;   // 0x00718fb0
extern uint8_t quit_confirm_error_is_error; // 0x00718fb1

extern int16_t player_profile_find_index_by_id(int16_t id); // 0x4954f0
extern void player_profile_load(int16_t player_index, void *source_profile, int32_t profile_id); // 0x495970
extern void widget_play_sound_effect(int16_t effect_id); // 0x498e90
extern uint8_t player_profile_get(int32_t slot, void *out_profile); // 0x53a770; blam-cc: ECX -> out_profile

// blam-cc: matches ui_event_function (widget, event, out_handled)
uint8_t ui_profile_list_apply_selection(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    widget_instance *list_widget = widget->parent;
    int32_t *slot_ids = (int32_t *)list_widget->list_items;
    int32_t entry_id = slot_ids[list_widget->selection_index];
    uint8_t profile_data[0x1ffc];

    (void)event;

    if (entry_id == -1) {
        widget_play_sound_effect(0); // UNSURE: effect id read from an unresolved register here
        return 0;
    }

    if (entry_id > -1) {
        if (quit_confirm_error_string_index == -1) {
            quit_confirm_error_string_index = 0x1f;
            quit_confirm_error_unknown_ae = 0xffff;
            quit_confirm_error_modal = 1;
            quit_confirm_error_is_error = 0;
        }
        widget_play_sound_effect(0); // UNSURE: effect id read from an unresolved register here
        *out_handled = 1;
        return 0;
    }

    if (player_profile_get(entry_id, profile_data) != 0) {
        int32_t player_index = player_profile_find_index_by_id((int16_t)entry_id);

        player_profile_load((int16_t)player_index, profile_data, entry_id);
        return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x49dfc0):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

undefined4 FUN_0049dfc0(int param_1,undefined4 param_2,undefined1 *param_3)

{
  int iVar1;
  int iVar2;
  int iVar3;
  char cVar4;

  iVar1 = *(int *)(param_1 + 0x34);
  iVar2 = *(int *)(iVar1 + 0x44);
  iVar3 = *(int *)(iVar2 + *(short *)(iVar1 + 0x40) * 4);
  if (iVar3 == -1) {
    widget_play_sound_effect();
  }
  else {
    if (-1 < iVar3) {
      if (DAT_00718fac == -1) {
        DAT_00718fac = 0x1f;
        DAT_00718fae = 0xffff;
        DAT_00718fb0 = 1;
        DAT_00718fb1 = 0;
      }
      widget_play_sound_effect();
      *param_3 = 1;
      return 0;
    }
    cVar4 = player_profile_get(iVar3);
    if (cVar4 != '\0') {
      player_profile_find_index_by_id(*(undefined4 *)(iVar2 + *(short *)(iVar1 + 0x40) * 4));
      player_profile_load();
      return 1;
    }
  }
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
