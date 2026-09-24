// ui_profile_list_apply_selection_for_player  (Ghidra: FUN_0049e090, renamed)
// renamed from FUN_0049e090 in the naming pass
// address 0x49e090, size 212 bytes, callers=0 in this build
// name confidence: 0.35   rewrite confidence: 0.25
// evidence: functions.md: "Validates and loads the profile selected in a per-player (e.g.
// split-screen) profile-list widget, recording a help-text prompt for that player slot if no
// profile has been created yet." Locates the first spinner_list child of `widget` itself (rather
// than reading widget->parent directly, unlike the sibling FUN_0049dfc0). This function's own
// `(&quit_confirm_error_string_index)[player*3]` / `[player*6]` indexing is new evidence for
// types/interface.h's open question on whether 0x00718fac is a per-player-slot triple (it cites
// this exact address as one of two data points); reusing the already-established scalar externs
// via pointer arithmetic keeps this file link-consistent with the five files that already treat
// them as scalars while still preserving the per-slot addressing this function actually performs.
// register convention: cdecl, both recognized stack parameters (widget, a per-player event/context
// record whose offset +2 gives the player index -- the same shape as ui_input_event).
// UNSURE: player_profile_load's arguments are inferred the same way as the sibling
// FUN_0049dfc0.c (source_profile is the buffer player_profile_get just filled, profile_id is the
// validated entry id).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

extern int16_t quit_confirm_error_string_index; // 0x00718fac
extern int16_t quit_confirm_error_unknown_ae;   // 0x00718fae
extern uint8_t quit_confirm_error_modal;        // 0x00718fb0
extern uint8_t quit_confirm_error_is_error;     // 0x00718fb1

extern uint8_t player_profile_get(int32_t slot, void *out_profile); // 0x53a770; blam-cc: ECX -> out_profile
extern void player_profile_load(int16_t player_index, void *source_profile, int32_t profile_id); // 0x495970
extern void widget_play_sound_effect(int16_t effect_id); // 0x498e90

// Finds the first spinner_list child of `widget`, validates the profile entry at its current
// selection, and either loads it or (if unpopulated) arms the per-player-slot help prompt.
uint32_t ui_profile_list_apply_selection_for_player(widget_instance *widget, int16_t *context)
{
    int16_t requested_player = context[1]; // offset +2, matching ui_input_event::controller_index
    widget_instance *list_widget;
    int32_t *slot_ids;
    int32_t entry_id;
    int32_t player_slot;
    uint8_t profile_data[0x1ffc];

    for (list_widget = widget->first_child;
         list_widget != (widget_instance *)0 && list_widget->widget_type != 2;
         list_widget = list_widget->next_sibling) {
    }

    slot_ids = (int32_t *)list_widget->list_items;
    entry_id = slot_ids[list_widget->selection_index];

    if (entry_id < 0) {
        if (entry_id != -1 && player_profile_get(entry_id, profile_data) != 0) {
            player_profile_load((int16_t)entry_id, profile_data, entry_id);
            return 1;
        }
        return 0;
    }

    player_slot = (requested_player == -1) ? 0 : (int32_t)requested_player;
    if ((&quit_confirm_error_string_index)[player_slot * 3] == -1) {
        (&quit_confirm_error_string_index)[player_slot * 3] = 0x1f;
        (&quit_confirm_error_unknown_ae)[player_slot * 3] = requested_player;
        (&quit_confirm_error_modal)[player_slot * 6] = 1;
        (&quit_confirm_error_is_error)[player_slot * 6] = 0;
    }
    widget_play_sound_effect(0); // UNSURE: effect id read from an unresolved register here
    return 0;
}

#if 0
Original Ghidra decompilation (0x49e090):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

undefined4 FUN_0049e090(int param_1,int param_2)

{
  short sVar1;
  int iVar2;
  int iVar3;
  char cVar4;
  int iVar5;

  sVar1 = *(short *)(param_2 + 2);
  for (iVar5 = *(int *)(param_1 + 0x34); (iVar5 != 0 && (*(short *)(iVar5 + 0xe) != 2));
      iVar5 = *(int *)(iVar5 + 0x2c)) {
  }
  iVar2 = *(int *)(iVar5 + 0x44);
  iVar3 = *(int *)(iVar2 + *(short *)(iVar5 + 0x40) * 4);
  if (iVar3 < 0) {
    if ((iVar3 != -1) && (cVar4 = player_profile_get(iVar3), cVar4 != '\0')) {
      player_profile_load(*(undefined4 *)(iVar2 + *(short *)(iVar5 + 0x40) * 4));
      return 1;
    }
  }
  else {
    if (sVar1 == -1) {
      iVar5 = 0;
    }
    else {
      iVar5 = (int)sVar1;
    }
    if ((&DAT_00718fac)[iVar5 * 3] == -1) {
      (&DAT_00718fac)[iVar5 * 3] = 0x1f;
      (&DAT_00718fae)[iVar5 * 3] = sVar1;
      (&DAT_00718fb0)[iVar5 * 6] = 1;
      (&DAT_00718fb1)[iVar5 * 6] = 0;
    }
    widget_play_sound_effect();
  }
  return 0;
}
#endif
