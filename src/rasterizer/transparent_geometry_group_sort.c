// transparent_geometry_group_sort  (Ghidra: transparent_geometry_group_sort, already named)
// address 0x5156d0, size 109 bytes
// name confidence: 0.55  rewrite confidence: 0.8
// evidence: resets transparent_geometry_group_sorted_indices to identity, qsorts it with
// transparent_geometry_group_compare, then re-derives each group's sorted_index (+0x98) from
// its new position.
// register convention: none -- __cdecl, no parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "fn_rasterizer.h"
#include <stdlib.h>

extern int32_t transparent_geometry_group_count;           // 0x0071d154
extern int16_t *transparent_geometry_group_sorted_indices; // 0x0071d15c
extern transparent_geometry_group *transparent_geometry_groups; // 0x0071d14c


// Re-sorts the active transparent-geometry groups by transparent_geometry_group_compare and
// records each group's new sorted index.
void __cdecl transparent_geometry_group_sort(void)
{
    int32_t i;

    for (i = 0; i < transparent_geometry_group_count; i++) {
        transparent_geometry_group_sorted_indices[i] = (int16_t)i;
    }

    qsort(transparent_geometry_group_sorted_indices, (size_t)transparent_geometry_group_count,
          sizeof(int16_t), (int (*)(const void *, const void *))transparent_geometry_group_compare);

    for (i = 0; i < transparent_geometry_group_count; i++) {
        transparent_geometry_groups[transparent_geometry_group_sorted_indices[i]].sorted_index = i;
    }
}

#if 0
Original Ghidra decompilation (0x5156d0):

void __cdecl transparent_geometry_group_sort(void)

{
  size_t sVar1;
  void *pvVar2;
  short sVar3;
  int iVar4;
  int iVar5;

  pvVar2 = DAT_0071d15c;
  sVar1 = DAT_0071d154;
  sVar3 = 0;
  if (0 < (int)DAT_0071d154) {
    iVar5 = 0;
    do {
      *(short *)((int)pvVar2 + iVar5 * 2) = sVar3;
      sVar3 = sVar3 + 1;
      iVar5 = (int)sVar3;
    } while (iVar5 < (int)sVar1);
  }
  _qsort(pvVar2,sVar1,2,transparent_geometry_group_compare);
  pvVar2 = DAT_0071d15c;
  sVar1 = DAT_0071d154;
  iVar5 = DAT_0071d14c;
  sVar3 = 0;
  if (0 < (int)DAT_0071d154) {
    iVar4 = 0;
    do {
      sVar3 = sVar3 + 1;
      *(int *)(*(short *)((int)pvVar2 + iVar4 * 2) * 0xa8 + 0x98 + iVar5) = iVar4;
      iVar4 = (int)sVar3;
    } while (iVar4 < (int)sVar1);
  }
  return;
}
#endif
