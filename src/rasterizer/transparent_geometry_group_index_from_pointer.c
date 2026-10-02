// transparent_geometry_group_index_from_pointer  (Ghidra:
// transparent_geometry_group_index_from_pointer, already named)
// address 0x5152d0, size 53 bytes
// name confidence: 0.55  rewrite confidence: 0.85
// evidence: bounds-checks a pointer against the primary pool's [base, base + count*0xa8) range
//   and divides the byte offset by sizeof(transparent_geometry_group).
// register convention: candidate pointer in in_ECX. // blam-cc: ECX -> group

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern transparent_geometry_group *transparent_geometry_groups; // 0x0071d14c
extern int32_t transparent_geometry_group_count;                // 0x0071d154

// blam-cc: ECX -> group
// Converts a pointer into the primary transparent-geometry-group pool back into its slot index,
// or -1 if it is out of range.
int32_t transparent_geometry_group_index_from_pointer(transparent_geometry_group *group)
{
    uint8_t *base = (uint8_t *)transparent_geometry_groups;
    uint8_t *p = (uint8_t *)group;

    if (base <= p && p < base + (uint32_t)transparent_geometry_group_count * 0xa8) {
        return (int32_t)(p - base) / 0xa8;
    }
    return -1;
}

#if 0
Original Ghidra decompilation (0x5152d0):

int transparent_geometry_group_index_from_pointer(void)

{
  int iVar1;
  uint in_ECX;

  iVar1 = -1;
  if ((DAT_0071d14c <= in_ECX) && (in_ECX < DAT_0071d154 * 0xa8 + DAT_0071d14c)) {
    iVar1 = (int)(in_ECX - DAT_0071d14c) / 0xa8;
  }
  return iVar1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
