// main_save_map_private  (Ghidra: main_save_map_private, already named)
// address 0x4c9a70, size 228 bytes
// name confidence: 0.8   rewrite confidence: 0.8
// evidence: matches the given name exactly (cea-pdb string match "gave up trying to save",
// "unsafe save"). Every 0x0071973c..0x0071974c field is main_globals (types/main.h):
// save_map (0x03c), save_map_require_safe (0x03d), save_map_with_timeout (0x03e),
// save_map_write_pending (0x03f), save_map_retry_countdown (0x040), save_map_attempt_count
// (0x044), save_map_safe_streak (0x04c), and debug_game_save (0x3a9, "read by 0x4c9a70 and
// eight times by game_safe_to_save, never written"). game_time (0x006f1d6c) reuses
// src/interface/display_error.c's name; console_print_error_va and hud_display_checkpoint_message
// (renamed from FUN_004aa310) reuse their own established signatures. game_time_force_single_tick
// (0x007196d8) is main.h's own name for the -timedemo frame counter.
// register convention: cdecl, no parameters.
// phase 4 review (disassembly 0x4c9a70..0x4c9b54: attempt / countdown / streak arithmetic and the DL = 1 checkpoint message match; no drift.
// UNSURE: hud_display_checkpoint_message's is_begin argument is elided by Ghidra at this call
// site; passed 1 here (announcing the save is starting), matching the point in the control flow
// where the code has just decided to actually perform the save.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "interface.h"
#include "game.h"
#include "main.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern main_globals main_globals_data;   // 0x00719700
extern game_time_globals *game_time;     // 0x006f1d6c, foreign (game module)
extern int32_t game_time_force_single_tick; // 0x007196d8, foreign (main-owned global per main.h)

extern char game_safe_to_save(void); // 0x45ba50, foreign (game module)
extern void console_print_error_va(uint8_t clear_first, const char *format, ...); // 0x4c67c0, this module
extern void hud_display_checkpoint_message(uint8_t is_begin); // 0x4aa310, foreign (interface module)

// Services a pending main_globals.save_map request: if it doesn't require a safe moment, saves
// immediately (reporting "unsafe save" if debug_game_save is set); otherwise counts attempts,
// gives up (reporting an error) after 0xf0 attempts when save_map_with_timeout is set, waits out
// a retry countdown, then polls game_safe_to_save, requiring three consecutive safe checks
// before actually committing. On commit (unless a timedemo tick is in flight), shows the
// checkpoint HUD message and arms save_map_write_pending; either way clears save_map.
void main_save_map_private(void)
{
    uint8_t ready_to_save;

    if (game_time->paused != 0) {
        return;
    }

    ready_to_save = 0;

    if (main_globals_data.save_map_require_safe == 0) {
        if (main_globals_data.debug_game_save != 0) {
            console_print_error_va(0, "unsafe save");
        }
    } else {
        int32_t next_attempt_count = main_globals_data.save_map_attempt_count + 1;

        if (main_globals_data.save_map_attempt_count > 0xef &&
            main_globals_data.save_map_with_timeout != 0) {
            main_globals_data.save_map_attempt_count = next_attempt_count;
            if (main_globals_data.debug_game_save == 0) {
                main_globals_data.save_map = 0;
                return;
            }
            console_print_error_va(0, "gave up trying to save");
            main_globals_data.save_map = 0;
            return;
        }

        if (main_globals_data.save_map_retry_countdown > 0) {
            main_globals_data.save_map_retry_countdown =
                main_globals_data.save_map_retry_countdown - 1;
            main_globals_data.save_map_attempt_count = next_attempt_count;
            return;
        }

        main_globals_data.save_map_retry_countdown =
            main_globals_data.save_map_retry_countdown - 1;
        main_globals_data.save_map_attempt_count = next_attempt_count;

        if (game_safe_to_save() == 0) {
            main_globals_data.save_map_safe_streak = 0;
        } else {
            int16_t new_streak = main_globals_data.save_map_safe_streak + 1;
            uint8_t streak_reached = main_globals_data.save_map_safe_streak > 2;

            main_globals_data.save_map_safe_streak = new_streak;
            if (streak_reached) {
                ready_to_save = 1;
            }
        }

        main_globals_data.save_map_retry_countdown = 10;
        if (!ready_to_save) {
            return;
        }
    }

    if (game_time_force_single_tick == 0) {
        hud_display_checkpoint_message(1);
        main_globals_data.save_map_write_pending = 1;
    }
    main_globals_data.save_map = 0;
}

#if 0
Original Ghidra decompilation (0x4c9a70):

void __cdecl main_save_map_private(void)

{
  bool bVar1;
  bool bVar2;
  char cVar3;
  short sVar4;
  int iVar5;

  if (*(char *)(DAT_006f1d6c + 2) != '\0') {
    return;
  }
  bVar2 = false;
  if (DAT_0071973d == '\0') {
    if (DAT_00719aa9 != '\0') {
      console_print_error_va("unsafe save");
    }
  }
  else {
    iVar5 = DAT_00719744 + 1;
    if ((0xef < DAT_00719744) && (DAT_0071973e != '\0')) {
      if (DAT_00719aa9 == '\0') {
        DAT_0071973c = 0;
        DAT_00719744 = iVar5;
        return;
      }
      DAT_00719744 = iVar5;
      console_print_error_va("gave up trying to save");
      DAT_0071973c = 0;
      return;
    }
    if (0 < DAT_00719740) {
      DAT_00719740 = DAT_00719740 + -1;
      DAT_00719744 = iVar5;
      return;
    }
    DAT_00719740 = DAT_00719740 + -1;
    DAT_00719744 = iVar5;
    cVar3 = game_safe_to_save();
    if (cVar3 == '\0') {
      DAT_0071974c = 0;
    }
    else {
      sVar4 = DAT_0071974c + 1;
      bVar1 = 2 < DAT_0071974c;
      DAT_0071974c = sVar4;
      if (bVar1) {
        bVar2 = true;
      }
    }
    DAT_00719740 = 10;
    if (!bVar2) {
      DAT_00719740 = 10;
      return;
    }
  }
  if (DAT_007196d8 == 0) {
    FUN_004aa310();
    DAT_0071973f = 1;
  }
  DAT_0071973c = 0;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
