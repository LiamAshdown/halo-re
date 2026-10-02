// datum_new  (Ghidra: datum_new, already named)
// address 0x4d0480, size 131 bytes
// name confidence: 0.65   rewrite confidence: 0.75
// evidence: out/phase4/memory_types_notes.md "datum_new @0x4d0480: +0x2c = next free index
// cursor, +0x30 = actual count, +0x32 = next identifier (post-increment, reseeded to 0x8000 when
// it wraps to 0), +0x2e = high-water mark." All field offsets match types/memory.h data_array.
// register convention: data_array* in EDX (in_EDX); no other explicit registers.

#include "tags.h"
#include "memory.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// blam-cc: array in EDX
// Allocates the next free slot in `array` (scanning forward from next_index), zero-initializing
// it and assigning it a fresh generation salt. Returns the new handle (salt<<16 | index), or
// k_datum_index_none if the array is full.
datum_index datum_new(data_array *array)
{
    int16_t element_size;
    int16_t index;
    uint32_t result;
    int16_t *element;
    int16_t *scan;
    uint32_t words;
    uint32_t bytes;
    uint8_t *dst;

    element_size = array->size;
    index = array->next_index;
    result = 0xffffffff;
    element = (int16_t *)((int32_t)index * (int32_t)element_size + (int32_t)array->data);
    if (index < array->maximum_count) {
        while (*element != 0) {
            index = index + 1;
            element = (int16_t *)((uint8_t *)element + element_size);
            if (array->maximum_count <= index) {
                return result;
            }
        }
        dst = (uint8_t *)element;
        for (words = (uint32_t)(int32_t)element_size >> 2; words != 0; words = words - 1) {
            *(uint32_t *)dst = 0;
            dst = dst + 4;
        }
        for (bytes = (uint32_t)(int32_t)element_size & 3; bytes != 0; bytes = bytes - 1) {
            *dst = 0;
            dst = dst + 1;
        }
        *element = array->next_identifier;
        array->next_identifier = array->next_identifier + 1;
        if (array->next_identifier == 0) {
            array->next_identifier = (int16_t)k_datum_identifier_wrap;
        }
        array->actual_count = array->actual_count + 1;
        array->next_index = index + 1;
        if (array->last_index <= index) {
            array->last_index = index + 1;
        }
        result = (uint32_t)((int32_t)*element << 0x10) | (uint16_t)index;
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x4d0480):

uint datum_new(void)

{
  uint uVar1;
  uint uVar2;
  int in_EDX;
  short sVar3;
  short *psVar4;
  short *psVar5;

  uVar2 = (uint)*(short *)(in_EDX + 0x22);
  sVar3 = *(short *)(in_EDX + 0x2c);
  uVar1 = 0xffffffff;
  psVar4 = (short *)((int)sVar3 * uVar2 + *(int *)(in_EDX + 0x34));
  if (sVar3 < *(short *)(in_EDX + 0x20)) {
    while (*psVar4 != 0) {
      sVar3 = sVar3 + 1;
      psVar4 = (short *)((int)psVar4 + uVar2);
      if (*(short *)(in_EDX + 0x20) <= sVar3) {
        return uVar1;
      }
    }
    psVar5 = psVar4;
    for (uVar1 = uVar2 >> 2; uVar1 != 0; uVar1 = uVar1 - 1) {
      psVar5[0] = 0;
      psVar5[1] = 0;
      psVar5 = psVar5 + 2;
    }
    for (uVar2 = uVar2 & 3; uVar2 != 0; uVar2 = uVar2 - 1) {
      *(undefined1 *)psVar5 = 0;
      psVar5 = (short *)((int)psVar5 + 1);
    }
    *psVar4 = *(short *)(in_EDX + 0x32);
    *(short *)(in_EDX + 0x32) = *(short *)(in_EDX + 0x32) + 1;
    if (*(short *)(in_EDX + 0x32) == 0) {
      *(undefined2 *)(in_EDX + 0x32) = 0x8000;
    }
    *(short *)(in_EDX + 0x30) = *(short *)(in_EDX + 0x30) + 1;
    *(short *)(in_EDX + 0x2c) = sVar3 + 1;
    if (*(short *)(in_EDX + 0x2e) <= sVar3) {
      *(short *)(in_EDX + 0x2e) = sVar3 + 1;
    }
    uVar1 = (int)*psVar4 << 0x10 | (int)sVar3;
  }
  return uVar1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
