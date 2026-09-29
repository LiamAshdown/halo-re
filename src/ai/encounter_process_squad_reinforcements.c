// encounter_process_squad_reinforcements  (Ghidra: encounter_process_squad_reinforcements, renamed)
// address 0x4390a0, size 454 bytes
// name confidence: 0.45  rewrite confidence: 0.85 (VERIFIED 2026-09-27 static loop against objdump 0x4390a0..0x439266; offsets probed)
// evidence: types/ai.h encounter / encounter_squad_state; types/tags.h ScenarioEncounter
//   (squads at 0x80, confirmed) and ScenarioSquad (respawn_min_actors 0x84, respawn_max_actors
//   0x86, respawn_total 0x88, confirmed by offsetof()). Calls
//   encounter_squad_spawn_reinforcement @0x438f60 (this rewrite), forwarding this function's
//   own encounter_index/squad_index the same way its own callers forward them (Ghidra shows
//   the two call sites here with zero visible arguments, i.e. pure register reuse -- see that
//   file's header note on the same pattern).
// register convention: stack -> encounter_index (Ghidra recovered a normal `uint param_1`
//   here, so this one is not register-implicit).
//   // blam-cc: stack -> encounter_index
//
// UNSURE: encounter_squad_state.maneuver_distance (+0x0c) is read here as the "still wants to
// spawn" gate and squad_state.weighted_actor_count as a running spawn count; see
// encounter_squad_spawn_reinforcement.c for the same field-name/usage mismatch. The
// `auStack_1008[1016]` stack array Ghidra shows in the original is never read or written
// anywhere in the decompiled body and is omitted here as dead/unused stack space.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *encounter_data;  // 0x008802c8
extern Scenario *global_scenario;   // 0x00746f8c
extern encounter_squad_state *encounter_squad_states; // 0x008802cc
extern uint32_t random_seed_global; // 0x00719cd0

extern uint32_t encounter_squad_spawn_reinforcement(datum_index encounter_index, int16_t squad_index); // 0x438f60

// blam-cc: stack -> encounter_index
void encounter_process_squad_reinforcements(datum_index encounter_index)
{
    encounter *self;
    ScenarioEncounter *encounter_definition;
    ScenarioSquad *squad_definition;
    encounter_squad_state *squad_state;
    uint32_t ready_mask[2];
    int16_t ready_count;
    int16_t squad_index;
    uint32_t roll;

    self = (encounter *)((uint8_t *)encounter_data->data + (encounter_index & 0xffff) * sizeof(encounter));

    if (self->respawn_enabled == 0) {
        return;
    }
    if (0xf < self->reinforcement_delay) {
        self->reinforcement_delay = self->reinforcement_delay - 0xf;
        return;
    }

    encounter_definition = &((ScenarioEncounter *)global_scenario->encounters.pointer)[encounter_index & 0xffff];
    ready_mask[0] = 0;
    self->reinforcement_delay = 0;
    ready_mask[1] = 0;
    ready_count = 0;
    squad_index = 0;

    if (0 < (int32_t)encounter_definition->squads.count) {
        do {
            squad_state = &encounter_squad_states[(int16_t)(self->first_squad + squad_index)];
            squad_definition = &((ScenarioSquad *)encounter_definition->squads.pointer)[squad_index];

            if (0 < squad_state->respawn_budget) {
                do {
                    if ((squad_definition->respawn_min_actors <= squad_state->weighted_actor_count) ||
                        ((int8_t)encounter_squad_spawn_reinforcement(encounter_index, squad_index) == 0)) {
                        break;
                    }
                } while (0 < squad_state->respawn_budget);

                if ((0 < squad_state->respawn_budget) &&
                    (squad_state->weighted_actor_count < squad_definition->respawn_max_actors)) {
                    if (squad_state->respawn_delay_ticks < 0x10) {
                        squad_state->respawn_delay_ticks = 0;
                        ready_count = ready_count + 1;
                        ready_mask[squad_index >> 5] = ready_mask[squad_index >> 5] | (1u << (squad_index & 0x1f));
                    } else {
                        squad_state->respawn_delay_ticks = squad_state->respawn_delay_ticks - 0xf;
                    }
                }
            }

            squad_index = squad_index + 1;
        } while (squad_index < (int32_t)encounter_definition->squads.count);

        if ((0 < ready_count) && (self->reinforcement_delay == 0)) {
            random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
            squad_index = 0;
            roll = (uint32_t)(((uint64_t)(random_seed_global >> 0x10) * (uint32_t)ready_count) >> 0x10);

            if (0 < self->squad_count) {
                do {
                    if ((ready_mask[squad_index >> 5] & (1u << (squad_index & 0x1f))) != 0) {
                        if ((int16_t)roll < 1) {
                            // NB: encounter_squad_spawn_reinforcement's low byte is always 0
                            // in the common (actors_valid) case (see that file's header), so
                            // this comparison -- taken verbatim from Ghidra's `char`-typed
                            // capture of the return value -- is always false here in
                            // practice, and the early `return` below is effectively dead:
                            // once `roll` reaches zero the loop keeps calling
                            // encounter_squad_spawn_reinforcement for every remaining ready
                            // squad rather than stopping at the first one. Preserved as-is.
                            if ((int8_t)encounter_squad_spawn_reinforcement(encounter_index, squad_index) != 0) {
                                return;
                            }
                        } else {
                            roll = roll - 1;
                        }
                    }
                    squad_index = squad_index + 1;
                } while (squad_index < self->squad_count);
            }
        }
    }
    return;
}

#if 0
// ---- original Ghidra decompilation (FUN_004390a0 @ 0x4390a0) ----
void FUN_004390a0(uint param_1)

{
  short sVar1;
  char cVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  uint uVar7;
  short sVar8;
  int iVar9;
  uint auStack_1008 [1016];
  uint local_8 [2];

  iVar3 = (param_1 & 0xffff) * 0x6c;
  iVar4 = iVar3 + *(int *)(DAT_008802c8 + 0x34);
  if (*(char *)(iVar3 + 0x3c + *(int *)(DAT_008802c8 + 0x34)) != '\0') {
    if (0xf < *(short *)(iVar4 + 0x3e)) {
      *(short *)(iVar4 + 0x3e) = *(short *)(iVar4 + 0x3e) + -0xf;
      return;
    }
    iVar3 = (param_1 & 0xffff) * 0xb0 + *(int *)(global_scenario + 0x430);
    local_8[0] = 0;
    *(undefined2 *)(iVar4 + 0x3e) = 0;
    local_8[1] = 0;
    sVar1 = 0;
    sVar8 = 0;
    if (0 < *(int *)(iVar3 + 0x80)) {
      iVar5 = 0;
      do {
        iVar6 = (short)(*(short *)(iVar4 + 4) + sVar8) * 0x20 + DAT_008802cc;
        iVar9 = iVar5 * 0xe8 + *(int *)(iVar3 + 0x84);
        if (0 < *(short *)(iVar6 + 0xc)) {
          do {
            if ((*(short *)(iVar9 + 0x84) <= *(short *)(iVar6 + 0x18)) ||
               (cVar2 = FUN_00438f60(), cVar2 == '\0')) break;
          } while (0 < *(short *)(iVar6 + 0xc));
          if ((0 < *(short *)(iVar6 + 0xc)) && (*(short *)(iVar6 + 0x18) < *(short *)(iVar9 + 0x86))
             ) {
            if (*(short *)(iVar6 + 0xe) < 0x10) {
              *(undefined2 *)(iVar6 + 0xe) = 0;
              sVar1 = sVar1 + 1;
              local_8[iVar5 >> 5] = local_8[iVar5 >> 5] | 1 << ((byte)iVar5 & 0x1f);
            }
            else {
              *(short *)(iVar6 + 0xe) = *(short *)(iVar6 + 0xe) + -0xf;
            }
          }
        }
        sVar8 = sVar8 + 1;
        iVar5 = (int)sVar8;
      } while (iVar5 < *(int *)(iVar3 + 0x80));
      if ((0 < sVar1) && (*(short *)(iVar4 + 0x3e) == 0)) {
        random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
        sVar8 = 0;
        uVar7 = (random_seed_global >> 0x10) * (int)sVar1 >> 0x10;
        if (0 < *(short *)(iVar4 + 6)) {
          do {
            if ((local_8[(int)sVar8 >> 5] & 1 << ((byte)sVar8 & 0x1f)) != 0) {
              if ((short)uVar7 < 1) {
                cVar2 = FUN_00438f60();
                if (cVar2 != '\0') {
                  return;
                }
              }
              else {
                uVar7 = uVar7 - 1;
              }
            }
            sVar8 = sVar8 + 1;
          } while (sVar8 < *(short *)(iVar4 + 6));
        }
      }
    }
  }
  return;
}
#endif
