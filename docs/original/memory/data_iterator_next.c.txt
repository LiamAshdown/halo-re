// data_iterator_next  (Ghidra: data_iterator_next, already named)
// address 0x4d05d0, size 85 bytes
// name confidence: 0.75   rewrite confidence: 0.75
// evidence: objdump 0x4d05d0..0x4d0624: reads [edi] (data_array*), reads and writes only
// WORD [edi+4] (resume index), writes [edi+8] (the handle of the element returned); the +0x0c
// signature is never touched. Matches types/memory.h data_iterator (0x10 bytes).
// register convention: data_iterator* in EDI (unaff_EDI).
// reconciled: R16 data_iterator is 0x10 bytes with an int16 next_index (0x4d05d0 uses only WORD [edi+4])

#include "tags.h"
#include "memory.h"

// blam-cc: iterator in EDI
// Advances `iterator` to the next in-use element at or after its resume index, returning that
// element's address (or NULL once the array is exhausted). On a real element, iterator->index is
// updated to that element's handle; either way iterator->next_index is updated so a later call
// resumes from where this one left off.
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
void *data_iterator_next(data_iterator *iterator)
{
    int16_t resume;
    int16_t element_size;
    int16_t *element;
    int16_t *found;
    uint32_t handle_index;

    resume = iterator->next_index;
    found = 0;
    element_size = iterator->data->size;
    element = (int16_t *)((int32_t)resume * (int32_t)element_size +
                          (int32_t)iterator->data->data);
    if (resume < iterator->data->last_index) {
        for (;;) {
            found = element;
            handle_index = (uint32_t)(uint16_t)resume;
            resume = resume + 1;
            if (*found != 0) {
                break;
            }
            element = (int16_t *)((uint8_t *)found + element_size);
            if (iterator->data->last_index <= resume) {
                iterator->next_index = resume;
                return 0;
            }
        }
        iterator->index = (uint32_t)((int32_t)*found << 0x10) | handle_index;
    }
    iterator->next_index = resume;
    return found;
}

#if 0
Original Ghidra decompilation (0x4d05d0):

short * data_iterator_next(void)

{
  int iVar1;
  short *psVar2;
  short *psVar3;
  uint uVar4;
  short sVar5;
  int *unaff_EDI;

  sVar5 = (short)unaff_EDI[1];
  iVar1 = *unaff_EDI;
  psVar3 = (short *)0x0;
  psVar2 = (short *)((int)sVar5 * (int)*(short *)(iVar1 + 0x22) + *(int *)(iVar1 + 0x34));
  if (sVar5 < *(short *)(iVar1 + 0x2e)) {
    while( true ) {
      psVar3 = psVar2;
      uVar4 = (uint)sVar5;
      sVar5 = sVar5 + 1;
      if (*psVar3 != 0) break;
      psVar2 = (short *)((int)psVar3 + (int)*(short *)(iVar1 + 0x22));
      if (*(short *)(*unaff_EDI + 0x2e) <= sVar5) {
        *(short *)(unaff_EDI + 1) = sVar5;
        return (short *)0x0;
      }
    }
    unaff_EDI[2] = (int)*psVar3 << 0x10 | uVar4;
  }
  *(short *)(unaff_EDI + 1) = sVar5;
  return psVar3;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
