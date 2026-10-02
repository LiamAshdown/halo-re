// cache_io_sound_decode_thunk  (Ghidra: missed_443e00; named directly by
// src/cache/sound_cache_page_allocate.c, which already declares
// "extern void cache_io_sound_decode_thunk(cache_io_completion *record); // 0x443e00")
// address 0x443e00, size 15 bytes
// name confidence: 0.8 (matches the existing caller-side extern)   rewrite confidence: 0.9
// evidence: out/phase4/cache_types_notes.md "`0x00443e00` is a 15-byte thunk Ghidra never made a
// function: `mov eax,[esp+4]; mov ecx,[eax+8]; mov eax,[ecx+0xc]; jmp 0x443d60`. It takes the
// completion record, walks `record->data` (the `sound_cache_entry`) to `entry->permutation`, and
// tail-calls the decoder. That is what pins `cache_io_completion::data` to +0x08." types/cache.h
// cache_io_completion (data at +0x08) and sound_cache_entry (permutation at +0x0c) match exactly.
// The branchless procedure-selection code in sound_cache_page_allocate installs this thunk for
// every SoundPermutation format except xbox_adpcm (format 1).
// register convention: the completion record pointer is the single recognized cdecl stack
// argument (matching cache_io_completion_routine's own APC signature); the callee it tail-calls,
// sound_cache_decode_permutation, takes its SoundPermutation* in EAX (in_EAX) -- confirmed by
// objdump: `mov eax,[esp+4]` loads the argument, the next two loads walk +0x08 then +0xc into
// EAX, and the function ends in a plain `jmp` (not `call`/`ret`), so EAX is still live as the
// callee's register argument when control reaches 0x443d60.

#include "tags.h"
#include "memory.h"
#include "cache.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void sound_cache_decode_permutation(SoundPermutation *permutation); // 0x443d60, this module

// Cache-I/O completion procedure installed by sound_cache_page_allocate for every sound format
// except xbox_adpcm. Recovers the SoundPermutation from the completion record's sound_cache_entry
// (record->data->permutation) and tail-calls sound_cache_decode_permutation on it.
void cache_io_sound_decode_thunk(cache_io_completion *record)
{
    sound_cache_entry *entry;

    entry = (sound_cache_entry *)record->data;
    sound_cache_decode_permutation(entry->permutation);
}

#if 0
Original Ghidra decompilation (0x443e00):

void missed_443e00(void)

{
  sound_cache_decode_permutation();
  return;
}

Raw disassembly (Ghidra did not give this address a function boundary):
  443e00: 8b 44 24 04    mov    eax,DWORD PTR [esp+0x4]
  443e04: 8b 48 08       mov    ecx,DWORD PTR [eax+0x8]
  443e07: 8b 41 0c       mov    eax,DWORD PTR [ecx+0xc]
  443e0a: e9 51 ff ff ff jmp    0x443d60
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
