// detail_object_cell_upper_bound  (Ghidra: FUN_00552780; named here)
// address 0x552780, size 107 bytes
// name confidence: 0.4   rewrite confidence: 0.7
// evidence: same as detail_object_cell_lower_bound.c (0x552710), its sibling -- disassembly
//   confirms the identical ECX/EAX/stack calling convention, and the comparison is the mirror
//   "<=" test that makes this the paired std::upper_bound.
// register convention: ECX -> begin, EAX -> end, stack -> key.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "structures.h"

// Returns the first cell in [begin, end) greater than `key`, comparing (cell_x, cell_y, cell_z)
// lexicographically.
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
ScenarioStructureBSPGlobalDetailObjectCell *detail_object_cell_upper_bound(
    ScenarioStructureBSPGlobalDetailObjectCell *begin, ScenarioStructureBSPGlobalDetailObjectCell *end,
    detail_object_cell_key *key)
    // blam-cc: ECX -> begin, EAX -> end
{
    int32_t count = (int32_t)(end - begin);

    while (count > 0) {
        int32_t half = count / 2;
        ScenarioStructureBSPGlobalDetailObjectCell *mid = begin + half;
        int not_greater = (mid->cell_x < key->cell_x) ||
            (mid->cell_x == key->cell_x &&
             (mid->cell_y < key->cell_y ||
              (mid->cell_y == key->cell_y && mid->cell_z <= key->cell_z)));

        if (not_greater) {
            begin = mid + 1;
            count = count - half - 1;
        } else {
            count = half;
        }
    }
    return begin;
}

#if 0
Original Ghidra decompilation (0x552780):

int FUN_00552780(short *param_1)

{
  short sVar1;
  int in_EAX;
  int iVar2;
  int in_ECX;
  int iVar3;
  int iVar4;

  iVar4 = in_EAX - in_ECX >> 5;
  if (0 < iVar4) {
    do {
      iVar2 = iVar4 / 2;
      sVar1 = *(short *)(iVar2 * 0x20 + in_ECX);
      iVar3 = iVar2 * 0x20 + in_ECX;
      if ((sVar1 <= *param_1) &&
         ((sVar1 != *param_1) ||
          ((*(short *)(iVar3 + 2) <= param_1[1]) &&
           ((*(short *)(iVar3 + 2) != param_1[1]) || (*(short *)(iVar3 + 4) <= param_1[2])))))
      {
        in_ECX = iVar3 + 0x20;
        iVar2 = iVar4 + (-1 - iVar2);
      }
      iVar4 = iVar2;
    } while (0 < iVar2);
  }
  return in_ECX;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
