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
    uint8_t *entry = (uint8_t *)texture_cache_entries->data + (handle & 0xffff) * 0x10;
    uint8_t *bitmap;
    void *texture;

    while (((uint8_t *)texture_cache_entries->data + (handle & 0xffff) * 0x10)[4] == 0) {
        Sleep(0);
    }
    bitmap = *(uint8_t **)(entry + 8);
    *(int32_t *)(bitmap + 0x24) = -1;
    if (*(void **)(bitmap + 0x2c) != 0) {
        GlobalFree(*(void **)(bitmap + 0x2c));
        *(void **)(bitmap + 0x2c) = 0;
    }
    bitmap = *(uint8_t **)(entry + 8);
    if ((bitmap[0xe] & 0x80) != 0) {
        if (*(int32_t *)(bitmap + 0x24) != -1) {
            cache_evict_entry(*(datum_index *)(bitmap + 0x24), texture_cache);
        }
        *(int32_t *)(bitmap + 0x24) = -1;
        *(void **)(bitmap + 0x2c) = 0;
    }
    texture = *(void **)(bitmap + 0x28);
    if (texture != 0) {
        ((d3d_release_fn)(*(void ***)texture)[2])(texture);
        *(void **)(bitmap + 0x28) = 0;
    }
    datum_delete(texture_cache_entries, handle);
}
