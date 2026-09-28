// main_level_transition_update  (Ghidra: main_level_transition_update, already named)
// address 0x4c9770, size 615 bytes
// name confidence: 0.5   rewrite confidence: 0.75
// evidence: matches the given name; out/phase4/main_functions.md summary ("Drives the
// level-change sequence: verifies the target map file exists, fades out, then unloads the
// current map and loads the next one"). map_path_prefix (0x006f16d8), ui_input_batch_mode
// (0x00718fc5) and main_menu_on_shown/player_profile_select_local_slot's real signatures reuse
// src/cache/cache_file_exists.c, src/interface/chimera__load_main_menu.c,
// src/interface/main_menu_on_shown.c and src/saved_games/player_profile_select_local_slot.c.
// Every 0x00719708..0x0071975a field is main_globals (types/main.h): frame_time_ms (0x008),
// level_transition (0x039, NOT the "network wait" guess other modules make for the same
// overlapping byte, per src/main/main_queue_map_change.c's correction), main_menu_scenario_
// loaded (0x056), return_to_main_menu (0x057), idle_timeout_reached (0x059), unknown_05a
// (0x05a), level_transition_fade_end_ms (0x048). main_pending_difficulty (0x00696564) and
// local_player_count (0x006894b8) reuse types/main.h and src/saved_games's established names.
// The stack-built network_scenario_load_request mirrors src/main/chimera__load_ui_map.c and
// src/main/game_scenario_session_begin.c's identical construction.
// register convention: cdecl, no parameters.
// phase 4 review (disassembly 0x4c9770..0x4c99d6): 0x006b2f28 and 0x006b2f68 are WORD stores
// (the phase 3 file wrote dwords), and 0x45aea0 is cache_file_switch_map_by_path with EAX =
// &request.map_name and BL = 1 (the phase 3 file called it without arguments).
// UNSURE: FUN_00624236 (0x624236) is undocumented elsewhere; its address and (path, mode)
// call shape match the CRT _access family used throughout this codebase (e.g. 0x623a90
// strncpy, 0x623693 sprintf), so it is declared as _access here.
// reconciled: R33 game_time_globals.unknown_00 -> initialized (uint8 at +0x00, same byte)
// reconciled: R13 network_scenario_load_request.seed (+0x06) -> difficulty (campaign difficulty, lands at game globals +0x0e)

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "interface.h"
#include "game.h"
#include "networking.h"
#include "main.h"
#include <stdio.h>
#include <string.h>

extern main_globals main_globals_data; // 0x00719700
extern char map_path_prefix[];         // 0x006f16d8, foreign (cache module)
extern uint8_t main_menu_music_pending; // 0x00718fc6, foreign (interface module)
extern uint8_t ui_input_batch_mode;     // 0x00718fc5, foreign (interface module)
extern float ui_unknown_718fa8;         // 0x00718fa8, foreign (interface module)
extern int32_t interface_loading_screen_address_a;   // 0x0068e680, foreign (interface module)
extern int32_t interface_loading_screen_address_b;   // 0x0068e684, foreign (interface module)
extern int32_t interface_loading_screen_progress;    // 0x00718f90, foreign (interface module)
extern uint16_t progress_screen_text[0x20];    // 0x006b2f28, foreign (types/interface.h); WORD stores only
extern uint16_t progress_screen_subtext[0x20]; // 0x006b2f68, foreign (types/interface.h); WORD stores only
extern int32_t interface_loading_screen_request_id;  // 0x0068e688, foreign (interface module)
extern int32_t interface_loading_screen_ui_state;    // 0x00718f8c, foreign (interface module)
extern game_time_globals *game_time;    // 0x006f1d6c, foreign (game module)
extern int16_t main_pending_difficulty; // 0x00696564, foreign (main-owned global per main.h)
extern int16_t local_player_count;      // 0x006894b8, foreign (saved_games module)

extern int32_t _access(const char *path, int32_t mode); // 0x624236, UNSURE, CRT-shaped; see file header
extern void main_menu_music_stop(void);            // 0x4c8b40, this module
extern void game_scenario_session_begin(network_scenario_load_request *request); // 0x4c95f0, this module
extern void main_menu_on_shown(int32_t fade_milliseconds); // 0x498ab0, foreign (interface module)
extern void player_profile_select_local_slot(int16_t local_player_index); // 0x539cb0, foreign (saved_games module)
extern void cache_file_switch_map_by_path(char *path, uint8_t apply_state); // 0x45aea0, foreign (game module)
    // blam-cc: EAX -> path, BL -> apply_state
extern void game_unload_map(void);         // 0x45afb0, foreign (game module)
extern void game_stop_current_map(void);   // 0x45b370, foreign (game module)

// Drives a queued level change: first confirms the target map file exists (aborting the
// transition if not), resets the loading screen when starting a local (non-networked) session,
// then runs and tracks a 1 second fade-out of the main menu's music/UI (only once the menu is
// actually shown) before proceeding. Once the fade has completed (or there was nothing to fade),
// unless an idle timeout deferred it, stops the map/game engine and starts loading
// scenario_path as a fresh session for every local player.
void main_level_transition_update(void)
{
    if (main_globals_data.level_transition_fade_end_ms == 0) {
        char basename[256];
        char *slash;
        char *name;
        char map_path[512];

        strncpy(basename, main_globals_data.scenario_path, 0xff);
        basename[0xff] = 0;
        slash = strrchr(basename, '\\');
        name = (slash == 0) ? basename : slash + 1;

        sprintf(map_path, "%s%s%s.map", map_path_prefix, "maps\\", name);
        if (_access(map_path, 0) != 0) {
            main_globals_data.level_transition = 0;
            return;
        }
    }

    if (main_globals_data.game_connection == 0) {
        interface_loading_screen_address_a = -1;
        interface_loading_screen_address_b = -1;
        interface_loading_screen_progress = 0;
        progress_screen_text[0] = 0;    // WORD store
        progress_screen_subtext[0] = 0; // WORD store
        interface_loading_screen_request_id = -1;
        interface_loading_screen_ui_state = 1;
    }

    if (main_globals_data.main_menu_scenario_loaded == 1) {
        if (main_globals_data.level_transition_fade_end_ms == 0) {
            if (main_menu_music_pending != 1) {
                goto after_fade;
            }
            main_globals_data.level_transition_fade_end_ms = main_globals_data.frame_time_ms + 1000;
            main_menu_on_shown(1000);
            ui_input_batch_mode = 1;
            ui_unknown_718fa8 = 0.0f;
        } else {
            float remaining = (float)(int32_t)(main_globals_data.level_transition_fade_end_ms -
                                                main_globals_data.frame_time_ms);
            if ((int32_t)(main_globals_data.level_transition_fade_end_ms -
                           main_globals_data.frame_time_ms) < 0) {
                remaining = remaining + 4.2949673e+09f;
            }
            ui_unknown_718fa8 = 1.0f - remaining * 0.001f;
        }
        if (main_globals_data.frame_time_ms < main_globals_data.level_transition_fade_end_ms) {
            return;
        }
    } else {
        main_globals_data.level_transition_fade_end_ms = 0;
    }

after_fade:
    if (main_globals_data.idle_timeout_reached == 0) {
        ui_unknown_718fa8 = -1.0f;
        main_menu_music_stop();
        ui_input_batch_mode = 0;

        if (game_time->initialized != 0 &&
            (game_time->active != 0 || game_time->paused != 0) &&
            main_globals_data.game_connection == 0) {
            network_scenario_load_request request;
            int16_t i;

            memset(&request, 0, sizeof(request));
            request.salt = 0xdeadbeef;
            strncpy(request.map_name, main_globals_data.scenario_path, 0xff);
            request.map_name[0xff] = 0;
            request.difficulty = main_pending_difficulty;

            game_stop_current_map();
            cache_file_switch_map_by_path(request.map_name, 1);   // 0x4c998e mov bl,1 ; lea eax,[esp+0x1c]
            game_unload_map();
            game_scenario_session_begin(&request);

            for (i = 0; i < local_player_count; i++) {
                player_profile_select_local_slot(i);
            }
        }
        main_globals_data.level_transition_fade_end_ms = 0;
        return;
    }

    main_globals_data.return_to_main_menu = 0;
    main_globals_data.unknown_05a = 1;
    main_globals_data.level_transition_fade_end_ms = 0;
}

#if 0
Original Ghidra decompilation (0x4c9770):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl main_level_transition_update(void)

{
  float fVar1;
  char *pcVar2;
  int iVar3;
  char local_214 [4];
  undefined2 local_210;
  undefined2 local_20e;
  undefined4 local_20c;
  char local_208 [255];
  undefined1 local_109;
  char local_104 [254];
  undefined1 local_6;

  if (DAT_00719748 == 0) {
    _strncpy(local_104,&DAT_00719779,0xff);
    local_6 = 0;
    pcVar2 = _strrchr(local_104,0x5c);
    if (pcVar2 == (char *)0x0) {
      pcVar2 = local_104;
    }
    else {
      pcVar2 = pcVar2 + 1;
    }
    _sprintf(local_214,"%s%s%s.map",&DAT_006f16d8,"maps\\",pcVar2);
    iVar3 = FUN_00624236(local_214,0);
    if (iVar3 != 0) {
      DAT_00719739 = 0;
      return;
    }
  }
  if (DAT_00719720 == 0) {
    DAT_0068e680 = 0xffffffff;
    DAT_0068e684 = 0xffffffff;
    DAT_00718f90 = 0;
    _DAT_006b2f28 = 0;
    _DAT_006b2f68 = 0;
    DAT_0068e688 = 0xffffffff;
    DAT_00718f8c = 1;
  }
  if (DAT_00719754._2_1_ == '\x01') {
    if (DAT_00719748 == 0) {
      if (DAT_00718fc6 != '\x01') goto LAB_004c98a4;
      DAT_00719748 = DAT_00719708 + 1000;
      FUN_00498ab0(1000);
      DAT_00718fc5 = 1;
      _DAT_00718fa8 = 0.0;
    }
    else {
      fVar1 = (float)(int)(DAT_00719748 - DAT_00719708);
      if ((int)(DAT_00719748 - DAT_00719708) < 0) {
        fVar1 = fVar1 + 4.2949673e+09;
      }
      _DAT_00718fa8 = 1.0 - fVar1 * 0.001;
    }
    if (DAT_00719708 < DAT_00719748) {
      return;
    }
  }
  else {
    DAT_00719748 = 0;
  }
LAB_004c98a4:
  if (DAT_00719759 == '\0') {
    _DAT_00718fa8 = -1.0;
    main_menu_music_stop();
    DAT_00718fc5 = 0;
    if ((*DAT_006f1d6c != '\0') &&
       (((DAT_006f1d6c[1] != '\0' || (DAT_006f1d6c[2] != '\0')) && (DAT_00719720 == 0)))) {
      pcVar2 = local_214;
      for (iVar3 = 0x43; iVar3 != 0; iVar3 = iVar3 + -1) {
        pcVar2[0] = '\0';
        pcVar2[1] = '\0';
        pcVar2[2] = '\0';
        pcVar2[3] = '\0';
        pcVar2 = pcVar2 + 4;
      }
      local_210 = 0;
      local_20e = 1;
      local_20c = 0xdeadbeef;
      _strncpy(local_208,&DAT_00719779,0xff);
      local_109 = 0;
      local_20e = DAT_00696564;
      FUN_0045b370();
      FUN_0045aea0();
      FUN_0045afb0();
      game_scenario_session_begin();
      iVar3 = 0;
      if (0 < DAT_006894b8) {
        do {
          FUN_00539cb0(iVar3);
          iVar3 = iVar3 + 1;
        } while ((short)iVar3 < DAT_006894b8);
      }
    }
    DAT_00719748 = 0;
    return;
  }
  DAT_00719754._3_1_ = 0;
  DAT_0071975a = 1;
  DAT_00719748 = 0;
  return;
}
#endif
