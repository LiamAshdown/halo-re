// player_profile_check_storage_and_defaults  (Ghidra: FUN_0049c680, renamed)
// renamed from FUN_0049c680 in the naming pass
// address 0x49c680, size 136 bytes
// name confidence: 0.35   rewrite confidence: 0.45
// evidence: functions.md: "Initializes the player profile system on startup: creates default
// profile files on disk if needed, then loads and selects the current profile." Near-duplicate of
// the more thoroughly analyzed player_profile_subsystem_initialize.c (0x495370, outside this
// range), which already established cached_profile_slot (0x0068e66c) and last_profile_name
// (0x00718e80); loading_thread_result (0x00718fc0) is reused here from chimera__load_main_menu.c/
// interface_tick.c since this function's storage-availability result is a plausible producer of
// that same status code.
// register convention: none (void); always returns 0.
// UNSURE: this decompile shows saved_game_find_by_name called with THREE arguments (name, 0, a
// local hard-set to 1), while player_profile_subsystem_initialize.c's already-written call to the
// same address uses only two -- a real cross-file arity disagreement, not resolved here.
// TYPES-GAP: need_default_profiles_pending_0069e8d0 is not documented by any header read this
// session.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

extern uint8_t loading_thread_result;                    // 0x00718fc0
extern uint8_t need_default_profiles_pending_0069e8d0;    // 0x0069e8d0, TYPES-GAP
extern char last_profile_name[];                          // 0x00718e80
extern int32_t cached_profile_slot;                       // 0x0068e66c

extern uint32_t saved_game_check_storage_availability(void); // 0x53d120
extern void playlist_profile_create_default_profiles_on_disk(void); // 0x53bc70
extern void saved_game_enumerate_by_type(int32_t type, int32_t *out_slots, int32_t unknown); // 0x53c4e0
extern uint8_t saved_game_last_profile_read(char *name_buffer); // 0x53d2b0
extern int32_t saved_game_find_by_name(char *name, int32_t unknown, int32_t unknown2); // 0x53d4a0, UNSURE arity

// Checks storage availability, creates the on-disk default profiles the first time this is
// needed, refreshes both saved-game enumeration lists, and resolves the cached profile slot from
// the last-used profile name if it is not already known. Always returns 0.
int32_t player_profile_check_storage_and_defaults(void)
{
    int32_t enumeration_scratch[1];

    loading_thread_result = (uint8_t)saved_game_check_storage_availability();
    if (loading_thread_result == 0) {
        int32_t find_flag = 1;

        if (need_default_profiles_pending_0069e8d0 == 1) {
            playlist_profile_create_default_profiles_on_disk();
            need_default_profiles_pending_0069e8d0 = 0;
        }
        saved_game_enumerate_by_type(1, enumeration_scratch, 1);
        saved_game_enumerate_by_type(0, enumeration_scratch, 1);
        if (last_profile_name[0] == '\0') {
            if (saved_game_last_profile_read(last_profile_name) != 0) {
                cached_profile_slot = saved_game_find_by_name(last_profile_name, 0, find_flag);
            }
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x49c680):

undefined4 FUN_0049c680(void)

{
  char cVar1;
  uint uVar2;
  undefined4 uVar3;
  undefined1 local_4 [4];

  uVar2 = saved_game_check_storage_availability();
  DAT_00718fc0 = (short)uVar2;
  if (DAT_00718fc0 == 0) {
    uVar3 = 1;
    if (DAT_0069e8d0 == '\x01') {
      playlist_profile_create_default_profiles_on_disk();
      DAT_0069e8d0 = '\0';
    }
    saved_game_enumerate_by_type(1,local_4,1);
    saved_game_enumerate_by_type(0,local_4,1);
    if (DAT_00718e80 == '\0') {
      cVar1 = saved_game_last_profile_read(0x718e80);
      if (cVar1 != '\0') {
        DAT_0068e66c = saved_game_find_by_name(&DAT_00718e80,0,uVar3);
      }
    }
  }
  return 0;
}
#endif
