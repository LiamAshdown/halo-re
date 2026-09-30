// engine_shutdown_subsystems  (Ghidra: engine_shutdown_subsystems, already named)
// address 0x541010, size 177 bytes
// name confidence: 0.65   rewrite confidence: 0.75
// evidence: mirrors engine_initialize_subsystems' subsystem list in reverse (cache, input,
// rasterizer, periodic function tables, data file, sound) plus timeEndPeriod(1) undoing
// timeBeginPeriod(1).
// register convention: __cdecl, no arguments, no return value.
// UNSURE: every global this function clears belongs to another module (hs / hs / structures /
// sound, per shell.h's notes and the conflicting claims across types/hs.h, types/rasterizer.h,
// types/structures.h and types/physics.h for the same addresses), so they are declared here only
// as untyped externs named after their address, not the contested cross-module names.
// reconciled: R06 external_00746f9c -> ScenarioStructureBSP *global_structure_bsp (scenario.h); R07 0x00746f94 comment: scenario_game_globals *, not sound

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"
#include "fn_sound.h"
#include "fn_math.h"
#include "fn_cache.h"

extern uint32_t global_scenario_index;   // hs.h: datum_index global_scenario_index, -1 = none
extern uint16_t global_structure_bsp_index;   // physics.h/items.h: int16_t structure bsp index
extern uint16_t *global_scenario_game_globals;  // scenario_game_globals * (0x7c-byte scenario game-state block; only +0x30..+0x7b is sound state)
extern uint32_t global_scenario;   // hs.h: Scenario *global_scenario
extern ScenarioStructureBSP *global_structure_bsp; // 0x00746f9c, scenario.h (the resident structure BSP)
extern uint32_t global_structure_collision_bsp;   // items.h: void *collision_bsp_globals
extern uint32_t global_collision_bsp;   // hs.h: void *global_globals (structures.h disagrees)
extern uint32_t global_globals;   // rasterizer.h: void *global_globals (Globals tag data)
extern void *sphere_point_table;      // GlobalFree argument, owner unknown
extern uint32_t external_00686b4c;   // cleared to -1
extern uint8_t external_00686b50;    // cleared to 0 (byte store, mov byte [0x686b50],bl at 0x54108d)
extern void *external_00686b58;      // GlobalFree'd if non-null, then cleared
extern void *external_00686b5c;      // GlobalFree'd if non-null, then cleared
extern uint32_t external_00686b54;   // cleared to 0


extern void input_directinput_release_devices(void);      // 0x00490580

extern void rasterizer_shutdown(void);                    // 0x00518450


// Tears down the engine subsystems started by engine_initialize_subsystems: unloads the cache
// file, shuts down the rasterizer, disposes sound, and releases assorted global buffers.
void engine_shutdown_subsystems(void)
{
    cache_file_unload();
    global_scenario_index = 0xffffffff;
    global_structure_bsp_index = 0xffff;
    *global_scenario_game_globals = 0xffff;
    global_scenario = 0;
    global_structure_bsp = 0;
    global_structure_collision_bsp = 0;
    global_collision_bsp = 0;
    global_globals = 0;

    input_directinput_release_devices();
    rasterizer_shutdown();
    GlobalFree(sphere_point_table);
    periodic_function_tables_free();
    data_file_close();
    sound_dispose();

    external_00686b4c = 0xffffffff;
    external_00686b50 = 0;
    if (external_00686b58 != 0) {
        GlobalFree(external_00686b58);
    }
    if (external_00686b5c != 0) {
        GlobalFree(external_00686b5c);
    }
    external_00686b54 = 0;
    external_00686b58 = 0;
    external_00686b5c = 0;

    timeEndPeriod(1);
}

#if 0
Original Ghidra decompilation (0x541010):


/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl engine_shutdown_subsystems(void)

{
  cache_file_unload();
  DAT_0069e8d4 = 0xffffffff;
  DAT_0069e8d8 = 0xffff;
  *DAT_00746f94 = 0xffff;
  global_scenario = 0;
  DAT_00746f9c = 0;
  DAT_00746f98 = 0;
  DAT_00746f90 = 0;
  DAT_00746fa0 = 0;
  input_directinput_release_devices();
  rasterizer_shutdown();
  GlobalFree(DAT_006b7af4);
  periodic_function_tables_free();
  data_file_close();
  sound_dispose();
  _DAT_00686b4c = 0xffffffff;
  DAT_00686b50 = 0;
  if (DAT_00686b58 != (HGLOBAL)0x0) {
    GlobalFree(DAT_00686b58);
  }
  if (DAT_00686b5c != (HGLOBAL)0x0) {
    GlobalFree(DAT_00686b5c);
  }
  _DAT_00686b54 = 0;
  DAT_00686b58 = (HGLOBAL)0x0;
  DAT_00686b5c = (HGLOBAL)0x0;
  timeEndPeriod(1);
  return;
}
#endif
