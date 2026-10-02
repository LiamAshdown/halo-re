// bitmap_data_free  (Ghidra: bitmap_group_free, renamed)
// address 0x43f880, size 94 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// evidence: out/phase4/bitmaps_types_notes.md ("bitmap_group_free frees one BitmapData, not a
// group: it reads the flags at +0x0e, the cache handle at +0x24, the texture at +0x28, the
// pixels at +0x2c, and GlobalFrees the ESI pointer itself. Better name: bitmap_data_free. Its
// caller 0x43f010, just below the range, indexes a reflexive (block->pointer + index*0x30) and
// passes the element in ESI."); types/bitmaps.h BitmapData::pointer (texture cache datum_index),
// _pad_28 (hardware texture) and _pad_2c (pixel base) notes; texture cache field reuse already
// established the same way in src/cache/texture_cache_get.c and
// src/cache/texture_cache_page_allocate.c (`*(void **)&bitmap->hardware_texture` / `_pad_2c`).
// register convention: ESI = BitmapData *bitmap_data.
//   // blam-cc: ESI -> bitmap_data

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "bitmaps.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern struct cache *texture_cache; // 0x006ac540
extern void cache_evict_entry(datum_index handle, struct cache *self); // 0x4d1c20, memory module

// blam-cc: ESI -> bitmap_data
void bitmap_data_free(BitmapData *bitmap_data)
{
    if (bitmap_data == 0) {
        return;
    }

    if (bitmap_data->flags & _bitmap_data_texture_cache_bit) {
        if (bitmap_data->pointer != (uint32_t)k_datum_index_none) {
            cache_evict_entry((datum_index)bitmap_data->pointer, texture_cache);
        }
        bitmap_data->pointer = (uint32_t)k_datum_index_none;
        bitmap_data->pixel_base = 0;
    }

    if (*(void **)&bitmap_data->hardware_texture != 0) {
        void *hardware_texture = *(void **)&bitmap_data->hardware_texture;
        void **vtable = *(void ***)hardware_texture;
        ((bitmap_hardware_texture_release_proc)vtable[2])(hardware_texture); // call [ecx+0x08]
        *(void **)&bitmap_data->hardware_texture = 0;
    }

    if (bitmap_data->flags & _bitmap_data_runtime_allocated_bit) {
        if (bitmap_data->pixel_base != 0) {
            GlobalFree(bitmap_data->pixel_base);
        }
        GlobalFree(bitmap_data);
    }
}

#if 0
Original Ghidra decompilation (0x43f880), Ghidra name bitmap_group_free:

void bitmap_group_free(void)

{
  int *piVar1;
  HGLOBAL unaff_ESI;

  if (unaff_ESI != (HGLOBAL)0x0) {
    if (*(char *)((int)unaff_ESI + 0xe) < '\0') {
      if (*(int *)((int)unaff_ESI + 0x24) != -1) {
        cache_evict_entry();
      }
      *(undefined4 *)((int)unaff_ESI + 0x24) = 0xffffffff;
      *(undefined4 *)((int)unaff_ESI + 0x2c) = 0;
    }
    piVar1 = *(int **)((int)unaff_ESI + 0x28);
    if (piVar1 != (int *)0x0) {
      (**(code **)(*piVar1 + 8))(piVar1);
      *(undefined4 *)((int)unaff_ESI + 0x28) = 0;
    }
    if ((*(byte *)((int)unaff_ESI + 0xe) & 0x40) != 0) {
      if (*(HGLOBAL *)((int)unaff_ESI + 0x2c) != (HGLOBAL)0x0) {
        GlobalFree(*(HGLOBAL *)((int)unaff_ESI + 0x2c));
      }
      GlobalFree(unaff_ESI);
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
