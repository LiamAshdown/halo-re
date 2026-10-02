// texture_cache_entry_in_use  (not a Ghidra function; the "pc texture" cache callback)
// address 0x444700, size 37 bytes
// name confidence: 0.75  rewrite confidence: 0.9
// evidence: texture_cache_new 0x4444d0 passes 0x444700 to cache_new; the cache calls it cdecl with an entry handle. Only reachable as that
//   pointer; first-boot track: reached once textures started streaming for the UI map.
// objdump 0x444700..0x444724: in use while the entry's read has not completed (loaded byte +4 == 0).
// blam-cc: stack -> handle (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *texture_cache_entries; // 0x006ac538

uint8_t texture_cache_entry_in_use(datum_index handle)
{
    uint8_t *entry = (uint8_t *)texture_cache_entries->data + (handle & 0xffff) * 0x10;
    return entry[4] == 0;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
