// encounters_spawn_initial  (Ghidra: encounters_spawn_initial; named for this rewrite)
// address 0x435d50, size 172 bytes
// name confidence: 0.45   rewrite confidence: 0.6
// evidence: phase-4 summary ("iterates all pending squads and activates each one that has
//   not already been flagged active"); it sweeps every live encounter and calls
//   encounter_spawn_squads (0x437510) with both filters wide open for any encounter whose
//   ScenarioEncounter.flags bit 0 is clear. The 0x18-byte encounter_iterator is confirmed by
//   the disassembly (objdump -d -M intel --start-address=0x435d50 --stop-address=0x435db0
//   bin/halo.exe: [esp+8] = encounter_data, [esp+0xc] = word 0, [esp+0x10] = -1,
//   [esp+0x14] = data XOR 'iter', [esp+0x1c] = byte 0).
// register convention: no arguments.
//
// UNSURE: ScenarioEncounter.flags bit 0 has no name in types/tags.h; "not initially created"
// is the obvious reading but is not established here.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern ai_globals *ai_globals_ptr;  // 0x00880354
extern Scenario *global_scenario;   // 0x00746f8c
extern data_array *encounter_data;  // 0x008802c8

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, blam-cc: EDI -> iterator
extern void encounter_spawn_squads(datum_index encounter_index, int16_t squad_filter,
                                   int16_t platoon_filter); // 0x437510

// Spawns the starting actors of every encounter the scenario does not mark as deferred.
void encounters_spawn_initial(void)
{
    encounter_iterator iterator;
    encounter *enc;
    ScenarioEncounter *definition;

    if (ai_globals_ptr->actors_valid != 0) {
        iterator.data = encounter_data;
        iterator.next_index = 0;
        iterator.index = (datum_index)k_datum_index_none;
        iterator.signature = (uint32_t)encounter_data ^ 0x69746572;
        iterator.active_only = 0;
    }

    for (;;) {
        if (ai_globals_ptr->actors_valid == 0) {
            return;
        }
        do {
            enc = (encounter *)data_iterator_next((data_iterator *)&iterator);
            if (enc == 0 || iterator.active_only == 0) {
                break;
            }
        } while (enc->units_active == 0);
        iterator.encounter_index = iterator.index;
        if (enc == 0) {
            return;
        }
        definition = &((ScenarioEncounter *)global_scenario->encounters.pointer)
            [iterator.encounter_index & 0xffff];
        if ((~definition->flags & 1) != 0) {
            encounter_spawn_squads(iterator.encounter_index, -1, -1);
        }
    }
}

#if 0
Original Ghidra decompilation (0x435d50):

void FUN_00435d50(void)

{
  int iVar1;
  int iVar2;
  uint local_14;
  char local_8;

  iVar1 = global_scenario;
  if (*(char *)(DAT_00880354 + 1) != '\0') {
    local_14 = 0xffffffff;
    local_8 = '\0';
  }
  do {
    if (*(char *)(DAT_00880354 + 1) == '\0') {
      return;
    }
    do {
      iVar2 = data_iterator_next();
      if ((iVar2 == 0) || (local_8 == '\0')) break;
    } while (*(char *)(iVar2 + 0xd) == '\0');
    if (iVar2 == 0) {
      return;
    }
    if ((~*(byte *)((local_14 & 0xffff) * 0xb0 + 0x20 + *(int *)(iVar1 + 0x430)) & 1) != 0) {
      FUN_00437510(local_14,0xffffffff,0xffffffff);
    }
  } while( true );
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
