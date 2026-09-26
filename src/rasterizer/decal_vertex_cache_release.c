// decal_vertex_cache_release  (not a Ghidra function; the "decal vertex cache" release procedure)
// address 0x51a660, size 9 bytes
// name confidence: 0.75  rewrite confidence: 0.95
// evidence: rasterizer_decals_initialize 0x51a6a0 passes 0x51a660 to cache_new as the release procedure
//   (cache_evict_entry calls it with the evicted entry's handle). objdump 0x51a660..0x51a668:
//   mov edx,[esp+4]; jmp decal_delete 0x44e3c0 -- the decal owning the evicted vertices is deleted.
//   First-boot track: reached (as LAB_0051a660) from decals_initialize in the standalone exe.
// blam-cc: stack -> handle (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"

extern void decal_delete(datum_index decal_index); // 0x44e3c0, blam-cc: EDX -> decal_index

void decal_vertex_cache_release(datum_index handle)
{
    decal_delete(handle);
}
