// datum_new_at_index  (Ghidra: datum_new_at_index, already named)
// address 0x4d0430, size 75 bytes
// name confidence: 0.5   rewrite confidence: 0.75
// evidence: out/phase4/memory_types_notes.md, same data_array fields as
// datum_new_at_index_with_salt (0x4d03d0); this variant auto-assigns the salt via
// datum_element_initialize instead of taking a caller-supplied one.
// register convention: index in AX (in_AX), data_array* in EDX (in_EDX).

#include "tags.h"
#include "memory.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void datum_element_initialize(data_array *array, void *element); // blam-cc: array in EDX, element in ESI

// blam-cc: index in AX, array in EDX
// Allocates the data_array slot at `index`, auto-assigning it a fresh generation salt via
// datum_element_initialize. Fails (returns k_datum_index_none) if the index is out of range or
// the slot is already in use.
datum_index datum_new_at_index(int16_t index, data_array *array)
{
    int16_t *element;

    if (-1 < index && index < array->maximum_count) {
        element = (int16_t *)((int32_t)array->size * (int32_t)index + (int32_t)array->data);
        if (*element == 0) {
            array->actual_count = array->actual_count + 1;
            if (array->last_index <= index) {
                array->last_index = index + 1;
            }
            datum_element_initialize(array, element);
            return (uint32_t)((int32_t)*element << 0x10) | (uint16_t)index;
        }
        return 0xffffffff;
    }
    return 0xffffffff;
}

#if 0
Original Ghidra decompilation (0x4d0430):

uint datum_new_at_index(void)

{
  short in_AX;
  int in_EDX;
  short *psVar1;

  if ((-1 < in_AX) && (in_AX < *(short *)(in_EDX + 0x20))) {
    psVar1 = (short *)((int)*(short *)(in_EDX + 0x22) * (int)in_AX + *(int *)(in_EDX + 0x34));
    if (*psVar1 == 0) {
      *(short *)(in_EDX + 0x30) = *(short *)(in_EDX + 0x30) + 1;
      if (*(short *)(in_EDX + 0x2e) <= in_AX) {
        *(short *)(in_EDX + 0x2e) = in_AX + 1;
      }
      FUN_004d06c0();
      return (int)*psVar1 << 0x10 | (int)in_AX;
    }
    return 0xffffffff;
  }
  return 0xffffffff;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
