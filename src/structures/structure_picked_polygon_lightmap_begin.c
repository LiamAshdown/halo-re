// structure_picked_polygon_lightmap_begin  (not a Ghidra function; a lightmap-begin callback)
// address 0x511f20, size 9 bytes
// name confidence: 0.5  rewrite confidence: 1.0
// evidence: pushed as the lightmap_begin callback of structure_leaf_faces_for_each by structure_picked_polygon_draw
//   (0x552935 push 0x511f20); only reachable through that pointer.
// objdump 0x511f20..0x511f28: EAX = the lightmap argument, tail jump to rasterizer_underwater_tint_jitter_update.
// blam-cc: stack -> bitmap_data (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void rasterizer_underwater_tint_jitter_update(BitmapData *lightmap); // 0x51f310, blam-cc: EAX

void structure_picked_polygon_lightmap_begin(void *bitmap_data)
{
    rasterizer_underwater_tint_jitter_update((BitmapData *)bitmap_data);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
