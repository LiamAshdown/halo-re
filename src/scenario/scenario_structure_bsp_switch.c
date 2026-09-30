// scenario_structure_bsp_switch  (Ghidra: already named scenario_structure_bsp_switch)
// address 0x53eeb0, size 259 bytes
// name confidence: 0.65   rewrite confidence: 0.8
// evidence: out/phase4/scenario_functions.md's own summary ("Switches the currently active
//   structure bsp to the requested index, running deactivate/activate callback tables and
//   refreshing the cached bsp tag-data pointers") and out/phase4/scenario_types_notes.md's
//   globals table (0x00746f8c global_scenario, 0x00746f90 global_collision_bsp, 0x00746f94
//   global_scenario_game_globals, 0x00746f98 global_structure_collision_bsp, 0x00746f9c
//   global_structure_bsp, 0x0069e8d8 global_structure_bsp_index, ScenarioBSP.structure_bsp.tag_id
//   +0x1c). Fully re-derived from objdump -d -M intel --start-address=0x53eeb0
//   --stop-address=0x53efc0 bin/halo.exe: this recovered the one register argument Ghidra could
//   not name -- structure_bsp_load (0x4424b0) is called with EDI already pointing at
//   &global_scenario->structure_bsps.pointer[bsp_index] (computed before the old bsp's teardown
//   and left untouched by the intervening calls, matching structure_bsp_load's own documented
//   "ScenarioBSP *bsp in EDI" convention in src/cache/structure_bsp_load.c) -- and confirmed
//   structure_bsp_dispose's argument is the OLD bsp entry, pushed on the stack, matching its own
//   documented stack-parameter convention in src/cache/structure_bsp_dispose.c.
// register convention: SI -> structure_bsp_index (int16_t); no stack parameters. EBX and EDI are
//   used internally across the two callee-invoking blocks (the "old bsp was active" flag and the
//   new bsp's ScenarioBSP * respectively) and saved/restored like any other callee-saved
//   register; ECX is pushed only to reserve one dword of stack space for a byte that always
//   holds 0 on every path that reads it back (`mov al,[esp+0xb]` / `mov al,[esp+0xb]` -- verified
//   against objdump: nothing ever writes a different value to that slot), so the two early-return
//   paths below are written as plain `return 0` rather than reproducing the stack indirection.
//   // blam-cc: SI -> structure_bsp_index
// UNSURE: none left in this function's own body.

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "scenario.h"
#include "fn_cache.h"

extern int16_t global_structure_bsp_index;              // 0x0069e8d8
extern Scenario *global_scenario;                        // 0x00746f8c
extern uint8_t unknown_00719769;                          // 0x00719769, UNSURE: main-loop latch,
                                                          //   not owned by this module (see
                                                          //   src/saved_games/game_state_perform_save.c)
extern uint8_t unknown_0071976a;                          // 0x0071976a, UNSURE: main-loop latch,
                                                          //   not owned by this module
extern scenario_game_globals *global_scenario_game_globals; // 0x00746f94
extern ModelCollisionGeometryBSP *global_structure_collision_bsp; // 0x00746f98
extern ModelCollisionGeometryBSP *global_collision_bsp;   // 0x00746f90
extern ScenarioStructureBSP *global_structure_bsp;         // 0x00746f9c
extern tag_instance *tag_instances;                        // 0x0087bc14

// this module, 0x53e660: runs the 10-slot deactivate table (out of this batch's range)
extern void scenario_structure_bsp_deactivate_callbacks(void);
// this module, 0x53e680: runs the 13-slot activate table (out of this batch's range)
extern void scenario_structure_bsp_activate_callbacks(void);

// blam-cc: EDI -> bsp (see file header)


// blam-cc: SI -> structure_bsp_index
// Switches the resident structure bsp to structure_bsp_index: runs the deactivate table and
// disposes the old bsp (if one was active), loads the new one, republishes its tag data and
// collision bsp pointers, then runs the activate table. No-op when structure_bsp_index is
// already resident, negative, or out of range. Returns whether the switch happened.
uint8_t scenario_structure_bsp_switch(int16_t structure_bsp_index)
{
    ScenarioBSP *new_entry;
    uint8_t old_bsp_was_active;
    uint32_t tag_index;

    if (structure_bsp_index == global_structure_bsp_index || structure_bsp_index < 0) {
        return 0;
    }
    if ((int32_t)global_scenario->structure_bsps.count <= (int32_t)structure_bsp_index) {
        return 0;
    }

    new_entry = &((ScenarioBSP *)global_scenario->structure_bsps.pointer)[structure_bsp_index];
    old_bsp_was_active = (global_structure_bsp_index != -1);

    unknown_00719769 = 0;
    unknown_0071976a = 0;

    if (old_bsp_was_active) {
        ScenarioBSP *old_entry;

        scenario_structure_bsp_deactivate_callbacks();
        old_entry = &((ScenarioBSP *)global_scenario->structure_bsps.pointer)
                        [global_structure_bsp_index];
        structure_bsp_dispose(old_entry);
        global_scenario_game_globals->structure_bsp_index = -1;
        global_structure_bsp_index = -1;
    }

    if (!structure_bsp_load(new_entry)) {
        unknown_0071976a = 1;
        return 0;
    }

    tag_index = new_entry->structure_bsp.tag_id.index;
    global_structure_bsp = (ScenarioStructureBSP *)tag_instances[tag_index].data;
    global_structure_collision_bsp =
        (ModelCollisionGeometryBSP *)global_structure_bsp->collision_bsp.pointer;
    global_collision_bsp = (ModelCollisionGeometryBSP *)global_structure_bsp->collision_bsp.pointer;
    global_scenario_game_globals->structure_bsp_index = structure_bsp_index;
    global_structure_bsp_index = structure_bsp_index;

    if (old_bsp_was_active) {
        scenario_structure_bsp_activate_callbacks();
    }

    unknown_0071976a = 1;
    return 1;
}

#if 0
Original Ghidra decompilation (0x53eeb0):

undefined1 scenario_structure_bsp_switch(void)

{
  int iVar1;
  char cVar2;
  short unaff_SI;
  bool bVar3;

  if ((unaff_SI == DAT_0069e8d8) || (unaff_SI < 0)) {
    return 0;
  }
  if (*(int *)(global_scenario + 0x5a4) <= (int)unaff_SI) {
    return 0;
  }
  iVar1 = *(int *)(global_scenario + 0x5a8);
  bVar3 = DAT_0069e8d8 != -1;
  DAT_00719769 = 0;
  DAT_0071976a = 0;
  if (bVar3) {
    FUN_0053e660();
    structure_bsp_dispose(DAT_0069e8d8 * 0x20 + *(int *)(global_scenario + 0x5a8));
    *DAT_00746f94 = -1;
    DAT_0069e8d8 = -1;
  }
  cVar2 = structure_bsp_load();
  if (cVar2 != '\0') {
    DAT_00746f9c = *(int *)((*(uint *)(unaff_SI * 0x20 + iVar1 + 0x1c) & 0xffff) * 0x20 + 0x14 +
                           DAT_0087bc14);
    DAT_00746f90 = *(undefined4 *)(DAT_00746f9c + 0xb4);
    DAT_00746f98 = DAT_00746f90;
    *DAT_00746f94 = unaff_SI;
    DAT_0069e8d8 = unaff_SI;
    if (bVar3) {
      FUN_0053e680();
    }
    DAT_0071976a = 1;
    return 1;
  }
  DAT_0071976a = 1;
  return 0;
}
#endif
