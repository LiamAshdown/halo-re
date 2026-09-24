// game_start_new_single_player_map  (Ghidra: game_start_new_single_player_map, already named)
// address 0x4c9dd0, size 187 bytes
// name confidence: 0.5   rewrite confidence: 0.8
// evidence: matches the given name exactly. 0x00719768 is main_globals.start_film_playback
// (types/main.h, offset 0x068); 0x00719757 is return_to_main_menu (see
// src/main/main_queue_map_change.c's correction). The stack-built network_scenario_load_request
// mirrors src/main/chimera__load_ui_map.c / game_scenario_session_begin.c / main_level_
// transition_update.c's identical construction.
// register convention: cdecl, no parameters.
// reconciled: R13 network_scenario_load_request.seed (+0x06) -> difficulty (campaign difficulty, lands at game globals +0x0e)

// phase 4 review (disassembly 0x4c9dd0..0x4c9e8a): 0x45aea0 is cache_file_switch_map_by_path with
// EAX = &request.map_name and BL = 1 (the phase 3 file called it without arguments).
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "interface.h"
#include "game.h"
#include "networking.h"
#include "main.h"
#include <string.h>

extern main_globals main_globals_data;  // 0x00719700
extern int16_t main_pending_difficulty; // 0x00696564, foreign (main-owned global per main.h)

extern void main_menu_return_and_reset(void); // 0x4c8a60, this module
extern void game_scenario_session_begin(network_scenario_load_request *request); // 0x4c95f0, this module
extern void cache_file_switch_map_by_path(char *path, uint8_t apply_state); // 0x45aea0, foreign (game module)
    // blam-cc: EAX -> path, BL -> apply_state
extern void game_stop_current_map(void);   // 0x45b370, foreign (game module)

// Starts loading the currently selected map (scenario_path) as a new single-player game
// session, unless film playback was requested (in which case it switches to that connection
// mode and returns to the main menu instead) or a return-to-main-menu is already pending (in
// which case it just services that instead).
void game_start_new_single_player_map(void)
{
    network_scenario_load_request request;

    if (main_globals_data.start_film_playback != 0) {
        main_globals_data.game_connection = _game_connection_film_playback;
        main_globals_data.return_to_main_menu = 1;
        main_menu_return_and_reset();
        return;
    }
    if (main_globals_data.return_to_main_menu != 0) {
        main_menu_return_and_reset();
        return;
    }

    memset(&request, 0, sizeof(request));
    main_globals_data.game_connection = _game_connection_local;
    request.salt = 0xdeadbeef;
    strncpy(request.map_name, main_globals_data.scenario_path, 0xff);
    request.map_name[0xff] = 0;
    request.difficulty = main_pending_difficulty;

    cache_file_switch_map_by_path(request.map_name, 1);   // 0x4c9e64 mov bl,1 ; lea eax,[esp+0x14]
    game_stop_current_map();
    game_scenario_session_begin(&request);
}

#if 0
Original Ghidra decompilation (0x4c9dd0):

void __cdecl game_start_new_single_player_map(void)

{
  int iVar1;
  undefined4 *puVar2;
  undefined4 local_114;
  undefined2 local_110;
  undefined2 local_10e;
  undefined4 local_10c;
  char local_108 [255];
  undefined1 local_9;

  if (DAT_00719768 != '\0') {
    DAT_00719720 = 3;
    DAT_00719754._3_1_ = 1;
    main_menu_return_and_reset();
    return;
  }
  if (DAT_00719754._3_1_ != '\0') {
    main_menu_return_and_reset();
    return;
  }
  puVar2 = &local_114;
  for (iVar1 = 0x43; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar2 = 0;
    puVar2 = puVar2 + 1;
  }
  DAT_00719720 = 0;
  local_110 = 0;
  local_10e = 1;
  local_10c = 0xdeadbeef;
  _strncpy(local_108,&DAT_00719779,0xff);
  local_9 = 0;
  local_10e = DAT_00696564;
  FUN_0045aea0();
  FUN_0045b370();
  game_scenario_session_begin();
  return;
}
#endif
