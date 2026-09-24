// transparent_geometry_group_get_next_sorted  (Ghidra: FUN_00515290, unnamed; named from its
// behaviour per out/phase4/rasterizer_functions.md's summary)
// address 0x515290, size 54 bytes
// name confidence: 0.4   rewrite confidence: 0.7
// evidence: reads group->sorted_index (+0x98), looks up
// transparent_geometry_group_sorted_indices[sorted_index + 1] and returns the primary pool
// group at that index -- i.e. the group drawn immediately after this one in the current
// depth-sorted order.
// register convention: group pointer in in_EAX. // blam-cc: EAX -> group

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

extern transparent_geometry_group *transparent_geometry_groups; // 0x0071d14c
extern int32_t transparent_geometry_group_count;                // 0x0071d154
extern int16_t *transparent_geometry_group_sorted_indices;      // 0x0071d15c

// blam-cc: EAX -> group
// Returns the transparent-geometry group that follows `group` in the current depth-sorted
// order, or NULL if `group` is NULL or already last.
transparent_geometry_group *transparent_geometry_group_get_next_sorted(transparent_geometry_group *group)
{
    int32_t next_position;

    if (group == (transparent_geometry_group *)0) {
        return (transparent_geometry_group *)0;
    }
    next_position = (int16_t)(group->sorted_index + 1);
    if (next_position >= transparent_geometry_group_count) {
        return (transparent_geometry_group *)0;
    }
    return &transparent_geometry_groups[transparent_geometry_group_sorted_indices[next_position]];
}

#if 0
Original Ghidra decompilation (0x515290):

int FUN_00515290(void)

{
  int in_EAX;
  int iVar1;

  if ((in_EAX != 0) && (iVar1 = (int)(short)(*(short *)(in_EAX + 0x98) + 1), iVar1 < DAT_0071d154))
  {
    return *(short *)(DAT_0071d15c + iVar1 * 2) * 0xa8 + DAT_0071d14c;
  }
  return 0;
}
#endif
