// encounter_decay_squad_spawn_delays  (Ghidra: encounter_decay_squad_spawn_delays, renamed)
// address 0x4392f0, size 184 bytes
// name confidence: 0.45  rewrite confidence: 0.4
// evidence: types/ai.h encounter / encounter_squad_state; types/tags.h ScenarioSquad.flags
//   (+0x28, confirmed). Calls encounter_squad_clear_spawn_delay @0x439270 once a squad's
//   delay expires. RENAMED in the phase-4 review: the field it decays is +0x12, which
//   encounter_new @0x437060 fills from ftol(ScenarioSquad.squad_delay_time * 30) (or 999
//   when ScenarioSquad.flags bit 3 is set), i.e. the squad SPAWN delay -- not the +0x1c
//   grenade cooldown the phase-4 summary assumed. types/ai.h now names it
//   encounter_squad_state.squad_delay_ticks, and +0x1c is average_vitality.
// register convention: stack -> encounter_index (a normal recovered `uint param_1`).
//   // blam-cc: stack -> encounter_index
//
// UNSURE: ScenarioSquadFlags bit 2 (start_timer_immediately, 0x4) and bit 3
// (no_timer_delay_forever, 0x8) are named in types/tags.h; the exact intent of gating
// encounter_squad_state.unknown_11 by (!start_timer_immediately && encounter.unknown_2e < 1)
// is not otherwise confirmed here.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *encounter_data;  // 0x008802c8
extern Scenario *global_scenario;   // 0x00746f8c
extern encounter_squad_state *encounter_squad_states; // 0x008802cc

extern void encounter_squad_clear_spawn_delay(datum_index encounter_index, int16_t squad_index); // 0x439270

// blam-cc: stack -> encounter_index
void encounter_decay_squad_spawn_delays(datum_index encounter_index)
{
    encounter *self;
    ScenarioEncounter *encounter_definition;
    encounter_squad_state *squad_state;
    int16_t cooldown;
    uint32_t squad_flags;
    int16_t squad_index;

    self = (encounter *)((uint8_t *)encounter_data->data + (encounter_index & 0xffff) * sizeof(encounter));
    encounter_definition = &((ScenarioEncounter *)global_scenario->encounters.pointer)[encounter_index & 0xffff];

    squad_index = 0;
    if (0 < self->squad_count) {
        do {
            squad_state = &encounter_squad_states[(int16_t)(self->first_squad + squad_index)];
            cooldown = squad_state->squad_delay_ticks;

            squad_flags = ((ScenarioSquad *)encounter_definition->squads.pointer)[squad_index].flags;
            if ((0 < cooldown) && ((squad_flags & 8) == 0)) {
                if (squad_state->timer_started == 0) {
                    if (((squad_flags & 4) == 0) && (self->unknown_2e < 1)) {
                        squad_state->timer_started = 0;
                    } else {
                        squad_state->timer_started = 1;
                    }
                } else if (cooldown < 0x10) {
                    encounter_squad_clear_spawn_delay(encounter_index, squad_index);
                } else {
                    squad_state->squad_delay_ticks = cooldown - 0xf;
                }
            }

            squad_index = squad_index + 1;
        } while (squad_index < self->squad_count);
    }
    return;
}

#if 0
// ---- original Ghidra decompilation (FUN_004392f0 @ 0x4392f0) ----
void FUN_004392f0(uint param_1)

{
  short sVar1;
  int iVar2;
  uint uVar3;
  undefined1 uVar4;
  int iVar5;
  int iVar6;
  short sVar7;

  iVar6 = (param_1 & 0xffff) * 0x6c + *(int *)(DAT_008802c8 + 0x34);
  iVar2 = *(int *)(global_scenario + 0x430);
  sVar7 = 0;
  if (0 < *(short *)(iVar6 + 6)) {
    do {
      iVar5 = (short)(*(short *)(iVar6 + 4) + sVar7) * 0x20 + DAT_008802cc;
      sVar1 = *(short *)(iVar5 + 0x12);
      if ((0 < sVar1) &&
         (uVar3 = *(uint *)(sVar7 * 0xe8 + *(int *)((param_1 & 0xffff) * 0xb0 + iVar2 + 0x84) + 0x28
                           ), (uVar3 & 8) == 0)) {
        if (*(char *)(iVar5 + 0x11) == '\0') {
          if (((uVar3 & 4) == 0) && (*(short *)(iVar6 + 0x2e) < 1)) {
            uVar4 = 0;
          }
          else {
            uVar4 = 1;
          }
          *(undefined1 *)(iVar5 + 0x11) = uVar4;
        }
        else if (sVar1 < 0x10) {
          FUN_00439270();
        }
        else {
          *(short *)(iVar5 + 0x12) = sVar1 + -0xf;
        }
      }
      sVar7 = sVar7 + 1;
    } while (sVar7 < *(short *)(iVar6 + 6));
  }
  return;
}
#endif
