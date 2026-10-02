// sound_cache_touch  (Ghidra: FUN_00443e10; renamed, referenced by name in
// out/phase4/cache_types_notes.md's cache_io_request priority paragraph and matches the
// cache_functions.md summary "Ensures a sound entry's sample data page is resident in the
// cache, optionally blocking (Sleep(0) loop) until the pending load completes")
// address 0x443e10, size 284 bytes
// name confidence: 0.6   rewrite confidence: 0.60
// evidence: field offsets (samples_pointer 0x2c on SoundPermutation; loaded 0x02, decoded 0x03,
// lock_count 0x05, playing 0x06, io_request_index 0x08 on sound_cache_entry; entries 0x3c, age
// 0x30 on cache and age 0x14 on cache_entry, types/memory.h) all match. The frame watchdog at
// DAT_0072520c and the priority-raise expression `*(index*0x30 + 0x1c + cache_io_requests) = 1`
// are exactly what cache_types_notes.md quotes for sound_cache_touch.
// register convention (corrected, objdump 0x443e10): allocate_if_missing and lock are the two stack
// arguments; wait_until_loaded in EBX (BL), permutation pointer in EDI.
//
// UNSURE: the exact register slots for allocate_if_missing/lock/wait_until_loaded/permutation
// are inferred from the EAX,ECX,EDX,EBX,ESI,EDI ordering convention, not individually confirmed
// by disassembly. The two 0/1 return paths pack a boolean into the return value's low byte with
// undefined high bits in the original (uVar4<<8, CONCAT31(uVar5,1)); only 0/1 is reproduced
// here, matching the house simplification in src/memory/bit_stream_write_bit.c. The call to
// sound_cache_page_allocate below shows zero visible arguments at this call site in the
// decompile (both its permutation and priority arguments are register/stack-implicit); it is
// called here with `permutation` and priority 0 pending confirmation from that function's own
// rewrite.

// phase-4 review pass: body re-checked instruction by instruction against `objdump -d -M
// intel` of this address range; every field offset, branch and argument below now matches
// the machine code rather than only Ghidra's pseudo-C.
#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "math.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *sound_cache_entries;  // 0x006ac528
extern struct cache *sound_cache;        // 0x006ac530
extern cache_io_request *cache_io_requests; // 0x006ac4a0
extern int64_t performance_frequency;    // 0x006ac8f8/0x006ac8fc
extern int32_t sound_time;      // 0x0072520c


extern void sound_cache_decode_permutation(SoundPermutation *permutation); // this module, sound_cache_decode_permutation.c
extern void sound_cache_page_allocate(SoundPermutation *permutation, uint8_t priority); // blam-cc:
    // permutation in EAX; this module, sound_cache_page_allocate.c
extern uint32_t sound_idle_update(void); // outside this module; frame-watchdog pump, UNSURE

// blam-cc: EBX -> wait_until_loaded, EDI -> permutation, stack -> allocate_if_missing, lock
// (the flags are read as bytes at [esp+0x14] and [esp+0x1c] inside the frame)
// Ensures a sound permutation's sample page is resident in the sound cache. If the permutation
// has no page (samples_pointer == -1) and allocate_if_missing is set, attempts to allocate one;
// gives up immediately if that still leaves no page. Otherwise raises the pending read's IO
// priority (when wait_until_loaded is set and the page has not started loading) and either polls
// once or busy-waits (Sleep(0), pumping the frame watchdog through FUN_00549960 when stalled too
// long) until the page is loaded, decoding it on first use via sound_cache_decode_permutation.
// Optionally pins the entry (lock_count++) once ready. Returns 1 on success, 0 if the page never
// becomes resident and wait_until_loaded is not set.
// The return value is byte-wide: the original leaves EAX's upper 24 bits as whatever the last
// call left there (`return uVar1 & 0xffffff00` / `CONCAT31(uVar5,1)`), so only AL is meaningful.
uint8_t sound_cache_touch(uint8_t allocate_if_missing, uint8_t lock, uint8_t wait_until_loaded,
    SoundPermutation *permutation)
{
    sound_cache_entry *entry;
    large_integer counter;
    int32_t elapsed_ms;
    uint32_t stall_ms;

    if (permutation->samples_pointer == 0xffffffff) {
        if (allocate_if_missing != 0) {
            // `push ebx` at 0x00443e2b: the priority handed to the allocator is
            // wait_until_loaded itself, the same idiom texture_cache_get uses at 0x00444574.
            sound_cache_page_allocate(permutation, wait_until_loaded);
        }
        if (permutation->samples_pointer == 0xffffffff) {
            return 0;
        }
    }

    entry = (sound_cache_entry *)((uint8_t *)sound_cache_entries->data +
        (permutation->samples_pointer & 0xffff) * sizeof(sound_cache_entry));

    // Touch the generic cache container's LRU bookkeeping entry for this datum directly
    // (cache_entry::age, types/memory.h), marking it as used at the cache's current age.
    ((cache_entry *)((uint8_t *)sound_cache->entries->data +
        (permutation->samples_pointer & 0xffff) * sizeof(cache_entry)))->age = sound_cache->age;

    if (wait_until_loaded != 0 && entry->loaded == 0) {
        cache_io_requests[entry->io_request_index].priority = 1;
    }

    for (;;) {
        if (entry->loaded != 0) {
            if (entry->decoded == 0) {
                entry->decoded = 1;
                entry->lock_count = 0;
                entry->playing = 0;
                sound_cache_decode_permutation(permutation);
            }
            if (lock != 0) {
                entry->lock_count = entry->lock_count + 1;
            }
            return 1;
        }

        QueryPerformanceCounter((LARGE_INTEGER *)&counter);
        elapsed_ms = (int32_t)((counter.quad_part * 1000) / performance_frequency);
        stall_ms = (uint32_t)(elapsed_ms - sound_time);
        if (0x84 < stall_ms) {
            sound_idle_update();
        }

        if (wait_until_loaded == 0) {
            break;
        }
        Sleep(0);
    }

    return 0;
}

#if 0
Original Ghidra decompilation (0x443e10):

uint FUN_00443e10(char param_1,char param_2)

{
  uint3 uVar4;
  uint3 extraout_var;
  uint uVar1;
  int iVar2;
  undefined3 uVar5;
  int iVar3;
  int extraout_EAX;
  undefined3 extraout_var_00;
  char unaff_BL;
  int iVar6;
  int unaff_EDI;
  undefined8 uVar7;
  LARGE_INTEGER local_8;

  if (*(int *)(unaff_EDI + 0x2c) == -1) {
    uVar4 = 0xffffff;
    if (param_1 != '\0') {
      FUN_004440e0();
      uVar4 = extraout_var;
    }
    if (*(int *)(unaff_EDI + 0x2c) == -1) {
      return (uint)uVar4 << 8;
    }
  }
  uVar1 = *(uint *)(unaff_EDI + 0x2c) & 0xffff;
  iVar2 = uVar1 * 0x1c;
  iVar6 = uVar1 * 0x10 + *(int *)(DAT_006ac528 + 0x34);
  *(undefined4 *)(*(int *)(*(int *)(DAT_006ac530 + 0x3c) + 0x34) + 0x14 + iVar2) =
       *(undefined4 *)(DAT_006ac530 + 0x30);
  iVar3 = DAT_006ac4a0;
  if ((unaff_BL != '\0') &&
     (iVar2 = CONCAT31((int3)((uint)iVar2 >> 8),*(char *)(iVar6 + 2)), *(char *)(iVar6 + 2) == '\0')
     ) {
    *(undefined1 *)(*(short *)(iVar6 + 8) * 0x30 + 0x1c + DAT_006ac4a0) = 1;
    iVar2 = iVar3;
  }
  while( true ) {
    uVar5 = (undefined3)((uint)iVar2 >> 8);
    if (*(char *)(iVar6 + 2) != '\0') {
      if (*(char *)(iVar6 + 3) == '\0') {
        *(undefined1 *)(iVar6 + 3) = 1;
        *(undefined1 *)(iVar6 + 5) = 0;
        *(undefined1 *)(iVar6 + 6) = 0;
        FUN_00443d60();
        uVar5 = extraout_var_00;
      }
      if (param_2 != '\0') {
        *(char *)(iVar6 + 5) = *(char *)(iVar6 + 5) + '\x01';
      }
      return CONCAT31(uVar5,1);
    }
    QueryPerformanceCounter(&local_8);
    uVar7 = __allmul(local_8.s.LowPart,local_8.s.HighPart,1000,0);
    iVar3 = __alldiv(uVar7,DAT_006ac8f8,DAT_006ac8fc);
    uVar1 = iVar3 - DAT_0072520c;
    if (0x84 < uVar1) {
      uVar1 = FUN_00549960();
    }
    if (unaff_BL == '\0') break;
    Sleep(0);
    iVar2 = extraout_EAX;
  }
  return uVar1 & 0xffffff00;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
