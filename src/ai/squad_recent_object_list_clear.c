// squad_recent_object_list_clear  (Ghidra: squad_recent_object_list_clear, already named)
// address 0x436c10, size 73 bytes
// name confidence: 0.5   rewrite confidence: 0.5
// evidence: destroys every ai_pursuit record chained off encounter.first_pursuit, matching
// the phase-4 summary and the already-established field names.
// register convention: Ghidra could not resolve the encounter index at all.
//   // blam-cc: EAX -> encounter_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *encounter_data;  // 0x008802c8
extern data_array *ai_pursuit_data; // 0x008802d0

extern void datum_delete(data_array *array, datum_index index); // 0x4d0510

void squad_recent_object_list_clear(datum_index encounter_index)
{
    encounter *enc = &((encounter *)encounter_data->data)[encounter_index & 0xffff];
    datum_index cursor = enc->first_pursuit;

    while (cursor != (datum_index)k_datum_index_none) {
        ai_pursuit *pursuit = &((ai_pursuit *)ai_pursuit_data->data)[cursor & 0xffff];
        enc->first_pursuit = pursuit->next;
        datum_delete(ai_pursuit_data, cursor);
        cursor = enc->first_pursuit;
    }
}

#if 0
Original Ghidra decompilation (0x436c10):

void squad_recent_object_list_clear(void)

{
  uint uVar1;
  uint in_EAX;
  int iVar2;
  int iVar3;

  iVar2 = (in_EAX & 0xffff) * 0x6c + *(int *)(DAT_008802c8 + 0x34);
  uVar1 = *(uint *)(iVar2 + 0x38);
  iVar3 = DAT_008802d0;
  while (uVar1 != 0xffffffff) {
    *(undefined4 *)(iVar2 + 0x38) =
         *(undefined4 *)(*(int *)(iVar3 + 0x34) + 0x24 + (uVar1 & 0xffff) * 0x28);
    iVar3 = datum_delete();
    uVar1 = *(uint *)(iVar2 + 0x38);
  }
  return;
}
#endif
