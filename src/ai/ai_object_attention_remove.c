// ai_object_attention_remove  (Ghidra: ai_object_attention_remove; named for this rewrite)
// address 0x435990, size 111 bytes
// name confidence: 0.4   rewrite confidence: 0.7
// evidence: phase-4 summary ("removes a per-object record from the fixed table populated by
//   ai_get_or_create_unit_record"); same table base (ai_globals + 0x3b8), same count
//   (ai_globals.unknown_3b6) and same 0x28-byte stride (the compaction copies 10 dwords) as
//   ai_object_attention_find_or_create @0x435900.
// register convention: EAX -> object_index (Ghidra's in_EAX); no stack arguments.
//   // blam-cc: EAX -> object_index
//
// Removal is an unordered swap-with-last: the count is decremented first, and the record is
// only overwritten when it was not already the last one.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "fn_ai.h"

extern ai_globals *ai_globals_ptr; // 0x00880354

// blam-cc: EAX -> object_index
void ai_object_attention_remove(datum_index object_index)
{
    ai_object_attention_record *table;
    int16_t index;
    int16_t last;
    int32_t i;
    int32_t *src;
    int32_t *dst;

    table = (ai_object_attention_record *)ai_globals_ptr->unknown_3b8;

    if (object_index == (datum_index)k_datum_index_none) {
        return;
    }
    if (ai_globals_ptr->object_record_count <= 0) {
        return;
    }

    index = 0;
    while (table[index].object_index != object_index) {
        index = index + 1;
        if (ai_globals_ptr->object_record_count <= index) {
            return;
        }
    }

    last = ai_globals_ptr->object_record_count - 1;
    ai_globals_ptr->object_record_count = last;
    if (index < last) {
        src = (int32_t *)&table[last];
        dst = (int32_t *)&table[index];
        for (i = 10; i != 0; i = i - 1) {
            *dst = *src;
            src = src + 1;
            dst = dst + 1;
        }
    }
}

#if 0
Original Ghidra decompilation (0x435990):

void FUN_00435990(void)

{
  short sVar1;
  int iVar2;
  int in_EAX;
  short sVar3;
  int iVar4;
  undefined4 *puVar5;
  undefined4 *puVar6;

  iVar2 = DAT_00880354;
  if (in_EAX != -1) {
    sVar3 = 0;
    if (0 < *(short *)(DAT_00880354 + 0x3b6)) {
      while (*(int *)(DAT_00880354 + 0x3b8 + sVar3 * 0x28) != in_EAX) {
        sVar3 = sVar3 + 1;
        if (*(short *)(DAT_00880354 + 0x3b6) <= sVar3) {
          return;
        }
      }
      sVar1 = *(short *)(DAT_00880354 + 0x3b6) + -1;
      *(short *)(DAT_00880354 + 0x3b6) = sVar1;
      if (sVar3 < sVar1) {
        puVar5 = (undefined4 *)(iVar2 + 0x3b8 + sVar1 * 0x28);
        puVar6 = (undefined4 *)(iVar2 + 0x3b8 + sVar3 * 0x28);
        for (iVar4 = 10; iVar4 != 0; iVar4 = iVar4 + -1) {
          *puVar6 = *puVar5;
          puVar5 = puVar5 + 1;
          puVar6 = puVar6 + 1;
        }
      }
    }
  }
  return;
}
#endif
