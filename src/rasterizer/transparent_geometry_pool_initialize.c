// transparent_geometry_pool_initialize  (Ghidra: transparent_geometry_pool_initialize, already
// named)
// address 0x5151c0, size 109 bytes
// name confidence: 0.55  rewrite confidence: 0.75
// evidence: allocation sizes match types/rasterizer.h exactly: 0xfc00 == 384 *
//   sizeof(transparent_geometry_group) (k_rasterizer_maximum_transparent_groups), 0x300 == 384 *
//   sizeof(int16_t) (the sorted index table), 0x1500 == 32 * sizeof(transparent_geometry_group)
//   (k_rasterizer_maximum_secondary_groups).
// register convention: none -- __cdecl, no parameters.
// UNSURE: rasterizer_misc_vertex_buffer_create (outside this session's range) is called with no visible arguments.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "fn_rasterizer.h"

extern transparent_geometry_group *transparent_geometry_groups;           // 0x0071d14c
extern transparent_geometry_group *transparent_geometry_groups_secondary; // 0x0071d150
extern int32_t transparent_geometry_group_count;           // 0x0071d154
extern int32_t transparent_geometry_group_secondary_count; // 0x0071d158
extern int16_t *transparent_geometry_group_sorted_indices;  // 0x0071d15c
extern uint32_t rasterizer_misc_vertex_buffer_create(void); // 0x534e50, UNSURE arguments, outside this session's range

// Allocates the primary (384 entry) and secondary (32 entry) transparent geometry group pools
// and the primary pool's sorted-index buffer, then performs further subsystem init via
// rasterizer_misc_vertex_buffer_create. Returns a nonzero low byte on success.
int32_t __cdecl transparent_geometry_pool_initialize(void)
{
    uint32_t secondary_pool;

    transparent_geometry_groups = (transparent_geometry_group *)GlobalAlloc(0, 0xfc00);
    transparent_geometry_group_sorted_indices = (int16_t *)GlobalAlloc(0, 0x300);
    secondary_pool = (uint32_t)GlobalAlloc(0, 0x1500);
    transparent_geometry_group_secondary_count = 0;
    transparent_geometry_group_count = 0;
    transparent_geometry_groups_secondary = (transparent_geometry_group *)secondary_pool;

    if (transparent_geometry_groups != (transparent_geometry_group *)0 &&
        transparent_geometry_group_sorted_indices != (int16_t *)0 && secondary_pool != 0) {
        uint32_t result = rasterizer_misc_vertex_buffer_create(); // UNSURE
        if ((uint8_t)result != 0) {
            return (int32_t)((result & 0xffffff00) | 1);
        }
        return (int32_t)(result & 0xffffff00);
    }
    return (int32_t)(secondary_pool & 0xffffff00);
}

#if 0
Original Ghidra decompilation (0x5151c0):

int __cdecl transparent_geometry_pool_initialize(void)

{
  HGLOBAL pvVar1;

  DAT_0071d14c = GlobalAlloc(0,0xfc00);
  DAT_0071d15c = GlobalAlloc(0,0x300);
  pvVar1 = GlobalAlloc(0,0x1500);
  DAT_0071d158 = 0;
  DAT_0071d154 = 0;
  DAT_0071d150 = pvVar1;
  if (((DAT_0071d14c != (HGLOBAL)0x0) && (DAT_0071d15c != (HGLOBAL)0x0)) && (pvVar1 != (HGLOBAL)0x0)
     ) {
    pvVar1 = (HGLOBAL)FUN_00534e50();
    if ((char)pvVar1 != '\0') {
      return CONCAT31((int3)((uint)pvVar1 >> 8),1);
    }
  }
  return (uint)pvVar1 & 0xffffff00;
}
#endif
