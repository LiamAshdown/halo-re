// transparent_geometry_group_allocate  (Ghidra: transparent_geometry_group_allocate, already
// named)
// address 0x515230, size 46 bytes
// name confidence: 0.55  rewrite confidence: 0.85
// evidence: indexes transparent_geometry_groups by transparent_geometry_group_count * 0xa8
//   (sizeof(transparent_geometry_group)) and stamps the new slot's sorted_index field (+0x98).
// register convention: none -- __cdecl, no parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

extern transparent_geometry_group *transparent_geometry_groups; // 0x0071d14c
extern int32_t transparent_geometry_group_count;                // 0x0071d154

// Allocates the next free slot from the primary pool (max 384 entries), or NULL when full.
transparent_geometry_group *__cdecl transparent_geometry_group_allocate(void)
{
    transparent_geometry_group *group;

    if (transparent_geometry_group_count >= 0x180) {
        return (transparent_geometry_group *)0;
    }
    group = &transparent_geometry_groups[transparent_geometry_group_count];
    group->sorted_index = transparent_geometry_group_count;
    transparent_geometry_group_count = transparent_geometry_group_count + 1;
    return group;
}

#if 0
Original Ghidra decompilation (0x515230):

int __cdecl transparent_geometry_group_allocate(void)

{
  int iVar1;

  iVar1 = 0;
  if (DAT_0071d154 < 0x180) {
    iVar1 = DAT_0071d154 * 0xa8 + DAT_0071d14c;
    *(int *)(iVar1 + 0x98) = DAT_0071d154;
    DAT_0071d154 = DAT_0071d154 + 1;
  }
  return iVar1;
}
#endif
