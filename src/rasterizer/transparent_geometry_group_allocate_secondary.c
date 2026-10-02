// transparent_geometry_group_allocate_secondary  (Ghidra:
// transparent_geometry_group_allocate_secondary, already named)
// address 0x515260, size 43 bytes
// name confidence: 0.55  rewrite confidence: 0.85
// evidence: same shape as transparent_geometry_group_allocate.c but against the 32 entry
//   secondary pool.
// register convention: none -- __cdecl, no parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern transparent_geometry_group *transparent_geometry_groups_secondary; // 0x0071d150
extern int32_t transparent_geometry_group_secondary_count;                // 0x0071d158

// Allocates the next free slot from the secondary pool (max 32 entries), or NULL when full.
transparent_geometry_group *__cdecl transparent_geometry_group_allocate_secondary(void)
{
    transparent_geometry_group *group;

    if (transparent_geometry_group_secondary_count >= 0x20) {
        return (transparent_geometry_group *)0;
    }
    group = &transparent_geometry_groups_secondary[transparent_geometry_group_secondary_count];
    group->sorted_index = transparent_geometry_group_secondary_count;
    transparent_geometry_group_secondary_count = transparent_geometry_group_secondary_count + 1;
    return group;
}

#if 0
Original Ghidra decompilation (0x515260):

int __cdecl transparent_geometry_group_allocate_secondary(void)

{
  int iVar1;

  iVar1 = 0;
  if (DAT_0071d158 < 0x20) {
    iVar1 = DAT_0071d158 * 0xa8 + DAT_0071d150;
    *(int *)(iVar1 + 0x98) = DAT_0071d158;
    DAT_0071d158 = DAT_0071d158 + 1;
  }
  return iVar1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
