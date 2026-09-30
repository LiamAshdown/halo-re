// main_loop_shutdown_cleanup  (Ghidra: main_loop_shutdown_cleanup, already named)
// address 0x4c9e90, size 147 bytes
// name confidence: 0.5   rewrite confidence: 0.8
// evidence: matches the given name exactly. ban_list (growable_array, 0x006b859c, element_size/
// count/data at +0x00/+0x04/+0x08 per types/memory.h) reuses src/networking/ban_list_find_by_
// name.c's name; network_server (0x0071c2d4) and network_server_host_valid (0x0071c2dd) reuse
// src/networking/network_game_scenario_load_request.c and src/interface/network_game_host_
// start.c's names; network_game_server_host_dispose reuses that same file's signature.
// network_game_mode (0x00719720) is established elsewhere in this codebase.
// register convention: cdecl, no parameters.
// phase 4 review (disassembly 0x4c9e90..0x4c9f23: no drift; 0x45b370 now called by its established name game_stop_current_map.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "interface.h"
#include "game.h"
#include "networking.h"
#include "main.h"
#include "fn_game.h"
#include "fn_networking.h"
#include "fn_main.h"
#include "fn_interface.h"

extern main_globals main_globals_data; // 0x00719700
extern growable_array ban_list;              // 0x006b859c, foreign (networking module)
extern network_server_globals *network_server; // 0x0071c2d4, foreign (networking module)
extern uint8_t network_server_host_valid;    // 0x0071c2dd, foreign (interface module)


extern void network_game_server_host_dispose(network_server_globals *server); // 0x4deda0, foreign (networking module)


extern void chat_close(void);                // 0x4aa900, foreign (interface module)

// Final teardown when the main loop exits: releases the map cache index, resets the ban list,
// disposes networking state appropriate to the current connection (client vs. host, disposing
// the host and its server object when hosting), then stops the current map, deactivates the
// console and closes chat either way.
void main_loop_shutdown_cleanup(void)
{
    map_list_free_all();
    ban_list.element_size = -1;
    ban_list.count = -1;
    if (ban_list.data != 0) {
        GlobalFree(ban_list.data);
        ban_list.data = 0;
    }

    network_buffer_pair_pool_clear();
    if (main_globals_data.game_connection == 1) {
        network_client_globals_dispose();
    } else if (main_globals_data.game_connection == 2) {
        network_client_globals_dispose();
        if (network_server != 0) {
            network_game_server_host_dispose(network_server);
            network_server = 0;
            network_server_host_valid = 0;
            game_stop_current_map();
            game_dispose();
            console_deactivate();
            chat_close();
            return;
        }
    }
    game_stop_current_map();
    game_dispose();
    console_deactivate();
    chat_close();
}

#if 0
Original Ghidra decompilation (0x4c9e90):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl main_loop_shutdown_cleanup(void)

{
  chimera__free_map_index();
  _DAT_006b859c = 0xffffffff;
  DAT_006b85a0 = 0xffffffff;
  if (DAT_006b85a4 != (HGLOBAL)0x0) {
    GlobalFree(DAT_006b85a4);
    DAT_006b85a4 = (HGLOBAL)0x0;
  }
  FUN_004e3ed0();
  if (DAT_00719720 == 1) {
    FUN_004dde70();
  }
  else if (DAT_00719720 == 2) {
    FUN_004dde70();
    if (DAT_0071c2d4 != (int *)0x0) {
      network_game_server_host_dispose(DAT_0071c2d4);
      DAT_0071c2d4 = (int *)0x0;
      DAT_0071c2dd = 0;
      FUN_0045b370();
      FUN_0045acd0();
      console_deactivate();
      chat_close();
      return;
    }
  }
  FUN_0045b370();
  FUN_0045acd0();
  console_deactivate();
  chat_close();
  return;
}
#endif
