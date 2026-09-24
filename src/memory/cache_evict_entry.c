// cache_evict_entry
// address 0x4d1c20, size 127 bytes
// name confidence: 0.7 (module summary, and consistent with being cache_flush's/
// cache_allocate_block's only eviction primitive)
// rewrite confidence: 0.6
// evidence: types/memory.h cache (first/last/entries/release_procedure at 0x34/0x38/0x3c/0x20)
// and cache_entry (next/previous at 0x0c/0x10) layouts, matched field-for-field.
// register convention: datum_index handle in EBX (unaff_EBX, low 16 bits are the index); cache*
// in EDI (unaff_EDI).
// UNSURE: `(**(code **)(unaff_EDI + 0x20))();` -- the release_procedure callback -- is called
// with an empty argument list in Ghidra's decompile, the same elision seen throughout this
// module's more register-heavy functions. It is reconstructed here as being called with the
// entry's own handle, the only value that is both in scope and matches how release_procedure is
// used from cache_allocate_block (which also just checks in_use_procedure against a handle); the
// real argument(s), if any differ, could not be recovered from the decompilation.

#include "tags.h"
#include "memory.h"

extern void datum_delete(data_array *array, datum_index handle); // 0x4d0510, below this batch's
    // assigned range

void cache_evict_entry(datum_index handle, cache *self)
{
    cache_entry *entry = (cache_entry *)((uint8_t *)self->entries->data +
        (uint32_t)(uint16_t)handle * sizeof(cache_entry));

    if (self->release_procedure != 0) {
        ((void (*)(datum_index))self->release_procedure)(handle); // UNSURE, see file header
    }

    if (entry->previous == (datum_index)0xffffffff) {
        self->first = entry->next;
    } else {
        cache_entry *previous = (cache_entry *)((uint8_t *)self->entries->data +
            (uint32_t)(uint16_t)entry->previous * sizeof(cache_entry));
        previous->next = entry->next;
    }

    if (entry->next != (datum_index)0xffffffff) {
        cache_entry *next = (cache_entry *)((uint8_t *)self->entries->data +
            (uint32_t)(uint16_t)entry->next * sizeof(cache_entry));
        next->previous = entry->previous;
        datum_delete(self->entries, handle);
        return;
    }
    self->last = entry->previous;
    datum_delete(self->entries, handle);
}

#if 0
Original Ghidra decompilation (0x4d1c20):

void cache_evict_entry(void)

{
  uint unaff_EBX;
  int iVar1;
  int unaff_EDI;

  iVar1 = (unaff_EBX & 0xffff) * 0x1c + *(int *)(*(int *)(unaff_EDI + 0x3c) + 0x34);
  if (*(code **)(unaff_EDI + 0x20) != (code *)0x0) {
    (**(code **)(unaff_EDI + 0x20))();
  }
  if (*(uint *)(iVar1 + 0x10) == 0xffffffff) {
    *(undefined4 *)(unaff_EDI + 0x34) = *(undefined4 *)(iVar1 + 0xc);
  }
  else {
    *(undefined4 *)
     ((*(uint *)(iVar1 + 0x10) & 0xffff) * 0x1c + 0xc + *(int *)(*(int *)(unaff_EDI + 0x3c) + 0x34))
         = *(undefined4 *)(iVar1 + 0xc);
  }
  if (*(uint *)(iVar1 + 0xc) != 0xffffffff) {
    *(undefined4 *)
     ((*(uint *)(iVar1 + 0xc) & 0xffff) * 0x1c + 0x10 + *(int *)(*(int *)(unaff_EDI + 0x3c) + 0x34))
         = *(undefined4 *)(iVar1 + 0x10);
    datum_delete();
    return;
  }
  *(undefined4 *)(unaff_EDI + 0x38) = *(undefined4 *)(iVar1 + 0x10);
  datum_delete();
  return;
}
#endif
