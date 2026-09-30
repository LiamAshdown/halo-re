// heap_compact  (Ghidra: FUN_004d2310)
// address 0x4d2310, size 94 bytes
// name confidence: 0.8 (module summary: "Defragments a bucketed heap in place by sliding its
// allocated blocks together to remove gaps left by frees")
// rewrite confidence: 0.55 -- every field access and the arithmetic is preserved exactly, but the
// higher-level intent of "only slide a block when its own size's bit 31 is clear" is not fully
// understood (bit 31 marks a block in-use everywhere else in this module, e.g. heap_allocate.c);
// preserved verbatim rather than reinterpreted.
// evidence: types/memory.h heap/heap_block layout.
// register convention: heap* in ECX (in_ECX).

#include "tags.h"
#include "memory.h"
#include "fn_memory.h"
#include <string.h>

void heap_compact(heap *self)
{
    heap_block *src = self->first_block;
    heap_block *dest;
    uint32_t shift;
    heap_block *prev;

    if (src == 0 || self->compaction_disabled != 0) {
        return;
    }

    shift = 0;
    prev = (heap_block *)self->base;
    do {
        dest = src;
        if (0 <= (int32_t)src->size &&
            (heap_block *)((uint8_t *)src - shift) != prev &&
            0 <= (int32_t)(((uint8_t *)src - shift) - (uint8_t *)prev)) {
            dest = (heap_block *)((uint8_t *)prev + shift);
            memmove(dest, src, src->size & k_heap_block_size_mask);
            if (dest->previous != 0) {
                dest->previous->next = dest;
            }
        }
        src = dest->next;
        shift = dest->size & k_heap_block_size_mask;
        prev = dest;
    } while (src != 0);
}

#if 0
Original Ghidra decompilation (0x4d2310):

void FUN_004d2310(void)

{
  uint *_Src;
  int in_ECX;
  uint uVar1;
  uint *_Dst;
  uint *puVar2;

  _Src = *(uint **)(in_ECX + 0x2c);
  if ((_Src != (uint *)0x0) && (*(char *)(in_ECX + 0x28) == '\0')) {
    uVar1 = 0;
    puVar2 = *(uint **)(in_ECX + 4);
    do {
      _Dst = _Src;
      if ((-1 < (int)*_Src) &&
         ((uint *)((int)_Src - uVar1) != puVar2 && -1 < (int)((int)_Src - uVar1) - (int)puVar2)) {
        _Dst = (uint *)(uVar1 + (int)puVar2);
        _memmove(_Dst,_Src,*_Src & 0x7fffffff);
        if (_Dst[2] != 0) {
          *(uint **)(_Dst[2] + 0xc) = _Dst;
        }
      }
      _Src = (uint *)_Dst[3];
      uVar1 = *_Dst & 0x7fffffff;
      puVar2 = _Dst;
    } while (_Src != (uint *)0x0);
  }
  return;
}
#endif
