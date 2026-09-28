// ui_start_campaign_from_level_one  (Ghidra: FUN_0049cfd0, renamed)
// renamed from FUN_0049cfd0 in the naming pass
// address 0x49cfd0, size 248 bytes, callers=0 in this build (dead code, same unreachable
// level-select cluster as its neighbors)
// name confidence: 0.3   rewrite confidence: 0.2
// evidence: functions.md: "Starts a new campaign game from the first level, applying the chosen
// difficulty/option index if valid, or reporting an error and falling back to single-player mode
// if not." types/game.h names 0x00719720 network_game_mode; player_profile_subsystem_initialize.c
// names 0x00714dde profile_slot_id.
// register convention: cdecl, both recognized stack parameters (param_1 unused, param_2 an
// options/difficulty record this function reads a single int16 out of at offset +2).
// UNSURE: param_2's real type was not resolved (no caller in this session; the function itself is
// unreachable). The validation loop's exact intent (walking joystick_slot_devices[] at 0x006b2ce8,
// while an entry is -1 or matches the requested index) is preserved with `goto` rather than
// restructured, since the two-label control flow (a shared error path reached from two different
// points) is not confidently reducible to a structured form without risking a behaviour change.
// reconciled: R02 0x006b2ce8 int16 campaign_option_table -> int32 joystick_slot_devices[4] (0x49cff3 is a DWORD cmp)

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

extern int16_t local_player_count;                 // 0x006894b8, TYPES-GAP
extern int32_t joystick_slot_devices[4];                   // 0x006b2ce8, input.h (DWORD cmp at 0x49cff3)
extern char known_campaign_level_one_path_00692acc[];        // 0x00692acc ("levels\\a10\\a10")
extern char selected_level_path_00719779[0x100];             // 0x00719779, TYPES-GAP
extern uint8_t campaign_start_pending_00696564;               // 0x00696564, TYPES-GAP
extern uint8_t selected_level_start_flags_00719754_byte3;    // 0x00719757, TYPES-GAP
extern uint8_t selected_level_active_00719878;                // 0x00719878, TYPES-GAP
extern uint8_t selected_level_pending_00719778;                // 0x00719778, TYPES-GAP
extern int16_t network_game_mode;                              // 0x00719720
extern uint8_t network_wait_flag_00719739;                     // 0x00719739
extern int16_t profile_slot_id[];                              // 0x00714dde
extern int16_t profile_slot_id_1_00714de0;                     // 0x00714de0, TYPES-GAP
extern int32_t current_profile_index;                          // 0x00714dd4
extern int32_t cached_profile_slot;                             // 0x0068e66c
extern char last_profile_name[];                                // 0x00718e80

extern void display_error(int16_t error_string_index, int32_t player_index, uint8_t modal, uint8_t is_error); // 0x498f20
extern uint8_t saved_game_get_directory_by_handle(int32_t slot, char *out_name); // 0x53d080
extern void saved_game_last_profile_clear(char *name);     // 0x53d220

// Validates the requested difficulty/option index (when more than one is configured), then starts
// a new campaign game at the first level; reports error 0x13 and forces single-option mode if the
// requested index is not valid.
uint32_t ui_start_campaign_from_level_one(void *param_1, int16_t *param_2)
{
    int16_t requested_index = *(int16_t *)((uint8_t *)param_2 + 2); // UNSURE: param_2's real shape
    int16_t i;

    (void)param_1;

    if (local_player_count >= 2) {
        i = 0;
        while (joystick_slot_devices[i] == -1 || i == requested_index) {
            i = i + 1;
            if (i > 0) {
                goto report_error;
            }
        }
        if (i == -1) {
            goto report_error;
        }
    } else {
        i = -1;
    }

    campaign_start_pending_00696564 = 1;
    selected_level_start_flags_00719754_byte3 = 0;
    strncpy(selected_level_path_00719779, known_campaign_level_one_path_00692acc, 0xff);
    selected_level_active_00719878 = 0;
    selected_level_pending_00719778 = 1;
    network_game_mode = 0;
    network_wait_flag_00719739 = 1;
    profile_slot_id[0] = requested_index;
    if (i != -1) {
        profile_slot_id_1_00714de0 = i;
    }
    if (cached_profile_slot != current_profile_index) {
        if (current_profile_index != -1) {
            saved_game_get_directory_by_handle(current_profile_index, last_profile_name);
        }
        cached_profile_slot = current_profile_index;
    }
    if (last_profile_name[0] != '\0') {
        saved_game_last_profile_clear(last_profile_name);
    }
    return 1;

report_error:
    local_player_count = 1;
    display_error(0x13, -1, 1, 0);
    return 0;
}

#if 0
Original Ghidra decompilation (0x49cfd0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 FUN_0049cfd0(undefined4 param_1,int param_2)

{
  int iVar1;

  iVar1 = -1;
  if (DAT_006894b8 < 2) {
LAB_0049d02f:
    DAT_00696564 = 1;
    DAT_00719754._3_1_ = 0;
    _strncpy(&DAT_00719779,PTR_s_levels_a10_a10_00692acc,0xff);
    DAT_00719878 = 0;
    DAT_00719778 = 1;
    DAT_00719720 = 0;
    DAT_00719739 = 1;
    _DAT_00714dde = *(undefined2 *)(param_2 + 2);
    if ((short)iVar1 != -1) {
      DAT_00714de0._0_2_ = (short)iVar1;
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
    return 1;
  }
  iVar1 = 0;
  while (((&DAT_006b2ce8)[(short)iVar1] == -1 || (iVar1 == *(short *)(param_2 + 2)))) {
    iVar1 = iVar1 + 1;
    if (0 < iVar1) {
LAB_0049d014:
      DAT_006894b8 = 1;
      display_error(0x13,-1,'\x01','\0');
      return 0;
    }
  }
  if ((short)iVar1 != -1) goto LAB_0049d02f;
  goto LAB_0049d014;
}
#endif
