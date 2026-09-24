// sound_cache_dispose  (Ghidra: sound_cache_dispose, already named)
// address 0x443f30, size 148 bytes
// name confidence: 0.85   rewrite confidence: 0.65
// evidence: per-entry body is byte-identical to sound_permutation_release_page (0x443d30:
// samples_pointer != -1 -> cache_evict_entry, then samples_pointer = -1, pad_30 = 0), called
// here through each live sound_cache_entry's permutation field (offset 0x0c). The trailing
// data_array::valid (0x24) clear and scratch-buffer GlobalFree match types/cache.h /
// types/memory.h exactly. Elided data_iterator_next() reconstructed the same way as in
// src/memory/cache_build_status_bitmap.c: a data_iterator built on this function's own stack
// over sound_cache_entries.
// register convention: none; plain __cdecl with no parameters.
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include <stdint.h>

extern data_array *sound_cache_entries; // 0x006ac528
extern void *sound_decode_buffer;       // 0x006f17ec
extern int32_t sound_decode_buffer_size; // 0x006f17f0

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0
extern void *GlobalFree(void *memory); // 0x0063a0bc IAT

extern void sound_permutation_release_page(SoundPermutation *permutation); // this module, sound_permutation_release_page.c

// Fully tears down the sound cache: releases every live entry's page-cache reference (through
// its owning permutation), invalidates the entry array, and frees the shared decode scratch
// buffer if one was ever allocated.
void sound_cache_dispose(void)
{
    data_iterator iterator;
    sound_cache_entry *entry;

    iterator.data = sound_cache_entries;
    iterator.next_index = 0;
    iterator.index = 0;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
    entry = (sound_cache_entry *)data_iterator_next(&iterator);
    while (entry != (sound_cache_entry *)0) {
        sound_permutation_release_page(entry->permutation);
        entry = (sound_cache_entry *)data_iterator_next(&iterator);
    }

    sound_cache_entries->valid = 0;

    if (sound_decode_buffer != (void *)0) {
        GlobalFree(sound_decode_buffer);
        sound_decode_buffer = (void *)0;
        sound_decode_buffer_size = 0;
    }
    return;
}

#if 0
Original Ghidra decompilation (0x443f30):

void __cdecl sound_cache_dispose(void)

{
  HGLOBAL hMem;
  int iVar1;
  int iVar2;

  iVar1 = DAT_006ac528;
  iVar2 = data_iterator_next();
  hMem = DAT_006f17ec;
  while (DAT_006f17ec = hMem, iVar2 != 0) {
    iVar2 = *(int *)(iVar2 + 0xc);
    if (*(int *)(iVar2 + 0x2c) != -1) {
      cache_evict_entry();
    }
    *(undefined4 *)(iVar2 + 0x2c) = 0xffffffff;
    *(undefined4 *)(iVar2 + 0x30) = 0;
    iVar2 = data_iterator_next();
    hMem = DAT_006f17ec;
    iVar1 = DAT_006ac528;
  }
  *(undefined1 *)(iVar1 + 0x24) = 0;
  if (hMem != (HGLOBAL)0x0) {
    GlobalFree(hMem);
    DAT_006f17ec = (HGLOBAL)0x0;
    DAT_006f17f0 = 0;
  }
  return;
}
#endif
