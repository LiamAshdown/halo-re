// encounter_squad_reset_starting_location_mask  (Ghidra: encounter_squad_reset_starting_location_mask; named for this rewrite)
// address 0x436f90, size 196 bytes
// name confidence: 0.45   rewrite confidence: 0.6
// evidence: phase-4 summary ("builds a per-starting-location exclusion bitmask from a list of
//   flagged sub-records"); it walks ScenarioSquad.starting_locations (0xd0 count / 0xd4
//   address, stride 0x1c = ScenarioActorStartingLocation) and sets one bit per location whose
//   flags byte at +0x13 has bit 0 set. squad_pick_random_starting_location @0x437220 is the
//   only consumer and reads exactly these two dwords.
// register convention: recovered from the disassembly
//   (objdump -d -M intel --start-address=0x436f90 --stop-address=0x437054 bin/halo.exe):
//   EAX -> encounter_index, ECX -> squad_index (movsx edx,cx then imul edx,edx,0xe8).
//   // blam-cc: EAX -> encounter_index, ECX -> squad_index
//
// UNSURE (verified against the disassembly, kept because it is what the binary does): the
// 0xff fill targets squad_state + 0x04 (`lea edi,[esi+0x4]` at 0x436ff9) while the bit-set
// loop writes squad_state + (index >> 5) * 4, i.e. squad_state + 0x00 for the first 32
// locations. The two runs therefore address different dwords. Reading them as the two
// parallel masks types/ai.h now documents (allowed set at +0x00, still-free set at +0x04)
// makes squad_pick_random_starting_location consistent, but the fill covering
// ((count + 31) >> 5) dwords starting at +0x04 would spill past +0x07 for a squad with more
// than 32 starting locations. Flagged for hook verification.
//
// The second `rep stosb` tail in the original is dead: its count is (dword_count * 4) & 3,
// which is always zero.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *encounter_data;                    // 0x008802c8
extern encounter_squad_state *encounter_squad_states; // 0x008802cc
extern Scenario *global_scenario;                     // 0x00746f8c

// blam-cc: EAX -> encounter_index, ECX -> squad_index
// Rebuilds the squad's starting-location masks: every location the tag marks usable gets a
// bit in starting_location_mask, and the whole still-free mask is set to all ones.
void encounter_squad_reset_starting_location_mask(datum_index encounter_index,
                                                  int16_t squad_index)
{
    encounter *enc;
    encounter_squad_state *squad_state;
    ScenarioEncounter *encounter_definition;
    ScenarioSquad *squad_definition;
    ScenarioActorStartingLocation *locations;
    uint32_t *fill;
    uint32_t dword_count;
    int16_t i;
    int32_t location_index;

    enc = &((encounter *)encounter_data->data)[encounter_index & 0xffff];
    squad_state = &encounter_squad_states[(int16_t)(enc->first_squad + squad_index)];

    encounter_definition = &((ScenarioEncounter *)global_scenario->encounters.pointer)
        [encounter_index & 0xffff];
    squad_definition = &((ScenarioSquad *)encounter_definition->squads.pointer)[squad_index];

    // memset(&squad_state->starting_location_free, 0xff, ((count + 31) >> 5) * 4)
    fill = &squad_state->starting_location_free;
    dword_count = (uint32_t)((squad_definition->starting_locations.count + 0x1f) >> 5);
    for (; dword_count != 0; dword_count = dword_count - 1) {
        *fill = 0xffffffff;
        fill = fill + 1;
    }

    locations = (ScenarioActorStartingLocation *)squad_definition->starting_locations.pointer;
    i = 0;
    location_index = 0;
    if (0 < squad_definition->starting_locations.count) {
        do {
            if ((locations[location_index].flags & 1) != 0) {
                (&squad_state->starting_location_mask)[location_index >> 5] =
                    (&squad_state->starting_location_mask)[location_index >> 5] |
                    (1 << (location_index & 0x1f));
            }
            i = i + 1;
            location_index = (int32_t)i;
        } while (location_index < squad_definition->starting_locations.count);
    }
}

#if 0
Original Ghidra decompilation (0x436f90):

void FUN_00436f90(void)

{
  uint in_EAX;
  short in_CX;
  uint uVar1;
  int iVar2;
  int iVar3;
  undefined4 *puVar4;
  short sVar5;
  undefined4 *puVar6;

  puVar4 = (undefined4 *)
           ((short)(*(short *)((in_EAX & 0xffff) * 0x6c + 4 + *(int *)(DAT_008802c8 + 0x34)) + in_CX
                   ) * 0x20 + DAT_008802cc);
  iVar2 = *(int *)((in_EAX & 0xffff) * 0xb0 + 0x84 + *(int *)(global_scenario + 0x430));
  iVar3 = in_CX * 0xe8 + iVar2;
  puVar6 = puVar4;
  for (uVar1 = *(int *)(in_CX * 0xe8 + 0xd0 + iVar2) + 0x1f >> 5 & 0x3fffffff; puVar6 = puVar6 + 1,
      uVar1 != 0; uVar1 = uVar1 - 1) {
    *puVar6 = 0xffffffff;
  }
  for (iVar2 = 0; iVar2 != 0; iVar2 = iVar2 + -1) {
    *(undefined1 *)puVar6 = 0xff;
    puVar6 = (undefined4 *)((int)puVar6 + 1);
  }
  sVar5 = 0;
  if (0 < *(int *)(iVar3 + 0xd0)) {
    iVar2 = 0;
    do {
      if ((*(byte *)(iVar2 * 0x1c + 0x13 + *(int *)(iVar3 + 0xd4)) & 1) != 0) {
        puVar4[iVar2 >> 5] = puVar4[iVar2 >> 5] | 1 << ((byte)iVar2 & 0x1f);
      }
      sVar5 = sVar5 + 1;
      iVar2 = (int)sVar5;
    } while (iVar2 < *(int *)(iVar3 + 0xd0));
  }
  return;
}
#endif
