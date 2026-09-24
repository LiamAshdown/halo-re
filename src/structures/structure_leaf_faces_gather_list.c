// structure_leaf_faces_gather_list  (Ghidra: FUN_00552cf0; named here)
// address 0x552cf0, size 95 bytes
// name confidence: 0.4   rewrite confidence: 0.5
// evidence: disassembly (objdump -d -M intel bin/halo.exe, 0x552cf0..0x552d51) resolves the
//   implicit registers Ghidra's decompilation left as in_EAX/in_ECX: EAX -> out_faces,
//   ECX -> face_indices, with face_count on the stack. It also shows a call to
//   qsort_dword_array (0x449590, foreign, already CEA-named) with the index list sorted in place
//   before gathering -- base in ECX, count in EAX, and a single stack argument that is a code
//   address (0x552c00), which is almost certainly a comparator function pointer, but the 32-byte
//   routine at that address falls just before this batch's first function (0x552c20) and is not
//   itself decompiled/named by this pass.
// register convention: EAX -> out_faces, ECX -> face_indices, stack -> face_count.
// UNSURE: qsort_dword_array's real signature/comparator are not resolved; declared here from
//   the visible register evidence only.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "structures.h"

extern ScenarioStructureBSP *global_structure_bsp; // 0x00746f9c, physics.h/objects.h (read, not owned)

// TYPES-GAP: the 32-byte comparator routine at 0x552c00 (immediately before this batch's first
// function), not examined by this pass.
extern int qsort_dword_array_leaf_index_compare(const void *a, const void *b); // 0x552c00, UNSURE
extern void qsort_dword_array(int32_t *base, int32_t count, int (*compare)(const void *, const void *));
    // 0x449590, foreign (CRT-shaped) sort; blam-cc: ECX -> base, EAX -> count, stack -> compare

// Sorts `face_indices` (ascending) and then copies each named surface's vertex/index record into
// `out_faces`, in the sorted order.
void structure_leaf_faces_gather_list(int16_t face_count, ScenarioStructureBSPSurface *out_faces,
    int32_t *face_indices)
    // blam-cc: EAX -> out_faces, ECX -> face_indices
{
    ScenarioStructureBSPSurface *surfaces = (ScenarioStructureBSPSurface *)global_structure_bsp->surfaces.pointer;
    int32_t i;

    qsort_dword_array(face_indices, face_count, qsort_dword_array_leaf_index_compare);

    for (i = 0; i < face_count; i = i + 1) {
        out_faces[i] = surfaces[face_indices[i]];
    }
}

#if 0
Original Ghidra decompilation (0x552cf0):

void FUN_00552cf0(ushort param_1)

{
  undefined2 *puVar1;
  int iVar2;
  undefined2 *in_EAX;
  int *in_ECX;
  uint uVar3;

  iVar2 = DAT_00746f9c;
  qsort_dword_array(&LAB_00552c00);
  if (0 < (short)param_1) {
    uVar3 = (uint)param_1;
    do {
      puVar1 = (undefined2 *)(*(int *)(iVar2 + 0xfc) + *in_ECX * 6);
      *in_EAX = *puVar1;
      in_EAX[1] = puVar1[1];
      in_EAX[2] = puVar1[2];
      in_ECX = in_ECX + 1;
      in_EAX = in_EAX + 3;
      uVar3 = uVar3 - 1;
    } while (uVar3 != 0);
  }
  return;
}
#endif
