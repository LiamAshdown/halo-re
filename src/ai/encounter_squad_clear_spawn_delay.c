// encounter_squad_clear_spawn_delay  (Ghidra: encounter_squad_clear_spawn_delay, renamed)
// address 0x439270, size 123 bytes
// name confidence: 0.4   rewrite confidence: 0.9 (checked against objdump 0x439270..0x4392ea)
// evidence: types/ai.h encounter / encounter_squad_state; types/tags.h ScenarioSquad.flags
//   confirmed at +0x28 by offsetof(), bit 4 = magic_sight_after_timer (see
//   ScenarioSquadFlags in types/tags.h). Called from encounter_decay_squad_spawn_delays
//   (0x4392f0, this rewrite) once a squad's cooldown at encounter_squad_state.unknown_12
//   expires.
// register convention: ECX -> encounter_index, DX -> squad_index (both `in_`-prefixed
//   unaffected registers).
//   // blam-cc: ECX -> encounter_index, EDX -> squad_index
//
// UNSURE: types/ai.h documents encounter_squad_state.grenade_cooldown at +0x1c as the field
// "0x4392f0 decays ... and triggers a throw when it expires", but the actual decompiled
// bytes for both 0x4392f0 and this function operate on +0x12 (unknown_12), not +0x1c. That
// header note appears to describe the wrong offset; kept using unknown_12 here to match
// what the code actually touches, without editing the header.
// UNSURE: ai_reference_respawn_all_players (phase-4: "iterates a datum collection via data_iterator_next and
// respawns every member found") is outside this rewrite's address range and still
// unnamed/unrewritten; called here exactly as Ghidra shows, with no visible arguments.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *encounter_data;  // 0x008802c8
extern Scenario *global_scenario;   // 0x00746f8c
extern encounter_squad_state *encounter_squad_states; // 0x008802cc

extern void ai_reference_respawn_all_players(uint32_t packed_reference); // 0x432d90, ESI

// blam-cc: ECX -> encounter_index, EDX -> squad_index
void encounter_squad_clear_spawn_delay(datum_index encounter_index, int16_t squad_index)
{
    encounter *self;
    ScenarioEncounter *encounter_definition;
    ScenarioSquad *squad_definition;
    encounter_squad_state *squad_state;

    self = (encounter *)((uint8_t *)encounter_data->data + (encounter_index & 0xffff) * sizeof(encounter));
    encounter_definition = &((ScenarioEncounter *)global_scenario->encounters.pointer)[encounter_index & 0xffff];
    squad_definition = &((ScenarioSquad *)encounter_definition->squads.pointer)[squad_index];

    squad_state = &encounter_squad_states[(int16_t)(self->first_squad + squad_index)];
    squad_state->squad_delay_ticks = 0;

    if ((squad_definition->flags & 0x10) != 0) {
        // 0x4392d0: ESI = the squad's packed ai reference ((squad & 0xff | 0x8000) << 16 | encounter)
        ai_reference_respawn_all_players(((uint32_t)(((uint16_t)squad_index & 0xff) | 0x8000) << 16) |
                                         (encounter_index & 0xffff));
    }
    return;
}

#if 0
// ---- original Ghidra decompilation (FUN_00439270 @ 0x439270) ----
void FUN_00439270(void)

{
  int iVar1;
  uint in_ECX;
  short in_DX;

  iVar1 = *(int *)((in_ECX & 0xffff) * 0xb0 + 0x84 + *(int *)(global_scenario + 0x430));
  *(undefined2 *)
   ((short)(*(short *)((in_ECX & 0xffff) * 0x6c + 4 + *(int *)(DAT_008802c8 + 0x34)) + in_DX) * 0x20
    + 0x12 + DAT_008802cc) = 0;
  if ((*(byte *)(in_DX * 0xe8 + iVar1 + 0x28) & 0x10) != 0) {
    FUN_00432d90();
  }
  return;
}
#endif
