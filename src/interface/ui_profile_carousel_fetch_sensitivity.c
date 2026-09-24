// ui_profile_carousel_fetch_sensitivity  (Ghidra: FUN_004a6b00, renamed)
// address 0x4a6b00, size 107 bytes
// name confidence: 0.3 (chosen)   rewrite confidence: 0.4
// evidence: types/interface.h widget_instance (controller_index as a carousel slot index,
// matching ui_profile_carousel_fetch_name.c); the "+0x58" field is widget_instance's
// background_bitmap_frame, reused by spinner_list-style value widgets to hold a live numeric
// value rather than an animation frame (same reuse seen throughout this list-widget family).
// UNSURE: the exact profile-record field read (Ghidra's own frame-slot naming puts it about
// 0x11a bytes into the record, close to but not exactly player_control_settings::unknown_008 at
// profile+0x134; not confirmed independently) is kept as a raw offset with no field name.
// register convention: widget as the recognized parameter (param_1).
// reconciled: R56 0x00712dd8 uint8_t saved_profile_records[3][0x2004] -> saved_games.h saved_player_profile_slot saved_player_profile_slots[k_maximum_local_player_profiles] (one 0x2004-byte slot; a second would overlap 0x00714dde); same bytes copied/read

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include <string.h>

extern saved_player_profile_slot saved_player_profile_slots[k_maximum_local_player_profiles]; // 0x00712dd8, saved_games.h

// Reads a raw int16 value out of the carousel-slot profile record and clamps it into 0..0x11
// before storing it in the widget's live-value field (background_bitmap_frame, offset 0x58).
void ui_profile_carousel_fetch_sensitivity(widget_instance *widget)
{
    uint8_t profile_record[0x1ffc]; // one dword short of the full 0x2004 byte record, as Ghidra shows
    int16_t raw_value; // UNSURE: exact profile-record field, see header note

    memcpy(profile_record, &saved_player_profile_slots[widget->controller_index].profile, sizeof(profile_record));
    raw_value = *(int16_t *)(profile_record + 0x11a); // UNSURE offset

    if (raw_value < 0) {
        widget->background_bitmap_frame = 0;
        return;
    }
    if (raw_value > 0x11) {
        widget->background_bitmap_frame = 0x11;
        return;
    }
    widget->background_bitmap_frame = raw_value;
}

#if 0
Original Ghidra decompilation (0x4a6b00):

void FUN_004a6b00(int param_1)

{
  int iVar1;
  undefined4 *puVar2;
  undefined4 *puVar3;
  undefined4 local_2008 [70];
  short local_1eee;

  puVar2 = &DAT_00712dd8 + *(short *)(param_1 + 8) * 0x801;
  puVar3 = local_2008;
  for (iVar1 = 0x7ff; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar3 = *puVar2;
    puVar2 = puVar2 + 1;
    puVar3 = puVar3 + 1;
  }
  if (local_1eee < 0) {
    *(undefined2 *)(param_1 + 0x58) = 0;
    return;
  }
  if (0x11 < local_1eee) {
    *(undefined2 *)(param_1 + 0x58) = 0x11;
    return;
  }
  *(short *)(param_1 + 0x58) = local_1eee;
  return;
}
#endif
