// numeric_countdown_timer_update  (Ghidra: game_timer_update)
// address 0x540240, size 86 bytes
// name confidence: 0.75   rewrite confidence: 0.9 (phase-4 review, objdump checked)
// evidence: out/phase4/shaders_types_notes.md; tail-called (E9, not E8) from
// game_effects_update 0x45b57f. Converts the current game tick to milliseconds and counts the
// numeric countdown timer down by however many ms elapsed since the last call, clamping at 0.
// register convention: no arguments, no return value (void -> void); globals only.
// blam-cc: none (plain void (void); clobbers EAX, ECX, EDX)
// objdump (0x540240..0x540295) confirms Ghidra's pseudocode exactly, including the control
// flow it renders as short-circuit &&: when the timer is not running, the function returns
// immediately without touching numeric_countdown_timer_last_update_ms at all (`je 0x540295`
// jumps straight to `ret`) -- Ghidra's "iVar1 = DAT_00721e58;" followed by an unconditional
// store back is a decompiler artifact of the SSA merge, not a real write in that path; written
// below as an early-out to match the disassembly. When running but the new tick-derived time
// hasn't advanced past the last update (`new_time < last_update_ms`), remaining_ms is left
// untouched and only last_update_ms is refreshed. The tick-to-ms conversion
// (`game_time->game_time * 1000) / 30`) is a magic-multiply constant in the binary; written
// here as plain integer arithmetic, which the compiler is free to re-optimize into the same
// idiom -- the observable result (truncating signed division) is identical either way.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "shaders.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern game_time_globals *game_time; // 0x006f1d6c
extern int32_t numeric_countdown_timer_remaining_ms;   // 0x00721e50
extern uint8_t numeric_countdown_timer_running;         // 0x00721e54
extern int32_t numeric_countdown_timer_last_update_ms;  // 0x00721e58

void numeric_countdown_timer_update(void)
{
    int32_t new_time;

    if (!numeric_countdown_timer_running) {
        return;
    }

    new_time = (game_time->game_time * k_numeric_countdown_timer_milliseconds_per_second) /
               k_numeric_countdown_timer_ticks_per_second;

    if (numeric_countdown_timer_last_update_ms <= new_time) {
        numeric_countdown_timer_remaining_ms +=
            (numeric_countdown_timer_last_update_ms - new_time);
        if (numeric_countdown_timer_remaining_ms < 0) {
            numeric_countdown_timer_remaining_ms = 0;
        }
    }

    numeric_countdown_timer_last_update_ms = new_time;
}

#if 0
Original Ghidra decompilation (0x540240):

void game_timer_update(void)

{
  int iVar1;

  iVar1 = DAT_00721e58;
  if (((DAT_00721e54 != '\0') &&
      (iVar1 = (*(int *)(DAT_006f1d6c + 0xc) * 1000) / 0x1e, DAT_00721e58 <= iVar1)) &&
     (DAT_00721e50 = DAT_00721e50 + (DAT_00721e58 - iVar1), DAT_00721e50 < 0)) {
    DAT_00721e50 = 0;
  }
  DAT_00721e58 = iVar1;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
