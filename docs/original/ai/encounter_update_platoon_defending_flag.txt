// encounter_update_platoon_defending_flag  (Ghidra: encounter_update_platoon_defending_flag, renamed)
// address 0x4393b0, size 225 bytes
// name confidence: 0.35  rewrite confidence: 0.9 (VERIFIED against objdump; condition arguments FIXED)
// evidence: types/ai.h encounter (platoon_count +0x0a, first_platoon +0x08, confirmed) /
//   encounter_platoon_state; types/tags.h ScenarioEncounter.platoons (+0x8c count / +0x90
//   pointer) and ScenarioPlatoon.flags (TagString name is 0x20 bytes, so flags is the first
//   field past it, at +0x20) with ScenarioPlatoonFlags bit 2 = start_in_defending_state,
//   matching encounter_platoon_state.unknown_00's header note ("ScenarioPlatoon.flags bit
//   2"). Calls encounter_evaluate_platoon_condition @0x439f20 (this rewrite).
// register convention: stack -> encounter_index (a normal recovered `uint param_1`).
//   // blam-cc: stack -> encounter_index
//
// UNSURE: the gate at encounter_platoon_state+0x06 (unknown_06, checked here as a ">0"
// count) does not match types/ai.h's placement of member_count at +0x04; kept as unknown_06
// per the header's own field layout rather than reinterpreted. unknown_01/unknown_02 (the
// first two bytes of the 3-byte pad array at +0x01) are used here as a one-shot latch and a
// "recompute pending" flag respectively; not independently named in the header.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *encounter_data;  // 0x008802c8
extern Scenario *global_scenario;   // 0x00746f8c
extern encounter_platoon_state *encounter_platoon_states; // 0x008802c4

extern uint8_t encounter_evaluate_platoon_condition(datum_index encounter_index, const ai_platoon_condition *condition); // 0x439f20, EAX, EDI

// blam-cc: stack -> encounter_index
void encounter_update_platoon_defending_flag(datum_index encounter_index)
{
    encounter *self;
    ScenarioEncounter *encounter_definition;
    encounter_platoon_state *platoon_state;
    uint8_t not_defending;
    int16_t platoon_index;

    self = (encounter *)((uint8_t *)encounter_data->data + (encounter_index & 0xffff) * sizeof(encounter));
    encounter_definition = &((ScenarioEncounter *)global_scenario->encounters.pointer)[encounter_index & 0xffff];

    platoon_index = 0;
    if (0 < self->platoon_count) {
        do {
            platoon_state = &encounter_platoon_states[(int16_t)(self->first_platoon + platoon_index)];

            if (0 < platoon_state->living_count) {
                if (((uint8_t *)platoon_state)[1] == 0) {
                    // FIXED (0x439438..0x43943f): EAX = the encounter, EDI = platoon definition +0x3c (maneuver_when)
                    ((uint8_t *)platoon_state)[1] = encounter_evaluate_platoon_condition(encounter_index,
                        (const ai_platoon_condition *)((uint8_t *)encounter_definition->platoons.pointer + platoon_index * 0xac + 0x3c));
                }
                if ((((uint8_t *)platoon_state)[2] != 0) || (((uint8_t *)platoon_state)[1] == 0)) {
                    not_defending = ~(uint8_t)(((ScenarioPlatoon *)encounter_definition->platoons.pointer)[platoon_index].flags >> 2) & 1;
                    if ((platoon_state->defending != not_defending) &&
                        (encounter_evaluate_platoon_condition(encounter_index, // FIXED: EDI = platoon +0x30 (0x43946a)
                            (const ai_platoon_condition *)((uint8_t *)encounter_definition->platoons.pointer + platoon_index * 0xac + 0x30)) != 0)) {
                        platoon_state->defending = not_defending;
                    }
                }
            }

            platoon_index = platoon_index + 1;
        } while (platoon_index < self->platoon_count);
    }
    return;
}

#if 0
// ---- original Ghidra decompilation (FUN_004393b0 @ 0x4393b0) ----
void FUN_004393b0(uint param_1)

{
  int iVar1;
  int iVar2;
  byte bVar3;
  char cVar4;
  short sVar5;
  int iVar6;
  byte *pbVar7;

  iVar1 = *(int *)(global_scenario + 0x430);
  iVar6 = (param_1 & 0xffff) * 0x6c + *(int *)(DAT_008802c8 + 0x34);
  sVar5 = 0;
  if (0 < *(short *)(iVar6 + 10)) {
    do {
      pbVar7 = (byte *)((short)(*(short *)(iVar6 + 8) + sVar5) * 0x10 + DAT_008802c4);
      if (0 < *(short *)(pbVar7 + 6)) {
        iVar2 = *(int *)((param_1 & 0xffff) * 0xb0 + iVar1 + 0x90);
        if (pbVar7[1] == 0) {
          bVar3 = FUN_00439f20();
          pbVar7[1] = bVar3;
        }
        if ((((pbVar7[2] != 0) || (pbVar7[1] == 0)) &&
            (bVar3 = ~(byte)(*(uint *)(sVar5 * 0xac + iVar2 + 0x20) >> 2) & 1, *pbVar7 != bVar3)) &&
           (cVar4 = FUN_00439f20(), cVar4 != '\0')) {
          *pbVar7 = bVar3;
        }
      }
      sVar5 = sVar5 + 1;
    } while (sVar5 < *(short *)(iVar6 + 10));
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
