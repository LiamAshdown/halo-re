// ai_reference_face_starting_location  (Ghidra: ai_reference_face_starting_location; named for this rewrite)
// address 0x4349d0, size 276 bytes
// name confidence: 0.35   rewrite confidence: 0.5
// evidence: phase-4 summary ("orients idle squad members to face a designer-specified
//   default direction taken from their squad placement data"). For each actor a packed ai
//   reference names it draws a starting location through
//   squad_pick_random_starting_location @0x437220 and re-orients the unit to that location's
//   ScenarioActorStartingLocation.facing (+0x0c of the 0x1c-byte record), then wakes the
//   object and cancels any queued movement action.
// register convention: EAX -> packed_reference (consumed by the iterator), one stack
//   argument (an "only idle members" flag).
//   // blam-cc: EAX -> packed_reference, stack -> idle_only
//
// UNSURE:
//  - object_set_position_and_orientation's canonical signature in this repo is
//    (object_index, forward, up, position) with the position in EDI. This call site shows
//    only three operands (unit, &forward, 0), so the position argument is whatever was
//    already in EDI; the up vector is the null pointer, which the callee reads as "keep the
//    current up". The global 0x0069672c (global_down3d_pointer) is referenced by this
//    function but does not appear in Ghidra's output, so it is most likely the elided
//    argument. Not resolved.
//  - FUN_0055aa20(0x40000000, 0) is a units-module query (an actor in a seat returns its
//    seat index); it is only called to check for "not in a vehicle".

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "ai.h"

extern Scenario *global_scenario;               // 0x00746f8c
extern const real_vector3d *global_down3d_pointer; // 0x0069672c

extern void ai_reference_actor_iterator_new(uint32_t packed_reference,
    ai_reference_actor_iterator *out_iterator);                             // 0x432650
extern actor *ai_reference_actor_iterator_next(ai_reference_actor_iterator *iterator); // 0x4326d0
extern int32_t unit_test_placement_candidate(uint32_t flags, int32_t unused); // 0x55aa20, not yet rewritten
extern int16_t squad_pick_random_starting_location(datum_index encounter_index,
    int16_t squad_index); // 0x437220, blam-cc: EAX -> squad_index, ECX -> encounter_index
extern void object_set_position_and_orientation(datum_index object_index,
    real_vector3d *forward, real_vector3d *up, real_point3d *position); // 0x4f51c0
extern void object_reset_velocity_and_wake(datum_index object_index); // 0x4f5160
extern void actor_movement_action_stop(datum_index actor_index);      // 0x417570
extern double fcos(double angle); // x87 fcos
extern double fsin(double angle); // x87 fsin

// blam-cc: EAX -> packed_reference, stack -> idle_only
void ai_reference_face_starting_location(uint32_t packed_reference, uint8_t idle_only)
{
    ai_reference_actor_iterator iterator;
    actor *a;
    ScenarioEncounter *definition;
    ScenarioSquad *squads;
    int16_t squad_index;
    int16_t location_index;
    float facing;
    real_vector3d forward;

    ai_reference_actor_iterator_new(packed_reference, &iterator);
    a = ai_reference_actor_iterator_next(&iterator);
    while (a != 0) {
        if (a->unit_index != (datum_index)k_datum_index_none &&
            (idle_only == 0 ||
             (a->active_unit_index == (datum_index)k_datum_index_none &&
              unit_test_placement_candidate(0x40000000, 0) == -1)) &&
            a->encounter_index != (datum_index)k_datum_index_none) {

            squad_index = a->squad_index;
            definition = &((ScenarioEncounter *)global_scenario->encounters.pointer)
                [a->encounter_index & 0xffff];
            squads = (ScenarioSquad *)definition->squads.pointer;

            location_index = squad_pick_random_starting_location(a->encounter_index, squad_index);
            if (location_index != -1) {
                facing = ((ScenarioActorStartingLocation *)
                    squads[squad_index].starting_locations.pointer)[location_index].facing;
                forward.k = 0.0f;
                forward.i = (float)fcos((double)facing);
                forward.j = (float)fsin((double)facing);
                object_set_position_and_orientation(a->unit_index, &forward, 0, 0);
                object_reset_velocity_and_wake(a->unit_index);
                actor_movement_action_stop(iterator.actor_index);
            }
        }
        a = ai_reference_actor_iterator_next(&iterator);
    }
}

#if 0
Original Ghidra decompilation (0x4349d0):

void FUN_004349d0(char param_1)

{
  short sVar1;
  short sVar2;
  int iVar3;
  int iVar4;
  float10 fVar5;
  float10 fVar6;
  float local_24;
  float local_20;
  undefined4 local_1c;

  FUN_00432650();
  iVar3 = FUN_004326d0();
  while (iVar3 != 0) {
    if ((*(int *)(iVar3 + 0x18) != -1) &&
       (((param_1 == '\0' ||
         ((*(int *)(iVar3 + 0x158) == -1 && (iVar4 = FUN_0055aa20(0x40000000,0), iVar4 == -1)))) &&
        (*(uint *)(iVar3 + 0x34) != 0xffffffff)))) {
      sVar1 = *(short *)(iVar3 + 0x3a);
      iVar4 = *(int *)((*(uint *)(iVar3 + 0x34) & 0xffff) * 0xb0 + 0x84 +
                      *(int *)(global_scenario + 0x430));
      sVar2 = squad_pick_random_starting_location();
      if (sVar2 != -1) {
        fVar5 = (float10)*(float *)(sVar2 * 0x1c + 0xc + *(int *)(sVar1 * 0xe8 + iVar4 + 0xd4));
        fVar6 = (float10)fcos(fVar5);
        local_1c = 0;
        local_24 = (float)fVar6;
        fVar5 = (float10)fsin(fVar5);
        local_20 = (float)fVar5;
        object_set_position_and_orientation(*(undefined4 *)(iVar3 + 0x18),&local_24,0);
        object_reset_velocity_and_wake(*(undefined4 *)(iVar3 + 0x18));
        actor_movement_action_stop();
      }
    }
    iVar3 = FUN_004326d0();
  }
  return;
}
#endif
