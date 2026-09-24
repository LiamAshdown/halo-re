// actor_reject_firing_position_unreachable  (Ghidra: actor_reject_firing_position_unreachable, renamed)
// address 0x412290, size 184 bytes
// name confidence: 0.5   rewrite confidence: 0.7
// evidence: row 0 of the rejection table at 0x006555f8 with the kinds mask 0xffff, so it
//   runs for every goal kind. Its whole body is gated on query.flying (0x44), which is the
//   case the path-based candidate filling in actor_find_best_firing_position skips, and it
//   falls back on the two direct reachability helpers 0x41aab0 and 0x43a0a0.
// register convention: actor_index, the query and the candidate are the three
//   Ghidra-recognized stack parameters; the two helpers take theirs in registers.
// reconciled: R06 0x00746f9c is ScenarioStructureBSP *global_structure_bsp (was extern int32_t bsp_generation); ai.h path_find_context/actor_movement_context bsp_generation -> structure_bsp, bsp_index -> collision_bsp

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *actor_data; // 0x00880360
extern ScenarioStructureBSP *global_structure_bsp; // 0x00746f9c, scenario.h (formerly bsp_generation)

extern uint8_t actor_movement_flying_needs_steering(void);                              // 0x41aab0, not yet rewritten: step / drop test
extern uint8_t path_find_test_direct_reachability(int32_t generation, int32_t unused); // 0x43a0a0, not yet rewritten: direct-line reachability

// blam-cc: stack -> actor_index, query, candidate
// The always-on rejection rule. Ground actors pass unconditionally because their candidates
// were already filled in by a real pathfind. A flying actor has to prove it can fly to the
// position: if both direct tests pass the candidate keeps its place with a flat 15.0
// penalty, otherwise it is rejected (or merely marked, when the query collects everything).
// The probe call with no candidate just charges the baseline the same 15.0.
uint8_t actor_reject_firing_position_unreachable(datum_index actor_index,
                                                 actor_firing_position_query *query,
                                                 actor_firing_position_candidate *candidate)
{
    if (query->flying != 0) {
        if (candidate == (actor_firing_position_candidate *)0) {
            query->baseline_penalty = query->baseline_penalty + 15.0f;
            return 1;
        }
        // UNSURE: both helpers are called with no visible arguments. The candidate position
        // and the actor are the only live values, so they are what is being tested.
        if (actor_movement_flying_needs_steering() != 0 && path_find_test_direct_reachability((uint32_t)global_structure_bsp, 0) != 0) {
            candidate->score = candidate->score + 15.0f;
            return candidate->valid;
        }
        candidate->rejected = 1;
        if (query->collect_all == 0) {
            candidate->valid = 0;
        }
    }

    if (candidate == (actor_firing_position_candidate *)0) {
        return 1;
    }
    return candidate->valid;
}

#if 0
Original Ghidra decompilation (0x412290):

undefined8 FUN_00412290(undefined4 param_1,int param_2,int param_3)

{
  char cVar1;
  undefined4 extraout_EDX;
  undefined4 extraout_EDX_00;
  undefined4 uVar2;

  uVar2 = *(undefined4 *)(DAT_00880360 + 0x34);
  if (*(char *)(param_2 + 0x44) != '\0') {
    if (param_3 == 0) {
      *(float *)(param_2 + 0x660) = *(float *)(param_2 + 0x660) + 15.0;
      return CONCAT44(uVar2,1);
    }
    param_1 = 0;
    cVar1 = FUN_0041aab0();
    uVar2 = extraout_EDX;
    if (cVar1 != '\0') {
      cVar1 = FUN_0043a0a0(DAT_00746f9c,0);
      uVar2 = extraout_EDX_00;
      if (cVar1 != '\0') {
        *(float *)(param_3 + 0x38) = *(float *)(param_3 + 0x38) + 15.0;
        goto LAB_00412333;
      }
    }
    *(undefined1 *)(param_3 + 0x31) = 1;
    if (*(char *)(param_2 + 0x14) == '\0') {
      *(undefined1 *)(param_3 + 0x30) = 0;
    }
  }
LAB_00412333:
  if (param_3 == 0) {
    return CONCAT44(uVar2,1);
  }
  return CONCAT44(uVar2,(uint)*(byte *)(param_3 + 0x30));
}
#endif
