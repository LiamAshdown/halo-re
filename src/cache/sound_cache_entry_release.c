// sound_cache_entry_release  (not a Ghidra function; the "pc sound" cache callback)
// address 0x4440a0, size 52 bytes
// name confidence: 0.75  rewrite confidence: 0.9
// evidence: sound_cache_new 0x443ca0 passes 0x4440a0 to cache_new; the cache calls it cdecl with an entry handle. Only reachable as that
//   pointer; first-boot track: reached once textures started streaming for the UI map.
// objdump 0x4440a0..0x4440d3: the owning permutation (+0xc) forgets its cache handle (+0x2c = -1) and sample
//   pointer (+0x30 = 0), then the entry is deleted (tail jump to datum_delete).
// blam-cc: stack -> handle (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"

extern data_array *sound_cache_entries; // 0x006ac528
extern void datum_delete(data_array *array, datum_index handle); // 0x4d0510, blam-cc: EAX -> array, EDX -> handle

void sound_cache_entry_release(datum_index handle)
{
    uint8_t *entry = (uint8_t *)sound_cache_entries->data + (handle & 0xffff) * 0x10;
    uint8_t *permutation = *(uint8_t **)(entry + 0xc);

    *(int32_t *)(permutation + 0x2c) = -1;
    *(uint32_t *)(permutation + 0x30) = 0;
    datum_delete(sound_cache_entries, handle);
}
