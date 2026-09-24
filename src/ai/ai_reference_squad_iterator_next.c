// ai_reference_squad_iterator_next  (Ghidra: ai_reference_squad_iterator_next; named for this rewrite)
// address 0x4325b0, size 151 bytes
// name confidence: 0.4   rewrite confidence: 0.4
// evidence: partner of ai_reference_squad_iterator_new (0x4324f0, this batch); see that
// file for the shared ai_reference_squad_iterator layout and the misattribution note (this
// returns an encounter_squad_state pointer, not a starting-location record). The return
// expression indexes types/ai.h's global encounter_squad_states by
// (encounter.first_squad + cursor), stride k_encounter_squad_state_size (0x20) -- confirmed
// by encounter.first_squad's own established offset (+0x04).
// register convention: matches ai_reference_squad_iterator_new: the iterator pointer is
// inherited, ECX here.
//   // blam-cc: ECX -> iterator

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "ai.h"

// TYPES-GAP: see ai_reference_squad_iterator_new.c.
extern data_array *encounter_data;                             // 0x008802c8
extern Scenario *global_scenario;                               // 0x00746f8c
extern encounter_squad_state *encounter_squad_states;           // 0x008802cc

// blam-cc: ECX -> iterator
// Advances the iterator to the next squad in [squad_start, squad_end] whose platoon matches
// platoon_filter (or any squad, if platoon_filter is -1), and returns a pointer to that
// squad's encounter_squad_state, or 0 once the range or the iterator itself is exhausted.
encounter_squad_state *ai_reference_squad_iterator_next(ai_reference_squad_iterator *iterator)
{
    encounter *enc;
    ScenarioEncounter *encounter_definition;
    ScenarioSquad *squads;

    if (iterator->encounter_index == -1) {
        return 0;
    }

    enc = &((encounter *)encounter_data->data)[iterator->encounter_index & 0xffff];
    encounter_definition =
        &((ScenarioEncounter *)global_scenario->encounters.pointer)[iterator->encounter_index & 0xffff];
    squads = (ScenarioSquad *)encounter_definition->squads.pointer;

    if (iterator->squad_start > iterator->squad_end) {
        return 0;
    }

    for (;;) {
        iterator->cursor = iterator->squad_start;
        iterator->squad_start = iterator->squad_start + 1;
        if (iterator->platoon_filter == -1 || squads[iterator->cursor].platoon == iterator->platoon_filter) {
            break;
        }
        if (iterator->squad_end < iterator->squad_start) {
            return 0;
        }
    }

    return &encounter_squad_states[enc->first_squad + iterator->cursor];
}

#if 0
Original Ghidra decompilation (0x4325b0):

int FUN_004325b0(void)

{
  int iVar1;
  int iVar2;
  int iVar3;
  uint *in_ECX;
  uint uVar4;

  iVar3 = 0;
  if (*in_ECX != 0xffffffff) {
    iVar1 = *(int *)(DAT_008802c8 + 0x34);
    iVar2 = *(int *)(global_scenario + 0x430);
    uVar4 = *in_ECX & 0xffff;
    if ((int)in_ECX[3] <= (int)in_ECX[4]) {
      while( true ) {
        in_ECX[2] = in_ECX[3];
        in_ECX[3] = in_ECX[3] + 1;
        if ((in_ECX[1] == 0xffffffff) ||
           ((int)*(short *)(in_ECX[2] * 0xe8 + 0x22 + *(int *)(uVar4 * 0xb0 + iVar2 + 0x84)) ==
            in_ECX[1])) break;
        if ((int)in_ECX[4] < (int)in_ECX[3]) {
          return iVar3;
        }
      }
      iVar3 = (short)(*(short *)(uVar4 * 0x6c + iVar1 + 4) + (short)in_ECX[2]) * 0x20 + DAT_008802cc
      ;
    }
  }
  return iVar3;
}
#endif
