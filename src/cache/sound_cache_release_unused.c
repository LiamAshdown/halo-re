// sound_cache_release_unused  (Ghidra: FUN_00443fd0; renamed, named directly in
// out/phase4/cache_types_notes.md: "sound_cache_release_unused @0x443fd0 evicts exactly the
// entries with entry[5] == 0 && entry[6] == 0")
// address 0x443fd0, size 134 bytes
// name confidence: 0.75   rewrite confidence: 0.7
// evidence: same per-entry release body as sound_cache_dispose/sound_permutation_release_page;
// guard condition matches sound_cache_entry::lock_count (0x05) / playing (0x06) in
// types/cache.h. Elided data_iterator_next() reconstructed as in sound_cache_dispose.c.
// register convention: none; plain __cdecl with no parameters.
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include <stdint.h>

extern data_array *sound_cache_entries; // 0x006ac528

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0

extern void sound_permutation_release_page(SoundPermutation *permutation); // this module, sound_permutation_release_page.c

// Releases page-cache references for every sound entry that is neither locked nor currently
// playing, without a full cache teardown. A no-op if the sound cache has not been created yet.
void sound_cache_release_unused(void)
{
    data_iterator iterator;
    sound_cache_entry *entry;

    if (sound_cache_entries != (data_array *)0 && sound_cache_entries->valid != 0) {
        iterator.data = sound_cache_entries;
        iterator.next_index = 0;
        iterator.index = 0;
        iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
        entry = (sound_cache_entry *)data_iterator_next(&iterator);
        while (entry != (sound_cache_entry *)0) {
            if (entry->lock_count == 0 && entry->playing == 0) {
                sound_permutation_release_page(entry->permutation);
            }
            entry = (sound_cache_entry *)data_iterator_next(&iterator);
        }
    }
    return;
}

#if 0
Original Ghidra decompilation (0x443fd0):

void FUN_00443fd0(void)

{
  int iVar1;

  if ((DAT_006ac528 != 0) && (*(char *)(DAT_006ac528 + 0x24) != '\0')) {
    iVar1 = data_iterator_next();
    while (iVar1 != 0) {
      if ((*(char *)(iVar1 + 5) == '\0') && (*(char *)(iVar1 + 6) == '\0')) {
        iVar1 = *(int *)(iVar1 + 0xc);
        if (*(int *)(iVar1 + 0x2c) != -1) {
          cache_evict_entry();
        }
        *(undefined4 *)(iVar1 + 0x2c) = 0xffffffff;
        *(undefined4 *)(iVar1 + 0x30) = 0;
      }
      iVar1 = data_iterator_next();
    }
  }
  return;
}
#endif
