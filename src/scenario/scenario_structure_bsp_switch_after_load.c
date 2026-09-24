// scenario_structure_bsp_switch_after_load  (Ghidra: FUN_0053efc0, still unnamed there; named
// for this rewrite from out/phase4/scenario_functions.md's summary: "Checks whether a new
// structure bsp has been requested and, if so, unloads the previous one and performs the
// switch.")
// address 0x53efc0, size 96 bytes
// name confidence: 0.55   rewrite confidence: 0.85
// evidence: out/phase4/scenario_types_notes.md's own account of this function under
//   scenario_game_globals: "0x53efc0 compares [structure_bsp_index] against 0x0069e8d8 after a
//   game-state load and re-switches when they differ, which is why the index is kept in the game
//   state at all" -- i.e. this is the game-state-load counterpart of
//   scenario_structure_bsp_switch, resyncing the resident bsp to whatever a loaded save expects.
//   Confirmed instruction by instruction against objdump -d -M intel
//   --start-address=0x53efc0 --stop-address=0x53f020 bin/halo.exe: structure_bsp_data is loaded
//   into EAX immediately before the call at 0x53efeb, matching
//   structure_bsp_dispose_material_vertex_buffers's documented EAX convention
//   (src/cache/structure_bsp_dispose.c); the tail call to scenario_structure_bsp_switch at
//   0x53f019 passes the game state's (re-read, post-clear) structure_bsp_index in SI, matching
//   that function's own documented SI convention (src/scenario/scenario_structure_bsp_switch.c).
// register convention: no register or stack parameters.
// caller: no direct call; its only reference is slot 0 of game_state_after_load_procs
//   (0x0069e7b4, types/saved_games.h), found by scanning the image for the dword 0x0053efc0.
// UNSURE: none left in this function's own body.

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "scenario.h"

extern int16_t global_structure_bsp_index;                  // 0x0069e8d8
extern scenario_game_globals *global_scenario_game_globals; // 0x00746f94
extern Scenario *global_scenario;                            // 0x00746f8c
extern void *structure_bsp_data;                             // 0x006a8958
extern tag_instance *tag_instances;                           // 0x0087bc14

// blam-cc: EAX -> compiled_header
extern void structure_bsp_dispose_material_vertex_buffers(
    ScenarioStructureBSPCompiledHeader *compiled_header); // src/cache module, 0x4431a0

// blam-cc: SI -> structure_bsp_index
extern uint8_t scenario_structure_bsp_switch(int16_t structure_bsp_index); // this module, 0x53eeb0

// Resyncs the resident structure bsp to the game state after a save-game load: when the loaded
// scenario_game_globals->structure_bsp_index no longer matches the currently resident bsp, tears
// down the old bsp's material vertex buffers and tag data pointer directly (without running the
// deactivate/activate callback tables scenario_structure_bsp_switch would use for a live switch),
// then performs the switch to the requested bsp.
void scenario_structure_bsp_switch_after_load(void)
{
    ScenarioBSP *old_entry;
    int32_t tag_index;
    int16_t requested_index;

    if (global_scenario_game_globals->structure_bsp_index == global_structure_bsp_index) {
        return;
    }

    old_entry = &((ScenarioBSP *)global_scenario->structure_bsps.pointer)
                    [global_structure_bsp_index];
    structure_bsp_dispose_material_vertex_buffers(
        (ScenarioStructureBSPCompiledHeader *)structure_bsp_data);

    tag_index = (int16_t)old_entry->structure_bsp.tag_id.index; // movsx at 0x53eff0, unlike the
                                                                //   and 0xffff of every other lookup
    tag_instances[tag_index].data = 0;
    structure_bsp_data = 0;

    requested_index = global_scenario_game_globals->structure_bsp_index;
    global_structure_bsp_index = -1;
    scenario_structure_bsp_switch(requested_index);
}

#if 0
Original Ghidra decompilation (0x53efc0):

void FUN_0053efc0(void)

{
  int iVar1;
  int iVar2;

  if (*DAT_00746f94 != DAT_0069e8d8) {
    iVar1 = *(int *)(global_scenario + 0x5a8);
    iVar2 = (int)DAT_0069e8d8;
    structure_bsp_dispose_material_vertex_buffers();
    *(undefined4 *)(*(short *)(iVar2 * 0x20 + iVar1 + 0x1c) * 0x20 + 0x14 + DAT_0087bc14) = 0;
    DAT_006a8958 = 0;
    DAT_0069e8d8 = -1;
    scenario_structure_bsp_switch();
  }
  return;
}
#endif
