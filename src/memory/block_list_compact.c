// block_list_compact
// address 0x4d1eb0, size 82 bytes
// name confidence: 0.85 (module summary)
// rewrite confidence: 0.65
// evidence: types/memory.h memory_pool/memory_pool_block.
// register convention: memory_pool* in EBX (unaff_EBX).

#include "tags.h"
#include "memory.h"
#include <string.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
void block_list_compact(memory_pool *arena)
{
    memory_pool_block *src = arena->first_block;
    memory_pool_block *dest;
    memory_pool_block *block = 0;
    memory_pool_block *previous = 0;

    if (src == 0) {
        return;
    }

    dest = (memory_pool_block *)arena->base;
    do {
        block = src;
        if (dest < src) {
            memmove(dest, src, (size_t)src->size);
            *dest->address = (uint8_t *)dest + 0x18; // fix up the owner's data pointer to the
                // block's new (lower) address.
            block = dest;
        }
        block->previous = previous;
        if (previous == 0) {
            arena->first_block = block;
        } else {
            previous->next = block;
        }
        dest = (memory_pool_block *)((uint8_t *)block + block->size);
        src = block->next;
        previous = block;
    } while (src != 0);

    block->next = 0;
    arena->last_block = block;
}

#if 0
Original Ghidra decompilation (0x4d1eb0):

void block_list_compact(void)

{
  void *_Src;
  void *pvVar1;
  int unaff_EBX;
  void *_Dst;
  void *pvVar2;

  _Src = *(void **)(unaff_EBX + 0x30);
  if (_Src != (void *)0x0) {
    _Dst = *(void **)(unaff_EBX + 0x24);
    pvVar2 = (void *)0x0;
    do {
      pvVar1 = _Src;
      if (_Dst < _Src) {
        _memmove(_Dst,_Src,*(size_t *)((int)_Src + 4));
        **(int **)((int)_Dst + 8) = (int)_Dst + 0x18;
        pvVar1 = _Dst;
      }
      *(void **)((int)pvVar1 + 0x10) = pvVar2;
      if (pvVar2 == (void *)0x0) {
        *(void **)(unaff_EBX + 0x30) = pvVar1;
      }
      else {
        *(void **)((int)pvVar2 + 0xc) = pvVar1;
      }
      _Dst = (void *)(*(int *)((int)pvVar1 + 4) + (int)pvVar1);
      _Src = *(void **)((int)pvVar1 + 0xc);
      pvVar2 = pvVar1;
    } while (_Src != (void *)0x0);
    *(undefined4 *)((int)pvVar1 + 0xc) = 0;
    *(void **)(unaff_EBX + 0x34) = pvVar1;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
