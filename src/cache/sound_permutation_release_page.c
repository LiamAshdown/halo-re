// sound_permutation_release_page  (Ghidra: FUN_00443d30; renamed, no Blam-style hint in
// cache_functions.md, chosen from the summary "Releases a single cached sound entry's
// page-cache reference, if it holds one")
// address 0x443d30, size 38 bytes
// name confidence: 0.45   rewrite confidence: 0.65
// evidence: out/phase4/cache_types_notes.md's "SoundPermutation::samples_pointer (0x2c) is the
// sound cache handle, -1 when not resident, and _pad_30 is the address of the resident page"
// paragraph describes exactly this field pair; sound_cache global (DAT_006ac530) matches
// types/cache.h sound_cache. This is a distinct, higher-level helper from the cache container's
// own registered release callback at 0x4440a0 the notes attribute that paragraph to -- this one
// is the manual "release if resident" entry point called elsewhere in the engine.
// register convention: SoundPermutation pointer in ESI (unaff_ESI).
// UNSURE: cache_evict_entry's handle argument is not visible in the decompile (no explicit
// stack push), inferred to be permutation->samples_pointer since that is the only datum_index
// available and it is the field being reset immediately afterward.

#include "tags.h"
#include "memory.h"
#include "cache.h"

extern struct cache *sound_cache; // 0x006ac530

extern void cache_evict_entry(datum_index handle, struct cache *self); // 0x4d1c20

// blam-cc: SoundPermutation pointer in ESI (unaff_ESI)
// Releases a single sound permutation's page-cache reference, if it currently holds one
// (samples_pointer != -1), then always resets the handle to -1 and clears the resident-page
// address.
void sound_permutation_release_page(SoundPermutation *permutation)
{
    if (permutation->samples_pointer != 0xffffffff) {
        cache_evict_entry((datum_index)permutation->samples_pointer, sound_cache);
    }
    permutation->samples_pointer = 0xffffffff;
    permutation->cache_page = 0;
    return;
}

#if 0
Original Ghidra decompilation (0x443d30):

void FUN_00443d30(void)

{
  int unaff_ESI;

  if (*(int *)(unaff_ESI + 0x2c) != -1) {
    cache_evict_entry();
  }
  *(undefined4 *)(unaff_ESI + 0x2c) = 0xffffffff;
  *(undefined4 *)(unaff_ESI + 0x30) = 0;
  return;
}
#endif
