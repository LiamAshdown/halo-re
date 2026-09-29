// encounters_reset  (Ghidra: encounters_reset, already named)
// address 0x435cb0, size 149 bytes
// name confidence: 0.85   rewrite confidence: 0.85
// evidence: types/ai.h header note ("encounter_new @0x437060 hands out consecutive runs of
//   both, one run per ScenarioEncounter ... encounters_reset @0x435cb0 calls it once per
//   Scenario.encounters entry"). The two rep-stosd runs are 0x2000 and 0x400 dwords, i.e.
//   exactly the 0x8000 and 0x1000 byte tables encounters_initialize reserved.
// register convention: no arguments. The call to encounter_new passes EAX -> &squad_cursor,
//   EBX -> the ScenarioEncounter, stack -> &platoon_cursor; recovered from the disassembly
//   (objdump -d -M intel --start-address=0x435d10 --stop-address=0x435d2c bin/halo.exe:
//   lea eax,[esp+0x10] / push eax / add ebx,edi / lea eax,[esp+0x18] / call 0x437060).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "fn_ai.h"

extern Scenario *global_scenario;                             // 0x00746f8c
extern data_array *encounter_data;                            // 0x008802c8
extern data_array *ai_pursuit_data;                           // 0x008802d0
extern encounter_squad_state *encounter_squad_states;         // 0x008802cc
extern encounter_platoon_state *encounter_platoon_states;     // 0x008802c4

extern void data_delete_all(data_array *array); // 0x4d0580, blam-cc: ESI -> array


// Empties both AI datum arrays, wipes the two flat sub-record tables, and rebuilds one
// encounter datum per Scenario.encounters entry, threading the running squad-state and
// platoon-state cursors through encounter_new so each encounter gets a contiguous run of
// both.
void encounters_reset(void)
{
    Scenario *scenario;
    int32_t i;
    int16_t encounter_index;
    uint32_t *zero;
    int16_t platoon_cursor;
    int16_t squad_cursor;

    scenario = global_scenario;
    squad_cursor = 0;
    platoon_cursor = 0;

    encounter_data->valid = 1;
    data_delete_all(encounter_data);
    ai_pursuit_data->valid = 1;
    data_delete_all(ai_pursuit_data);

    zero = (uint32_t *)encounter_squad_states;
    for (i = 0x2000; i != 0; i = i - 1) {
        *zero = 0;
        zero = zero + 1;
    }
    zero = (uint32_t *)encounter_platoon_states;
    for (i = 0x400; i != 0; i = i - 1) {
        *zero = 0;
        zero = zero + 1;
    }

    encounter_index = 0;
    if (0 < scenario->encounters.count) {
        do {
            encounter_new(&squad_cursor,
                &((ScenarioEncounter *)scenario->encounters.pointer)[encounter_index],
                &platoon_cursor);
            encounter_index = encounter_index + 1;
        } while ((int32_t)encounter_index < scenario->encounters.count);
    }
}

#if 0
Original Ghidra decompilation (0x435cb0):

void __cdecl encounters_reset(void)

{
  int iVar1;
  int iVar2;
  short sVar3;
  undefined4 *puVar4;
  undefined4 local_8;
  undefined4 local_4;

  iVar1 = global_scenario;
  local_4 = 0;
  local_8 = 0;
  *(undefined1 *)(DAT_008802c8 + 0x24) = 1;
  data_delete_all();
  *(undefined1 *)(DAT_008802d0 + 0x24) = 1;
  data_delete_all();
  puVar4 = DAT_008802cc;
  for (iVar2 = 0x2000; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar4 = 0;
    puVar4 = puVar4 + 1;
  }
  puVar4 = DAT_008802c4;
  for (iVar2 = 0x400; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar4 = 0;
    puVar4 = puVar4 + 1;
  }
  sVar3 = 0;
  if (0 < *(int *)(iVar1 + 0x42c)) {
    do {
      squad_create(&local_8);
      sVar3 = sVar3 + 1;
    } while ((int)sVar3 < *(int *)(iVar1 + 0x42c));
  }
  return;
}
#endif
