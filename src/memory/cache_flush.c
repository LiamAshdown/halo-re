// cache_flush
// address 0x4d17f0, size 78 bytes
// name confidence: 0.75 (module summary)
// rewrite confidence: 0.4 -- Ghidra shows this function with a literally empty signature and no
// locals at all (`void cache_flush(void)`), which is only possible if the data_iterator both
// calls share lives in this function's own stack frame, set up by prologue code Ghidra's P-code
// analysis dropped entirely (not even an unaff_/in_ register hint remains). The `cache *self`
// parameter and the local data_iterator below are reconstructed from data_iterator_next's own
// known signature (types/memory.h data_iterator) and the module summary ("Evicts every entry
// currently stored in a cache by iterating its data_array and evicting each one"); the exact
// register/stack convention this function itself used cannot be recovered from the decompilation.
// evidence: out/phase4/memory_types_notes.md, data_iterator_next.c, cache_evict_entry.c.
// register convention: UNSURE -- reconstructed as cache* in a single argument; see above.

#include "tags.h"
#include "memory.h"

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, below this batch's
    // assigned range
extern void cache_evict_entry(datum_index handle, cache *self); // this batch, cache_evict_entry.c

void cache_flush(cache *self)
{
    data_iterator iterator;

    iterator.data = self->entries;
    iterator.next_index = 0;
    iterator.index = 0;

    while (data_iterator_next(&iterator) != 0) {
        cache_evict_entry(iterator.index, self);
    }
}

#if 0
Original Ghidra decompilation (0x4d17f0):

void cache_flush(void)

{
  int iVar1;

  iVar1 = data_iterator_next();
  while (iVar1 != 0) {
    cache_evict_entry();
    iVar1 = data_iterator_next();
  }
  return;
}
#endif
