// chimera__rasterizer_dispose_free_memory  (Ghidra: chimera__rasterizer_dispose_free_memory,
// already named -- Chimera name, hint only)
// address 0x515430, size 100 bytes
// name confidence: 0.6   rewrite confidence: 0.75
// evidence: releases rasterizer_misc_vertex_buffer (0x0071d270, types/rasterizer.h) via its
//   vtable slot 8 (Release, matching an IDirect3D COM object), then frees the transparent
//   geometry group pools and resets their counts to zero.
// register convention: none -- __cdecl, no parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

extern void *rasterizer_misc_vertex_buffer; // 0x0071d270
extern transparent_geometry_group *transparent_geometry_groups;           // 0x0071d14c
extern transparent_geometry_group *transparent_geometry_groups_secondary; // 0x0071d150
extern int16_t *transparent_geometry_group_sorted_indices;                // 0x0071d15c
extern int32_t transparent_geometry_group_count;                          // 0x0071d154
extern int32_t transparent_geometry_group_secondary_count;                // 0x0071d158
extern uint32_t __stdcall GlobalFree(void *mem); // Win32

// Releases rasterizer_misc_vertex_buffer's COM object and frees the transparent-geometry-group
// pools and their sort-index buffer, resetting all counts to zero.
void __cdecl chimera__rasterizer_dispose_free_memory(void)
{
    if (rasterizer_misc_vertex_buffer != (void *)0) {
        void **vtable = *(void ***)rasterizer_misc_vertex_buffer;
        ((void (__stdcall *)(void *))vtable[2])(rasterizer_misc_vertex_buffer); // Release
        rasterizer_misc_vertex_buffer = (void *)0;
    }
    if (transparent_geometry_groups != (transparent_geometry_group *)0) {
        GlobalFree(transparent_geometry_groups);
    }
    transparent_geometry_groups = (transparent_geometry_group *)0;
    if (transparent_geometry_group_sorted_indices != (int16_t *)0) {
        GlobalFree(transparent_geometry_group_sorted_indices);
    }
    transparent_geometry_group_sorted_indices = (int16_t *)0;
    if (transparent_geometry_groups_secondary != (transparent_geometry_group *)0) {
        GlobalFree(transparent_geometry_groups_secondary);
    }
    transparent_geometry_groups_secondary = (transparent_geometry_group *)0;
    transparent_geometry_group_secondary_count = 0;
    transparent_geometry_group_count = 0;
}

#if 0
Original Ghidra decompilation (0x515430):

void __cdecl chimera__rasterizer_dispose_free_memory(void)

{
  if (DAT_0071d270 != (int *)0x0) {
    (**(code **)(*DAT_0071d270 + 8))(DAT_0071d270);
    DAT_0071d270 = (int *)0x0;
  }
  if (DAT_0071d14c != (HGLOBAL)0x0) {
    GlobalFree(DAT_0071d14c);
  }
  DAT_0071d14c = (HGLOBAL)0x0;
  if (DAT_0071d15c != (HGLOBAL)0x0) {
    GlobalFree(DAT_0071d15c);
  }
  DAT_0071d15c = (HGLOBAL)0x0;
  if (DAT_0071d150 != (HGLOBAL)0x0) {
    GlobalFree(DAT_0071d150);
  }
  DAT_0071d150 = (HGLOBAL)0x0;
  DAT_0071d158 = 0;
  DAT_0071d154 = 0;
  return;
}
#endif
