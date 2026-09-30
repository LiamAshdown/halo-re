// game_engine_flush_pending_simulation_ticks  (Ghidra: game_engine_flush_pending_simulation_ticks,
// already named)
// address 0x4c99e0, size 141 bytes
// name confidence: 0.55   rewrite confidence: 0.8
// evidence: matches the given name exactly; out/phase4/main_functions.md summary ("Runs a fixed
// number of queued simulation ticks back-to-back at a fixed 30Hz timestep, used to fast-forward
// the simulation"). 0x0071976e/0x0071976c are main_globals.skip_tick_count/skip_ticks
// (types/main.h, offsets 0x06e/0x06c). game_time (0x006f1d6c) and cinematic_globals_ptr (0x006f187c)
// reuse src/interface/display_error.c and src/interface/ui_error_modal_update.c's names;
// game_time->speed (0x18) matches types/game.h's own field exactly ("time scale; forced to 1.0
// in any networked game").
// register convention: cdecl, no parameters.
// phase 4 review (disassembly 0x4c99e0..0x4c9a6c: no drift.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "interface.h"
#include "game.h"
#include "main.h"
#include "units.h"
#include "cutscene.h"
#include "fn_game.h"
#include "fn_main.h"

extern main_globals main_globals_data; // 0x00719700
extern game_time_globals *game_time;   // 0x006f1d6c, foreign (game module)
extern cinematic_globals *cinematic_globals_ptr; // 0x006f187c


// While skip_tick_count is armed and a cinematic isn't suppressing it, runs that many simulation
// ticks back-to-back at a fixed 1/30s timestep (forcing game_time->speed to 1.0 for the
// duration, restoring it afterward -- already 1.0 in any networked game), then clears both the
// tick count and the pending flag.
void game_engine_flush_pending_simulation_ticks(void)
{
    if (main_globals_data.skip_tick_count != 0 && cinematic_globals_ptr->in_progress != 0) {
        float saved_speed = (main_globals_data.game_connection == 1 || main_globals_data.game_connection == 2)
                                 ? 1.0f
                                 : game_time->speed;

        game_time->speed = 1.0f;
        while (main_globals_data.skip_tick_count > 0) {
            main_globals_data.skip_tick_count = main_globals_data.skip_tick_count - 1;
            game_engine_advance_simulation_ticks(0.033333335f);
        }
        game_time->speed = saved_speed;
    }
    main_globals_data.skip_tick_count = 0;
    main_globals_data.skip_ticks = 0;
}

#if 0
Original Ghidra decompilation (0x4c99e0):

void __cdecl game_engine_flush_pending_simulation_ticks(void)

{
  int iVar1;
  undefined4 local_4;

  iVar1 = DAT_006f1d6c;
  if ((DAT_0071976e != 0) && (*(char *)(DAT_006f187c + 9) != '\0')) {
    if ((DAT_00719720 == 1) || (DAT_00719720 == 2)) {
      local_4 = 0x3f800000;
    }
    else {
      local_4 = *(undefined4 *)(DAT_006f1d6c + 0x18);
    }
    *(undefined4 *)(DAT_006f1d6c + 0x18) = 0x3f800000;
    while (0 < DAT_0071976e) {
      DAT_0071976e = DAT_0071976e + -1;
      game_engine_advance_simulation_ticks(0.033333335);
      iVar1 = DAT_006f1d6c;
    }
    *(undefined4 *)(iVar1 + 0x18) = local_4;
  }
  DAT_0071976e = 0;
  DAT_0071976c = 0;
  return;
}
#endif
