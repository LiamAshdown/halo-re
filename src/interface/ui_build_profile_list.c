// ui_build_profile_list  (Ghidra: ui_build_profile_list, already named)
// address 0x49dd70, size 492 bytes, callers=0 in this build
// name confidence: 0.7   rewrite confidence: 0.3
// evidence: matches the given name; functions.md: "Builds the player-profile selection list:
// resets scroll bookkeeping, allocates the selection array, and records every valid saved profile,
// pre-selecting whichever profile is currently loaded." Reuses ui_list_add_entry's established
// signature (0x4a7ba0, already rewritten elsewhere in the module), default_profile_data /
// cached_profile_slot / last_profile_name (player_profile_subsystem_initialize.c), and
// player_profile_get's established (slot, out_profile) signature.
// register convention: cdecl, the one recognized stack parameter (widget).
// UNSURE: ui_list_add_entry's group_index (EAX) and is_default (CL) are not visible at this call
// site; group_index=1 matches the DAT_006b3838-based addressing this same pattern uses in
// ui_build_level_select_list.c, and is_default is modeled as always 0 since no boolean computing
// it is visible here (the widget's own selection_index, set separately below, is what actually
// tracks the current profile). The packed return value (CONCAT31 of either 0 or 0xffffff with a
// literal 1) is preserved numerically rather than collapsed to a plain `return 1`, since no caller
// exists in this session to confirm only the low byte matters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include <string.h>
#include "objects.h"
#include "units.h"
#include "fn_memory.h"
#include "fn_interface.h"

extern int32_t profile_slot_lookup_cache_00692ac8; // 0x00692ac8, TYPES-GAP
extern profile_carousel_slot profile_carousel_slots[3]; // 0x00873d60, reset to 0xff (0x1800 dwords)
extern growable_array ui_lists[3];                    // 0x006b3830
extern char last_profile_name[];                      // 0x00718e80
extern int32_t cached_profile_slot;                   // 0x0068e66c
extern int32_t ui_list_current;                       // 0x00692c04
extern uint8_t ui_list_has_default;                   // 0x007192f8
extern uint8_t default_profile_data[0x1ffc]; // 0x0071d280

extern heap *widget_memory_pool; // 0x006926c4

extern void saved_game_enumerate_by_type(uint16_t type, int32_t *out_handles, uint8_t builtin_only,
    uint16_t *capacity_and_count); // 0x53c4e0, stack (type, out, builtin_only), EBX &count
extern uint8_t saved_game_last_profile_read(char *name_buffer); // 0x53d2b0
extern int32_t saved_game_find_by_name(char *name, int32_t unknown); // 0x53d4a0
extern uint8_t player_profile_get(int32_t slot, void *out_profile); // 0x53a770; blam-cc: ECX -> out_profile


// Resets the shared UI list arrays, allocates a 100-slot profile-id buffer, enumerates saved
// profiles into it, and adds one list entry per occupied slot (skipping empty ones), while
// tracking which list index corresponds to the currently loaded profile.
uint32_t ui_build_profile_list(widget_instance *widget)
{
    int32_t *slot_ids;
    uint32_t high_bits = 0;

    profile_slot_lookup_cache_00692ac8 = -1;
    memset(profile_carousel_slots, 0xff, sizeof(profile_carousel_slots));

    slot_ids = (int32_t *)heap_reallocate(widget->list_items, 400, widget_memory_pool);
    widget->list_items = slot_ids;
    if (slot_ids != (int32_t *)0) {
        uint8_t profile_buffer[0x1ffc];
        int32_t matched_profile;
        int32_t i;

        {   // 0x49ddd8..0x49dddc: EBX = &count preset to 0x64
            uint32_t count = 0x64;
            saved_game_enumerate_by_type(0, (int32_t *)slot_ids, 0, (uint16_t *)&count);
        }
        widget->item_count = 100;

        ui_lists[0].element_size = 0x10;
        ui_lists[1].element_size = 0x10;
        ui_lists[2].element_size = 0x10;
        ui_lists[0].count = 0;
        ui_lists[1].count = 0;
        ui_lists[2].count = 0;
        ui_lists[0].data = (void *)0;
        ui_lists[1].data = (void *)0;
        ui_lists[2].data = (void *)0;
        ui_list_current = -1;
        ui_list_has_default = 0;

        if (last_profile_name[0] == '\0' && saved_game_last_profile_read(last_profile_name) != 0) {
            cached_profile_slot = saved_game_find_by_name(last_profile_name, 0);
        }

        matched_profile = cached_profile_slot;
        widget->selection_index = -1;

        for (i = 0; i < (int16_t)widget->item_count; i++) {
            int32_t slot_id = slot_ids[i];

            if (matched_profile != -1 && slot_id == matched_profile) {
                widget->selection_index = (int16_t)i;
            }
            if (slot_id == -1) {
                memcpy(profile_buffer, default_profile_data, sizeof(profile_buffer));
            } else if (player_profile_get(slot_id, profile_buffer) != 0) {
                ui_list_add_entry(1, (const uint16_t *)(profile_buffer + 2), i, profile_buffer, 0x1ffc, 0);
            }
        }

        high_bits = 0xffffff;
        if (widget->selection_index == -1) {
            widget->selection_index = 0;
        }
        // UNSURE: raw offsets, see the sibling level-select files' identical note.
        *(int16_t *)&((struct widget_instance *)widget)->text = widget->selection_index;
        *(int16_t *)((uint8_t *)widget + 0x3e) = -1;
    }
    return (high_bits << 8) | 1;
}

#if 0
Original Ghidra decompilation (0x49dd70):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */
/* WARNING: Removing unreachable block (ram,0x0049de05) */
/* WARNING: Removing unreachable block (ram,0x0049de20) */
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 ui_build_profile_list(int param_1)

{
  undefined3 uVar1;
  int iVar2;
  char cVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  undefined4 *puVar7;
  undefined4 *puVar8;
  undefined4 local_2008;
  undefined4 uStack_c;

  uStack_c = 0x49dd80;
  DAT_00692ac8 = 0xffffffff;
  puVar7 = &DAT_00873d60;
  for (iVar4 = 0x1800; iVar4 != 0; iVar4 = iVar4 + -1) {
    *puVar7 = 0xffffffff;
    puVar7 = puVar7 + 1;
  }
  iVar4 = heap_reallocate(400);
  *(int *)(param_1 + 0x44) = iVar4;
  uVar1 = 0;
  if (iVar4 != 0) {
    saved_game_enumerate_by_type(0,iVar4,0);
    *(undefined2 *)(param_1 + 0x48) = 100;
    DAT_006b3830 = 0x10;
    DAT_006b383c = 0x10;
    _DAT_006b3848 = 0x10;
    DAT_006b3834 = 0;
    DAT_006b3838 = 0;
    DAT_006b3840 = 0;
    DAT_006b3844 = 0;
    _DAT_006b384c = 0;
    _DAT_006b3850 = 0;
    DAT_00692c04 = 0xffffffff;
    DAT_007192f8 = 0;
    if ((DAT_00718e80 == '\0') && (cVar3 = saved_game_last_profile_read(0x718e80), cVar3 != '\0')) {
      DAT_0068e66c = saved_game_find_by_name(&DAT_00718e80,0);
    }
    iVar2 = DAT_0068e66c;
    iVar4 = *(int *)(param_1 + 0x44);
    *(undefined2 *)(param_1 + 0x40) = 0xffff;
    iVar6 = 0;
    if (*(short *)(param_1 + 0x48) != 0) {
      do {
        if ((iVar2 != -1) && (*(int *)(iVar4 + iVar6 * 4) == iVar2)) {
          *(short *)(param_1 + 0x40) = (short)iVar6;
        }
        iVar5 = *(int *)(iVar4 + iVar6 * 4);
        if (iVar5 == -1) {
          puVar7 = &DAT_0071d280;
          puVar8 = &local_2008;
          for (iVar5 = 0x7ff; iVar5 != 0; iVar5 = iVar5 + -1) {
            *puVar8 = *puVar7;
            puVar7 = puVar7 + 1;
            puVar8 = puVar8 + 1;
          }
        }
        else {
          cVar3 = player_profile_get(iVar5);
          if (cVar3 != '\0') {
            FUN_004a7ba0((int)&local_2008 + 2,iVar6,&local_2008,0x1ffc);
          }
        }
        iVar6 = iVar6 + 1;
      } while (iVar6 < (int)(uint)*(ushort *)(param_1 + 0x48));
    }
    uVar1 = 0xffffff;
    if (*(short *)(param_1 + 0x40) == -1) {
      *(undefined2 *)(param_1 + 0x40) = 0;
    }
    *(undefined2 *)(param_1 + 0x3c) = *(undefined2 *)(param_1 + 0x40);
    *(undefined2 *)(param_1 + 0x3e) = 0xffff;
  }
  return CONCAT31(uVar1,1);
}
#endif
