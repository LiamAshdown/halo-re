// ui_profile_select_or_create  (Ghidra: FUN_004a29a0, renamed)
// renamed from FUN_004a29a0 in the naming pass
// address 0x4a29a0, size 96 bytes, callers=0 in this build
// name confidence: 0.25   rewrite confidence: 0.8
// evidence: rewritten in the phase-4 review from objdump -d 0x4a29a0..0x4a29ff. Same shape as
// FUN_004a1c30: with at least one saved profile (0x53c4e0 with EBX pointing at a count of 1) it
// calls saved_item_select with the current profile index in EBX and returns 1; otherwise it opens the
// new profile name entry (ui_new_profile_name_entry_open with the three event arguments), sets the byte at
// 0x0071916e and returns 0. Ghidra dropped that branch and the EBX arguments.
// register convention: cdecl, three stack parameters; returns a byte.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void saved_game_enumerate_by_type(uint16_t type, int32_t *out_handles, uint8_t builtin_only,
    uint16_t *capacity_and_count); // 0x53c4e0, stack (type, out, builtin_only), EBX &count
extern uint8_t ui_new_profile_name_entry_open(void *widget, int16_t *event, uint8_t *out_handled); // 0x4a1940, new profile name entry

extern int32_t saved_player_profile_slots_handle;            // 0x00714dd4
extern uint8_t new_profile_name_flag_0071916e;   // 0x0071916e, cleared by ui_new_profile_name_entry_open
extern void saved_item_select(int32_t profile_index); // 0x495be0, blam-cc: EBX profile_index

uint8_t ui_profile_select_or_create(void *widget, int16_t *event, uint8_t *out_handled)
{
    int16_t count = 1;
    int32_t slot;

    saved_game_enumerate_by_type(0, &slot, 0, (uint16_t *)&count);
    if (count > 0) {
        saved_item_select(saved_player_profile_slots_handle);
        return 1;
    }
    ui_new_profile_name_entry_open(widget, event, out_handled);
    new_profile_name_flag_0071916e = 1;
    return 0;
}

#if 0
Original Ghidra decompilation (0x4a29a0):

/* WARNING: Removing unreachable block (ram,0x004a29db) */

undefined4 FUN_004a29a0(void)

{
  undefined1 local_4 [4];

  saved_game_enumerate_by_type(0,local_4,0);
  FUN_00495be0();
  return 1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
