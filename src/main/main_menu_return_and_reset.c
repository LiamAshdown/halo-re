// main_menu_return_and_reset  (Ghidra: main_menu_return_and_reset, already named)
// address 0x4c8a60, size 214 bytes
// name confidence: 0.55   rewrite confidence: 0.75
// evidence: matches the given name exactly. 0x00719756 is main_globals.main_menu_scenario_loaded
// (types/main.h, offset 0x056; confirmed here directly, since this function reads it to decide
// whether chimera__load_ui_map is needed and clears main_globals.return_to_main_menu at the end
// -- the same field src/main/main_queue_map_change.c's correction relies on). The interface
// "loading screen" globals (0x0068e680/4/8, 0x00718f8c/90, 0x006b2f28/68) reuse
// src/networking/network_join_request_resolve_host.c's names; ui_network_wait_active/
// _timed_out/_start_time (0x00718fcd/cc, 0x006927c4) reuse types/interface.h's own documented
// names (src/interface/ui_network_wait_timeout_check.c). game_time (0x006f1d6c) and
// input_mode_flags (0x00712542) reuse src/interface/display_error.c and
// src/camera/camera_update.c's names.
// register convention: cdecl, no parameters (Ghidra fully resolved it as void(void)).
// phase 4 review (disassembly 0x4c8a60..0x4c8b35: predicted_resource_list_touch now gets ESI = &global_scenario->predicted_resources; 0x00712542 is input_globals.mode_flags.
// UNSURE: the game_time_globals reset at the end dereferences game_time unconditionally after an
// initial null-gated pair of byte writes (`if (game_time != 0) { unknown_00 = 0; active = 0; }`
// followed by an UNGATED 8-dword zero and `unknown_00 = 1`) -- a literal NULL game_time would
// crash on retail hardware past that point. Preserved exactly as disassembled (confirmed: no
// second null check appears anywhere in this function's disassembly, 0x4c8adb..0x4c8b1e).
// reconciled: R33 game_time_globals.unknown_00 -> initialized (uint8 at +0x00, same byte)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "interface.h"
#include "game.h"
#include "networking.h"
#include "saved_games.h"
#include "input.h"
#include "main.h"
#include "fn_hs.h"
#include <string.h>

extern main_globals main_globals_data; // 0x00719700
extern Scenario *global_scenario;      // 0x00746f8c, foreign (game module)
extern game_time_globals *game_time;   // 0x006f1d6c, foreign (game module)
extern input_abstraction_globals input_globals; // 0x00710328, foreign (input module); mode_flags at 0x00712542

extern int32_t interface_loading_screen_address_a;   // 0x0068e680, foreign (interface module)
extern int32_t interface_loading_screen_address_b;   // 0x0068e684, foreign (interface module)
extern int32_t join_ui_state;    // 0x00718f8c, foreign (interface module)
extern int32_t interface_loading_screen_progress;    // 0x00718f90, foreign (interface module)
extern uint16_t progress_screen_text[0x20];    // 0x006b2f28, foreign (types/interface.h); WORD stores only
extern uint16_t progress_screen_subtext[0x20]; // 0x006b2f68, foreign (types/interface.h); WORD stores only
extern int32_t interface_loading_screen_request_id;  // 0x0068e688, foreign (interface module)
extern uint8_t ui_network_wait_timed_out;  // 0x00718fcc, foreign (interface module)
extern uint8_t ui_network_wait_active;     // 0x00718fcd, foreign (interface module)
extern int32_t ui_network_wait_start_time; // 0x006927c4, foreign (interface module)

extern void chimera__load_ui_map(char play_title_music); // 0x4c8930, this module
extern void chimera__load_main_menu(void);         // 0x4989f0, foreign (interface module)
extern void predicted_resource_list_touch(TagReflexive *resources); // 0x4449f0, foreign (cache); blam-cc: ESI -> resources
extern void hud_chat_listbox_clear(void);          // 0x4ab400, foreign (interface module)
extern void update_queues_dispose(void);           // 0x472b00, foreign (game module)
extern void update_server_new(void);               // 0x472aa0, foreign (game module)
extern void update_server_dispose(void);                    // 0x472b70, foreign (game module)
extern void game_engine_init_tick_record_for_mode(void);                    // 0x470ae0, foreign (game module)


// Tears down the current game session and returns to the main menu: (re)loads the UI map if it
// is not already loaded, always (re)loads the main menu widget itself and touches the predicted
// resource list if a scenario was loaded, resets the loading screen and network-wait UI state,
// clears the chat box, tears down and re-creates the update-queue/server bookkeeping, resets the
// game clock, disposes and re-initializes the hs dynamic globals and scenario scripts, clears
// main_globals.return_to_main_menu now that it has been serviced, and re-arms the menu-navigation
// input mode bit.
void main_menu_return_and_reset(void)
{
    if (main_globals_data.main_menu_scenario_loaded == 0) {
        chimera__load_ui_map(0);
    }
    chimera__load_main_menu();
    if (global_scenario != 0) {
        predicted_resource_list_touch(&global_scenario->predicted_resources); // 0x4c8a85 lea esi,[eax+0xec]
    }

    interface_loading_screen_address_a = -1;
    interface_loading_screen_address_b = -1;
    join_ui_state = 0;
    interface_loading_screen_progress = 0;
    progress_screen_text[0] = 0;    // WORD store
    progress_screen_subtext[0] = 0; // WORD store
    interface_loading_screen_request_id = -1;

    hud_chat_listbox_clear();
    ui_network_wait_active = 0;
    ui_network_wait_start_time = -1;
    ui_network_wait_timed_out = 0;

    update_queues_dispose();
    update_server_new();
    update_server_dispose();

    if (game_time != 0) {
        game_time->initialized = 0;
        game_time->active = 0;
    }
    memset(game_time, 0, 0x20); // UNSURE: unconditional even if game_time is NULL, see file header
    game_time->initialized = 1;

    game_engine_init_tick_record_for_mode();
    hs_dispose_dynamic_globals();
    hs_scenario_scripts_initialize();

    main_globals_data.return_to_main_menu = 0;
    input_globals.mode_flags = input_globals.mode_flags | 2;
}

#if 0
Original Ghidra decompilation (0x4c8a60):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl main_menu_return_and_reset(void)

{
  undefined4 *puVar1;

  if (DAT_00719754._2_1_ == '\0') {
    chimera__load_ui_map('\0');
  }
  chimera__load_main_menu();
  if (global_scenario != 0) {
    predicted_resource_list_touch();
  }
  DAT_0068e680 = 0xffffffff;
  DAT_0068e684 = 0xffffffff;
  DAT_00718f8c = 0;
  DAT_00718f90 = 0;
  _DAT_006b2f28 = 0;
  _DAT_006b2f68 = 0;
  DAT_0068e688 = 0xffffffff;
  hud_chat_listbox_clear();
  DAT_00718fcd = 0;
  DAT_006927c4 = 0xffffffff;
  DAT_00718fcc = 0;
  update_queues_dispose();
  update_server_new();
  FUN_00472b70();
  puVar1 = DAT_006f1d6c;
  if (DAT_006f1d6c != (undefined4 *)0x0) {
    *(undefined1 *)DAT_006f1d6c = 0;
    *(undefined1 *)((int)puVar1 + 1) = 0;
  }
  *puVar1 = 0;
  puVar1[1] = 0;
  puVar1[2] = 0;
  puVar1[3] = 0;
  puVar1[4] = 0;
  puVar1[5] = 0;
  puVar1[6] = 0;
  puVar1[7] = 0;
  *(undefined1 *)puVar1 = 1;
  FUN_00470ae0();
  hs_dispose_dynamic_globals();
  hs_scenario_scripts_initialize();
  DAT_00719754._3_1_ = 0;
  DAT_00712542 = DAT_00712542 | 2;
  return;
}
#endif
