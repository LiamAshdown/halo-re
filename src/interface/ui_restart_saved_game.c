// ui_restart_saved_game  (Ghidra: FUN_004a1110, renamed)
// renamed from FUN_004a1110 in the naming pass
// address 0x4a1110, size 109 bytes, callers=0 in this build
// name confidence: 0.35   rewrite confidence: 0.4
// evidence: functions.md: "Resumes the in-progress saved campaign game (unless a save operation
// is already underway), applying the current level/profile state so the game begins loading."
// Reuses the same cached_profile_slot bookkeeping already established in FUN_0049ce00.c/
// FUN_0049cfd0.c.
// register convention: none (void).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

extern uint8_t save_in_progress_00719010;   // 0x00719010, TYPES-GAP
extern int32_t saved_player_profile_slots_handle;        // 0x00714dd4
extern int32_t cached_profile_slot;          // 0x0068e66c
extern int16_t network_game_mode;             // 0x00719720
extern uint8_t network_wait_flag_00719739;    // 0x00719739
extern char last_profile_name[];              // 0x00718e80

extern void saved_game_delete_files(void); // 0x5388c0, UNSURE args
extern uint8_t saved_game_get_directory_by_handle(int32_t slot, char *out_name); // 0x53d080
extern void saved_game_last_profile_clear(char *name);     // 0x53d220

uint32_t ui_restart_saved_game(void)
{
    if (save_in_progress_00719010 != 0) {
        return 0;
    }
    saved_game_delete_files();
    network_game_mode = 0;
    network_wait_flag_00719739 = 1;
    if (cached_profile_slot != saved_player_profile_slots_handle) {
        if (saved_player_profile_slots_handle != -1) {
            saved_game_get_directory_by_handle(saved_player_profile_slots_handle, last_profile_name);
        }
        cached_profile_slot = saved_player_profile_slots_handle;
    }
    if (last_profile_name[0] != '\0') {
        saved_game_last_profile_clear(last_profile_name);
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4a1110):

undefined4 FUN_004a1110(void)

{
  if (DAT_00719010 != '\0') {
    return 0;
  }
  saved_game_delete_files();
  DAT_00719720 = 0;
  DAT_00719739 = 1;
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
#endif
