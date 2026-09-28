// texture_cache_entry_release  (not a Ghidra function; the "pc texture" cache callback)
// address 0x444730, size 193 bytes
// name confidence: 0.75  rewrite confidence: 0.9
// evidence: texture_cache_new 0x4444d0 passes 0x444730 to cache_new; the cache calls it cdecl with an entry handle. Only reachable as that
//   pointer; first-boot track: reached once textures started streaming for the UI map.
// objdump 0x444730..0x4447f0: waits (Sleep(0)) until the entry's read has completed; its bitmap (+8) loses its
//   cache handle (+0x24 = -1) and its GlobalAlloc'd pixels (+0x2c, GlobalFree); a bitmap flagged at +0xe bit 7 is
//   evicted from texture_cache when its handle is still set (never, as the handle was just cleared -- reproduced)
//   and loses handle and pixels again; its D3D texture (+0x28) is released; the entry is deleted.
// blam-cc: stack -> handle (cdecl)

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"

extern data_array *texture_cache_entries; // 0x006ac538
extern void *texture_cache; // 0x006ac540
extern void cache_evict_entry(datum_index handle, void *self); // 0x4d1c20, blam-cc: EBX -> handle, EDI -> self
extern void datum_delete(data_array *array, datum_index handle); // 0x4d0510, blam-cc: EAX -> array, EDX -> handle
typedef int32_t (__stdcall *d3d_release_fn)(void *object);

void texture_cache_entry_release(datum_index handle)
{
    texture_cache_entry *entry = (texture_cache_entry *)texture_cache_entries->data + (handle & 0xffff);
    BitmapData *bitmap;
    void *texture;

    while (((texture_cache_entry *)texture_cache_entries->data + (handle & 0xffff))->loaded == 0) {
        Sleep(0);
    }
    bitmap = entry->bitmap;
    bitmap->pointer = (uint32_t)-1;       // the texture cache handle
    if (bitmap->pixel_base != 0) {
        GlobalFree(bitmap->pixel_base);
        bitmap->pixel_base = 0;
    }
    bitmap = entry->bitmap;
    if ((*(uint8_t *)&bitmap->flags & 0x80) != 0) { // flags bit 7, read as a byte like the original
        if ((int32_t)bitmap->pointer != -1) {
            cache_evict_entry((datum_index)bitmap->pointer, texture_cache);
        }
        bitmap->pointer = (uint32_t)-1;
        bitmap->pixel_base = 0;
    }
    texture = (void *)bitmap->hardware_texture;
    if (texture != 0) {
        ((d3d_release_fn)(*(void ***)texture)[2])(texture);
        bitmap->hardware_texture = 0;
    }
    datum_delete(texture_cache_entries, handle);
}
