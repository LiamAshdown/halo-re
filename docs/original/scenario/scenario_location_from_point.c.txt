// scenario_location_from_point  (Ghidra: FUN_0053e780, still unnamed there; first written as
// scenario_structure_bsp_find_leaf, renamed in the phase-4 review to the Blam name of the one
// function that builds a scenario_location (bsp_leaf_reference) from a point, which also puts it
// in the scenario_location_* family of its consumers 0x53e810 / 0x53ec30 / 0x53ed60 / 0x53ee00)
// address 0x53e780, size 57 bytes
// name confidence: 0.8   rewrite confidence: 0.8
// evidence: out/phase4/scenario_types_notes.md: "ESI = bsp_leaf_reference *out, EDX = point
// (forwarded to bsp3d_node_find_leaf 0x5013a0, which takes ECX = collision bsp, EAX = 0 root
// node, EDX = point)"; raw disassembly confirms ECX is loaded from global_collision_bsp
// (0x746f90) and EAX is zeroed immediately before the call, with EDX left untouched (the
// point argument this function itself received). This is the one function in the module that
// builds a bsp_leaf_reference (the {leaf_index, cluster_index} pair every other query function
// in this batch takes as input) from a raw point, by resolving the leaf and then reading its
// cluster out of global_structure_bsp->leaves.
// register convention: ESI -> out (bsp_leaf_reference *), EDX -> point (real_point3d *), no
// stack parameters, no return value.
//   // blam-cc: ESI -> out, EDX -> point

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "scenario.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint32_t bsp3d_node_find_leaf(int32_t node_index, ModelCollisionGeometryBSP *bsp, real_point3d *point); // 0x5013a0, physics module

extern ModelCollisionGeometryBSP *global_collision_bsp;   // 0x00746f90
extern ScenarioStructureBSP *global_structure_bsp;         // 0x00746f9c

// blam-cc: ESI -> out, EDX -> point
// Resolves `point` to a structure-bsp leaf and its cluster, writing both into *out. Sets
// out->cluster_index to -1 alongside a -1 leaf_index when the point is outside all geometry.
void scenario_location_from_point(bsp_leaf_reference *out, real_point3d *point)
{
    ScenarioStructureBSPLeaf *leaves;

    out->leaf_index = (int32_t)bsp3d_node_find_leaf(0, global_collision_bsp, point);
    if (out->leaf_index == -1) {
        out->cluster_index = -1;
        return;
    }

    // the index is masked with 0x7fffffff before the stride multiply (0x53e7a7)
    leaves = (ScenarioStructureBSPLeaf *)global_structure_bsp->leaves.pointer;
    out->cluster_index = (int16_t)leaves[out->leaf_index & 0x7fffffff].cluster;
}

#if 0
Original Ghidra decompilation (0x53e780):

void FUN_0053e780(void)

{
  int iVar1;
  int *unaff_ESI;

  iVar1 = FUN_005013a0();
  *unaff_ESI = iVar1;
  if (iVar1 == -1) {
    *(undefined2 *)(unaff_ESI + 1) = 0xffff;
    return;
  }
  *(undefined2 *)(unaff_ESI + 1) = *(undefined2 *)(iVar1 * 0x10 + 8 + *(int *)(DAT_00746f9c + 0xe4))
  ;
  return;
}

Raw disassembly (0x53e780-0x53e7b8):

  53e780: mov    ecx,DWORD PTR ds:0x746f90   ; ECX = global_collision_bsp
  53e786: xor    eax,eax                     ; EAX = 0 (root node)
  53e788: call   0x5013a0                    ; bsp3d_node_find_leaf(EAX=0, ECX=collision bsp, EDX=point)
  53e78d: cmp    eax,0xffffffff
  53e790: mov    DWORD PTR [esi],eax          ; out->leaf_index = result
  53e792: jne    0x53e79b
  53e794: or     eax,eax
  53e796: mov    WORD PTR [esi+0x4],ax        ; out->cluster_index = 0xffff
  53e79a: ret
  53e79b: mov    ecx,DWORD PTR ds:0x746f9c    ; ECX = global_structure_bsp
  53e7a1: mov    edx,DWORD PTR [ecx+0xe4]     ; EDX = leaves.pointer
  53e7a7: and    eax,0x7fffffff
  53e7ac: shl    eax,0x4                      ; EAX = leaf_index * 0x10 (ScenarioStructureBSPLeaf stride)
  53e7af: movsx  eax,WORD PTR [eax+edx*1+0x8] ; leaves[leaf_index].cluster
  53e7b4: mov    WORD PTR [esi+0x4],ax        ; out->cluster_index = cluster
  53e7b8: ret
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
