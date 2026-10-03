// heap_find_free_block  (Ghidra: FUN_004d2370)
// address 0x4d2370, size 85 bytes
// name confidence: 0.85 (module summary: "Searches a bucketed heap's free-block chain for the
// first block large enough to satisfy a size request, returning its address and predecessor")
// rewrite confidence: 0.65
// evidence: types/memory.h heap/heap_block layout.
// register convention: heap* in ECX (in_ECX), size needed in ESI (unaff_ESI); out-predecessor
// pointer on the stack (param_1).

#include "tags.h"
#include "memory.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
int32_t heap_find_free_block(heap *self, uint32_t size_needed, void **out_predecessor)
{
    heap_block *prev = self->first_block;
    int32_t result = 0;

    if (prev == 0) {
        return result;
    }
    if (size_needed <= (uint32_t)((uint8_t *)prev - self->base)) {
        return (int32_t)self->base;
    }
    if (prev->next != 0) {
        heap_block *next = prev->next;
        heap_block *cur;
        while (cur = next,
            (uint32_t)((uint8_t *)cur - ((uint8_t *)prev + (prev->size & k_heap_block_size_mask))) <
                size_needed) {
            next = cur->next;
            prev = cur;
            if (cur->next == 0) {
                return result;
            }
        }
        result = (int32_t)((prev->size & k_heap_block_size_mask) + (uint8_t *)prev);
        *out_predecessor = prev;
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x4d2370):

int FUN_004d2370(undefined4 *param_1)

{
  uint *puVar1;
  int iVar2;
  int in_ECX;
  uint *puVar3;
  uint *puVar4;
  uint unaff_ESI;

  puVar4 = *(uint **)(in_ECX + 0x2c);
  iVar2 = 0;
  if (puVar4 != (uint *)0x0) {
    if (unaff_ESI <= (uint)((int)puVar4 - *(int *)(in_ECX + 4))) {
      return *(int *)(in_ECX + 4);
    }
    if ((puVar4 != (uint *)0x0) && (puVar1 = (uint *)puVar4[3], (uint *)puVar4[3] != (uint *)0x0)) {
      while (puVar3 = puVar1, (int)puVar3 + (-(int)puVar4 - (*puVar4 & 0x7fffffff)) < unaff_ESI) {
        puVar1 = (uint *)puVar3[3];
        puVar4 = puVar3;
        if ((uint *)puVar3[3] == (uint *)0x0) {
          return iVar2;
        }
      }
      iVar2 = (*puVar4 & 0x7fffffff) + (int)puVar4;
      *param_1 = puVar4;
    }
  }
  return iVar2;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
