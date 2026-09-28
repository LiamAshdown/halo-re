// chimera__load_ui_map  (Ghidra: chimera__load_ui_map, already named)
// address 0x4c8930, size 303 bytes
// name confidence: 0.6   rewrite confidence: 0.75
// evidence: out/phase4/main_functions.md summary; the "levels\\ui\\ui" string. Confirmed
// against objdump -d -M intel bin/halo.exe at 0x4c8930..0x4c8a5e: `mov ebx,0x1` at entry means
// every `mov byte ptr ds:<addr>,bl` in this function writes 1, not 0 (Ghidra's own
// `DAT_x = 1;` lines already say so). The network_scenario_load_request built on the stack
// (types/networking.h, size 0x10c) is filled the same way src/networking/
// network_game_scenario_load_request.c's already-committed rewrite fills its own copy: zero,
// then unknown_04=0, seed=1, salt=0xdeadbeef, map_name = strncpy(..., "levels\\ui\\ui", 0xff)
// (map_name[0xff] cleared separately). main_game_globals (0x006b0b80) and
// game_scenario_session_begin's EAX -> request convention are reused from that same evidence and
// from main_types_notes.md. current_game_engine (0x006f1d20, game_engine_definition *) and its
// dispose function pointer (+0x08) are established in src/camera/camera_track_compute_pov.c and
// types/game.h. hs_camera_control_pointer (0x0087bc0c), camera_script (0x006869d0,
// camera_script_globals) and directors[0] (0x006ac560, director, pov_proc/look_scale/unknown_c0
// at +0x08/+0xc4/+0xc0) are established in src/camera/camera_control.c and types/camera.h.
// cache_file_switch_map_by_path (0x45aea0, EAX -> path, EBX -> apply_state),
// game_stop_current_map (0x45b370) and game_unload_map (0x45afb0) are established in
// src/game/cache_file_switch_map_by_path.c, game_stop_current_map.c and game_unload_map.c.
// game_time (0x006f1d6c) reuses src/interface/display_error.c's name.
// register convention: cdecl, one recognized parameter (play_title_music).
// phase 4 review (disassembly 0x4c8930..0x4c8a5e): the SECOND call to
// cache_file_switch_map_by_path (0x4c898a) passes EAX = &request.map_name (`lea eax,[esp+0x1c]`
// with the request at [esp+0x10]) and BL = 1 still; predicted_resource_list_touch takes ESI =
// &global_scenario->predicted_resources (+0xec); camera_debug_start gets AX = 0 and the stack
// (0, -1); 0x0071976b is main_globals.unknown_06b.
// UNSURE: 0x006f1d38 (a gating byte) and the two blocks it and this function unconditionally
// zero (0x006b0b88, 0xc0 dwords; 0x0087ab20, 0x26 dwords) have no established name or type
// anywhere in the codebase; declared here as opaque byte blocks with TYPES-GAP-style names.
// reconciled: R13 network_scenario_load_request.seed (+0x06) -> difficulty (campaign difficulty, lands at game globals +0x0e)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "interface.h"
#include "game.h"
#include "camera.h"
#include "networking.h"
#include "main.h"
#include <string.h>

extern main_globals main_globals_data; // 0x00719700
extern Scenario *global_scenario;      // 0x00746f8c, foreign (game module)
extern game_engine_definition *current_game_engine; // 0x006f1d20, foreign (ai/camera modules)
extern uint8_t player_profile_cache_initialized;          // 0x006f1d38, TYPES-GAP, UNSURE identity
extern uint8_t player_profile_cache[0xc0 * 4]; // 0x006b0b88, TYPES-GAP, UNSURE identity/type (0xc0 dwords)
extern uint8_t game_engine_active_variant[0x26 * 4]; // 0x0087ab20, TYPES-GAP, UNSURE identity/type (0x26 dwords)
extern uint8_t *hs_camera_control_pointer; // 0x0087bc0c, foreign (camera module)
extern camera_script_globals camera_script; // 0x006869d0, foreign (camera module)
extern director directors[1];              // 0x006ac560, foreign (camera module)
extern uint8_t ui_split_screen;            // 0x00718fc9, foreign (interface module)
extern uint8_t *main_game_globals;     // 0x006b0b80, foreign (networking module), UNSURE
                                            // identity/type; see network_game_scenario_load_request.c

extern void cache_file_switch_map_by_path(char *path, uint8_t apply_state); // 0x45aea0, foreign (game module)
    // blam-cc: EAX -> path, BL -> apply_state
extern void game_stop_current_map(void);   // 0x45b370, foreign (game module)
extern void game_unload_map(void);         // 0x45afb0, foreign (game module)
extern void game_scenario_session_begin(network_scenario_load_request *request); // 0x4c95f0, this module
extern void camera_debug_start(int16_t camera_point_index, int16_t ticks,
                                datum_index relative_object); // 0x444c00, foreign (camera module)
extern void camera_debug_compute_pov(director_camera_data *data, camera_input *input,
                                      observer_command *command); // 0x444d50, foreign (camera module)
extern void predicted_resource_list_touch(TagReflexive *resources); // 0x4449f0, foreign (cache); blam-cc: ESI -> resources

// Loads the front-end map ("levels\\ui\\ui") as a scenario session: switches the cache file to
// it, tears down whatever map/game engine was running, resets the camera to the scripted
// (cutscene) director, and starts game_scenario_session_begin on a freshly built scenario load
// request. Optionally starts the main menu title music afterward.
void chimera__load_ui_map(char play_title_music)
{
    network_scenario_load_request request;

    cache_file_switch_map_by_path("levels\\ui\\ui", 1);

    memset(&request, 0, sizeof(request));
    request.difficulty = 1;
    request.salt = 0xdeadbeef;
    strncpy(request.map_name, "levels\\ui\\ui", 0xff);
    request.map_name[0xff] = 0;

    cache_file_switch_map_by_path(request.map_name, 1);  // EAX = &request.map_name, BL = 1
    game_stop_current_map();
    game_unload_map();

    if (current_game_engine != 0) {
        if (current_game_engine->dispose != 0) {
            ((void (*)(void))current_game_engine->dispose)();
        }
        current_game_engine = 0;
    }
    if (player_profile_cache_initialized == 1) {
        memset(player_profile_cache, 0, sizeof(player_profile_cache));
        player_profile_cache_initialized = 0;
    }
    memset(game_engine_active_variant, 0, sizeof(game_engine_active_variant));

    main_globals_data.main_menu_scenario_loaded = 1;
    game_scenario_session_begin(&request);

    *hs_camera_control_pointer = 1;
    directors[0].pov_proc = camera_debug_compute_pov;
    directors[0].look_scale = 1.0f;
    directors[0].unknown_c0 = 0;
    camera_script.camera_control = 1;
    camera_script.changed = 1;
    camera_debug_start(0, 0, (datum_index)-1);

    ui_split_screen = 1;
    main_globals_data.unknown_06b = 1;

    if (play_title_music != 0 && global_scenario != 0) {
        predicted_resource_list_touch(&global_scenario->predicted_resources); // ESI
    }
}

#if 0
Original Ghidra decompilation (0x4c8930):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl chimera__load_ui_map(char play_title_music)

{
  int iVar1;
  undefined4 *puVar2;
  undefined4 local_110;
  undefined2 local_10c;
  undefined2 local_10a;
  undefined4 local_108;
  char local_104 [255];
  undefined1 local_5;

  FUN_0045aea0();
  puVar2 = &local_110;
  for (iVar1 = 0x43; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar2 = 0;
    puVar2 = puVar2 + 1;
  }
  local_10c = 0;
  local_10a = 1;
  local_108 = 0xdeadbeef;
  _strncpy(local_104,"levels\\ui\\ui",0xff);
  local_5 = 0;
  FUN_0045aea0();
  FUN_0045b370();
  FUN_0045afb0();
  if (DAT_006f1d20 != 0) {
    if (*(code **)(DAT_006f1d20 + 8) != (code *)0x0) {
      (**(code **)(DAT_006f1d20 + 8))();
    }
    DAT_006f1d20 = 0;
  }
  if (DAT_006f1d38 == '\x01') {
    puVar2 = &DAT_006b0b88;
    for (iVar1 = 0xc0; iVar1 != 0; iVar1 = iVar1 + -1) {
      *puVar2 = 0;
      puVar2 = puVar2 + 1;
    }
    DAT_006f1d38 = '\0';
  }
  puVar2 = &DAT_0087ab20;
  for (iVar1 = 0x26; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar2 = 0;
    puVar2 = puVar2 + 1;
  }
  DAT_00719754._2_1_ = 1;
  game_scenario_session_begin();
  *DAT_0087bc0c = 1;
  DAT_006ac568 = camera_debug_compute_pov;
  _DAT_006ac624 = 0x3f800000;
  DAT_006ac620 = 0;
  DAT_006869d0 = 1;
  DAT_006869d1 = 1;
  camera_debug_start(0,0xffffffff);
  DAT_00718fc9 = 1;
  DAT_0071976b = 1;
  if ((play_title_music != '\0') && (global_scenario != 0)) {
    predicted_resource_list_touch();
  }
  return;
}
#endif
