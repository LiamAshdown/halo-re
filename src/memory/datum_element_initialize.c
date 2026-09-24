// datum_element_initialize  (Ghidra: FUN_004d06c0)
// address 0x4d06c0, size 51 bytes
// name confidence: 0.45   rewrite confidence: 0.75
// evidence: out/phase4/memory_types_notes.md "datum_new @0x4d0480: ... FUN_004d06c0
// (datum_element_initialize) ... all consistent."; out/phase4/memory_functions.md summary
// "Zero-initializes a newly allocated data_array element and stamps it with the array's next
// salt/generation value."
// register convention: data_array* in EDX (in_EDX), element pointer in ESI (unaff_ESI).

#include "tags.h"
#include "memory.h"

// blam-cc: array in EDX, element in ESI
// Zero-fills one element's worth of bytes at `element`, then stamps its datum_header::identifier
// with array->next_identifier and advances/reseeds that salt counter (wrapping to
// k_datum_identifier_wrap when it would otherwise become k_datum_identifier_none).
void datum_element_initialize(data_array *array, void *element)
{
    int16_t element_size;
    uint32_t words;
    uint32_t bytes;
    uint8_t *dst;

    element_size = array->size;
    dst = (uint8_t *)element;
    for (words = (uint32_t)(int32_t)element_size >> 2; words != 0; words = words - 1) {
        *(uint32_t *)dst = 0;
        dst = dst + 4;
    }
    for (bytes = (uint32_t)(int32_t)element_size & 3; bytes != 0; bytes = bytes - 1) {
        *dst = 0;
        dst = dst + 1;
    }
    *(int16_t *)element = array->next_identifier;
    array->next_identifier = array->next_identifier + 1;
    if (array->next_identifier == 0) {
        array->next_identifier = (int16_t)k_datum_identifier_wrap;
    }
}

#if 0
Original Ghidra decompilation (0x4d06c0):

void FUN_004d06c0(void)

{
  short sVar1;
  uint uVar2;
  int in_EDX;
  undefined4 *unaff_ESI;
  undefined4 *puVar3;

  sVar1 = *(short *)(in_EDX + 0x22);
  puVar3 = unaff_ESI;
  for (uVar2 = (uint)(int)sVar1 >> 2; uVar2 != 0; uVar2 = uVar2 - 1) {
    *puVar3 = 0;
    puVar3 = puVar3 + 1;
  }
  for (uVar2 = (int)sVar1 & 3; uVar2 != 0; uVar2 = uVar2 - 1) {
    *(undefined1 *)puVar3 = 0;
    puVar3 = (undefined4 *)((int)puVar3 + 1);
  }
  *(undefined2 *)unaff_ESI = *(undefined2 *)(in_EDX + 0x32);
  *(short *)(in_EDX + 0x32) = *(short *)(in_EDX + 0x32) + 1;
  if (*(short *)(in_EDX + 0x32) == 0) {
    *(undefined2 *)(in_EDX + 0x32) = 0x8000;
  }
  return;
}
#endif
