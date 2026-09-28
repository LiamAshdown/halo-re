// encounter_add_actor  (Ghidra: encounter_add_actor; named for this rewrite)
// address 0x436770, size 452 bytes
// name confidence: 0.6   rewrite confidence: 0.6
// evidence: out/phase4/ai_types_notes.md ("squad_remove_actor @0x436620 and its partner
//   0x436770 remove/add an actor from an *encounter* member list, adjusting the per-squad
//   and per-platoon counters as a side effect"). types/ai.h already names every counter it
//   touches: encounter.member_count / live_count, encounter_squad_state.member_count,
//   encounter_platoon_state.member_count, actor.next_in_encounter / encounter_index /
//   squad_index / platoon_index, and actor.unknown_1c9 / unknown_374 ("encounter_add_actor
//   copies the platoon state byte here").
// register convention: recovered from the disassembly
//   (objdump -d -M intel --start-address=0x436770 --stop-address=0x4367d0 bin/halo.exe):
//   [esp+0x1c] and [esp+0x20] after two pushes are the first two stack arguments, and the
//   squad index arrives in DX and is never loaded from the frame.
//   // blam-cc: DX -> squad_index, stack -> (actor_index, encounter_index, keep_team)
//
// UNSURE: the three calls Ghidra renders without arguments (actor_propagate_unit_field
// 0x4276e0, 0x4277c0, actor_set_units_active 0x427860) take the actor index in a register
// this rewrite could not pin down; they are written with the actor index they must be
// operating on. encounter+0x0e is set to 0x96 (150 ticks) before encounter_activate, which
// is the same "activation delay" slot encounters_update_activation writes.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern ai_globals *ai_globals_ptr;                        // 0x00880354
extern data_array *actor_data;                            // 0x00880360
extern data_array *encounter_data;                        // 0x008802c8
extern Scenario *global_scenario;                         // 0x00746f8c
extern encounter_squad_state *encounter_squad_states;     // 0x008802cc
extern encounter_platoon_state *encounter_platoon_states; // 0x008802c4

extern void actor_propagate_unit_field(datum_index actor_index, int16_t value); // 0x4276e0, blam-cc: EAX, ESI
extern uint8_t actor_toggle_active_state(uint8_t activate, datum_index actor_index); // 0x4277c0, blam-cc: AL, EDI
extern void actor_set_units_active(datum_index actor_index, uint8_t dormant); // 0x427860, blam-cc: EAX, BL
extern void ai_recompute_all_relationship_flags(void); // 0x42bbb0
extern void ai_encounter_stamp_team_from_unit(datum_index encounter_index, datum_index unit_index); // 0x436710, blam-cc: EAX, ECX
extern uint8_t encounter_activate(datum_index encounter_index);       // 0x437710, blam-cc: ECX -> encounter_index

// blam-cc: DX -> squad_index, stack -> (actor_index, encounter_index, keep_team)
// Links an actor into an encounter's member list under one of the encounter's squads (and,
// through ScenarioSquad.platoon, one of its platoons), bumps the member counts on the
// encounter, the squad record and the platoon record, and brings the encounter to life if
// the actor is already active.
void encounter_add_actor(int16_t squad_index, datum_index actor_index,
                         datum_index encounter_index, uint8_t keep_team)
{
    actor *a;
    encounter *enc;
    ScenarioEncounter *definition;
    encounter_squad_state *squad_state;
    encounter_platoon_state *platoon_state;
    int16_t platoon_index;
    uint8_t activated;

    if (ai_globals_ptr->actors_valid == 0) {
        return;
    }

    a = &((actor *)actor_data->data)[actor_index & 0xffff];
    enc = &((encounter *)encounter_data->data)[encounter_index & 0xffff];
    definition = &((ScenarioEncounter *)global_scenario->encounters.pointer)
        [encounter_index & 0xffff];
    squad_state = &encounter_squad_states[(int16_t)(enc->first_squad + squad_index)];

    platoon_index = (int16_t)((ScenarioSquad *)definition->squads.pointer)[squad_index].platoon;

    a->unknown_30 = (datum_index)k_datum_index_none;
    a->unknown_38 = -1;
    a->next_in_encounter = enc->first_actor;
    enc->first_actor = actor_index;

    if (platoon_index < 0 || definition->platoons.count <= (int32_t)platoon_index) {
        platoon_index = -1;
    }
    a->squad_index = squad_index;
    a->platoon_index = platoon_index;
    a->encounter_index = encounter_index;

    activated = 0;
    if (a->active != 0 && a->keep_unit_alive == 0) {
        enc->activation_delay = 0x96;
        activated = encounter_activate(encounter_index);
    }
    if (activated == 0) {
        actor_toggle_active_state(enc->units_active, actor_index); // 0x43687d: AL = encounter +0xd, EDI = actor
        if (enc->units_active != 0) {
            actor_set_units_active(actor_index, 0); // 0x43688c: BL = 0
        }
    }

    if (a->unit_index != (datum_index)k_datum_index_none) {
        ai_encounter_stamp_team_from_unit(encounter_index, a->unit_index); // 0x436899..0x4368a5: EAX encounter, ECX unit
    }

    if (a->team != enc->team) {
        if (keep_team == 0 || enc->unknown_2a != 0) {
            actor_propagate_unit_field(actor_index, enc->team); // 0x4368d1: ESI = the encounter's team
        } else {
            enc->team = a->team;
            ai_recompute_all_relationship_flags();
        }
    }

    enc->member_count = enc->member_count + 1;
    squad_state->member_count = squad_state->member_count + 1;
    if (a->counts_toward_encounter != 0) {
        enc->live_count = enc->live_count + 1;
    }

    if (platoon_index != -1) {
        platoon_state = &encounter_platoon_states[(int16_t)(enc->first_platoon + platoon_index)];
        a->unknown_1c9 = platoon_state->unknown_00;
        a->unknown_374 = platoon_state->unknown_00;
        platoon_state->member_count = platoon_state->member_count + 1;
    }
    enc->dirty = 1;
}

#if 0
Original Ghidra decompilation (0x436770):

void FUN_00436770(uint param_1,uint param_2,char param_3)

{
  short *psVar1;
  short sVar2;
  char cVar3;
  int iVar4;
  undefined1 *puVar5;
  int iVar6;
  short in_DX;
  int iVar7;
  int iVar8;
  int iVar9;

  if (*(char *)(DAT_00880354 + 1) == '\0') {
    return;
  }
  iVar8 = (param_1 & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  iVar6 = (param_2 & 0xffff) * 0x6c;
  iVar9 = *(int *)(DAT_008802c8 + 0x34) + iVar6;
  iVar4 = (param_2 & 0xffff) * 0xb0 + *(int *)(global_scenario + 0x430);
  iVar7 = (short)(*(short *)(*(int *)(DAT_008802c8 + 0x34) + 4 + iVar6) + in_DX) * 0x20 +
          DAT_008802cc;
  sVar2 = *(short *)(in_DX * 0xe8 + 0x22 + *(int *)(iVar4 + 0x84));
  *(undefined4 *)(iVar8 + 0x30) = 0xffffffff;
  *(undefined2 *)(iVar8 + 0x38) = 0xffff;
  *(undefined4 *)(iVar8 + 0x2c) = *(undefined4 *)(iVar9 + 0x14);
  *(uint *)(iVar9 + 0x14) = param_1;
  if ((sVar2 < 0) || (*(int *)(iVar4 + 0x8c) <= (int)sVar2)) {
    sVar2 = -1;
  }
  *(short *)(iVar8 + 0x3a) = in_DX;
  *(short *)(iVar8 + 0x3c) = sVar2;
  *(uint *)(iVar8 + 0x34) = param_2;
  if ((*(char *)(iVar8 + 8) != '\0') && (*(char *)(iVar8 + 0x13) == '\0')) {
    *(undefined2 *)(iVar6 + 0xe + *(int *)(DAT_008802c8 + 0x34)) = 0x96;
    cVar3 = squad_activate();
    if (cVar3 != '\0') goto LAB_00436899;
  }
  FUN_004277c0();
  if (*(char *)(iVar9 + 0xd) != '\0') {
    actor_set_units_active();
  }
LAB_00436899:
  if (*(int *)(iVar8 + 0x18) != -1) {
    FUN_00436710();
  }
  if (*(short *)(iVar8 + 0x3e) != *(short *)(iVar9 + 2)) {
    if ((param_3 == '\0') || (*(short *)(iVar9 + 0x2a) != 0)) {
      FUN_004276e0();
    }
    else {
      *(short *)(iVar9 + 2) = *(short *)(iVar8 + 0x3e);
      FUN_0042bbb0();
    }
  }
  *(short *)(iVar9 + 0x18) = *(short *)(iVar9 + 0x18) + 1;
  psVar1 = (short *)(iVar7 + 0x16);
  *psVar1 = *psVar1 + 1;
  if (*(char *)(iVar8 + 0x1c) != '\0') {
    *(short *)(iVar9 + 0x1c) = *(short *)(iVar9 + 0x1c) + 1;
  }
  if (sVar2 != -1) {
    puVar5 = (undefined1 *)((short)(*(short *)(iVar9 + 8) + sVar2) * 0x10 + DAT_008802c4);
    *(undefined1 *)(iVar8 + 0x1c9) = *puVar5;
    *(undefined1 *)(iVar8 + 0x374) = *puVar5;
    *(short *)(puVar5 + 4) = *(short *)(puVar5 + 4) + 1;
  }
  *(undefined1 *)(iVar9 + 0x28) = 1;
  return;
}
#endif
