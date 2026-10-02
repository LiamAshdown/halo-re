// encounter_build_firing_position_claims  (Ghidra: encounter_build_firing_position_claims; named for this rewrite)
// address 0x4360d0, size 178 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: phase-4 summary ("builds a lookup table mapping each connected zone/cluster
//   index to the actor id currently occupying it"); the table it fills is one entry per
//   ScenarioEncounter.firing_positions (count at +0x98), and the index it writes each member
//   at is actor.firing_position_index (+0x3b8), which types/ai.h already documents as "the
//   encounter firing position this actor has claimed, or -1".
//   actor_find_best_firing_position @0x41f1e0 (already rewritten) is the only caller and
//   already declares a `uint claims[512]` output buffer for it.
// register convention: EAX -> encounter_index, EBX -> out_claims.
//   // blam-cc: EAX -> encounter_index, EBX -> out_claims
//
// The second, zero-trip `for` loop Ghidra prints is the byte tail of an inlined memset whose
// count is always zero (the fill size is a whole number of dwords); it is dropped here.
//
// UNSURE: the fill count is ScenarioEncounter.firing_positions.count masked with 0x3fffffff
// and is a *dword* count, so the buffer is one dword per firing position, not a bitmap.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern Scenario *global_scenario;  // 0x00746f8c
extern ai_globals *ai_globals_ptr; // 0x00880354
extern data_array *encounter_data; // 0x008802c8
extern data_array *actor_data;     // 0x00880360

// blam-cc: EAX -> encounter_index, EBX -> out_claims
// Fills out_claims with one entry per firing position of the encounter: the handle of the
// member that has claimed it, or none.
void encounter_build_firing_position_claims(datum_index encounter_index, datum_index *out_claims)
{
    ScenarioEncounter *definition;
    encounter *enc;
    actor *a;
    datum_index *cursor;
    datum_index actor_index;
    datum_index current;
    uint32_t remaining;
    int16_t position_index;

    definition = &((ScenarioEncounter *)global_scenario->encounters.pointer)
        [encounter_index & 0xffff];

    cursor = out_claims;
    for (remaining = (uint32_t)definition->firing_positions.count & 0x3fffffff;
         remaining != 0; remaining = remaining - 1) {
        *cursor = (datum_index)k_datum_index_none;
        cursor = cursor + 1;
    }

    actor_index = (datum_index)k_datum_index_none;
    if (ai_globals_ptr->actors_valid != 0) {
        if (encounter_index == (datum_index)k_datum_index_none) {
            actor_index = ai_globals_ptr->first_encounterless_actor;
        } else {
            enc = &((encounter *)encounter_data->data)[encounter_index & 0xffff];
            actor_index = enc->first_actor;
        }
    }

    while (ai_globals_ptr->actors_valid != 0 && actor_index != (datum_index)k_datum_index_none) {
        current = actor_index;
        a = &((actor *)actor_data->data)[current & 0xffff];
        actor_index = a->next_in_encounter;
        position_index = a->firing_position_index;
        if (position_index != -1) {
            out_claims[position_index] = current;
        }
    }
}

#if 0
Original Ghidra decompilation (0x4360d0):

void FUN_004360d0(void)

{
  short sVar1;
  int iVar2;
  int iVar3;
  uint in_EAX;
  uint uVar4;
  int iVar5;
  undefined4 *unaff_EBX;
  undefined4 *puVar6;
  uint local_4;

  puVar6 = unaff_EBX;
  for (uVar4 = *(uint *)((in_EAX & 0xffff) * 0xb0 + 0x98 + *(int *)(global_scenario + 0x430)) &
               0x3fffffff; uVar4 != 0; uVar4 = uVar4 - 1) {
    *puVar6 = 0xffffffff;
    puVar6 = puVar6 + 1;
  }
  for (iVar5 = 0; iVar3 = DAT_00880360, iVar2 = DAT_00880354, iVar5 != 0; iVar5 = iVar5 + -1) {
    *(undefined1 *)puVar6 = 0xff;
    puVar6 = (undefined4 *)((int)puVar6 + 1);
  }
  if (*(char *)(DAT_00880354 + 1) != '\0') {
    if (in_EAX == 0xffffffff) {
      local_4 = *(uint *)(DAT_00880354 + 8);
    }
    else {
      local_4 = *(uint *)((in_EAX & 0xffff) * 0x6c + 0x14 + *(int *)(DAT_008802c8 + 0x34));
    }
  }
  while ((uVar4 = local_4, *(char *)(iVar2 + 1) != '\0' && (uVar4 != 0xffffffff))) {
    iVar5 = (uVar4 & 0xffff) * 0x724 + *(int *)(iVar3 + 0x34);
    local_4 = *(uint *)(iVar5 + 0x2c);
    sVar1 = *(short *)(iVar5 + 0x3b8);
    if (sVar1 != -1) {
      unaff_EBX[sVar1] = uVar4;
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
