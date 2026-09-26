// decal_vertex_cache_in_use  (not a Ghidra function; the "decal vertex cache" in-use procedure)
// address 0x51a670, size 42 bytes
// name confidence: 0.75  rewrite confidence: 0.9
// evidence: rasterizer_decals_initialize 0x51a6a0 passes 0x51a670 to cache_new as the in-use procedure
//   (cache_allocate_block asks it before evicting an entry). objdump 0x51a670..0x51a699: the handle is stored in
//   0x0069c6b0, then the result is whether the decal's flags byte (+2 of its 0x38-byte decal_data element, indexed by
//   the handle's low word) has bit 0 or 1 set.
//   First-boot track: reached (as LAB_0051a670) from decals_initialize in the standalone exe.
// blam-cc: stack -> handle (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"

extern data_array *decal_data; // 0x0087abe4
extern datum_index decal_vertex_cache_last_queried; // 0x0069c6b0

uint8_t decal_vertex_cache_in_use(datum_index handle)
{
    uint8_t *element = (uint8_t *)decal_data->data + (uint32_t)(handle & 0xffff) * 0x38;

    decal_vertex_cache_last_queried = handle;
    return (element[2] & 3) != 0;
}
