// chimera__rasterizer_memory_alloc  (Ghidra: chimera__rasterizer_memory_alloc, already named --
// Chimera name, hint only per halo-retail-not-ce.md)
// address 0x514560, size 59 bytes
// name confidence: 0.6   rewrite confidence: 0.75
// evidence: bump-allocates out of rasterizer_scratch_memory (0x0071d13c), advancing
//   rasterizer_scratch_memory_used (0x0071d140) and refusing once it would exceed
//   k_rasterizer_scratch_memory_size (0x18000, types/rasterizer.h).
// register convention: source data pointer (or NULL) in in_EAX, size in in_ECX.
//   // blam-cc: EAX -> source, ECX -> size

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include <string.h>

extern void *rasterizer_scratch_memory;      // 0x0071d13c
extern uint32_t rasterizer_scratch_memory_used; // 0x0071d140

// blam-cc: EAX -> source, ECX -> size
// Carves a `size` byte block from the fixed scratch memory pool, or returns NULL if the pool
// would overflow. When `source` is non-NULL, copies `size` bytes from it into the new block.
void *chimera__rasterizer_memory_alloc(void *source, uint32_t size)
{
    uint32_t new_used;
    void *block;

    new_used = rasterizer_scratch_memory_used + size;
    block = (void *)0;
    if (new_used < 0x18001) {
        block = (uint8_t *)rasterizer_scratch_memory + rasterizer_scratch_memory_used;
        rasterizer_scratch_memory_used = new_used;
        if (source != (void *)0) {
            memcpy(block, source, size);
        }
    }
    return block;
}

#if 0
Original Ghidra decompilation (0x514560):

undefined4 * chimera__rasterizer_memory_alloc(void)

{
  undefined4 *in_EAX;
  undefined4 *puVar1;
  uint in_ECX;
  uint uVar2;
  undefined4 *puVar3;

  uVar2 = DAT_0071d140 + in_ECX;
  puVar1 = (undefined4 *)0x0;
  if (uVar2 < 0x18001) {
    puVar1 = (undefined4 *)(DAT_0071d13c + DAT_0071d140);
    DAT_0071d140 = uVar2;
    if (in_EAX != (undefined4 *)0x0) {
      puVar3 = puVar1;
      for (uVar2 = in_ECX >> 2; uVar2 != 0; uVar2 = uVar2 - 1) {
        *puVar3 = *in_EAX;
        in_EAX = in_EAX + 1;
        puVar3 = puVar3 + 1;
      }
      for (uVar2 = in_ECX & 3; uVar2 != 0; uVar2 = uVar2 - 1) {
        *(undefined1 *)puVar3 = *(undefined1 *)in_EAX;
        in_EAX = (undefined4 *)((int)in_EAX + 1);
        puVar3 = (undefined4 *)((int)puVar3 + 1);
      }
    }
  }
  return puVar1;
}
#endif
