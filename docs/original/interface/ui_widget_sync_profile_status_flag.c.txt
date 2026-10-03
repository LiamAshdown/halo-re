// ui_widget_sync_profile_status_flag  (Ghidra: FUN_004a7360, renamed)
// address 0x4a7360, size 106 bytes
// name confidence: 0.3 (chosen)   rewrite confidence: 0.55
// evidence: types/interface.h widget_instance (parent, controller_index, selection_index) and
// player_control_settings (profile+0x12f -> ((uint8_t *)&profile_globals_block[slot])[0x12f], the same
// "unknown_858" field the header already quotes as "profile+0x12f"); profile_slot_id
// (0x00714dde) per the header's own comment.
// UNSURE: Ghidra's search loop is bounded `sVar6 < 1`, so it only ever inspects slot 0 no
// matter how many profile slots exist; transcribed as-is rather than "fixed" into a real scan.
// register convention: widget as the recognized parameter (param_1).
// reconciled: R56 0x00712dd8 uint8_t saved_profile_records[3][0x2004] -> saved_games.h saved_player_profile_slot profile_globals_block[k_maximum_local_player_profiles] (one 0x2004-byte slot; a second would overlap 0x00714dde); same bytes copied/read

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
extern int16_t profile_slot_id[];                // 0x00714dde
extern saved_player_profile_slot profile_globals_block[k_maximum_local_player_profiles]; // 0x00712dd8, saved_games.h

// Walks up to the root ancestor widget, and if profile slot 0's id matches the root's
// controller_index, mirrors that profile's status byte (profile+0x12f) into this widget's
// selection_index as a 0/1 flag.
void ui_widget_sync_profile_status_flag(widget_instance *widget)
{
    widget_instance *root;
    int32_t slot; // sVar4, -1 when profile_slot_id[0] does not match
    uint8_t status;

    root = widget;
    while (root->parent != 0) {
        root = root->parent;
    }

    slot = (profile_slot_id[0] == root->controller_index) ? 0 : -1; // Ghidra's loop only checks index 0
    if (slot == -1) {
        slot = 0;
    }

    status = 0;
    if (slot != -1) {
        status = ((uint8_t *)&profile_globals_block[slot])[0x12f];
    }
    widget->selection_index = (status != 0);
}

#if 0
Original Ghidra decompilation (0x4a7360):

void FUN_004a7360(int param_1)

{
  int iVar1;
  int iVar2;
  int iVar3;
  short sVar4;
  char cVar5;
  short sVar6;

  iVar3 = *(int *)(param_1 + 0x30);
  iVar2 = param_1;
  while (iVar1 = iVar3, iVar1 != 0) {
    iVar2 = iVar1;
    iVar3 = *(int *)(iVar1 + 0x30);
  }
  sVar6 = 0;
  do {
    sVar4 = sVar6;
    if (*(short *)(&DAT_00714dde + sVar6 * 2) == *(short *)(iVar2 + 8)) break;
    sVar6 = sVar6 + 1;
    sVar4 = -1;
  } while (sVar6 < 1);
  if (sVar4 == -1) {
    sVar4 = 0;
  }
  cVar5 = '\0';
  if (sVar4 != -1) {
    cVar5 = (&DAT_00712f07)[sVar4 * 0x2004];
  }
  *(ushort *)(param_1 + 0x40) = (ushort)(cVar5 != '\0');
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
