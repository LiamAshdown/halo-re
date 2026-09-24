// game_engine_end_game_sequence_stage3  (Ghidra: FUN_00467180; named per
// out/phase4/game_functions.md: "Third stage of the countdown sequence: sets state 3 and
// configures a follow-on duration from global settings.")
// address 0x467180, size 68 bytes
// name confidence: 0.4   rewrite confidence: 0.55
// evidence: types/game.h game_engine_state (_game_engine_state_post_game == 3, "carnage report
//   is up; 0x0087aa0c fades 0 -> 1") and game_engine_dedicated_idle /
//   game_engine_dedicated_idle_timer (0x0087aa18 / 0x0087aa1c).
// register convention: no parameters.
// UNSURE: 0x006f1d25 and 0x006f1d28 (a "dedicated server enabled" flag and a configured idle
//   duration, by inference from how they gate game_engine_dedicated_idle/_timer) are not
//   attributed to any module here; kept as raw externs.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern game_engine_state game_engine_state_value;      // 0x0087aa10
extern uint8_t game_engine_dedicated_idle;             // 0x0087aa18
extern float game_engine_dedicated_idle_timer;         // 0x0087aa1c

extern uint8_t g_006f1d25; // 0x006f1d25, UNSURE: not owned by this module
extern int32_t g_006f1d28; // 0x006f1d28, UNSURE: not owned by this module

// Enters the "post game" (carnage report) end-of-game state. On a dedicated server
// (g_006f1d25), clears the idle countdown outright; otherwise, if a positive idle duration is
// configured (g_006f1d28), starts the dedicated-idle countdown with it.
void game_engine_end_game_sequence_stage3(void)
{
    game_engine_state_value = _game_engine_state_post_game;

    if (g_006f1d25 == 1) {
        game_engine_dedicated_idle = 0;
        game_engine_dedicated_idle_timer = 0.0f;
        return;
    }

    if (g_006f1d28 > 0) {
        game_engine_dedicated_idle_timer = (float)g_006f1d28;
        game_engine_dedicated_idle = 1;
    }
}

#if 0
Original Ghidra decompilation (0x467180), from tools/pack.py 0x467180:

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00467180(void)

{
  DAT_0087aa10 = 3;
  if (DAT_006f1d25 == '\x01') {
    DAT_0087aa18 = 0;
    _DAT_0087aa1c = 0.0;
    return;
  }
  if (0 < DAT_006f1d28) {
    _DAT_0087aa1c = (float)DAT_006f1d28;
    DAT_0087aa18 = 1;
  }
  return;
}
#endif
