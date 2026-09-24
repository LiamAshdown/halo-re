// datum_next  (Ghidra: FUN_004d0630)
// address 0x4d0630, size 69 bytes
// name confidence: 0.45   rewrite confidence: 0.75
// evidence: out/phase4/memory_types_notes.md "datum_get, FUN_004d0630 (datum_next),
// FUN_004d06c0 (datum_element_initialize), ... all consistent."; out/phase4/memory_functions.md
// summary "Finds and returns the handle of the next in-use data_array slot after index in_DX, or
// -1 if none remain."
// register convention: starting index in DX (in_DX), data_array* in EDI (unaff_EDI).

#include "tags.h"
#include "memory.h"

// blam-cc: index in DX, array in EDI
// Finds the handle of the next in-use slot strictly after `after_index`, or k_datum_index_none
// if none remain.
datum_index datum_next(int16_t after_index, data_array *array)
{
    uint32_t result;
    int16_t index;
    int16_t *element;

    result = 0xffffffff;
    index = after_index + 1;
    if (-1 < index && index < array->last_index) {
        element = (int16_t *)((int32_t)index * (int32_t)array->size + (int32_t)array->data);
        while (*element == 0) {
            index = index + 1;
            element = (int16_t *)((uint8_t *)element + array->size);
            if (array->last_index <= index) {
                return result;
            }
        }
        result = (uint32_t)((int32_t)*element << 0x10) | (uint16_t)index;
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x4d0630):

uint FUN_004d0630(void)

{
  uint uVar1;
  short *psVar2;
  short in_DX;
  short sVar3;
  int unaff_EDI;

  uVar1 = 0xffffffff;
  sVar3 = in_DX + 1;
  if ((-1 < sVar3) && (sVar3 < *(short *)(unaff_EDI + 0x2e))) {
    psVar2 = (short *)((int)sVar3 * (int)*(short *)(unaff_EDI + 0x22) + *(int *)(unaff_EDI + 0x34));
    while (*psVar2 == 0) {
      sVar3 = sVar3 + 1;
      psVar2 = (short *)((int)psVar2 + (int)*(short *)(unaff_EDI + 0x22));
      if (*(short *)(unaff_EDI + 0x2e) <= sVar3) {
        return uVar1;
      }
    }
    uVar1 = (int)*psVar2 << 0x10 | (int)sVar3;
  }
  return uVar1;
}
#endif
