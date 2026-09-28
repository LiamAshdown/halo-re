// texture_cache_new  (Ghidra: texture_cache_new, already named)
// address 0x4444d0, size 125 bytes
// name confidence: 0.8   rewrite confidence: 0.8
// evidence: types/cache.h texture_cache_entry section: "texture_cache_new @0x4444d0:
// data_new(\"pc texture\", 0x1000) then GlobalAlloc(0, 0x1c07c) == 0x7c + 0x1000*0x1c ... then
// cache_new(that, 0x1000, 2, 0x1000, 0x444730, 0x444700)"; strings "pc texture"/"pc texture
// cache" in this function's own decompile. Directly parallel to sound_cache_new @0x443ca0,
// already rewritten in this module, which fixes the argument order of cache_new/data_new.
// register convention: none; plain __cdecl with no parameters (matches sound_cache_new).
//
// UNSURE: LAB_00444730 (texture_cache_entry release) and LAB_00444700 (in-use predicate) are
// outside this batch's assigned range and are not given C definitions here; only their
// addresses are taken as function pointers, matching the original.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "cache.h"

extern data_array *texture_cache_entries; // 0x006ac538
extern struct cache *texture_cache;       // 0x006ac540
extern void *texture_cache_base;          // 0x006ac53c
extern void *texture_cache_memory;        // 0x006ac550, VirtualAlloc'd elsewhere

extern data_array *data_new(int16_t element_size, char *name, int16_t maximum_count); // 0x4d0370
extern void cache_new(char *name, struct cache *self, int32_t block_count, int32_t block_shift,
    int16_t maximum_count, void *release_procedure, void *in_use_procedure); // 0x4d1750

extern void texture_cache_entry_release(void); // 0x444730, outside this module
extern void texture_cache_entry_in_use(void);  // 0x444700, outside this module

// Initializes the runtime texture-page memory pool and LRU page cache used to stream bitmap
// data: a "pc texture" data_array of up to 0x1000 texture_cache_entry datums, and a cache
// container rationing 4-byte slots (block_shift 2) over the 0x4000-byte texture_cache_memory
// VirtualAlloc -- it hands out cache slot handles, not pixel storage; the pixels live in a
// per-bitmap GlobalAlloc at BitmapData::_pad_2c (see texture_cache_page_allocate.c).
void texture_cache_new(void)
{
    void *cache_memory;

    texture_cache_entries = data_new(sizeof(texture_cache_entry), "pc texture", k_texture_cache_maximum_entries);

    cache_memory = GlobalAlloc(0, 0x1c07c);
    if (cache_memory != (void *)0) {
        cache_new("pc texture cache", (struct cache *)cache_memory, k_texture_cache_maximum_entries,
            k_texture_cache_block_shift, k_texture_cache_maximum_entries,
            (void *)texture_cache_entry_release, (void *)texture_cache_entry_in_use);
    }
    texture_cache = (struct cache *)cache_memory;
    texture_cache_base = texture_cache_memory;
    return;
}

#if 0
Original Ghidra decompilation (0x4444d0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void texture_cache_new(void)

{
  HGLOBAL pvVar1;

  DAT_006ac538 = data_new("pc texture",0x1000);
  pvVar1 = GlobalAlloc(0,0x1c07c);
  if (pvVar1 != (HGLOBAL)0x0) {
    cache_new(pvVar1,0x1000,2,0x1000,&LAB_00444730,&LAB_00444700);
    DAT_006ac540 = pvVar1;
    _DAT_006ac53c = DAT_006ac550;
    return;
  }
  DAT_006ac540 = pvVar1;
  _DAT_006ac53c = DAT_006ac550;
  return;
}
#endif
