// encounter_propagate_platoon_state_to_actors  (Ghidra: encounter_propagate_platoon_state_to_actors, renamed)
// address 0x439d80, size 397 bytes
// name confidence: 0.4   rewrite confidence: 0.85 (VERIFIED 2026-09-27 static loop against objdump 0x439d80..0x439f0e, tail-jumps into the argless 0x435f00; offsets probed)
// evidence: types/ai.h ai_globals.unknown_08 ("also the head of the unassigned actor list"),
//   encounter.first_actor (+0x14), actor.next_in_encounter (+0x2c), actor.platoon_index
//   (+0x3c), actor.squad_index (+0x3a), encounter_platoon_state (unknown_00/01/02);
//   types/tags.h ScenarioSquad.maneuver_to_squad confirmed at +0x4e by offsetof(),
//   ScenarioPlatoonFlags bit 0 = flee_when_maneuvering. Calls
//   actor_reset_squad_link_for_type_change (0x4290f0, already rewritten, called here with a
//   squad index rather than its own established (actor_index, encounter_index) signature --
//   see UNSURE) and actor_notify_squad_and_flag_danger (0x423600, already rewritten, same
//   arity mismatch) and encounters_recompute_dirty (0x435f00, not yet rewritten).
// register convention: stack -> encounter_index (0xffffffff selects the unassigned-actor
//   list instead of one encounter's members; a normal recovered `uint param_1`).
//   // blam-cc: stack -> encounter_index
//
// UNSURE: when encounter_index is 0xffffffff, `self` below is still computed with the exact
// same (index & 0xffff) * sizeof(encounter) arithmetic Ghidra shows unconditionally at the
// top of the original function, which for index 0xffff reads about 2.9MB past
// encounter_data->data. Ghidra's decompile reads through that same pointer inside the loop
// body (self->unknown_42, self->unknown_60, self->unknown_47) with no visible guard, so this
// rewrite preserves that literally rather than adding a bounds check that is not in the
// original. Whether it is ever actually reached in that mode (vs. the loop body only running
// this way when the unassigned list is non-empty and the reads are relatively harmless
// process-heap garbage) is not established here.
// UNSURE: actor_reset_squad_link_for_type_change and actor_notify_squad_and_flag_danger are called here with only one argument each, fewer
// than their own established multi-argument signatures elsewhere in this module (see those
// files); declared locally below with the reduced arity this call site actually shows.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *encounter_data;  // 0x008802c8
extern data_array *actor_data;      // 0x00880360
extern Scenario *global_scenario;   // 0x00746f8c
extern ai_globals *ai_globals_ptr;  // 0x00880354
extern encounter_platoon_state *encounter_platoon_states; // 0x008802c4

extern void actor_reset_squad_link_for_type_change(datum_index actor_index, datum_index encounter_index,
    int16_t squad_index); // 0x4290f0, EAX, EBX, stack
extern void actor_notify_squad_and_flag_danger(datum_index actor_index, uint8_t alternate_event,
    uint8_t raise_danger_flag); // 0x423600, EAX, ECX, stack
extern void encounters_recompute_dirty(void);                           // 0x435f00, not yet rewritten: re-runs morale for dirty squads

// blam-cc: stack -> encounter_index
void encounter_propagate_platoon_state_to_actors(datum_index encounter_index)
{
    encounter *self;
    ScenarioEncounter *encounter_definition;
    ScenarioSquad *squad_definition;
    ScenarioPlatoon *platoon_definition;
    encounter_platoon_state *platoon_state;
    actor *member;
    datum_index actor_index;
    int16_t platoon_index;
    int16_t maneuver_to_squad;
    uint8_t attacking_flag;
    uint8_t ready;

    self = (encounter *)((uint8_t *)encounter_data->data + (encounter_index & 0xffff) * sizeof(encounter));
    encounter_definition = &((ScenarioEncounter *)global_scenario->encounters.pointer)[encounter_index & 0xffff];

    actor_index = (datum_index)0xffffffff;
    if (ai_globals_ptr->actors_valid != 0) {
        if (encounter_index == (datum_index)0xffffffff) {
            actor_index = ai_globals_ptr->unknown_08;
        } else {
            actor_index = self->first_actor;
        }
    }

    while ((ai_globals_ptr->actors_valid != 0) && (actor_index != (datum_index)0xffffffff)) {
        datum_index current = actor_index;

        member = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
        actor_index = member->next_in_encounter;

        member->unknown_1c8 = self->unknown_42;
        member->unknown_1ca = self->unknown_60;
        attacking_flag = 0;
        ready = 0;

        if (self->unknown_47 == 0) {
            member->command_status = 0;
            member->unknown_1e8 = (datum_index)0xffffffff;
        }

        platoon_index = member->platoon_index;
        if (platoon_index != -1) {
            platoon_state = &encounter_platoon_states[(int16_t)(self->first_platoon + platoon_index)];
            attacking_flag = platoon_state->defending;
            if ((((uint8_t *)platoon_state)[1] == 0) || (((uint8_t *)platoon_state)[2] != 0)) {
                ready = 0;
            } else {
                ready = 1;
            }
        }
        member->platoon_defending_pending = attacking_flag;

        if (ready != 0) {
            squad_definition = &((ScenarioSquad *)encounter_definition->squads.pointer)[member->squad_index];
            maneuver_to_squad = squad_definition->maneuver_to_squad;
            if ((-1 < maneuver_to_squad) && (maneuver_to_squad < (int32_t)encounter_definition->squads.count)) {
                // FIXED (objdump 0x439ed5..0x439efa): the squad move gets (EAX = this actor, EBX = the encounter,
                //   squad) and the notify gets (actor, CL = platoon flags bit 1, stack = bit 0). The draft passed one
                //   argument to each.
                actor_reset_squad_link_for_type_change(current, encounter_index, maneuver_to_squad);
                platoon_definition = &((ScenarioPlatoon *)encounter_definition->platoons.pointer)[platoon_index];
                actor_notify_squad_and_flag_danger(current, (uint8_t)((*(uint32_t *)&platoon_definition->flags >> 1) & 1),
                    (uint8_t)(*(uint32_t *)&platoon_definition->flags & 1));
            }
        }
    }

    encounters_recompute_dirty();
    return;
}

#if 0
// ---- original Ghidra decompilation (FUN_00439d80 @ 0x439d80) ----
void FUN_00439d80(uint param_1)

{
  int iVar1;
  short sVar2;
  short sVar3;
  bool bVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  undefined1 *puVar8;
  undefined1 uVar9;
  uint local_4;

  iVar1 = (param_1 & 0xffff) * 0x6c + *(int *)(DAT_008802c8 + 0x34);
  iVar5 = (param_1 & 0xffff) * 0xb0 + *(int *)(global_scenario + 0x430);
  if (*(char *)(DAT_00880354 + 1) != '\0') {
    if (param_1 == 0xffffffff) {
      local_4 = *(uint *)(DAT_00880354 + 8);
    }
    else {
      local_4 = *(uint *)(iVar1 + 0x14);
    }
  }
  while ((*(char *)(DAT_00880354 + 1) != '\0' && (local_4 != 0xffffffff))) {
    iVar6 = (local_4 & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
    local_4 = *(uint *)(iVar6 + 0x2c);
    *(undefined1 *)(iVar6 + 0x1c8) = *(undefined1 *)(iVar1 + 0x42);
    *(undefined1 *)(iVar6 + 0x1ca) = *(undefined1 *)(iVar1 + 0x60);
    uVar9 = 0;
    bVar4 = false;
    if (*(char *)(iVar1 + 0x47) == '\0') {
      *(undefined2 *)(iVar6 + 0x1e4) = 0;
      *(undefined4 *)(iVar6 + 0x1e8) = 0xffffffff;
    }
    sVar2 = *(short *)(iVar6 + 0x3c);
    if (sVar2 != -1) {
      iVar7 = (short)(*(short *)(iVar1 + 8) + sVar2) * 0x10;
      puVar8 = (undefined1 *)(iVar7 + DAT_008802c4);
      uVar9 = *puVar8;
      if ((*(char *)(iVar7 + 1 + DAT_008802c4) == '\0') || (puVar8[2] != '\0')) {
        bVar4 = false;
      }
      else {
        bVar4 = true;
      }
    }
    *(undefined1 *)(iVar6 + 0x1c9) = uVar9;
    if (bVar4) {
      iVar7 = *(int *)(iVar5 + 0x90);
      sVar3 = *(short *)(*(short *)(iVar6 + 0x3a) * 0xe8 + *(int *)(iVar5 + 0x84) + 0x4e);
      if ((-1 < sVar3) && ((int)sVar3 < *(int *)(iVar5 + 0x80))) {
        FUN_004290f0((int)sVar3);
        FUN_00423600(*(byte *)(sVar2 * 0xac + iVar7 + 0x20) & 1);
      }
    }
  }
  FUN_00435f00();
  return;
}
#endif
