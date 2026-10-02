// cache_build_status_bitmap  (Ghidra: FUN_004d1ca0)
// address 0x4d1ca0, size 190 bytes
// name confidence: 0.8 (out/phase4/memory_types_notes.md names it directly and documents the
// exact status bits produced, matching cache_block_status_flags in types/memory.h)
// rewrite confidence: 0.6
// evidence: types/memory.h cache/cache_entry layout and cache_block_status_flags enum, matched
// bit-for-bit against this function's `bVar5` assignments (1 allocated, 2 current-age, 4 stale,
// 8 locked/in-use).
// register convention: cdecl, both parameters (cache* and the destination bitmap) recovered
// cleanly by Ghidra as ordinary stack parameters. The one elided call, data_iterator_next(), is
// reconstructed the same way as in cache_flush.c: a data_iterator built on this function's own
// stack over self->entries.
// UNSURE: `in_use_procedure` is called with the literal handle 0xffffffff for every entry (not
// that entry's own handle) -- preserved exactly as decompiled rather than "fixed" to pass the
// entry's real handle; it means every entry gets the same locked/not-locked bit.
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original

#include "tags.h"
#include "memory.h"
#include <string.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, below this batch's
    // assigned range

void cache_build_status_bitmap(cache *self, uint8_t *bitmap)
{
    data_iterator iterator;
    cache_entry *entry;

    memset(bitmap, 0, (uint32_t)self->block_count);

    iterator.data = self->entries;
    iterator.next_index = 0;
    iterator.index = 0;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
    entry = (cache_entry *)data_iterator_next(&iterator);
    while (entry != 0) {
        uint8_t status = _cache_block_allocated_bit;

        if (self->in_use_procedure != 0 &&
            ((int32_t (*)(datum_index))self->in_use_procedure)((datum_index)0xffffffff) != 0) {
            status = 9; // _cache_block_allocated_bit | _cache_block_locked_bit
        }
        if ((uint32_t)entry->age == (uint32_t)self->age) {
            status = status | _cache_block_current_bit;
        }
        if ((uint32_t)entry->age + 0x1e < (uint32_t)self->age) {
            status = status | _cache_block_stale_bit;
        }

        memset(bitmap + entry->offset, status, (uint32_t)entry->size);

        entry = (cache_entry *)data_iterator_next(&iterator);
    }
}

#if 0
Original Ghidra decompilation (0x4d1ca0):

void FUN_004d1ca0(int param_1,undefined4 *param_2)

{
  char cVar1;
  int iVar2;
  uint uVar3;
  uint uVar4;
  byte bVar5;
  undefined4 *puVar6;
  byte *pbVar7;

  uVar4 = *(uint *)(param_1 + 0x28);
  puVar6 = param_2;
  for (uVar3 = uVar4 >> 2; uVar3 != 0; uVar3 = uVar3 - 1) {
    *puVar6 = 0;
    puVar6 = puVar6 + 1;
  }
  for (uVar4 = uVar4 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
    *(undefined1 *)puVar6 = 0;
    puVar6 = (undefined4 *)((int)puVar6 + 1);
  }
  iVar2 = data_iterator_next();
  while (iVar2 != 0) {
    bVar5 = 1;
    if ((*(code **)(param_1 + 0x24) != (code *)0x0) &&
       (cVar1 = (**(code **)(param_1 + 0x24))(0xffffffff), cVar1 != '\0')) {
      bVar5 = 9;
    }
    if (*(uint *)(iVar2 + 0x14) == *(uint *)(param_1 + 0x30)) {
      bVar5 = bVar5 | 2;
    }
    if (*(uint *)(iVar2 + 0x14) + 0x1e < *(uint *)(param_1 + 0x30)) {
      bVar5 = bVar5 | 4;
    }
    uVar4 = *(uint *)(iVar2 + 4);
    pbVar7 = (byte *)(*(int *)(iVar2 + 8) + (int)param_2);
    for (uVar3 = uVar4 >> 2; uVar3 != 0; uVar3 = uVar3 - 1) {
      *(uint *)pbVar7 = CONCAT22(CONCAT11(bVar5,bVar5),CONCAT11(bVar5,bVar5));
      pbVar7 = pbVar7 + 4;
    }
    for (uVar4 = uVar4 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
      *pbVar7 = bVar5;
      pbVar7 = pbVar7 + 1;
    }
    iVar2 = data_iterator_next();
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
