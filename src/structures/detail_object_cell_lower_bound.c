// detail_object_cell_lower_bound  (Ghidra: FUN_00552710; named here)
// address 0x552710, size 107 bytes
// name confidence: 0.4   rewrite confidence: 0.7
// evidence: disassembly (objdump -d -M intel bin/halo.exe, 0x552710..0x55277a) confirms
//   begin/end arrive as ECX/EAX (the sole caller, detail_objects_update_render_list 0x5522d0,
//   loads `mov eax,esi(end) / mov ecx,edi(begin)` immediately before each call) and the key as a
//   stack pointer. The loop is the textbook std::lower_bound bisection specialised to the
//   3 x int16 lexicographic key of ScenarioStructureBSPGlobalDetailObjectCell (stride 0x20,
//   types/structures.h).
// register convention: ECX -> begin, EAX -> end, stack -> key.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "structures.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// Returns the first cell in [begin, end) not less than `key`, comparing (cell_x, cell_y, cell_z)
// lexicographically.
ScenarioStructureBSPGlobalDetailObjectCell *detail_object_cell_lower_bound(
    ScenarioStructureBSPGlobalDetailObjectCell *begin, ScenarioStructureBSPGlobalDetailObjectCell *end,
    detail_object_cell_key *key)
    // blam-cc: ECX -> begin, EAX -> end
{
    int32_t count = (int32_t)(end - begin);

    while (count > 0) {
        int32_t half = count / 2;
        ScenarioStructureBSPGlobalDetailObjectCell *mid = begin + half;
        int less = (mid->cell_x < key->cell_x) ||
            (mid->cell_x == key->cell_x &&
             (mid->cell_y < key->cell_y ||
              (mid->cell_y == key->cell_y && mid->cell_z < key->cell_z)));

        if (less) {
            begin = mid + 1;
            count = count - half - 1;
        } else {
            count = half;
        }
    }
    return begin;
}

#if 0
Original Ghidra decompilation (0x552710):

int FUN_00552710(short *param_1)

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
      if ((sVar1 < *param_1) ||
         ((sVar1 == *param_1 &&
          ((*(short *)(iVar3 + 2) < param_1[1] ||
           ((*(short *)(iVar3 + 2) == param_1[1] && (*(short *)(iVar3 + 4) < param_1[2])))))))) {
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
