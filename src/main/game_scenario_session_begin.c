// game_scenario_session_begin  (Ghidra: game_scenario_session_begin, already named)
// address 0x4c95f0, size 384 bytes
// name confidence: 0.5   rewrite confidence: 0.75
// evidence: out/phase4/main_types_notes.md "Register arguments confirmed at call sites":
// "game_scenario_session_begin 0x4c95f0: EAX = network_scenario_load_request
// (0x4c89de, 0x4c999e)". scenario_load_staging (0x006b0b80) and scenario_load's signature reuse
// src/networking/network_game_scenario_load_request.c; FUN_0045aea0/_0045b050/_0045b8b0/
// _00470ae0 reuse that same file's externs. Every 0x00719738..0x00719753 field is main_globals
// (types/main.h, offsets 0x038..0x053), confirmed the same way as
// src/main/main_queue_map_change.c's correction (not any other module's less-informed guess for
// an overlapping address). last_activity_time_ms (0x00719764) and restore_checkpoint_on_load
// (0x00719778) are main_globals fields too. ui_pause_pending_count_00718fa0 (0x00718fa0),
// interface_loading_screen_ui_state (0x00718f8c) and interface_loading_screen_address_a/_b
// (0x0068e680/4) reuse src/interface/ui_check_for_pause_game.c and
// src/networking/network_join_request_resolve_host.c's names. network_game_mode (0x00719720)
// and game_state_load_checkpoint (0x538280) are established elsewhere in this codebase.
// register convention: EAX -> request (network_scenario_load_request *).
// phase 4 review (disassembly 0x4c95f0..0x4c976f): 0x45aea0 is cache_file_switch_map_by_path
// with EAX = request->map_name and BL = 1, scenario_load takes EAX = request->map_name (not the
// staging block), and 0x0087ac08 is a WORD store; the phase 3 file had all three wrong.
// UNSURE: 0x0087ac00/0x0087ac08 have no established name anywhere; declared as opaque bytes.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "interface.h"
#include "game.h"
#include "networking.h"
#include "main.h"
#include <string.h>

extern main_globals main_globals_data; // 0x00719700
extern uint8_t game_globals_initialized_flag;    // 0x0087ac00, TYPES-GAP, UNSURE identity
extern int16_t game_globals_unknown_0087ac08;    // 0x0087ac08, TYPES-GAP, UNSURE identity (WORD store)
extern int32_t ui_pause_pending_count_00718fa0;  // 0x00718fa0, foreign (interface module)
extern int32_t interface_loading_screen_ui_state;    // 0x00718f8c, foreign (interface module)
extern int32_t interface_loading_screen_address_a;   // 0x0068e680, foreign (interface module)
extern int32_t interface_loading_screen_address_b;   // 0x0068e684, foreign (interface module)
extern uint8_t *scenario_load_staging;           // 0x006b0b80, foreign (networking module)

extern void input_reset_state_and_axis_configs(void); // 0x490aa0, foreign (input module)
extern void input_bind_capture_reset(void);            // 0x48b5f0, foreign (input module)
extern void cache_file_switch_map_by_path(char *path, uint8_t apply_state); // 0x45aea0, foreign (game module)
    // blam-cc: EAX -> path, BL -> apply_state
extern void game_start_new_map(void); // 0x45b050, foreign (game module)
extern void game_engine_reset_all_players(void); // 0x45b8b0, foreign (game module)
extern void game_engine_init_tick_record_for_mode(void); // 0x470ae0, foreign (game module)
extern void main_ensure_local_players(void);    // 0x4c8800, this module
extern char scenario_load(char *scenario_path); // 0x53e6a0, foreign (game module)
    // blam-cc: EAX -> scenario_path (0x4c961d mov eax,ebp; the callee hands EAX to 0x442290)
extern void game_state_load_checkpoint(void);    // 0x538280, foreign (game module)
extern uint32_t time_query_performance_counter_ms(void); // 0x449210, foreign (math module)
extern int64_t performance_counter_frequency; // 0x006ac8f8, foreign (math module)

// Loads a scenario per `request`, seeds the game timer and pending-pause bookkeeping, and (on
// the first session only) ensures the local players exist. Copies `request` into the persistent
// scenario_load_staging buffer, calls scenario_load, resets the level/save/won/lost/respawn/
// core-load request flags, re-baselines last_activity_time_ms from QueryPerformanceCounter,
// restores a checkpoint if one was requested, and -- outside a networked game with the loading
// screen up -- re-arms the loading screen's minimum-display-time deadline.
void game_scenario_session_begin(network_scenario_load_request *request)
{
    int64_t counter;
    int64_t counter_ms;
    uint8_t already_initialized;

    input_reset_state_and_axis_configs();
    input_bind_capture_reset();
    cache_file_switch_map_by_path(request->map_name, 1);   // 0x4c9603 lea ebp,[esi+0xc] ; mov bl,1

    memcpy(scenario_load_staging + 8, request, sizeof(network_scenario_load_request));

    if (scenario_load(request->map_name) == 0) {
        if (*scenario_load_staging == 0) {
            goto after_load;
        }
    } else {
        *scenario_load_staging = 1;
    }
    game_start_new_map();

after_load:
    already_initialized = game_globals_initialized_flag != 0;
    game_globals_initialized_flag = 0;
    game_globals_unknown_0087ac08 = 0;
    if (!already_initialized) {
        main_ensure_local_players();
        game_engine_init_tick_record_for_mode();
    }
    game_engine_reset_all_players();

    main_globals_data.switch_structure_bsp_index = -1;
    main_globals_data.reset_map = 0;
    main_globals_data.level_transition = 0;
    main_globals_data.revert_map = 0;
    main_globals_data.revert_map_if_allowed = 0;
    main_globals_data.save_map = 0;
    main_globals_data.won_map = 0;
    main_globals_data.lost_map = 0;
    main_globals_data.respawn_coop_players = 0;
    main_globals_data.save_core = 0;
    main_globals_data.load_core = main_globals_data.load_core_next_session;
    main_globals_data.load_core_next_session = 0;

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    counter_ms = counter * 1000;
    main_globals_data.last_activity_time_ms = (int32_t)(counter_ms / performance_counter_frequency);

    if (main_globals_data.restore_checkpoint_on_load != 0) {
        game_state_load_checkpoint();
    }

    ui_pause_pending_count_00718fa0 = 0x1e;
    if (main_globals_data.game_connection == 0 && interface_loading_screen_ui_state != 0) {
        int32_t now = time_query_performance_counter_ms();
        uint32_t extra = 0;

        if (interface_loading_screen_address_b != -1) {
            uint32_t elapsed = (uint32_t)(now - interface_loading_screen_address_b);
            if (elapsed < 2000 && interface_loading_screen_ui_state != 1) {
                extra = (uint32_t)(interface_loading_screen_address_b - now) + 2000;
                if (extra > 2000) {
                    extra = 2000;
                }
            }
        }
        interface_loading_screen_address_a = (int32_t)(extra + 0x6d6 + now);
    }
}

#if 0
Original Ghidra decompilation (0x4c95f0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void game_scenario_session_begin(void)

{
  undefined4 *in_EAX;
  uint uVar1;
  int iVar2;
  char *pcVar3;
  bool bVar4;
  undefined8 uVar5;
  LARGE_INTEGER local_c;

  input_reset_state_and_axis_configs();
  input_bind_capture_reset();
  FUN_0045aea0();
  pcVar3 = DAT_006b0b80 + 8;
  for (iVar2 = 0x43; iVar2 != 0; iVar2 = iVar2 + -1) {
    *(undefined4 *)pcVar3 = *in_EAX;
    in_EAX = in_EAX + 1;
    pcVar3 = pcVar3 + 4;
  }
  uVar1 = scenario_load();
  if ((char)uVar1 == '\0') {
    if (*DAT_006b0b80 == '\0') goto LAB_004c9645;
  }
  else {
    *DAT_006b0b80 = '\x01';
  }
  FUN_0045b050();
LAB_004c9645:
  bVar4 = DAT_0087ac00 == '\0';
  DAT_0087ac00 = 0;
  _DAT_0087ac08 = 0;
  if (bVar4) {
    FUN_004c8800();
    FUN_00470ae0();
  }
  FUN_0045b8b0();
  DAT_00719754._0_2_ = 0xffff;
  DAT_00719738 = 0;
  DAT_00719739 = 0;
  DAT_0071973a = 0;
  DAT_0071973b = 0;
  DAT_0071973c = 0;
  DAT_0071974e = 0;
  DAT_0071974f = 0;
  DAT_00719750 = 0;
  DAT_00719751 = 0;
  DAT_00719752 = DAT_00719753;
  DAT_00719753 = 0;
  QueryPerformanceCounter(&local_c);
  uVar5 = __allmul(local_c.s.LowPart,local_c.s.HighPart,1000,0);
  DAT_00719764 = __alldiv(uVar5,DAT_006ac8f8,DAT_006ac8fc);
  if (DAT_00719778 != '\0') {
    game_state_load_checkpoint();
  }
  DAT_00718fa0 = 0x1e;
  if ((DAT_00719720 == 0) && (DAT_00718f8c != 0)) {
    iVar2 = FUN_00449210();
    uVar1 = 0;
    if ((DAT_0068e684 != -1) &&
       ((((uint)(iVar2 - DAT_0068e684) < 2000 && (DAT_00718f8c != 1)) &&
        (uVar1 = (DAT_0068e684 - iVar2) + 2000, 2000 < uVar1)))) {
      uVar1 = 2000;
    }
    DAT_0068e680 = uVar1 + 0x6d6 + iVar2;
  }
  return;
}
#endif
