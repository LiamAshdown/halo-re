// main_queue_map_change  (Ghidra: main_queue_map_change, already named)
// address 0x4c8740, size 83 bytes
// name confidence: 0.55   rewrite confidence: 0.8
// evidence: out/phase4/main_types_notes.md "Register arguments confirmed at call sites":
// "main_queue_map_change 0x4c8740: EAX = map name (0x4c7106, 0x4c9c39)". Globals mapped against
// main_globals (types/main.h): 0x00719779 = scenario_path[0], 0x00719878 = scenario_path[0xff]
// (one of the four string terminators main.h's smoke test checks), 0x00719778 =
// restore_checkpoint_on_load, 0x00719757 = return_to_main_menu (DAT_00719754._3_1_; confirmed
// against main_menu_return_and_reset 0x4c8a60, which clears this same byte once serviced --
// NOT the interface module's own, less-informed "split_screen_quit_prompt_armed" guess for the
// same address in src/interface/interface_handle_quit_request.c, superseded here by main.h's
// byte-width-confirmed field). game_time (0x006f1d6c) and network_game_mode (0x00719720) reuse
// the names already established in src/interface/display_error.c; network_wait_flag_00719739
// likewise.
// register convention: EAX -> map_name.
// phase 4 review (disassembly 0x4c8740..0x4c8792: no drift; 0x00719739 is main_globals.level_transition.
// UNSURE: types/game.h documents game_time_globals.unknown_00 as "never read or written", but
// out/phase4/main_types_notes.md's own correction (backed by the objdump field-width survey)
// says it is read here; kept as unknown_00 (the header name), not renamed, per this project's
// per-file-independent extern convention.
// reconciled: R33 game_time_globals.unknown_00 -> initialized (uint8 at +0x00, same byte)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "interface.h"
#include "game.h"
#include "main.h"
#include <string.h>

extern main_globals main_globals_data; // 0x00719700

extern game_time_globals *game_time; // 0x006f1d6c, foreign (game module)

// blam-cc: EAX -> map_name
// Stages map_name as the next scenario to load and arms restore_checkpoint_on_load, clearing any
// pending "confirm quit" prompt first. If the simulation is currently active or paused in a
// local (non-networked) game, also arms main_globals.level_transition.
void main_queue_map_change(char *map_name)
{
    main_globals_data.return_to_main_menu = 0;
    strncpy(main_globals_data.scenario_path, map_name, 0xff);
    main_globals_data.scenario_path[0xff] = 0;
    main_globals_data.restore_checkpoint_on_load = 1;
    if (game_time->initialized != 0 &&
        (game_time->active != 0 || game_time->paused != 0) &&
        main_globals_data.game_connection == 0) {
        main_globals_data.level_transition = 1;
    }
}

#if 0
Original Ghidra decompilation (0x4c8740):

void main_queue_map_change(void)

{
  char *in_EAX;

  DAT_00719754._3_1_ = 0;
  _strncpy(&DAT_00719779,in_EAX,0xff);
  DAT_00719878 = 0;
  DAT_00719778 = 1;
  if ((*DAT_006f1d6c != '\0') &&
     (((DAT_006f1d6c[1] != '\0' || (DAT_006f1d6c[2] != '\0')) && (DAT_00719720 == 0)))) {
    DAT_00719739 = 1;
  }
  return;
}
#endif
