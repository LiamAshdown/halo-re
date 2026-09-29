// ai_reset_for_new_map  (Ghidra: ai_reset_for_new_map, already named)
// address 0x42a840, size 184 bytes
// name confidence: 0.5   rewrite confidence: 0.55
// evidence: types/ai.h ai_globals field-by-field ("ai_reset_for_new_map pins 0x00, 0x01,
//   0x02, 0x08, 0x10, 0x14..0x28, 0x130, 0x132, the 0xa0-dword block at 0x134, and 0x3b4").
//   types/memory.h data_array.valid (0x24). Calls data_delete_all (0x4d0580, ESI -> array),
//   encounters_reset and ai_communication_reset, both outside this rewrite's range.
// register convention: plain __cdecl, no parameters.
//   // blam-cc: (no arguments)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include <string.h>
#include "objects.h"
#include "units.h"

extern ai_globals *ai_globals_ptr;       // 0x00880354
extern data_array *actor_data;           // 0x00880360
extern data_array *swarm_data;           // 0x0088035c
extern data_array *swarm_component_data; // 0x00880358
extern data_array *prop_data;            // 0x008802c0

extern void data_delete_all(data_array *array); // 0x4d0580, blam-cc: ESI -> array
extern void encounters_reset(void); // 0x435cb0, not in this rewrite range
extern void ai_communication_reset(void); // 0x42d230, not in this rewrite range (resets communication timestamp tables)

// Clears all AI data arrays (actors, swarms, position cache, props) and resets the AI
// globals structure to its default state, e.g. when restarting or reloading a map.
void ai_reset_for_new_map(void)
{
    ai_globals *g = ai_globals_ptr;

    memset(g, 0, k_ai_globals_size);

    g->initialized = 1;
    g->unknown_02 = 1;
    g->unknown_08 = (datum_index)k_datum_index_none;
    g->grenades_enabled = 1;
    g->communication_valid = 1;
    g->unknown_14 = (datum_index)k_datum_index_none;
    g->weighted_actor_count = (datum_index)k_datum_index_none;
    g->unknown_1c = (datum_index)k_datum_index_none;
    g->unknown_20 = (datum_index)k_datum_index_none;
    g->unknown_24 = (datum_index)k_datum_index_none;
    g->unknown_28 = (datum_index)k_datum_index_none;

    actor_data->valid = 1;
    data_delete_all(actor_data);
    swarm_data->valid = 1;
    data_delete_all(swarm_data);
    swarm_component_data->valid = 1;
    data_delete_all(swarm_component_data);
    prop_data->valid = 1;
    data_delete_all(prop_data);

    encounters_reset();
    ai_communication_reset();

    g->unknown_132 = 0;
    g->unknown_130 = 0;
    memset(g->unknown_134, 0, sizeof(g->unknown_134));

    g->actors_valid = 1;
}

#if 0
Original Ghidra decompilation (0x42a840):

void ai_reset_for_new_map(void)

{
  int iVar1;
  int iVar2;
  undefined4 *puVar3;
  undefined4 *puVar4;

  iVar2 = DAT_00880360;
  puVar4 = DAT_00880354;
  puVar3 = DAT_00880354;
  for (iVar1 = 0x237; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar3 = 0;
    puVar3 = puVar3 + 1;
  }
  *(undefined1 *)puVar4 = 1;
  *(undefined1 *)((int)puVar4 + 2) = 1;
  puVar4[2] = 0xffffffff;
  *(undefined1 *)(puVar4 + 0xed) = 1;
  *(undefined1 *)(puVar4 + 4) = 1;
  puVar4[5] = 0xffffffff;
  puVar4[6] = 0xffffffff;
  puVar4[7] = 0xffffffff;
  puVar4[8] = 0xffffffff;
  puVar4[9] = 0xffffffff;
  puVar4[10] = 0xffffffff;
  *(undefined1 *)(iVar2 + 0x24) = 1;
  data_delete_all();
  *(undefined1 *)(DAT_0088035c + 0x24) = 1;
  data_delete_all();
  *(undefined1 *)(DAT_00880358 + 0x24) = 1;
  data_delete_all();
  *(undefined1 *)(DAT_008802c0 + 0x24) = 1;
  data_delete_all();
  encounters_reset();
  FUN_0042d230();
  puVar3 = DAT_00880354;
  puVar4 = DAT_00880354 + 0x4d;
  *(undefined2 *)((int)DAT_00880354 + 0x132) = 0;
  *(undefined2 *)(puVar3 + 0x4c) = 0;
  for (iVar2 = 0xa0; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar4 = 0;
    puVar4 = puVar4 + 1;
  }
  *(undefined1 *)((int)puVar3 + 1) = 1;
  return;
}
#endif
