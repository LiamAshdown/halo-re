// ai_object_attention_find_or_create  (Ghidra: ai_object_attention_find_or_create; named for this rewrite)
// address 0x435900, size 133 bytes
// name confidence: 0.35   rewrite confidence: 0.65
// evidence: phase-4 summary ("finds or allocates a per-object record in a small fixed table,
//   initializing a new entry with a default 8.0 timer value"); the table is
//   ai_globals + 0x3b8 with the count in ai_globals.unknown_3b6, records of 0x28 bytes
//   (10 dwords are zeroed), capacity 32 (the `0x1f < index` bail), key at +0x00 and 8.0f
//   (0x41000000) at +0x04. 0x3b8 + 32 * 0x28 = 0x8b8, which is exactly where
//   ai_globals.vehicle_entry_count starts, so the capacity is pinned by the layout.
// register convention: EDI -> object_index (Ghidra's unaff_EDI); no stack arguments.
//   // blam-cc: EDI -> object_index
//
// UNSURE: the key is compared as a full 32-bit handle, so it is a datum_index, but which
// datum array it indexes is not established here (zero callers in this build). The record
// body past +0x08 is never read inside this module.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern ai_globals *ai_globals_ptr; // 0x00880354

// blam-cc: EDI -> object_index
// Returns the attention record for object_index, creating (and zero-initialising) one at the
// end of the 32-entry table if it is not there yet. Returns NULL for a null handle, or when
// the table is full.
ai_object_attention_record *ai_object_attention_find_or_create(datum_index object_index)
{
    ai_object_attention_record *table;
    ai_object_attention_record *record;
    int16_t index;
    int32_t i;

    table = (ai_object_attention_record *)ai_globals_ptr->object_attention_table;
    record = 0;

    if (object_index == (datum_index)k_datum_index_none) {
        return 0;
    }

    index = 0;
    if (0 < ai_globals_ptr->object_attention_count) {
        index = 0;
        do {
            if (table[index].object_index == object_index) {
                break;
            }
            index = index + 1;
        } while (index < ai_globals_ptr->object_attention_count);
        if (0x1f < index) {
            return 0;
        }
    }

    record = &table[index];
    if (ai_globals_ptr->object_attention_count <= index) {
        for (i = 0; i < 10; i = i + 1) {
            ((int32_t *)record)[i] = 0;
        }
        record->object_index = object_index;
        record->weight = 8.0f; // 0x41000000
        ai_globals_ptr->object_attention_count = ai_globals_ptr->object_attention_count + 1;
    }
    return record;
}

#if 0
Original Ghidra decompilation (0x435900):

int * FUN_00435900(void)

{
  int iVar1;
  int *piVar2;
  short sVar3;
  int unaff_EDI;

  iVar1 = DAT_00880354;
  piVar2 = (int *)0x0;
  if (unaff_EDI != -1) {
    sVar3 = 0;
    if (0 < *(short *)(DAT_00880354 + 0x3b6)) {
      sVar3 = 0;
      do {
        if (*(int *)(DAT_00880354 + 0x3b8 + sVar3 * 0x28) == unaff_EDI) break;
        sVar3 = sVar3 + 1;
      } while (sVar3 < *(short *)(DAT_00880354 + 0x3b6));
      if (0x1f < sVar3) {
        return (int *)0x0;
      }
    }
    piVar2 = (int *)(DAT_00880354 + 0x3b8 + sVar3 * 0x28);
    if (*(short *)(DAT_00880354 + 0x3b6) <= sVar3) {
      *piVar2 = 0;
      piVar2[1] = 0;
      piVar2[2] = 0;
      piVar2[3] = 0;
      piVar2[4] = 0;
      piVar2[5] = 0;
      piVar2[6] = 0;
      piVar2[7] = 0;
      piVar2[8] = 0;
      piVar2[9] = 0;
      *piVar2 = unaff_EDI;
      piVar2[1] = 0x41000000;
      *(short *)(iVar1 + 0x3b6) = *(short *)(iVar1 + 0x3b6) + 1;
    }
  }
  return piVar2;
}
#endif
