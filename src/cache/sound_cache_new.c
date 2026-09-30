// sound_cache_new  (Ghidra: sound_cache_new, already named)
// address 0x443ca0, size 138 bytes
// name confidence: 0.85   rewrite confidence: 0.75
// evidence: out/phase4/cache_types_notes.md sound_cache_entry/texture_cache_entry section:
// "sound_cache_new @0x443ca0: data_new(\"pc sound\", 0x200) then GlobalAlloc(0, 0x387c) ==
// 0x7c + 0x200*0x1c ... then cache_new(that, page_count, 0xc, 0x200, 0x4440a0, 0x444060)";
// the two release/in-use procedures (LAB_004440a0, LAB_00444060) are the "small procedures not
// in cache_functions.md" the notes list under misattributed item 7 and are not rewritten here.
// register convention: none; plain __cdecl with no parameters.
//
// UNSURE: LAB_004440a0 (sound_cache_entry release) and LAB_00444060 (in-use predicate) are
// outside this batch's assigned range and are not given C definitions here; only their
// addresses are taken as function pointers, matching the original.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "fn_cache.h"
#include "fn_memory.h"

extern data_array *sound_cache_entries; // 0x006ac528
extern int32_t sound_cache_size_megabytes; // 0x006869c4, read but not owned by this module
extern int32_t sound_cache_page_count;  // 0x006f17e4
extern struct cache *sound_cache;       // 0x006ac530
extern void *sound_cache_base;          // 0x006ac52c
extern void *sound_cache_memory;        // 0x006ac554, VirtualAlloc'd elsewhere
extern uint8_t sound_cache_initialized; // 0x006ac534


extern void cache_new(char *name, struct cache *self, int32_t block_count, int32_t block_shift,
    int16_t maximum_count, void *release_procedure, void *in_use_procedure); // 0x4d1750

extern void sound_cache_entry_release(void); // 0x4440a0, outside this module
extern void sound_cache_entry_in_use(void);  // 0x444060, outside this module

// Initializes the runtime sound-page memory pool and LRU page cache used to stream sound sample
// data: a "pc sound" data_array of up to 0x200 sound_cache_entry datums, and a cache container
// carving sound_cache_memory into 4096-byte pages, sized from sound_cache_size_megabytes.
void sound_cache_new(void)
{
    int32_t scaled_megabytes;
    void *cache_memory;

    sound_cache_entries = data_new(sizeof(sound_cache_entry), "pc sound", k_sound_cache_maximum_entries);

    scaled_megabytes = sound_cache_size_megabytes * 0x100000;
    sound_cache_page_count = (scaled_megabytes + ((scaled_megabytes >> 0x1f) & 0xfff)) >> k_sound_cache_page_shift;

    cache_memory = GlobalAlloc(0, 0x387c);
    if (cache_memory != (void *)0) {
        cache_new("pc sound cache", (struct cache *)cache_memory, sound_cache_page_count,
            k_sound_cache_page_shift, k_sound_cache_maximum_entries,
            (void *)sound_cache_entry_release, (void *)sound_cache_entry_in_use);
    }
    sound_cache = (struct cache *)cache_memory;
    sound_cache_base = sound_cache_memory;
    sound_cache_initialized = 1;
    return;
}

#if 0
Original Ghidra decompilation (0x443ca0):

void sound_cache_new(void)

{
  int iVar1;
  HGLOBAL pvVar2;

  DAT_006ac528 = data_new("pc sound",0x200);
  iVar1 = (int)(DAT_006869c4 * 0x100000 + (DAT_006869c4 * 0x100000 >> 0x1f & 0xfffU)) >> 0xc;
  DAT_006f17e4 = iVar1;
  pvVar2 = GlobalAlloc(0,0x387c);
  if (pvVar2 != (HGLOBAL)0x0) {
    cache_new(pvVar2,iVar1,0xc,0x200,&LAB_004440a0,&LAB_00444060);
  }
  DAT_006ac530 = pvVar2;
  DAT_006ac52c = DAT_006ac554;
  DAT_006ac534 = 1;
  return;
}
#endif
