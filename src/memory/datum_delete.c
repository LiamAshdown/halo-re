// datum_delete  (Ghidra: datum_delete, already named)
// address 0x4d0510, size 108 bytes
// name confidence: 0.7   rewrite confidence: 0.65
// evidence: out/phase4/memory_types_notes.md "datum_delete @0x4d0510: validates index against
// +0x2e (not +0x20), clears the element identifier, rewinds +0x2c, walks +0x2e back over
// trailing free slots, decrements +0x30."
// register convention: data_array* in EAX (in_EAX), datum_index handle in EDX (in_EDX).
// UNSURE: when the handle fails validation (bad index, empty slot, or salt mismatch), the
// original code falls through to an unconditional `*psVar1 = 0` with psVar1 set to NULL -- a
// guaranteed null-pointer write/crash. This looks like the original relies on callers never
// passing an invalid handle (an assert-by-crash rather than a graceful failure return), and is
// preserved exactly rather than "fixed" into an early return.

#include "tags.h"
#include "memory.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// blam-cc: array in EAX, handle in EDX
// Frees the datum slot named by `handle` (index in the low 16 bits, salt in the high 16, with a
// zero salt acting as a wildcard that matches any live slot), recycling its slot and compacting
// the array's trailing free space. Passing an invalid handle (bad index, empty slot, or salt
// mismatch) is not a supported call and crashes via a null-pointer write -- see UNSURE above.
void datum_delete(data_array *array, datum_index handle)
{
    int16_t index;
    int16_t salt;
    int16_t *element;

    index = (int16_t)handle;
    element = 0;
    if (-1 < index && index < array->last_index) {
        element = (int16_t *)((int32_t)array->size * (int32_t)index + (int32_t)array->data);
        if (*element != 0) {
            salt = (int16_t)(handle >> 0x10);
            if (salt == 0 || salt == *element) {
                goto do_delete;
            }
        }
        element = 0;
    }

do_delete:
    *element = 0;
    if (index < array->next_index) {
        array->next_index = index;
    }
    if (index + 1 == (int32_t)array->last_index) {
        do {
            element = (int16_t *)((uint8_t *)element - array->size);
            array->last_index = array->last_index - 1;
            if (array->last_index < 1) {
                break;
            }
        } while (*element == 0);
    }
    array->actual_count = array->actual_count - 1;
}

#if 0
Original Ghidra decompilation (0x4d0510):

void datum_delete(void)

{
  int in_EAX;
  short *psVar1;
  short sVar2;
  undefined4 in_EDX;
  short sVar3;

  sVar2 = (short)in_EDX;
  if ((-1 < sVar2) && (sVar2 < *(short *)(in_EAX + 0x2e))) {
    psVar1 = (short *)((int)*(short *)(in_EAX + 0x22) * (int)sVar2 + *(int *)(in_EAX + 0x34));
    if ((*psVar1 != 0) &&
       ((sVar3 = (short)((uint)in_EDX >> 0x10), sVar3 == 0 || (sVar3 == *psVar1))))
    goto LAB_004d0543;
  }
  psVar1 = (short *)0x0;
LAB_004d0543:
  *psVar1 = 0;
  if (sVar2 < *(short *)(in_EAX + 0x2c)) {
    *(short *)(in_EAX + 0x2c) = sVar2;
  }
  if (sVar2 + 1 == (int)*(short *)(in_EAX + 0x2e)) {
    do {
      psVar1 = (short *)((int)psVar1 - (int)*(short *)(in_EAX + 0x22));
      *(short *)(in_EAX + 0x2e) = *(short *)(in_EAX + 0x2e) + -1;
      if (*(short *)(in_EAX + 0x2e) < 1) break;
    } while (*psVar1 == 0);
  }
  *(short *)(in_EAX + 0x30) = *(short *)(in_EAX + 0x30) + -1;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
